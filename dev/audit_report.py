#!/usr/bin/env python3
"""Check report tables, source excerpts, fixed references, and SVG artifacts.

Use --update-tables after intentionally changing the measured dataset.
The ordinary invocation only reads files and verifies the saved report.
--html additionally creates an ignored HTML preview with markdown-it-py.
"""
import argparse
import csv
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "HW2_REPORT.md"
CODE = "dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae"
DATA = "dfe705594fe6105f59158e184ca3a884b1338673"
REVIEW = "308e139f6f1b4c4bde1e27c9b1e5adf2f5fa2b29"
SETUP = REVIEW
REPO = "https://github.com/EricHuang0302/arch2026-progs"


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True)


def table(headers, rows):
    return ("| " + " | ".join(headers) + " |\n|" + "|".join("---" for _ in headers)
            + "|\n" + "\n".join("| " + " | ".join(map(str, row)) + " |" for row in rows))


def measured_tables():
    with (ROOT / "measurements/report-final/cycles.csv").open(newline="") as stream:
        records = list(csv.DictReader(stream))
    cycles, speedups = [], []
    for program, function in sorted({(r["prog"], r["fn"]) for r in records}):
        selected = [r for r in records if (r["prog"], r["fn"]) == (program, function)]
        c = {r["opt"]: float(r["per_call"]) for r in selected if r["impl"] == "c"}
        a = {r["opt"]: float(r["per_call"]) for r in selected if r["impl"] == "asm"}
        for r in selected:
            cost = (int(r["cycles"]) - int(r["overhead"])) / int(r["n"])
            assert abs(cost - float(r["per_call"])) < 1e-9
        best = min(c, key=c.get)
        label = program + "." + function
        cycles.append([label, *[f"{c[o]:.4f}" for o in ("O1", "O2", "O3", "Os")],
                       f"{a['O2']:.4f}"])
        speedups.append([label, "-" + best, f"{c[best]:.4f}", f"{a[best]:.4f}",
                         f"{c[best] / a[best]:.2f}×"])
    with (ROOT / "measurements/report-final/footprint.csv").open(newline="") as stream:
        footprints = list(csv.DictReader(stream))
    sizes = [[r["prog"], r["variant"], *[r[s] for s in (".text", ".rodata", ".data", ".bss")]]
             for r in footprints]
    return {
        "CYCLE": table(["Function", "C -O1", "C -O2", "C -O3", "C -Os", "Assembly @ -O2"], cycles),
        "SPEEDUP": table(["Function", "Best C level", "C cost", "Assembly in same ELF", "C / assembly"], speedups),
        "FOOTPRINT": table(["Program", "Build", ".text", ".rodata", ".data", ".bss"], sizes),
        "BUDGET": budget_table(),
        "FIXED_ERROR": fixed_error_table(),
    }


def fixed_error_table():
    path = ROOT / "measurements/review-2026-11-03/numeric-reference.log"
    text = path.read_text()
    records = re.findall(
        r"fixed error ([\w-]+): tested=(\d+) nonwrapping=(\d+) wrapped=(\d+) "
        r"max_residual=(\d+)/2\^40 max_abs_error=([\de+.-]+) "
        r"at raw a=(-?\d+) b=(-?\d+) result=(-?\d+)", text)
    assert [r[0] for r in records] == ["directed", "bounded", "full-range"]
    expected = {"directed": (169, 163, 6), "bounded": (100000, 100000, 0),
                "full-range": (100000, 402, 99598)}
    labels = {"directed": "Directed edges and ties", "bounded": "Bounded random",
              "full-range": "Full-range random"}
    rows = []
    for group, count, normal, wrapped, residual, error, a, b, result in records:
        assert tuple(map(int, (count, normal, wrapped))) == expected[group]
        assert int(count) == int(normal) + int(wrapped)
        product = int(a) * int(b)
        assert int(result) == (product + (1 << 19)) // (1 << 20)
        assert -(1 << 31) <= int(result) < (1 << 31)
        assert int(residual) == abs(int(result) * (1 << 20) - product)
        assert 0 <= int(residual) <= (1 << 19)
        assert error == format(int(residual) / (1 << 40), ".15g")
        if group in ("directed", "bounded"):
            assert int(residual) == (1 << 19)
        rows.append([labels[group], f"{int(count):,}", f"{int(normal):,}",
                     f"{int(wrapped):,}", f"`{error}`"])
    assert "fixed ties: raw (3,524288)->2 (-3,524288)->-1;" in text
    return table(["Host input group", "Pairs checked", "Nonwrapping", "Wrapping",
                  "Largest absolute error, real units"], rows)


def budget_table():
    maxima = {}
    for program, variant in (("bf16", "narrow_guard"), ("gcd", "fallthrough"),
                             ("fixed", "shorter_compact")):
        for opt in ("O1", "O2", "O3", "Os"):
            path = ROOT / f"measurements/optimization/{program}-{variant}-{opt}.log"
            for line in path.read_text().splitlines():
                if "@@perf" not in line:
                    continue
                version = "asm" if f"{program}_asm_perf" in line else "c"
                for function, count in re.findall(r"(\w+)=(\d+)", line):
                    key = program, version, function
                    maxima[key] = max(maxima.get(key, 0), int(count))
    definitions = {}
    for program in ("rubik", "bf16", "gcd", "fixed"):
        definitions.update({name: int(value) for name, value in re.findall(
            r"#define\s+(\w+)\s+(\d+)u", (ROOT / f"src/{program}/{program}.h").read_text())})
    rows = []
    for program, function, c_name, a_name in (
        ("bf16", "from_f32", "BF16_C_F32_INSN_BUDGET", "BF16_F32_INSN_BUDGET"),
        ("bf16", "mul", "BF16_C_MUL_INSN_BUDGET", "BF16_MUL_INSN_BUDGET"),
        ("bf16", "sqrt", "BF16_C_SQRT_INSN_BUDGET", "BF16_SQRT_INSN_BUDGET"),
        ("gcd", "worst", "GCD_C_INSN_BUDGET", "GCD_INSN_BUDGET"),
        *[("fixed", name, f"FIXED_C_{name.upper().replace('_', '')}_INSN_BUDGET",
           f"FIXED_{name.upper().replace('_', '')}_INSN_BUDGET")
          for name in ("op1", "op2", "op3", "q_mul")],
    ):
        label = program + "." + ("gcd" if function == "worst" else function)
        rows.append([label, definitions[c_name], maxima[program, "c", function],
                     definitions[a_name], maxima[program, "asm", function]])
    c_max, a_max = 0, 0
    for path in (ROOT / "measurements/report-final").glob("rubik-diameter-*.log"):
        match = re.search(r"diameter checked=2644 c=(\d+) at \w+ asm=(\d+)", path.read_text())
        assert match
        c_max, a_max = max(c_max, int(match[1])), max(a_max, int(match[2]))
    rows.append(["rubik.solve", definitions["RUBIK_C_INSN_BUDGET"], c_max,
                 definitions["RUBIK_INSN_BUDGET"], a_max])
    return table(["Function", "C budget", "Largest recorded C call", "Assembly budget",
                  "Largest recorded assembly call"], rows)


def references():
    links = {
        "assignment": "https://hackmd.io/@sysprog/2026-arch-homework2",
        "fork": REPO,
        "upstream": "https://github.com/sysprog21/arch2026-progs",
        "line-note": "https://hackmd.io/@sysprog/line-draw-fixed-point",
        "sine-note": "https://hackmd.io/@sysprog/bf16-sine",
        "source-ci": REPO + "/actions/runs/41432002649",
        "handoff": REPO + f"/blob/{SETUP}/HANDOFF.md",
        "numeric-review": REPO + f"/tree/{REVIEW}/measurements/review-2026-11-03",
        "binary-gcd": "https://xlinux.nist.gov/dads/HTML/binaryGCD.html",
        "korf": "https://cdn.aaai.org/AAAI/1997/AAAI97-109.pdf",
        "tpu-bf16": "https://docs.cloud.google.com/tpu/docs/bfloat16",
        "cmsis-multiply": "https://arm-software.github.io/CMSIS-DSP/v1.14.2/group__BasicMult.html",
        "asm-manual": "https://github.com/riscv-non-isa/riscv-asm-manual/blob/7ae3a91c7baf12b1ac58d27885288a2450e6d2c6/src/asm-manual.adoc",
        "isa-manual": "https://github.com/riscv/riscv-isa-manual/releases/tag/20240411",
        "xpack-release": "https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/tag/v15.2.0-1",
    }
    code_paths = {"startup": "common/start.S", "perf": "common/perf.S", "bench": "common/bench.h"}
    for program in ("rubik", "bf16", "gcd", "fixed"):
        for suffix, extension in (("c", "c"), ("asm", "S"), ("h", "h")):
            code_paths[f"{program}-{suffix}"] = f"src/{program}/{program}.{extension}"
    data_paths = {
        "experiments": "mytests/optimization_experiments.py",
        "stress-source": "mytests/target_stress.c",
        "full-cube-source": "mytests/rubik_full_domain.c",
        "diameter-source": "mytests/rubik_diameter.c",
        "numeric-reference": "mytests/numeric_reference.c",
        "rubik-generator": "mytests/generate_rubik_tables.c",
        "bf16-generator": "mytests/generate_bf16_table.py",
    }
    for key, path in code_paths.items():
        links[key] = REPO + f"/blob/{CODE}/{path}"
    for key, path in data_paths.items():
        revision = REVIEW if key == "numeric-reference" else DATA
        links[key] = REPO + f"/blob/{revision}/{path}"
    for key, path in {
        "benchmark-sources": "tests",
        "lab-logs": "measurements/lab2",
        "optimization-data": "measurements/optimization",
        "current-data": "measurements/report-final",
        "earlier-measurements": "measurements/initial",
        "error-data": "dev/plot_bf16_error.py",
        "all-disassembly": "measurements/report-disassembly",
    }.items():
        links[key] = REPO + f"/tree/{DATA}/{path}" if key != "error-data" else REPO + f"/blob/{DATA}/{path}"
    for program in ("rubik", "bf16", "gcd", "fixed"):
        links[program + "-dis"] = REPO + f"/blob/{DATA}/measurements/report-disassembly/{program}-O2.dis"
    return links


def canonical(source):
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
    source = re.sub(r"(?m)#.*$", "", source)
    return re.sub(r"\s+", "", source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update-tables", action="store_true")
    parser.add_argument("--html", action="store_true")
    args = parser.parse_args()
    text = REPORT.read_text(encoding="utf-8")
    for key, body in measured_tables().items():
        pattern = rf"(<!-- BEGIN {key} TABLE -->)(.*?)(<!-- END {key} TABLE -->)"
        match = re.search(pattern, text, flags=re.S)
        assert match, f"Missing table marker {key}"
        if args.update_tables:
            text = re.sub(pattern, lambda m: m[1] + "\n" + body + "\n" + m[3], text, flags=re.S)
        else:
            assert match[2].strip() == body, f"Table differs from CSV: {key}"
    links = references()
    definitions = "\n".join(f"[{key}]: {url}" for key, url in links.items()) + "\n"
    if args.update_tables:
        text = text.replace("EVIDENCE_SHA", DATA)
        text = re.sub(r"\n\[assignment\]:.*\Z", "", text, flags=re.S)
        text = text.rstrip() + "\n\n" + definitions
        REPORT.write_text(text, encoding="utf-8", newline="\n")
    saved = dict(re.findall(r"(?m)^\[([^\]]+)\]:\s+(\S+)", text))
    for key, url in links.items():
        assert saved.get(key) == url, f"Missing or incorrect reference {key}"
    used = re.findall(r"\[[^\]\n]+\]\[([^\]\n]+)\]", text)
    assert not set(used) - saved.keys(), f"Undefined references: {set(used) - saved.keys()}"
    assert not re.search(r"\b(?:TODO|TBD|FIXME|EVIDENCE_SHA)\b", text)
    assert not re.search(r"[\u3400-\u9fff]", text), "Report must be in English"
    sources = []
    for program in ("rubik", "bf16", "gcd", "fixed"):
        for extension in ("c", "S"):
            sources.append(canonical(git("show", f"{CODE}:src/{program}/{program}.{extension}")))
    sources.append(canonical(git("show", f"{CODE}:common/perf.S")))
    for path in (ROOT / "measurements/report-disassembly").glob("*.dis"):
        lines = re.findall(r"(?m)^\s*[0-9a-f]+:\s+[0-9a-f]{8}\s+([^\n]+)", path.read_text())
        sources.append(canonical("\n".join(lines)))
    for language, snippet in re.findall(r"```(c|asm)\n(.*?)\n```", text, flags=re.S):
        assert any(canonical(snippet) in source for source in sources), f"Unmatched {language} excerpt: {snippet[:60]}"
    for url in list(saved.values()) + re.findall(r"!\[[^\]]*\]\(([^)]+)\)", text):
        match = re.search(r"/(?:blob|tree)/(\w{40})/(.+)", url)
        if "raw.githubusercontent.com/EricHuang0302/arch2026-progs/" in url:
            match = re.search(r"arch2026-progs/(\w{40})/(.+)", url)
        if match and (url.startswith(REPO) or "raw.githubusercontent.com/EricHuang0302/arch2026-progs/" in url):
            git("cat-file", "-e", match[1] + ":" + match[2])
    figures = re.findall(r"!\[[^\]]*\]\([^)]*/(measurements/report-final/[^)]+\.svg)\)", text)
    assert len(figures) == 4
    for path in figures:
        ET.parse(ROOT / path)
    screenshots = re.findall(r"!\[[^\]]*\]\([^)]*/(measurements/screenshots/[^)]+\.jpg)\)", text)
    assert len(screenshots) == 2
    for path in screenshots:
        data = (ROOT / path).read_bytes()
        assert data.startswith(b"\xff\xd8\xff") and data.endswith(b"\xff\xd9")
    assert text.count(":::spoiler ") == len(re.findall(r"(?m)^:::$", text))
    if args.html:
        import html
        from markdown_it import MarkdownIt
        preview_text = re.sub(r"(?m)^:::spoiler (.+)$",
                              lambda m: "<details><summary>" + html.escape(m[1]) + "</summary>\n",
                              text)
        preview_text = re.sub(r"(?m)^:::$", "</details>", preview_text)
        rendered = MarkdownIt("commonmark").enable("table").render(preview_text)
        preview = ROOT / "build/report-preview.html"
        preview.parent.mkdir(exist_ok=True)
        preview.write_text("<!doctype html><meta charset='utf-8'><title>HW2 report preview</title>"
                           "<style>body{max-width:1100px;margin:40px auto;font:17px/1.6 system-ui;padding:0 24px}"
                           "img{max-width:100%}table{border-collapse:collapse;font-size:14px}td,th{border:1px solid #bbb;padding:5px 9px}"
                           "pre{overflow:auto;background:#f4f4f4;padding:16px}</style>" + rendered,
                           encoding="utf-8")
        print("HTML preview:", preview)
    print(f"PASS: 5 measured tables, {len(used)} reference uses, {len(figures)} SVG figures, 2 JPEG screenshots, balanced spoilers, source excerpts and pinned file targets")
    print("English report words:", len(text.split()))


if __name__ == "__main__":
    main()
