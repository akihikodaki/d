# SPDX-License-Identifier: 0BSD

CFLAGS += -fPIC -O2 -Wall -Wextra -pedantic -std=c11 $(shell pkg-config --cflags glib-2.0)
QEMU = qemu-x86_64

.PHONY: clean

obj/d.txt: obj/d.bin d
	./d < $< > $@

obj/d.bin: obj/libd.so | obj
	$(QEMU) -plugin "$$PWD"/$<,outfile=$@ /usr/bin/env true

obj/libd.so: d.c | obj
	$(CC) -shared $(CFLAGS) -o $@ $^

obj:
	mkdir $@

d:

clean:
	$(RM) obj
