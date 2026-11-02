#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
prefix=${CROSS_COMPILE:-$(scripts/toolchain.sh --detect)}
make elfs CROSS_COMPILE="$prefix"
mkdir -p measurements/disassembly
for program in rubik bf16 gcd fixed; do
    for opt in O1 O2 O3 Os; do
        "${prefix}objdump" -d -M no-aliases "build/$program/$opt/$program.elf" \
            > "measurements/disassembly/$program-$opt.dis"
    done
    "${prefix}objdump" -d -M no-aliases "build/$program/impl_asm.o" \
        > "measurements/disassembly/$program-asm.dis"
done
