#!/usr/bin/env python3
"""Controlled assembly experiments against an immutable submitted baseline.

Run in a POSIX shell with the course toolchain on PATH. No instructor file or
implementation is changed. Each variant links the same C, main, test, and
runtime objects, executes the whole course suite, and preserves its output.
"""

import argparse
import csv
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = "382db5609e3d66a1b7d15f23396b77eae024c7d8"
OUT = ROOT / "measurements/optimization"
BUILD = ROOT / "build/optimization"
OPTS = ("O1", "O2", "O3", "Os")


def run(args, **kwargs):
    result = subprocess.run(args, cwd=ROOT, text=True, capture_output=True,
                            check=True, **kwargs)
    return result.stdout


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"Expected one occurrence of {old!r}")
    return text.replace(old, new, 1)


def variants(program, baseline):
    yield "baseline", baseline
    if program == "bf16":
        yield "narrow_guard", replace_once(
            baseline,
            "    li      t0, 0x7f800000\n",
            "    # A discarded half below 0x8000 never rounds upward.\n"
            "    slli    t0, a0, 16\n"
            "    bgez    t0, .Lf32_top\n"
            "    li      t0, 0x7f800000\n")
    elif program == "gcd":
        fallthrough = replace_once(
            baseline,
            "    beqz    t1, .Lgcd_a_shift\n    j       .Lgcd_b\n",
            "    bnez    t1, .Lgcd_b\n")
        yield "fallthrough", fallthrough
        yield "no_byte_skip", replace_once(
            baseline,
            "    andi    t1, a0, 255\n"
            "    bnez    t1, .Lgcd_a_test\n"
            "    srli    a0, a0, 8\n"
            "    j       .Lgcd_a_shift\n",
            "    j       .Lgcd_a_test\n")
        yield "common_byte", replace_once(
            fallthrough, ".Lgcd_common:\n",
            ".Lgcd_common:\n"
            "    or      t1, a0, a1\n"
            "    andi    t1, t1, 255\n"
            "    bnez    t1, .Lgcd_common_bit\n"
            "    srli    a0, a0, 8\n"
            "    srli    a1, a1, 8\n"
            "    addi    t0, t0, 8\n"
            "    j       .Lgcd_common\n"
            ".Lgcd_common_bit:\n")
    elif program == "fixed":
        magnitude = replace_once(
            baseline,
            "    srai    t1, a0, 31\n"
            "    and     a4, t1, a1\n"
            "    srai    t1, a1, 31\n"
            "    and     t1, t1, a0\n"
            "    add     a4, a4, t1\n",
            "    xor     a4, a0, a1\n"
            "    srai    t1, a0, 31\n"
            "    xor     a0, a0, t1\n"
            "    sub     a0, a0, t1\n"
            "    srai    t1, a1, 31\n"
            "    xor     a1, a1, t1\n"
            "    sub     a1, a1, t1\n")
        magnitude = replace_once(
            magnitude, "    sub     a3, a3, a4\n",
            "    bgez    a4, .Lq_positive\n"
            "    seqz    t1, a2\n"
            "    neg     a2, a2\n"
            "    not     a3, a3\n"
            "    add     a3, a3, t1\n"
            ".Lq_positive:\n")
        magnitude = replace_once(
            magnitude,
            "    # a4 holds the signed high-word correction, t1/t2 are carry scratch.\n",
            "    # The sign bit of a4 selects full-product negation; t1/t2 are scratch.\n")
        yield "magnitude", magnitude
        shorter = replace_once(
            magnitude, "    li      a2, 0\n",
            "    bgeu    a0, a1, .Lq_ordered\n"
            "    mv      t1, a0\n"
            "    mv      a0, a1\n"
            "    mv      a1, t1\n"
            ".Lq_ordered:\n"
            "    li      a2, 0\n")
        yield "shorter_magnitude", shorter
        branch = replace_once(
            shorter,
            "    srai    t1, a0, 31\n"
            "    xor     a0, a0, t1\n"
            "    sub     a0, a0, t1\n"
            "    srai    t1, a1, 31\n"
            "    xor     a1, a1, t1\n"
            "    sub     a1, a1, t1\n",
            "    bgez    a0, .Lq_abs_b\n"
            "    neg     a0, a0\n"
            ".Lq_abs_b:\n"
            "    bgez    a1, .Lq_abs_done\n"
            "    neg     a1, a1\n"
            ".Lq_abs_done:\n")
        yield "shorter_branch", branch
        yield "shorter_compact", replace_once(
            branch,
            "    add     t2, a2, a0\n"
            "    sltu    t1, t2, a2\n"
            "    mv      a2, t2\n",
            "    add     a2, a2, a0\n"
            "    sltu    t1, a2, a0\n")
    elif program == "rubik":
        compact = replace_once(
            baseline,
            "    li      t3, 32767\n    and     t0, a0, t3\n",
            "    slli    t0, a0, 17\n    srli    t0, t0, 17\n")
        yield "compact_key", compact
        hash_b = ("    srli    t2, t0, 5\n"
                  "    srli    t3, t0, 17\n"
                  "    xor     t2, t2, t3\n"
                  "    xor     t2, t2, t0\n")
        scale_b = "    slli    t2, t2, 18\n    srli    t2, t2, 16\n"
        for name, source in (("lazy_hash", baseline),
                             ("lazy_compact", compact)):
            source = replace_once(source, hash_b, "")
            source = replace_once(source, scale_b, "")
            source = replace_once(
                source, "    add     t4, t3, t2\n",
                hash_b.replace("t3, t0", "t4, t0")
                .replace("t2, t3", "t2, t4") + scale_b +
                "    add     t4, t3, t2\n")
            yield name, source
        start = baseline.index("    # .Lnear leaves the injective key")
        end = baseline.index(".Ldepth_loop:\n", start)
        yield "all_prefixes", baseline[:start] + "    li      s5, 1\n" + baseline[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", choices=("rubik", "bf16", "gcd", "fixed"),
                        action="append")
    args = parser.parse_args()
    programs = args.program or ["rubik", "bf16", "gcd", "fixed"]
    prefix = os.environ.get("CROSS_COMPILE") or run(
        ["bash", "scripts/toolchain.sh", "--detect"]).strip()
    emulator = os.environ.get("EMU", "third_party/rv32emu/build/rv32emu")
    if not (ROOT / emulator).is_file() and (ROOT / (emulator + ".exe")).is_file():
        emulator += ".exe"
    run(["make", "elfs", f"CROSS_COMPILE={prefix}"])
    OUT.mkdir(parents=True, exist_ok=True)
    BUILD.mkdir(parents=True, exist_ok=True)
    (OUT / "sources").mkdir(exist_ok=True)
    records, sizes = [], []
    for program in programs:
        baseline = run(["git", "show", f"{BASE}:src/{program}/{program}.S"])
        for name, source in variants(program, baseline):
            tag = f"{program}-{name}"
            path = OUT / "sources" / (tag + ".S")
            path.write_text(source, encoding="utf-8", newline="\n")
            obj = BUILD / (tag + ".o")
            run([prefix + "as", "-march=rv32i_zicsr", "-mabi=ilp32",
                 "-g", str(path), "-o", str(obj)])
            sections = {section: 0 for section in (".text", ".rodata", ".data", ".bss")}
            for line in run([prefix + "size", "-A", str(obj)]).splitlines():
                fields = line.split()
                if len(fields) >= 2 and fields[0] in (".text", ".rodata", ".data", ".bss"):
                    sections[fields[0]] = int(fields[1])
            sizes.append(dict(program=program, variant=name, **sections))
            for opt in OPTS:
                elf = BUILD / f"{tag}-{opt}.elf"
                run([prefix + "ld", "-T", "common/linker.ld", "--no-warn-rwx-segments",
                     "-o", str(elf), str(obj),
                     *[f"build/{program}/{opt}/{part}.o" for part in ("impl", "main", "test")],
                     *[f"build/common/{part}.o" for part in
                       ("start", "perf", "rt", "print", "utest", "trt", "tutest")]])
                result = subprocess.run([emulator, str(elf)], cwd=ROOT, text=True,
                                        capture_output=True, timeout=120)
                (OUT / f"{tag}-{opt}.log").write_text(
                    result.stdout + result.stderr, encoding="utf-8", newline="\n")
                if result.returncode:
                    raise RuntimeError(f"{tag}-{opt} failed: {result.returncode}")
                for line in result.stdout.splitlines():
                    if line.startswith("@@bench "):
                        data = dict(part.split("=", 1) for part in line.split()[1:])
                        records.append(dict(
                            variant=name, **data,
                            per_call=(int(data["cycles"]) - int(data["overhead"])) / int(data["n"])))
            print(f"PASS {tag}: all four levels", flush=True)
    for name, rows in (("cycles", records), ("footprint", sizes)):
        suffix = "-" + "-".join(programs) if args.program else ""
        with (OUT / (name + suffix + ".csv")).open("w", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)


if __name__ == "__main__":
    main()
