#!/usr/bin/env bash
# Run from any directory after installing the repository prerequisites.
set -euo pipefail
cd "$(dirname "$0")/.."
prefix=${CROSS_COMPILE:-$(scripts/toolchain.sh --detect)}
emu=${EMU:-third_party/rv32emu/build/rv32emu}
if [ ! -f "$emu" ] && [ -f "$emu.exe" ]; then
    emu="$emu.exe"
fi
make elfs CROSS_COMPILE="$prefix"
mkdir -p build/mytests measurements/stress
"${prefix}gcc" -march=rv32i_zicsr -mabi=ilp32 -O2 -ffreestanding \
    -fno-pic -fno-stack-protector -Wall -Wextra -Werror -iquote . -Icommon \
    -c mytests/rubik_diameter.c -o build/mytests/rubik_diameter.o
for opt in O1 O2 O3 Os; do
    "${prefix}ld" -T common/linker.ld --no-warn-rwx-segments \
        -o build/mytests/rubik_diameter.elf build/mytests/rubik_diameter.o \
        "build/rubik/$opt/impl.o" build/rubik/impl_asm.o \
        build/common/start.o build/common/perf.o build/common/rt.o \
        build/common/print.o
    "$emu" build/mytests/rubik_diameter.elf \
        > "measurements/stress/rubik-diameter-$opt.log"
    tail -1 "measurements/stress/rubik-diameter-$opt.log"
done
