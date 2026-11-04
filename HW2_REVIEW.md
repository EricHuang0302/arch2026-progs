# Independent review and response

Review date: November 3, 2026 (Asia/Taipei).
Reviewed version: [1b034e8](https://github.com/EricHuang0302/arch2026-progs/tree/1b034e8056f15bd1b2deb3f3d6a724c1da339cf0).

The user requested a separate grading agent to examine the programs and
submission Markdown objectively before turning its advice into a correction
goal. The reviewer read the official assignment, authoritative headers,
implementation, report, handoff, and saved evidence without editing files.
This is an independent AI review, not an instructor grade or a guarantee
against unknown private tests. CodeTrial, online publication, and form
submission were excluded by the user and are not findings.

## Findings at the reviewed version

One confirmed P2 finding; no P0/P1 finding or proven implementation error.

**P2: Fixed lacked a measured maximum quantization error.** The original
[report, lines 654 onward](https://github.com/EricHuang0302/arch2026-progs/blob/1b034e8056f15bd1b2deb3f3d6a724c1da339cf0/HW2_REPORT.md#L654)
gave the theoretical bound `2^-21` and 100,000 bit-exact comparisons against a
rounded integer reference. The original
[host checker](https://github.com/EricHuang0302/arch2026-progs/blob/1b034e8056f15bd1b2deb3f3d6a724c1da339cf0/mytests/numeric_reference.c#L36)
did not measure error against the unrounded exact product. The final paragraph
of the [official assignment](https://hackmd.io/@sysprog/2026-arch-homework2)
requires the largest measured error against a host reference for BF16 and fixed
point; item 6 also requires error analysis and verification. Bit-exact contract
agreement alone did not satisfy this numerical reporting requirement.

Acceptance: retain exact contract checks, measure nonwrapping directed and
fixed-seed samples, report the absolute-error maximum and witness, separate
wrapping cases, preserve source and log links, and audit the result.

## Coverage and initial assessment

| Requirement | Review result at 1b034e8 |
|---|---|
| Lab 2 at -O0/-O2, cleaning between levels | Recorded evidence present |
| Four programs in C and hand-written RV32I | Present; no identified contract violation |
| Four compiler levels, ISA, symbols, sanitizers | Existing executed evidence |
| Instruction, allocation, and time limits | Checks pass; measured and analytical claims distinguished |
| Disassembly, same-ELF comparisons, section sizes | Present; recorded tables match CSVs |
| Accepted and unsuccessful optimizations | Present; controlled ablations identified honestly |
| English chapters, mathematics, error, verification, applications | Present except the Fixed measured-error gap |
| Short explained excerpts and pinned links | Audit passes; relevant external sources accessible |
| HW1 writing approach | Stage structure, short excerpts, and stepwise explanations retained |
| Instructor harness, linear history, fork CI | Evidence present; instructor files unchanged |
| macOS handoff | Documented; destination Mac execution remains unverified |

The reviewer's initial nonofficial assessment was **97/100**, with this explicit
weighting. The instructor's automated score was independently **100/100** and
does not grade the report.

| Area | Initial assessment |
|---|---:|
| Programs and contracts | 30/30 |
| Performance and resources | 20/20 |
| Mathematics and error analysis | 22/25 |
| Verification and reproduction | 15/15 |
| Documentation and prior style | 10/10 |

## Correction and retained evidence

The follow-up goal is recorded in [PROJECT_GOAL.md](PROJECT_GOAL.md).
The submitted implementations remain unchanged. Evidence commit
[308e139](https://github.com/EricHuang0302/arch2026-progs/commit/308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29)
adds exact integer-residual analysis to the host checker and preserves a
successful ASan/UBSan run in
[measurements/review-2026-11-03](https://github.com/EricHuang0302/arch2026-progs/tree/308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29/measurements/review-2026-11-03).

All 200,169 products pass the bit-exact reference check. The 100,565
nonwrapping cases have maximum absolute error `2^-21`, attained by directed
ties and the bounded random sample. The 99,604 wrapping cases are separately
counted and excluded from that small-error maximum. The updated report gives
the method, input distributions, witness, counts, measured table, and fixed
source/log references. The read-only report audit checks the new table and
exact witness residuals. HANDOFF.md includes reproduction details.

Independent recheck completed on November 4. The same reviewer independently
replayed all 200,169 input pairs and confirmed the counts, maximum residuals,
and witnesses. The original P2 is resolved with no concrete remaining finding.
The reproduction checkout now selects `308e139`, which contains the updated
checker. The reviewer also verified the two original JPEG screenshots, their
fixed-version references, twelve balanced spoiler blocks, and preservation of
the required report content. The five measured tables and 67 reference uses
pass the read-only audit. This closes the finding without asserting an
instructor report grade or an invented new numerical score.

## Limits retained after the correction

The reviewer did not classify these as known errors: Rubik's non-diameter
instruction bound is a conservative manual argument; full-state host testing
executes C, while target execution exhausts the distance-eleven class; unknown
private tests remain unknown; and the destination macOS setup has not run.
No arbitrary new score is inferred from fixing the report gap.
