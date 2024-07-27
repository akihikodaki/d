// SPDX-License-Identifier: 0BSD

import stream from "node:stream";

export class Insn {
    toString() {
        const vaddr = this.vaddr.toString(16);
        const data = this.data.toString(16);
        const mems = this.mems.map(mem => `{ ${mem} }`).join(", ");

        return `vaddr: 0x${vaddr}, data: 0x${data}, mems: ${mems}`;
    }
};

export class Mem {
    toString() {
        return `vaddr: 0x${this.vaddr.toString(16)}, size: ${this.size}, write: ${this.write}`;
    }
};

export default class Transform extends stream.Transform {
    #buffer = Buffer.alloc(16);
    #size = 0;
    #insn;

    constructor(options) {
        super({ ...options, readableObjectMode: true });
    }

    #consumeBuffer() {
        const vaddr = this.#buffer.readBigUInt64LE(0);
        const type = this.#buffer.readUInt32LE(8);
        const data = this.#buffer.readUInt32LE(12);

        switch (type) {
        case 0:
            if (this.#insn) {
                this.push(this.#insn);
            }

            this.#insn = new Insn;
            this.#insn.vaddr = vaddr;
            this.#insn.data = data;
            this.#insn.mems = [];
            break;

        case 1:
        case 2:
            if (!this.#insn) {
                throw new Error("MEM before INSN");
            }

            const mem = new Mem;
            mem.vaddr = vaddr;
            mem.write = type == 2;
            mem.size = data;
            this.#insn.mems.push(mem);
            break;

        default:
            throw new Error("unknown type");
        }
    }

    _transform(data, encoding, callback) {
        let start = this.#buffer.length - this.#size;

        this.#size += data.copy(this.#buffer, this.#size, 0, start);

        while (this.#size >= this.#buffer.length) {
            this.#consumeBuffer();
            const end = start + this.#buffer.length;
            this.#size = data.copy(this.#buffer, 0, start, end);
            start += this.#size;
        }

        callback();
    }
};
