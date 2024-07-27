#!/usr/bin/env node
// SPDX-License-Identifier: 0BSD

import("./index.mjs").then(async ({ default: Transform }) => {
    const transform = new Transform;

    process.stdin.pipe(transform);

    for await (const insn of transform) {
        console.log(insn.toString());
    }
});
