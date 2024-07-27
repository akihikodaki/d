// SPDX-License-Identifier: 0BSD

#include <stdio.h>
#include <qemu-plugin.h>

enum {
    INSN,
    MEM_R,
    MEM_W,
};

typedef struct Packet {
    uint64_t vaddr;
    uint32_t type;
    uint32_t data;
} Packet;

typedef struct PacketList PacketList;

struct PacketList {
    PacketList *next;
    Packet packets[];
};

static FILE *file;
static PacketList *insn_packet_list;

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

static void plugin_vcpu_insn_exec(G_GNUC_UNUSED unsigned int vcpu_index,
                                  void *userdata)
{
    fwrite(userdata, sizeof(Packet), 1, file);
}

static void plugin_vcpu_mem(G_GNUC_UNUSED unsigned int vcpu_index,
                            qemu_plugin_meminfo_t info, uint64_t vaddr,
                            G_GNUC_UNUSED void *userdata)
{
    Packet packet = {
        .vaddr = GUINT64_TO_LE(vaddr),
        .type = GUINT32_TO_LE(qemu_plugin_mem_is_store(info) ? MEM_W : MEM_R),
        .data = GUINT32_TO_LE(qemu_plugin_mem_size_shift(info))
    };

    fwrite(&packet, sizeof(packet), 1, file);
}

static void plugin_vcpu_tb_trans(G_GNUC_UNUSED qemu_plugin_id_t id,
                                 struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    PacketList *list = malloc(sizeof(PacketList) + sizeof(Packet) * n);

    list->next = insn_packet_list;
    insn_packet_list = list;

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);

        list->packets[i].vaddr = GUINT64_TO_LE(qemu_plugin_insn_vaddr(insn));
        list->packets[i].type = GUINT32_TO_LE(INSN);
        list->packets[i].data = 0;

        qemu_plugin_insn_data(insn, &list->packets[i].data,
                              sizeof(list->packets[i].data));

        qemu_plugin_register_vcpu_insn_exec_cb(insn, plugin_vcpu_insn_exec,
                                               QEMU_PLUGIN_CB_NO_REGS,
                                               list->packets + i);

        qemu_plugin_register_vcpu_mem_cb(insn, plugin_vcpu_mem,
                                         QEMU_PLUGIN_CB_NO_REGS,
                                         QEMU_PLUGIN_MEM_RW, NULL);
    }
}

static void plugin_flush(G_GNUC_UNUSED qemu_plugin_id_t id)
{
    while (insn_packet_list) {
        PacketList *prev = insn_packet_list;
        insn_packet_list = insn_packet_list->next;
        free(prev);
    }
}

static void plugin_atexit(qemu_plugin_id_t id, G_GNUC_UNUSED void *userdata)
{
    plugin_flush(id);
    fclose(file);
}

QEMU_PLUGIN_EXPORT
int qemu_plugin_install(qemu_plugin_id_t id,
                        G_GNUC_UNUSED const qemu_info_t *info,
                        int argc, char **argv)
{
    for (int i = 0; i < argc; i++) {
        if (memcmp(argv[i], "outfile=", sizeof("outfile=") - 1)) {
            fputs("unknown option\n", stderr);
            return -1;
        }

        file = fopen(argv[i] + sizeof("outfile=") - 1, "w");
        if (!file) {
            perror("outfile");
            return -1;
        }
    }

    qemu_plugin_register_vcpu_tb_trans_cb(id, plugin_vcpu_tb_trans);
    qemu_plugin_register_flush_cb(id, plugin_flush);
    qemu_plugin_register_atexit_cb(id, plugin_atexit, NULL);

    return 0;
}
