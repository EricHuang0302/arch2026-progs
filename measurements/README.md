# Reproducible measurement evidence

This directory contains experimental records supporting HW2_REPORT.md.
`report-final/` and `report-disassembly/` are the current report measurements;
the older `initial/`, `final/`, `disassembly/`, `stress/`, and `trusted/` records
are preserved below as historical evidence, not overwritten by the new work.
See `optimization/README.md` for the new controlled experiments and references.
See [the transfer instructions](../HANDOFF.md) for setup and rerun commands.

## Historical provenance and checks

- Cross compiler: xPack RISC-V GCC 15.2.0-1, GCC 15.2.0, binutils 2.45.
- Target: `-march=rv32i_zicsr -mabi=ilp32 -ffreestanding`; the complete flags
  are in the unchanged instructor Makefile. C is built with each of -O1,
  -O2, -O3, and -Os; the same hand-written assembly object is used in each.
- Emulator: `158b57e9eab99d59e01141cd0e477b06b44b8fe3`, system emulation,
  ELF_LOADER, interpreter, with M/A/F/C disabled.
- Instructor harness: `fd2d7c17db223807c342e3f210c769d23d1865b1`.
- Final implementation: `8eddc2357136725d36f30ce1ef6be47bc923ea29`;
  later commits add tests and reproducibility records.
- Playground: `914f7715f8d7f8678892a9fd91f78b40de351fc6`.
- Local host checks: Clang 21.1.1 on MSYS2 clang64 with AddressSanitizer and
  UndefinedBehaviorSanitizer, without sanitizer recovery. Hook formatter:
  clang-format 20.1.8. MSYS2 Python: 3.12.11; chart Python: 3.13.11.
- Local Windows emulator adjustments are disclosed in HANDOFF.md. The first
  independent [Linux CI run](https://github.com/EricHuang0302/arch2026-progs/actions/runs/41424722846)
  passed lint, chain, grade, and aggregate CI with the instructor's unmodified
  emulator. Check the latest main result on the repository's Actions page;
  final delivery also provides its exact run link.
- `trusted/grade.log`: fresh full instructor clone, committed submission,
  history check, host sanitizers, all four target levels, 120-second limit,
  and 100/100. The framework files are unchanged.
- `lab2/O0.log` and `lab2/O2.log`: all eight playground checks pass; `make clean`
  preceded the optimization change. All six cycle totals match the handout.

## How to read the numbers

`final/cycles.csv` preserves raw 64-bit cycle/instruction differences and
loop-baseline measurements for identical input arrays and timed regions.
Initialization and printing are outside those regions. Per-call cost is
`(cycles - overhead) / n`; use the assembly and best-C rows from the same ELF
for speedup, because caller optimization can slightly change the measurement.
One run suffices for these deterministic instruction counts. No wall-clock
performance claim is made.

rv32emu has no timing model. Its cycle CSR counts executed guest instructions;
the results do not measure pipeline stalls, cache misses, or physical clock
cycles. Separate reads and caller instructions can create small boundary
differences between measured cycle and instruction intervals.

`final/footprint.csv` reports .text, .rodata, .data, and .bss separately for
each implementation object, excluding the shared test harness. Compiler
debug sections are excluded from allocated size. The SVG charts are tracked
text files; the PNG originals are reproducible with `make cycles` and also
appear in the GitHub Actions `cycles` artifact. `disassembly/` preserves all
16 complete ELF listings and four assembly-object listings.

## Compiler comparison and useful limits

Exact numbers are in [the final tables](final/cycles.md).

- Rubik assembly takes about 30,926 instructions per benchmark call versus
  33,934 for the fastest C build. It uses 102,294 allocated bytes versus the
  smallest C build's 102,452 bytes. The target searches a prefix, then follows
  decreasing exact radius-five distances. The three-choice static table
  bounds each membership query by three probes. The generator retains
  12,224 near states and 2,644 diameter states, not the complete state space.
- GCD assembly takes about 85.8 instructions per call versus 101.6 for the
  best C build. Removing swaps, tightening the even-factor loops, and skipping
  initial zero bytes trades 36 extra code bytes against the smallest C build
  for faster execution.
- BF16 multiply specializes an eight-bit significand product and takes about
  57.3 instructions versus 87.0 for the best C build. The square root replaces
  repeated arithmetic with a small exact integer table. The narrowing path
  uses the same essential rounding operations as the best compiler result;
  its small measured deficit is within the prescribed two-cycle allowance.
- Fixed-point assembly takes about 241.0 instructions per product versus
  321.4 for the best C build. It keeps both product words in caller-saved
  registers and handles signed corrections and rounding without stack
  spills. The complete assembly is 192 bytes versus 204 for the smallest C
  build. Plain subtraction and the carry-safe average already reach the
  compiler's minimal instruction sequences; they match rather than beat it.

`initial/` precedes the BF16 table compression and the final spelling/static
analysis cleanup. `final/` follows them. Square root data shrank from 384 to
256 bytes and its code by four bytes; the timed average dropped from 19.4375
to 19.1875 instructions per call. The fixed C carry expression was written
as a pre-addition threshold to avoid a static analyzer's incorrect conclusion
about unsigned wraparound; final C product costs are slightly higher. The
assembly product is unchanged. Neither change relaxes correctness checks.

## Independent numerical checks

`stress/numeric-reference.log` comes from a host-only test using double
precision square roots and native signed 64-bit multiplication:

- All 32,512 positive normal BF16 square roots: maximum relative error
  0.388350982%, at input 0x0081.
- All 16,384 pairs of normal significands at unit exponents: maximum relative
  product error 0.769184180%, below 1/128. Scaling normal products by powers
  of two preserves this relative error until a special/underflow boundary.
- 100,000 full-range signed Q11.20 operand pairs agree exactly, modulo 2^32,
  with native 64-bit arithmetic and the specified tie rule.

BF16 has one sign bit, eight exponent bits with bias 127, and seven fraction
bits. Normal values are `(-1)^s * (128+f) * 2^(e-134)`. A normal product
truncation loses less than one stored unit; this bound does not describe the
quiz's special underflow behavior, which discards the hidden bit before
shifting the stored fraction. The contract deliberately differs from full
IEEE arithmetic there. Infinities and NaNs follow the contract's operand
order, not a symmetric NaN propagation rule.

Q11.20 stores `A / 2^20`, with range `[-2048, 2048 - 2^-20]` and resolution
`2^-20`. Products add half a stored unit before taking the arithmetic scale
shift, so ties go toward positive infinity, including negative ties. Keeping
the low 32 result bits wraps on overflow. The implementation never relies
on undefined signed overflow or a 64-bit software multiply helper.

## Rubik worst-case evidence

`stress/rubik-diameter-*.log` sweeps all 2,644 distance-eleven states for all
four C levels. Every returned length is eleven; an independent cubie model
parses and applies every returned C and assembly solution. The maximums are:

| Build | C instructions | Assembly instructions |
|---|---:|---:|
| -O1 | 4,748,454 | 3,592,364 |
| -O2 | 4,295,305 | 3,592,364 |
| -O3 | 4,038,954 | 3,592,364 |
| -Os | 4,748,384 | 3,592,364 |

All maximums occur at state `54721631111111`. Counts include counter-read
overhead. Both limits have ample room: 50,000,000 for C and 7,500,000 for
assembly. The generator's complete host BFS histogram is retained in
`stress/rubik-generator.log` and sums to 3,674,160.

Non-diameter states require a searched prefix of at most five moves. With
consecutive same-face turns omitted, each search has at most nine first
children and six children thereafter. Across prefix limits one through five,
there are 16,790 search-node visits including the roots. A conservative
400-instruction exclusive bound per visit (charging a quarter-turn to its
child) plus 20,000 for parsing, table-guided suffix recovery, and formatting
is 6,736,000 instructions. The hand-written basic blocks are well below that
bound. The deeper six-move prefix is used only for the completely swept
diameter list. Stack use is bounded by the 64-byte entry frame, six 48-byte
search frames, and one 16-byte leaf plus 48-byte suffix frame: 416 bytes.

The normal course tests independently cover lengths zero through eleven,
invalid states, buffer boundaries, all nine moves, and C/assembly agreement.
These records supplement those tests rather than replacing the instructor's
harness.

## Reference access

The assignment page, Lab 2, README, CONTRIBUTING, all four contract headers,
test sources, and relevant specifications in all three quiz PDFs were read.
The PDFs' text layers contain unrelated material, so the visible pages were
rendered and reviewed: quiz 1 Problem B for BF16, quiz 2 Problem A for binary
GCD, and quiz 3 Problem C for fixed-point arithmetic. Encoding, special-value
order, square-root truncation, narrowing, and signed tie handling agree with
the implementations. The instructor also publishes the
[BF16 reference source](https://github.com/sysprog21/ca2025-quizzes/blob/6b27e0104a9be6014cd4f6ad2b1101c3af5c184b/q1-bfloat16.c).
The authoritative assignment headers and independent tests govern this
implementation. No PDF password or protected quiz file is included here.
The excluded CodeTrial interview and recording remain the student's separate
work. The English Markdown report is now included in the project scope.
