# Assignment 2 project goal

Updated scope: November 4, 2026 (Asia/Taipei).

Complete the programming, validation, optimization, reproducibility work,
and an English Markdown report ready to paste into HackMD
for [Assignment 2: Complete Applications](https://hackmd.io/@sysprog/2026-arch-homework2)
by November 16, 2026, 23:59 (Asia/Taipei). Follow the current instructor
contracts and framework in `sysprog21/arch2026-progs`.

The user will download this project onto a macOS computer and handle the
CodeTrial interview and recording there. CodeTrial is outside this agent's
scope and is not a condition for completing this project goal. This scope
change does not remove the instructor's interview requirement.

## Exclusions

- Publishing the report online or submitting the course form.
- Installing, building, configuring, or operating CodeTrial.
- Setting up Gemini, LiveKit, cameras, microphones, or recording software.
- Conducting, assisting with, recording, inspecting, or submitting the
  CodeTrial interview, practice sessions, result exports, or video.
- Optional bonus contributions, unless separately requested.

## Completion requirements

1. Read the applicable repository instructions, README, contribution rules,
   function contracts, lab, and relevant quiz specifications. Prepare a
   reproducible toolchain, emulator, host sanitizer environment, formatting,
   static analysis, spelling checks, Git hooks, and enabled fork workflows.
   Complete the Lab 2 system-emulation playground tests with ELF_LOADER at
   both -O0 and -O2, cleaning before changing optimization levels, and retain
   the tool versions and actual results.

2. Implement all required functions in rubik, bf16, gcd, and fixed, in both
   portable C and genuinely hand-written RV32I assembly with the required
   `_asm` symbols. Honor every valid input, special value, rounding rule,
   wraparound rule, error return, shortest-solution requirement, and buffer
   contract. Preserve the calling convention and 4 KiB stack constraint.
   Do not use prohibited instructions, unsupported external symbols,
   compiler assembly as a substitute for hand-written code, or a complete
   precomputed Rubik state table. Do not change the instructor's harness to
   evade its checks or specialize answers to public tests or benchmarks.

3. Pass all four optimization levels (-O1, -O2, -O3, -Os), C/assembly cross
   checks, ISA and symbol checks, host AddressSanitizer and UBSan checks,
   instruction budgets, static allocation budgets, and the instructor's
   120-second limit for each complete ELF run. The Rubik limits are
   7,500,000 instructions for assembly, 50,000,000 for C, and 128 KiB per
   implementation including code and all allocated data. Keep additional
   tests outside the implementation directories. Run the latest instructor
   harness from a fresh complete clone against the submission and obtain
   100 points. Pass make test, make sanitize, and applicable repository checks.

4. Profile and improve all four assembly implementations. Compare cycles
   with the fastest of the four C builds and footprint with the smallest
   C build. Meet the automated best-C-plus-two-cycles threshold and seek
   an actual assembly speedup. Retain disassembly, before/after measurements,
   cycle and instruction tables, plots, and separate .text/.rodata/.data/.bss
   sizes excluding the shared harness. Record input sets, timed regions,
   64-bit counter differences, overhead treatment, compiler versions and
   flags, and emulator revisions. Explain that emulator cycles count guest
   instructions rather than physical pipeline or cache behavior. Preserve
   evidence in tracked deliverables instead of leaving it only in ignored
   build/results directories. Write HW2_REPORT.md using the user's HW1 style
   and the two instructor reference notes: explain each algorithm and its
   mathematics, correctness, numerical format and error, RV32I implementation,
   compiler comparison, measured optimization changes, and real applications.
   Use short explained code excerpts and pinned source/evidence links. Disclose
   the actual AI assistance. Reproduce missing measurements rather than invent
   development history, and retain unsuccessful controlled experiments.

5. Use the user's fork, https://github.com/EricHuang0302/arch2026-progs,
   with main as its default branch. Make small verified commits with the
   required American English messages and measured optimization effects.
   Keep a linear Change-Id chain and obey the hooks; never use --no-verify.
   Rechain when required after rebasing. Push regularly to the user's fork
   and never submit assignment solutions to the instructor's upstream.
   Before final delivery, update the harness and emulator, rerun make update
   and make grade, and verify lint, chain, grade, and aggregate CI are green
   for the final main commit. Record the commit and evidence links.

6. Make the project transferable to another computer. Include the complete
   tracked sources, any generated implementation tables and their generators,
   necessary configuration, dependency versions, setup/build/test commands,
   final commit, and a file checklist. Document what must be installed or
   rebuilt after a fresh full clone and how to reproduce the grading and
   measurements. Avoid machine-specific absolute paths in portable setup
   instructions. Do not bundle credentials, caches, or local account secrets.
   Cover native Apple Silicon and Intel macOS setup, modern Bash and GNU make,
   the pinned formatter, architecture-matched cross-toolchain archives and
   checksums. Distinguish documented setup from actual execution on that Mac.
   This handoff covers the assignment project and report, not CodeTrial setup.

7. Deliver the fork URL, local project path, final verification results,
   performance evidence, and transfer instructions. Clearly distinguish
   verified results from remaining work. Do not mark the goal complete until
   every in-scope requirement above has authoritative evidence and the report
   passes a content, number, and link audit. The user's independently managed
   CodeTrial work and online publication do not block this scoped project goal.

## Independent-review follow-up

The November 4 writing revision must also follow the user's HW1 and selected
classmate report: keep concrete changes, short explained excerpts, and results
visible; use HackMD `:::spoiler` for longer proofs, complete tables, and logs.
The subsequent concise revision targets approximately 4,000 words including
tables and references, removing repeated prose rather than only hiding it.
Keep the HW1 AI-use bullet wording and accurately disclose this assignment's
additional implementation and testing assistance in a short sentence.
The subsequent layout request uses a lab-report sequence: short annotated
implementation excerpt, actual execution output, local compiler comparison,
and a concise observation. Reduce repeated citations and retain three main
reference entries, with evidence links folded. Preserve measured data and
clearly distinguish archived screenshots from fresh execution transcripts.
Include genuine screenshots from this project's verification, identify their
source revision, and retain all current assignment requirements. Do not reuse
the classmate's code, measurements, outdated flags, or toolchain claims.

The November 3 review of `1b034e8056f15bd1b2deb3f3d6a724c1da339cf0`
identified one P2 report gap: Fixed had a theoretical error bound and exact
contract comparisons, but no measured maximum quantization error against an
unrounded host reference. It did not identify a proven implementation bug.
The review's 97/100 assessment is unofficial and distinct from the instructor's
100/100 automated program score.

Close this gap by measuring exact integer residuals for directed positive and
negative midpoint cases and reproducible random inputs. Report the maximum
absolute error, a witness, sample counts, and the exclusion of wrapping cases
from the small-error bound. Preserve bit-exact checks for those wrapping cases.
Track source, sanitizer logs, and reproduction commands; include the measured
table and witness residuals in the report audit. Preserve all existing program
behavior and performance evidence. Record the review, response, and independent
recheck in HW2_REVIEW.md. Deliver only after the recheck and final fork CI pass.

Continue autonomous work on these requirements. Request user input only when
information or an interactive action is actually required. Keep the active
goal open while in-scope work remains and do not reduce its success criteria
to fit a single turn.
