# SPDX-License-Identifier: 0BSD

CFLAGS += -fPIC -O2 -Wall -Wextra -pedantic -std=c11 $(shell pkg-config --cflags glib-2.0)
QEMU = qemu-x86_64

.PHONY: clean

obj/d.ndjson: obj/libd.so d index.mjs
	$(QEMU) -plugin "$$PWD"/$<,outfile=>(./d > $@) /usr/bin/env true

obj/libd.so: d.c | obj
	$(CC) -shared $(CFLAGS) -o $@ $^

obj:
	mkdir $@

d:

clean:
	$(RM) obj
