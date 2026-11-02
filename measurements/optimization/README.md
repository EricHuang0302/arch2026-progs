# Controlled optimization experiments

The baseline is the previously submitted source at
`382db5609e3d66a1b7d15f23396b77eae024c7d8`. The accepted implementation is
`dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae`. C implementations and instructor
framework files are identical between these revisions. The source checkpoint
[passed Linux CI with 100/100](https://github.com/EricHuang0302/arch2026-progs/actions/runs/41432002649).

These experiments were performed for the report on November 2, 2026. They are
controlled alternatives and ablations, not a reconstructed historical series
of edits. `sources/` retains each assembly variant. `mytests/optimization_experiments.py`
regenerates them from the fixed baseline without editing implementation or
instructor files, links identical C/main/test/runtime objects, executes the
whole course suite at all four levels, and saves every output. All variants
here passed those tests; this does not establish the budget for every input.

The full `cycles.csv` and `footprint.csv` contain the variants together.
`cycles-fixed.csv` and `footprint-fixed.csv` preserve an additional fixed-point
experiment batch. The combined files use those later fixed rows. Missing
allocated sections have size zero. The metadata is the same as the original
measurements: xPack GCC 15.2.0-1, RV32I/Zicsr ILP32, rv32emu
`158b57e9eab99d59e01141cd0e477b06b44b8fe3`, system interpreter with ELF_LOADER,
M/A/F/C disabled. Windows host adjustments remain disclosed in HANDOFF.md.

## Decisions

| Program and variant | Assembly instructions/call in the -O2 ELF | .text bytes | Decision |
|---|---:|---:|---|
| Rubik baseline | 30926.5000 | 1588 | Reference |
| Compact key extraction | 30634.1667 | 1584 | Keep |
| Lazy second hash only | 30865.5000 | 1588 | Keep with compact extraction |
| Both changes | 30573.1667 | 1584 | Accepted |
| No diameter-list selection | 30867.0000 | 1488 | Ablation; retain diameter selection |
| BF16 narrowing baseline | 11.2500 | 608 | Reference for complete BF16 object |
| Lower-half guard | 6.1250 | 616 | Accepted |
| GCD baseline | 85.7500 | 156 | Reference |
| Fall through to even path | 85.3125 | 152 | Accepted |
| Remove initial byte skip | 91.5000 | 144 | Ablation; retain byte skip |
| Check common zero bytes too | 88.1250 | 180 | Rejected |
| Fixed product baseline | 241.0000 | 192 | Reference for complete fixed object |
| Magnitudes without ordering | 244.7500 | 216 | Rejected alone |
| Shorter magnitude as multiplier | 204.3750 | 232 | Intermediate |
| Branch around positive magnitudes | 200.8125 | 224 | Intermediate |
| Carry directly in accumulator | 195.1250 | 220 | Accepted combination |

Instruction numbers above remove the loop baseline. They describe this input
distribution, not a universal average. `target_stress-*.log` uses a separate
256-pair full-range pseudorandom workload and shows improvements there too.
Those intervals include the input-array addressing in the call expression,
so their absolute costs are not compared with the course table.

The Rubik benchmark contains no distance-eleven input. Removing diameter
selection saves a lookup there, but the controlled worst-case call at
`54721631111111` takes 5,078,021 raw instructions without it, compared with
3,592,364 for the old baseline and 3,558,487 for the accepted implementation.
All three return the same shortest eleven-move solution. These counts include
counter-read overhead; the raw log preserves the exact caller and result.

## Independent checks

- `target_stress-host.log`: ASan/UBSan, with the four-part product reference
  also compared against native signed 64-bit arithmetic.
- `target_stress-baseline.log`, `target_stress-final.log`: both C and assembly
  match 100,000 random products and narrowings, 10,000 random Euclidean GCD
  references, and the 256 directed cross-products of boundary values.
- `../report-final/rubik-full-domain.log`: host C returns the BFS-optimal
  length and a successfully replayed solution on all 3,674,160 valid states.
  The checker reuses the offline enumeration and published move model;
  neither its BFS nor its path replay calls target move/lookup functions.
- `../report-final/rubik-diameter-*.log`: all 2,644 distance-eleven states
  run on target, C and assembly, at every optimization level. The largest
  accepted assembly call is 3,558,487 raw instructions, below 7,500,000.

Run `python3 mytests/optimization_experiments.py` before the stress script,
which consumes the generated baseline snapshots. Run
`bash mytests/run_diameter.sh measurements/report-final` for current target
records. Compiler and emulator setup is in HANDOFF.md. The report distinguishes
all-state host correctness, selected target cases, worst-distance target
coverage, and an analytical conservative bound; it does not claim that all
3,674,160 assembly paths were timed on the target.
