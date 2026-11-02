#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
prefix=${CROSS_COMPILE:-$(scripts/toolchain.sh --detect)}
make elfs CROSS_COMPILE="$prefix"
out=${1:-measurements/disassembly}
mkdir -p "$out"
for program in rubik bf16 gcd fixed; do
    for opt in O1 O2 O3 Os; do
        "${prefix}objdump" -d -M no-aliases "build/$program/$opt/$program.elf" \
            > "$out/$program-$opt.dis"
    done
    "${prefix}objdump" -d -M no-aliases "build/$program/impl_asm.o" \
        > "$out/$program-asm.dis"
done
