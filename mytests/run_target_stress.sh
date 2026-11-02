#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
prefix=${CROSS_COMPILE:-$(scripts/toolchain.sh --detect)}
emu=${EMU:-third_party/rv32emu/build/rv32emu}
if [ ! -f "$emu" ] && [ -f "$emu.exe" ]; then emu="$emu.exe"; fi
make elfs CROSS_COMPILE="$prefix"
mkdir -p build/optimization measurements/optimization
"${prefix}gcc" -march=rv32i_zicsr -mabi=ilp32 -O2 -ffreestanding \
    -fno-pic -fno-stack-protector -Wall -Wextra -Werror -iquote . -Icommon \
    -DOPT_NAME='"O2"' -c mytests/target_stress.c \
    -o build/optimization/target_stress.o
for variant in baseline final; do
    objects=()
    for program in bf16 gcd fixed; do
        source="src/$program/$program.S"
        if [ "$variant" = baseline ]; then
            source="measurements/optimization/sources/$program-baseline.S"
        fi
        object="build/optimization/stress-$program-$variant.o"
        "${prefix}as" -march=rv32i_zicsr -mabi=ilp32 "$source" -o "$object"
        objects+=("$object" "build/$program/O2/impl.o")
    done
    "${prefix}ld" -T common/linker.ld --no-warn-rwx-segments \
        -o "build/optimization/target_stress-$variant.elf" \
        build/optimization/target_stress.o "${objects[@]}" \
        build/common/start.o build/common/perf.o build/common/rt.o \
        build/common/print.o
    "$emu" "build/optimization/target_stress-$variant.elf" \
        > "measurements/optimization/target_stress-$variant.log"
    cat "measurements/optimization/target_stress-$variant.log"
done
