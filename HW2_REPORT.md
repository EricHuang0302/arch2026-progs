# Computer Architecture — Homework 2: Complete Applications on RV32I

## AI Tools Usage

OpenAI Codex was used for:

- Conceptual Explanation and Code debugging
- Code Annotation
- Translation and Technical Writing (Polishing)

For this assignment, the assistance also included C and hand-written RV32I implementation, test design, measurements, and numerical analysis.

## Project Overview

This report covers four bare-metal programs for [Homework 2][assignment]. The code and test evidence are in [my fork][fork]. Each function has a portable C version and a separately written RV32I assembly version.

- **Rubik:** replace the HW1 search with an exact perimeter and a short prefix search.
- **BF16:** preserve the specified bit behavior without floating-point instructions.
- **GCD:** use shifts and subtraction instead of division.
- **Fixed:** implement bitwise arithmetic and a signed Q11.20 product.

All four programs pass the instructor's tests at `-O1`, `-O2`, `-O3`, and `-Os`, including the instruction and allocation limits. The C versions also pass ASan/UBSan. Seven of the nine assembly functions beat the fastest measured C build; subtraction and the unsigned average match it. The automated program score is **100/100**; it does not grade this report.

## Execution Environment

| Item | Configuration |
|---|---|
| Host | Windows/MSYS2; independent Linux CI |
| Cross compiler | xPack RISC-V GCC 15.2.0-1 |
| Target | `-march=rv32i_zicsr -mabi=ilp32 -ffreestanding` |
| Emulator | rv32emu system interpreter with `ELF_LOADER`; M/A/F/C disabled |
| Recorded emulator revision | `158b57e9eab99d59e01141cd0e477b06b44b8fe3` |
| Instructor framework | `fd2d7c17db223807c342e3f210c769d23d1865b1` |
| Implementation revision | `dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae` |

The guest runs without an operating system, at load address `0x10000`, with a 4 KiB stack and no allocator. Only the harness performs environment calls and counter reads. The implementations use no CSR, `ecall`, `ebreak`, or `fence`. Windows uses a disclosed emulator host-compatibility patch; Linux CI uses the unmodified emulator. Setup details are in [HANDOFF.md][handoff]. No physical RISC-V board was used.

The Lab 2 playground also passed at `-O0` and `-O2`, cleaning between builds; the [logs][lab-logs] retain the results and tool versions.

### Measuring the programs

rv32emu counts guest instructions: its `cycle` and `instret` counters do not model pipelines, caches, or physical execution time. The harness reads high/low/high counter words and retries on rollover. It keeps results observable through a checksum and removes a baseline loop:

$$
C_{call}=\frac{C_{timed}-C_{baseline}}{N}.
$$

These are 64-bit differences. Initialization and printing are outside the timed region. Some call-site work remains, so corrected costs are not simply the instruction count of the function body. Inputs and timed regions are identical for C and assembly. The [published benchmark][benchmark-sources] uses six cube states and sixteen characteristic arithmetic inputs; these are workload averages, not averages over every valid input.

## Stage 1: Porting Mini-Rubik

### Search design

The input is seven cubie digits followed by seven orientation digits. `12345671111111` is solved. With one corner fixed, there are

$$
7!\times3^6=3,674,160
$$

valid states. The nine moves are `R R2 R' B B2 B' D D2 D'`; each costs one move. The solver must return a shortest sequence, reject invalid input with `-1`, and return `-2` when the output buffer is too small.

My HW1 version used projected distances with IDA*. Here, a radius-five table gives exact distances for 12,224 nearby states. The target searches a prefix, then follows neighbors whose table distance decreases by one.

For a state at distance `D>5`, an optimal path first reaches the perimeter after `D-5` moves. A shorter prefix reaching it would give a solution shorter than `D`, which is impossible. Trying prefix lengths in order therefore preserves shortest solutions. Consecutive turns of one face are skipped because they combine or cancel. The 2,644 distance-eleven keys allow those states to start directly at prefix length six.

### Key representation and assembly

The permutation uses seven three-bit lanes; six independent orientations use twelve bits. The lookup key retains five cubies and one ordering bit for the remaining two:

```c
return (p & 32767u)
     | (((p >> 15 & 7u) > (p >> 18)) << 15)
     | (o << 16);
```

The five identities determine which two are missing, so their ordering bit makes this 28-bit key unique. A three-choice hash table checks at most three slots and compares the complete key, preventing false matches. The stored distance occupies the high four bits.

The assembly replaces `li 32767` followed by `and` with:

```asm
slli t0, a0, 17
srli t0, t0, 17
```

The immediate needs two instructions to load, so the two shifts save one instruction per lookup. Computing the second hash only after the first misses saves another six instructions on first-slot hits.

| Change, course `-O2` workload | Corrected instructions/call |
|---|---:|
| Previous assembly | 30,926.5000 |
| Compact mask only | 30,634.1667 |
| Lazy second hash only | 30,865.5000 |
| Both changes | 30,573.1667 |

Removing the diameter list slightly improved this short benchmark, but increased one distance-eleven call from 3,592,364 to 5,078,021 instructions with the old lookup. I kept the list. This is why I also checked difficult states instead of selecting changes from the six-state average alone. [Experiment records][optimization-data] retain the alternatives.

### Verification and application

The [host checker][full-cube-source] compares C solution lengths with BFS and replays every solution over all 3,674,160 states; normal and sanitized runs pass. On target, [all 2,644 distance-eleven states][diameter-source] pass C/assembly replay at every compiler level. The maximum assembly call is **3,558,487**, below 7,500,000; the largest C count is 4,748,454, below 50,000,000.

:::spoiler Memory and the remaining instruction bound

Assembly uses 100,706 read-only data bytes: 65,536 for perimeter slots, 10,576 for diameter keys, 24,576 for orientation transitions, and 18 for move names. Including code, its allocation is **102,290 bytes**, below 128 KiB. Its own maximum stack use is `64+6×48+16+48=416` bytes; caller stack is additional.

A non-diameter search visits at most 16,790 nodes across prefix limits one through five. A conservative manual inspection allows 400 exclusive instructions per visit plus 20,000 for parsing, suffix recovery, and output: 6,736,000 instructions. This is an analytical bound, not an exhaustive assembly timing run or a mechanically certified proof. The full-state host check executes C; the complete target sweep covers the diameter class.

:::

An exact perimeter is useful when a reversible planning graph is too large for a complete answer table. [Korf's pattern-database work][korf] illustrates related memory/search tradeoffs, although this implementation stores full-state perimeter distances rather than projected heuristic distances. Different move costs would require a new shortest-path model.

Sources: [C][rubik-c], [RV32I][rubik-asm], [contract][rubik-h], [compiler listing][rubik-dis].

## Stage 2: BFloat16

### Format and narrowing

BF16 has one sign bit, eight exponent bits with bias 127, and seven fraction bits. For a normal encoding,

$$
x=(-1)^s(128+f)2^{E-134}.
$$

The smallest positive normal is `2^-126`; the smallest subnormal is `2^-133`. The largest finite magnitude is `(2-2^-7)2^127`. Normal spacing within `[2^e,2^(e+1))` is `2^(e-7)`. Round-to-nearest has at most half-spacing error, or relative error `2^-8` for a normal result. In the subnormal region the narrowing absolute-error bound is `2^-134`; overflow to infinity is excluded from these finite-error bounds.

Narrowing receives binary32 bits and uses:

```c
return (uint16_t) ((f32bits + 0x7fffu
                   + ((f32bits >> 16) & 1u)) >> 16);
```

Writing the bits as `H·2^16+L`, the addition carries when `L>0x8000`, or when `L==0x8000` and `H` is odd. This implements ties to even. Exponent 255 returns the high half unchanged, including NaN payload truncation.

The new assembly checks bit 15 of `L` first. If `L<0x8000`, even `32767+32767+1` cannot carry, so it immediately returns `H`. This also preserves the special-value rule. Cost falls from **11.25 to 6.125** on the course workload, at an eight-byte text cost. A separate full-range random workload improves from 15.0 to 12.7461, showing a smaller gain.

### Multiply and square root

For normalized significands `Ma,Mb` in `[128,255]`, let `P=Ma·Mb`. With `t=[P≥32768]`, the retained significand is `P>>(7+t)` and the result exponent is `Ea+Eb-127+t`. The assembly includes the known leading-bit term, then performs seven unrolled shift-and-add steps. It costs **57.3125** versus the best C cost of 87.0. No M-extension multiply is used.

The square-root rule is not ordinary round-to-nearest. Let `m=128+f`, doubled when stored exponent `E` is even. It seeks the largest `r` satisfying `floor(r²/128)≤m`:

$$
r^2\le128(m+1)-1,\qquad
r=\left\lfloor\sqrt{128(m+1)-1}\right\rfloor.
$$

When stored exponent `E` is even, double the significand to make its unbiased exponent even; the output exponent is `floor((E+127)/2)`. The C version uses a restoring integer square root. Assembly instead indexes two 128-byte tables, selected by exponent parity. These exhaust the internal significand domain. Compressing the old 384-byte table to 256 bytes reduced the corrected `-O2` cost from **19.4375 to 19.1875**.

:::spoiler Contract special cases

Operand order matters for multiply: a NaN first operand is returned unchanged, while an infinite first operand can swallow a NaN second operand. Infinity times zero produces quiet NaN. Finite zero products keep the XOR sign.

Underflow removes the hidden bit before shifting the fraction. Thus `0x0080 × 0x3f00` returns zero, despite the real product being the representable subnormal `2^-127`. Its real relative error is 100%; the normal-result error bound does not apply.

Square root preserves NaNs and positive infinity, maps negative nonzero values to quiet NaN, and maps either signed zero and positive subnormals to positive zero. At adjusted `m=130`, the required root is 129; using `sqrt(128m)` would incorrectly give 128.

:::

### Error and verification

Normal multiplication truncates less than one significand unit, giving relative error below `1/128`. For the square-root rule, `r>sqrt(128m)-1` and `r≤sqrt(128m+127)` give a conservative relative bound below `1/128`. The [host reference][numeric-reference] measures tighter maxima:

| Operation | Host comparison | Largest relative error |
|---|---|---:|
| Square root | All 32,512 positive normals versus double `sqrt` | 0.388350982%, first at `0x0081` |
| Multiply | All 16,384 normal significand pairs at unit exponents | 0.769184180%, at `0x3f9e,0x3fd1` |

For normal multiplication results, exponent scaling preserves relative truncation error. Square-root errors repeat when input scaling preserves exponent parity, such as multiplying by a power of four. Special values and underflow are checked by contract bits instead.

![BF16 square-root error for both exponent parities and multiplication error over all normal significand pairs](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-final/bf16-error.svg)

The left plot shows both exponent parities; the right shows all normal significand pairs. The instructor tests additionally cover all 65,536 square-root encodings, narrowing low-half sweeps, multiplication boundaries and reference grids. The target stress test adds 100,000 full-range narrowing inputs. [Sources and logs][current-data] preserve the results.

[Cloud TPU][tpu-bf16] uses BF16 products with FP32 accumulation to reduce storage and bandwidth. This assignment's integer implementation demonstrates the representation tradeoff; its special-value rules differ from TPU arithmetic.

Sources: [C][bf16-c], [RV32I][bf16-asm], [contract][bf16-h], [compiler listing][bf16-dis].

## Stage 3: Binary GCD

### Principle and implementation

Binary GCD uses three identities:

$$
\gcd(2a,2b)=2\gcd(a,b),\quad
\gcd(2a,b)=\gcd(a,b)\ (b\text{ odd}),\quad
\gcd(a,b)=\gcd(a,b-a)\ (0<a\le b).
$$

Removing common powers of two preserves `gcd(original)=2^k gcd(current)`. Subtracting odd operands produces an even difference; shifts reduce it before the next comparison. The positive sum decreases, so the process terminates. Zero checks handle `gcd(a,0)=a`, including `gcd(0,0)=0`. The result is exact, with no quantization error or lost bits in the restored shift.

For example, `(48,18)` becomes `(24,9)` with one common factor of two, then `(3,9)`, `(3,6)`, and `(3,3)`. Restoring the factor gives 6.

```asm
bltu a0, a1, .Lgcd_sub_b
sub  a0, a0, a1
beqz a0, .Lgcd_equal
```

Unsigned comparison is necessary for inputs above `INT32_MAX`. Assembly subtracts in either direction instead of swapping operands, removing moves found in the C listing. An initial byte test skips eight trailing zeros without an unavailable `ctz` instruction.

### Optimization and verification

| Variant, course `-O2` workload | Instructions/call | Text bytes |
|---|---:|---:|
| Previous assembly | 85.7500 | 156 |
| Remove initial byte skip | 91.5000 | 144 |
| Even path falls through, accepted | 85.3125 | 152 |
| Also test eight common zero bits | 88.1250 | 180 |

Inverting the branch removes an unconditional jump. The extra common-byte test adds work on this workload, so I rejected it. The accepted assembly beats best C at **101.625 versus 85.3125**, but is larger than the 120-byte `-Os` C object.

Course tests cover zeros, symmetry, scaling, divisibility, directed cases, and budgets. An additional 10,000 random pairs and 256 directed pairs match a Euclidean reference using binary long division. The largest recorded assembly call is 322 instructions, below 1,500; samples do not exhaust all operand pairs.

GCD reduces exact fractions, such as `48/18` to `8/3`, and tests coprimality before modular inversion. This branch-dependent implementation is not constant-time for secret inputs. The identities are also described by [NIST][binary-gcd].

Sources: [C][gcd-c], [RV32I][gcd-asm], [contract][gcd-h], [compiler listing][gcd-dis].

## Stage 4: Bitwise arithmetic and Q11.20

### The three logical operations

`op1` returns the modular sum and the number of carry-propagation passes:

```c
uint32_t carry = (a & b) << 1;
a ^= b;
b = carry;
++*n;
```

The identity `a+b=(a XOR b)+2(a AND b)` preserves the sum modulo `2^32`. The lowest carry bit moves upward, giving at most 32 passes. For `(0xffff,1)`, the result is `0x10000` after 17 passes. A direct `add` cannot replace this loop because the pass count is observable. Assembly removes an extra count-register move found in GCC's loop: **24.6875 to 22.125** versus best C.

`op2` is direct modular subtraction. `op3` computes an unsigned average without overflowing the intermediate sum:

$$
\left\lfloor\frac{a+b}{2}\right\rfloor
=(a\mathbin{\&}b)+((a\oplus b)\gg1).
$$

The result stays between the operands. At `a=b=0xffffffff`, it correctly returns `0xffffffff`. GCC and assembly already use minimal sequences here; measured costs match at 4.0 and 7.0 respectively.

### Signed fixed-point multiplication

Q11.20 uses a sign bit, eleven integer bits, and twenty fractional bits. Raw integer `A` represents `A/2^20`, with range `[-2048,2048-2^-20]` and resolution `2^-20`. Since represented inputs multiply to `AB/2^40`, the returned raw result is

$$
R=\left\lfloor\frac{AB+2^{19}}{2^{20}}\right\rfloor
\pmod{2^{32}}.
$$

Ties round toward positive infinity: raw `(3,524288)` returns 2, while `(-3,524288)` returns -1. Sign restoration must therefore precede rounding. Out-of-range results wrap rather than saturate.

The C version forms the product in two unsigned words, propagates carries, corrects the signed high word, adds the bias, and extracts the low 32 result bits. This avoids the missing `__muldi3` helper and undefined signed shifts. Assembly instead takes unsigned magnitudes and chooses the smaller multiplier to shorten the loop. `INT32_MIN` has valid unsigned magnitude `0x80000000`.

```asm
add  a2, a2, a0
sltu t1, a2, a0
add  a3, a3, t0
add  a3, a3, t1
```

The accumulator is `a3:a2`; `t0:a0` is the shifted multiplicand. Comparing the new low word against the addend detects carry without a temporary copy. The full 64-bit product is negated when necessary before adding the rounding bias.

| Product variant, course `-O2` workload | Instructions/call | Complete fixed text |
|---|---:|---:|
| Previous assembly | 241.0000 | 192 B |
| Magnitudes without ordering | 244.7500 | 216 B |
| Smaller magnitude as multiplier | 204.3750 | 232 B |
| Skip positive-magnitude conversion | 200.8125 | 224 B |
| In-place low-word accumulation, accepted | 195.1250 | 220 B |

Magnitude conversion alone loses; ordering pays for it by reducing loop passes. The accepted product beats best C's **321.375** cost, while spending 16 bytes more than the smallest complete C object. A separate full-range workload improves from 372.4844 to 344.4023, so the gain depends on the operands.

### Error and verification

Before wrapping, rounding costs at most half an output unit: `2^-21` real units. For raw inputs `A,B` and actual raw output `R`, the host checker computes the exact residual

$$
\epsilon=(R2^{20}-AB)/2^{40}.
$$

It subtracts integers before conversion to `double`. This measures output quantization for already represented inputs, excluding any earlier input quantization. The maximum observed absolute error is **`2^-21 = 4.76837158203125e-7`**, attained by raw `(1,524288)→1` and by bounded random inputs.

<!-- BEGIN FIXED_ERROR TABLE -->
| Host input group | Pairs checked | Nonwrapping | Wrapping | Largest absolute error, real units |
|---|---|---|---|---|
| Directed edges and ties | 169 | 163 | 6 | `4.76837158203125e-07` |
| Bounded random | 100,000 | 100,000 | 0 | `4.76837158203125e-07` |
| Full-range random | 100,000 | 402 | 99,598 | `4.76085915579461e-07` |
<!-- END FIXED_ERROR TABLE -->

The directed group is every ordered pair of 13 edge/tie values. Full-range xorshift32 uses seed `0x12345678`; the bounded stream uses `0x5eedc0de`, mapping raw operands to `(random & 0x3ffffff)-33554432`, or real `[-32,32)`. All 200,169 pairs pass bit-exact reference checks; the 100,565 nonwrapping cases enter the error maximum. The 99,604 wrapping cases are counted separately because the small-error bound does not apply. For example, real `1024×1024` wraps to zero. [Host source][numeric-reference] and [sanitized output][numeric-review] preserve the witnesses and counts.

On target, C and assembly additionally match a four-part 16×16 product reference on 100,000 random and 256 directed pairs. Course tests check pass counts, modular identities, wide averages, signed edges, and ties. These tests supplement the arithmetic argument; they do not exhaust all `2^64` pairs.

Fixed point suits graphics and DSP when scale and intermediate ranges are known. The instructor's [line-drawing note][line-note] demonstrates that range analysis. [CMSIS-DSP][cmsis-multiply] uses saturating Q-format products; this assignment's wrapping behavior requires a different overflow policy.

Sources: [C][fixed-c], [RV32I][fixed-asm], [contract][fixed-h], [compiler listing][fixed-dis].

## Performance Comparison

The table uses the four compiler levels and assembly in the `-O2` ELF. Values are corrected guest instructions per call, from the [saved CSVs][current-data].

:::spoiler Complete compiler-level results

<!-- BEGIN CYCLE TABLE -->
| Function | C -O1 | C -O2 | C -O3 | C -Os | Assembly @ -O2 |
|---|---|---|---|---|---|
| bf16.from_f32 | 11.0625 | 12.1875 | 13.8125 | 12.1875 | 6.1250 |
| bf16.mul | 94.1250 | 87.0000 | 88.4375 | 91.1250 | 57.3125 |
| bf16.sqrt | 56.1250 | 53.6250 | 38.5625 | 52.3125 | 19.1875 |
| fixed.op1 | 27.2500 | 24.6875 | 26.8125 | 24.6875 | 22.1250 |
| fixed.op2 | 4.0000 | 4.0000 | 5.9375 | 4.0000 | 4.0000 |
| fixed.op3 | 7.0000 | 7.0000 | 8.9375 | 7.0000 | 7.0000 |
| fixed.q_mul | 321.9375 | 321.3750 | 323.3125 | 343.4375 | 195.1250 |
| gcd.gcd | 101.6250 | 116.6250 | 118.8125 | 119.6875 | 85.3125 |
| rubik.solve | 40094.5000 | 36459.8333 | 33934.1667 | 40223.1667 | 30573.1667 |
<!-- END CYCLE TABLE -->

:::

For the best-C comparison, both costs must come from the same ELF:

<!-- BEGIN SPEEDUP TABLE -->
| Function | Best C level | C cost | Assembly in same ELF | C / assembly |
|---|---|---|---|---|
| bf16.from_f32 | -O1 | 11.0625 | 6.1250 | 1.81× |
| bf16.mul | -O2 | 87.0000 | 57.3125 | 1.52× |
| bf16.sqrt | -O3 | 38.5625 | 20.6250 | 1.87× |
| fixed.op1 | -O2 | 24.6875 | 22.1250 | 1.12× |
| fixed.op2 | -O1 | 4.0000 | 4.0000 | 1.00× |
| fixed.op3 | -O1 | 7.0000 | 7.0000 | 1.00× |
| fixed.q_mul | -O2 | 321.3750 | 195.1250 | 1.65× |
| gcd.gcd | -O1 | 101.6250 | 85.3125 | 1.19× |
| rubik.solve | -O3 | 33934.1667 | 30575.6667 | 1.11× |
<!-- END SPEEDUP TABLE -->

![Assembly speedup against the best C build in the same ELF](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-final/speedup.svg)

A ratio above one favors assembly. For example, BF16 square root compares C 38.5625 with assembly 20.6250 in the `-O3` ELF, rather than 19.1875 from `-O2`. Caller and baseline-loop optimization explain why the same assembly object can have different corrected costs.

:::spoiler All-level instruction chart

![Compiler optimization levels and hand-written RV32I instruction costs](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-final/cycles-per-call.svg)

:::

`-O3` helps Rubik and C square root but does not improve every small function; `-O1` wins for narrowing and GCD. Rubik's `-O3` inlining grows C text to 5,568 bytes, versus 1,740 at `-Os`. The listings show concrete differences: table lookup versus restoring-root arithmetic, extra GCD operand moves, and masked C accumulation versus the shorter-magnitude product loop.

### Footprint and budgets

`size -A` counts each implementation's allocated sections, excluding the shared harness. Smaller code and faster code are different comparisons:

| Program | Smallest C total | Assembly total |
|---|---:|---:|
| Rubik | 102,452 B | 102,290 B |
| BF16 | 600 B | 872 B |
| GCD | 120 B | 152 B |
| Fixed | 204 B | 220 B |

BF16 trades 256 table bytes for root speed; GCD and Fixed also spend bytes to reduce work. Rubik's largest C allocation is 106,280 bytes; every implementation remains below its 128 KiB limit.

:::spoiler Allocated sections and per-function budgets

<!-- BEGIN FOOTPRINT TABLE -->
| Program | Build | .text | .rodata | .data | .bss |
|---|---|---|---|---|---|
| rubik | asm | 1584 | 100706 | 0 | 0 |
| rubik | -O1 | 1940 | 100712 | 0 | 0 |
| rubik | -O2 | 1896 | 100712 | 0 | 0 |
| rubik | -O3 | 5568 | 100712 | 0 | 0 |
| rubik | -Os | 1740 | 100712 | 0 | 0 |
| bf16 | asm | 616 | 256 | 0 | 0 |
| bf16 | -O1 | 724 | 0 | 0 | 0 |
| bf16 | -O2 | 644 | 0 | 0 | 0 |
| bf16 | -O3 | 804 | 0 | 0 | 0 |
| bf16 | -Os | 600 | 0 | 0 | 0 |
| gcd | asm | 152 | 0 | 0 | 0 |
| gcd | -O1 | 140 | 0 | 0 | 0 |
| gcd | -O2 | 140 | 0 | 0 | 0 |
| gcd | -O3 | 140 | 0 | 0 | 0 |
| gcd | -Os | 120 | 0 | 0 | 0 |
| fixed | asm | 220 | 0 | 0 | 0 |
| fixed | -O1 | 248 | 0 | 0 | 0 |
| fixed | -O2 | 236 | 0 | 0 | 0 |
| fixed | -O3 | 236 | 0 | 0 | 0 |
| fixed | -Os | 204 | 0 | 0 | 0 |
<!-- END FOOTPRINT TABLE -->

![Allocated implementation bytes for all compiler levels and assembly](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-final/footprint.svg)

<!-- BEGIN BUDGET TABLE -->
| Function | C budget | Largest recorded C call | Assembly budget | Largest recorded assembly call |
|---|---|---|---|---|
| bf16.from_f32 | 150 | 15 | 60 | 16 |
| bf16.mul | 1000 | 162 | 400 | 129 |
| bf16.sqrt | 4000 | 105 | 1500 | 29 |
| gcd.gcd | 4000 | 457 | 1500 | 322 |
| fixed.op1 | 1500 | 233 | 600 | 169 |
| fixed.op2 | 150 | 6 | 40 | 6 |
| fixed.op3 | 150 | 9 | 40 | 9 |
| fixed.q_mul | 3000 | 536 | 1200 | 427 |
| rubik.solve | 50000000 | 4748454 | 7500000 | 3558487 |
<!-- END BUDGET TABLE -->

:::

The full suites report 52 Rubik, 42 BF16, 34 GCD, and 38 Fixed tests at each level, including shared runtime checks. All per-call budgets and the 120-second ELF limit pass. Recorded maxima are samples unless the input class is explicitly exhaustive. Host sanitizers test C; assembly is checked by target execution, cross-comparisons, and ISA/symbol scans.

## Test Results and Reproduction

![GitHub Actions success for 308e139](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/953ddcd3e1f8ddb10c2689dae02057ead131e6b6/measurements/screenshots/ci-308e139.jpg)

This is the successful `308e139` run: lint, chain, grade, and aggregate CI pass. The instructor-only reference job is skipped on forks. The image records that version, not later document edits.

:::spoiler Host numerical-check screenshot

![Archived host numerical-check output](https://raw.githubusercontent.com/EricHuang0302/arch2026-progs/953ddcd3e1f8ddb10c2689dae02057ead131e6b6/measurements/screenshots/numeric-reference-308e139.jpg)

This is a browser view of the archived [host log][numeric-review], not a fresh terminal run.

:::

Follow [HANDOFF.md][handoff] for dependencies, macOS Apple Silicon/Intel setup, Windows host settings, and extra verification commands. The destination Mac procedure is documented but has not been executed there.

:::spoiler Build and repeat the recorded checks

```sh
git clone https://github.com/EricHuang0302/arch2026-progs.git
cd arch2026-progs
git checkout 308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29
export CROSS_COMPILE=riscv-none-elf-
export RV32EMU_REV=158b57e9eab99d59e01141cd0e477b06b44b8fe3
make
make test
make sanitize
make cycles
make dump
make size
```

That revision includes the updated host checker. Its sanitizer command is in the [numerical reproduction notes][numeric-review]. For submission, return to current `main`, run `make update` and `make grade`, and retain the new emulator revision if it changes. The tables needed to build are tracked; regeneration is optional.

:::

The [full derivations and detailed experiment discussion](https://github.com/EricHuang0302/arch2026-progs/blob/39de99afbdc32259d9cce519172e46f6baddbae8/HW2_REPORT.md) remain available at a fixed revision. `python3 dev/audit_report.py` checks this report's five tables, source excerpts, pinned file paths, figures, screenshots, and spoiler pairing.

## References

- [Assignment and requirements][assignment], [instructor framework][upstream].
- [Fixed-point line drawing][line-note] and [BF16 sine approximation][sine-note].
- [RISC-V Assembly Programmer's Manual][asm-manual] and [ISA specification][isa-manual].
- [Optimization experiment sources][experiments], [raw alternatives][optimization-data], and [complete disassembly][all-disassembly].

[assignment]: https://hackmd.io/@sysprog/2026-arch-homework2
[fork]: https://github.com/EricHuang0302/arch2026-progs
[upstream]: https://github.com/sysprog21/arch2026-progs
[line-note]: https://hackmd.io/@sysprog/line-draw-fixed-point
[sine-note]: https://hackmd.io/@sysprog/bf16-sine
[source-ci]: https://github.com/EricHuang0302/arch2026-progs/actions/runs/41432002649
[handoff]: https://github.com/EricHuang0302/arch2026-progs/blob/308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29/HANDOFF.md
[numeric-review]: https://github.com/EricHuang0302/arch2026-progs/tree/308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29/measurements/review-2026-11-03
[binary-gcd]: https://xlinux.nist.gov/dads/HTML/binaryGCD.html
[korf]: https://cdn.aaai.org/AAAI/1997/AAAI97-109.pdf
[tpu-bf16]: https://docs.cloud.google.com/tpu/docs/bfloat16
[cmsis-multiply]: https://arm-software.github.io/CMSIS-DSP/v1.14.2/group__BasicMult.html
[asm-manual]: https://github.com/riscv-non-isa/riscv-asm-manual/blob/7ae3a91c7baf12b1ac58d27885288a2450e6d2c6/src/asm-manual.adoc
[isa-manual]: https://github.com/riscv/riscv-isa-manual/releases/tag/20240411
[xpack-release]: https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/tag/v15.2.0-1
[startup]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/common/start.S
[perf]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/common/perf.S
[bench]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/common/bench.h
[rubik-c]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/rubik/rubik.c
[rubik-asm]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/rubik/rubik.S
[rubik-h]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/rubik/rubik.h
[bf16-c]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/bf16/bf16.c
[bf16-asm]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/bf16/bf16.S
[bf16-h]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/bf16/bf16.h
[gcd-c]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/gcd/gcd.c
[gcd-asm]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/gcd/gcd.S
[gcd-h]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/gcd/gcd.h
[fixed-c]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/fixed/fixed.c
[fixed-asm]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/fixed/fixed.S
[fixed-h]: https://github.com/EricHuang0302/arch2026-progs/blob/dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae/src/fixed/fixed.h
[experiments]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/optimization_experiments.py
[stress-source]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/target_stress.c
[full-cube-source]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/rubik_full_domain.c
[diameter-source]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/rubik_diameter.c
[numeric-reference]: https://github.com/EricHuang0302/arch2026-progs/blob/308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29/mytests/numeric_reference.c
[rubik-generator]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/generate_rubik_tables.c
[bf16-generator]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/mytests/generate_bf16_table.py
[benchmark-sources]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/tests
[lab-logs]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/measurements/lab2
[optimization-data]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/measurements/optimization
[current-data]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-final
[earlier-measurements]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/measurements/initial
[error-data]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/dev/plot_bf16_error.py
[all-disassembly]: https://github.com/EricHuang0302/arch2026-progs/tree/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-disassembly
[rubik-dis]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-disassembly/rubik-O2.dis
[bf16-dis]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-disassembly/bf16-O2.dis
[gcd-dis]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-disassembly/gcd-O2.dis
[fixed-dis]: https://github.com/EricHuang0302/arch2026-progs/blob/dfe705594fe6105f59158e184ca3a884b1338673/measurements/report-disassembly/fixed-O2.dis
