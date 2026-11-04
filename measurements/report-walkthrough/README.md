# Per-program execution output

Recorded November 4, 2026 (Asia/Taipei), after source revision
`f4a0166` was checked out. Implementation files remain unchanged from
`dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae`.

These are complete, unedited stdout logs from executing the four existing
`-O2` ELFs with the recorded native Windows rv32emu build. Each process
returned zero. The emulator revision is
`158b57e9eab99d59e01141cd0e477b06b44b8fe3`; the compiler is xPack RISC-V GCC
15.2.0-1. The previously documented Windows host patch is still in use.

After the normal build, from the repository root in MSYS2 Bash:

```sh
for program in rubik bf16 gcd fixed; do
    third_party/rv32emu/build/rv32emu.exe \
        "build/$program/O2/$program.elf" > "$program-O2.log"
done
```

On Linux/macOS, the normal executable has no `.exe` suffix. Configure and
rebuild following HANDOFF.md rather than copying Windows host binaries.

The report uses short, contiguous excerpts from these logs. `ok` lines and
summaries are the instructor harness's actual output. They have not been
renamed to `PASSED`. Each summary includes shared runtime checks.
Corrected benchmark costs are computed from `@@bench` lines using
`(cycles-overhead)/n`; they match the report's recorded course dataset.
