# Cycle analysis

- compiler: riscv-none-elf-gcc 15.2.0, -march=rv32i_zicsr -mabi=ilp32 -ffreestanding
- rv32emu: 158b57e (system emulation, ELF_LOADER, interpreter)
- timed region: n calls on identical inputs; loop baseline subtracted

rv32emu has no timing model: `cycle` and `instret` are one counter of executed guest instructions. The figures below are instruction counts per call, with the loop control, input load and counter reads removed.

## Cycles per call

| Function | -O1 | -O2 | -O3 | -Os | asm | best C | asm vs best C |
|---|---|---|---|---|---|---|---|
| bf16.from_f32 | 11.1 | 12.2 | 13.8 | 12.2 | 11.2 | 11.1 | 0.98x |
| bf16.mul | 94.1 | 87.0 | 88.4 | 91.1 | 57.3 | 87.0 | 1.52x |
| bf16.sqrt | 56.1 | 53.6 | 38.6 | 52.3 | 19.2 | 38.6 | 1.87x |
| fixed.op1 | 27.2 | 24.7 | 26.8 | 24.7 | 22.1 | 24.7 | 1.12x |
| fixed.op2 | 4.0 | 4.0 | 5.9 | 4.0 | 4.0 | 4.0 | 1.00x |
| fixed.op3 | 7.0 | 7.0 | 8.9 | 7.0 | 7.0 | 7.0 | 1.00x |
| fixed.q_mul | 321.9 | 321.4 | 323.3 | 343.4 | 241.0 | 321.4 | 1.33x |
| gcd.gcd | 101.6 | 116.6 | 118.8 | 119.7 | 85.8 | 101.6 | 1.19x |
| rubik.solve | 40094.5 | 36459.8 | 33934.2 | 40223.2 | 30926.5 | 33934.2 | 1.10x |

## Ratio to -O2

| Function | -O1 | -O2 | -O3 | -Os | asm |
|---|---|---|---|---|---|
| bf16.from_f32 | 0.91 | 1.00 | 1.13 | 1.00 | 0.92 |
| bf16.mul | 1.08 | 1.00 | 1.02 | 1.05 | 0.66 |
| bf16.sqrt | 1.05 | 1.00 | 0.72 | 0.98 | 0.36 |
| fixed.op1 | 1.10 | 1.00 | 1.09 | 1.00 | 0.90 |
| fixed.op2 | 1.00 | 1.00 | 1.48 | 1.00 | 1.00 |
| fixed.op3 | 1.00 | 1.00 | 1.28 | 1.00 | 1.00 |
| fixed.q_mul | 1.00 | 1.00 | 1.01 | 1.07 | 0.75 |
| gcd.gcd | 0.87 | 1.00 | 1.02 | 1.03 | 0.74 |
| rubik.solve | 1.10 | 1.00 | 0.93 | 1.10 | 0.85 |

## Footprint (bytes, `size -A`, harness excluded)

| Program | Variant | .text | .rodata | .data | .bss |
|---|---|---|---|---|---|
| rubik | asm | 1588 | 100706 | 0 | 0 |
| rubik | -O1 | 1940 | 100712 | 0 | 0 |
| rubik | -O2 | 1896 | 100712 | 0 | 0 |
| rubik | -O3 | 5568 | 100712 | 0 | 0 |
| rubik | -Os | 1740 | 100712 | 0 | 0 |
| bf16 | asm | 608 | 256 | 0 | 0 |
| bf16 | -O1 | 724 | 0 | 0 | 0 |
| bf16 | -O2 | 644 | 0 | 0 | 0 |
| bf16 | -O3 | 804 | 0 | 0 | 0 |
| bf16 | -Os | 600 | 0 | 0 | 0 |
| gcd | asm | 156 | 0 | 0 | 0 |
| gcd | -O1 | 140 | 0 | 0 | 0 |
| gcd | -O2 | 140 | 0 | 0 | 0 |
| gcd | -O3 | 140 | 0 | 0 | 0 |
| gcd | -Os | 120 | 0 | 0 | 0 |
| fixed | asm | 192 | 0 | 0 | 0 |
| fixed | -O1 | 248 | 0 | 0 | 0 |
| fixed | -O2 | 236 | 0 | 0 | 0 |
| fixed | -O3 | 236 | 0 | 0 | 0 |
| fixed | -Os | 204 | 0 | 0 | 0 |
