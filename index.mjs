// SPDX-License-Identifier: 0BSD

export class Insn {
    toJSON() {
        return {
            ...this,
            vaddr: `0x${this.vaddr.toString(16)}`,
            data: `0x${this.data.toString(16)}`
        };
    }
};

export class Mem {
    toJSON() {
        return { ...this, vaddr: `0x${this.vaddr.toString(16)}` };
    }
};

export default async function* generate(iterator) {
    const buffer = Buffer.alloc(16);
    let size = 0;
    let insn;

    for await (const chunk of iterator) {
        let start = buffer.length - size;

        size += chunk.copy(buffer, size, 0, start);

        while (size >= buffer.length) {
            const vaddr = buffer.readBigUInt64LE(0);
            const type = buffer.readUInt32LE(8);
            const data = buffer.readUInt32LE(12);
    
            switch (type) {
            case 0:
                if (insn) {
                    yield insn;
                }
    
                insn = new Insn;
                insn.vaddr = vaddr;
                insn.data = data;
                insn.mems = [];
                break;
    
            case 1:
            case 2:
                if (!insn) {
                    throw new Error("MEM before INSN");
                }
    
                const mem = new Mem;
                mem.vaddr = vaddr;
                mem.write = type == 2;
                mem.size = data;
                insn.mems.push(mem);
                break;
    
            default:
                throw new Error("unknown type");
            }

            const end = start + buffer.length;
            size = chunk.copy(buffer, 0, start, end);
            start += size;
        }
    }

    if (insn) {
        yield insn;
    }
};
