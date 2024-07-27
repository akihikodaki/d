#!/usr/bin/env node
// SPDX-License-Identifier: 0BSD

import("./index.mjs").then(async ({ default: generate }) => {
    for await (const insn of generate(process.stdin)) {
        console.log(insn.toString());
    }
});
