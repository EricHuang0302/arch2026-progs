"""Render tracked vector charts from the instructor's cycle/size CSV files."""

import argparse
import csv
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--directory", default="measurements/final")
OUT = ROOT / parser.parse_args().directory
VARIANTS = ["O1", "O2", "O3", "Os", "asm"]
plt.rcParams["svg.fonttype"] = "none"

with (OUT / "cycles.csv").open(newline="") as file:
    rows = list(csv.DictReader(file))
keys = sorted({(r["prog"], r["fn"]) for r in rows})
data = {}
for key in keys:
    selected = [r for r in rows if (r["prog"], r["fn"]) == key]
    values = {r["opt"]: float(r["per_call"]) for r in selected if r["impl"] == "c"}
    values["asm"] = next(float(r["per_call"]) for r in selected
                         if r["impl"] == "asm" and r["opt"] == "O2")
    data[key] = values

figure, axis = plt.subplots(figsize=(12, 5))
for i, variant in enumerate(VARIANTS):
    axis.bar([j + (i - 2) * 0.16 for j in range(len(keys))],
             [data[k][variant] for k in keys], 0.16, label=variant)
axis.set_yscale("log")
axis.set_ylabel("Guest instructions per call (loop baseline removed)")
axis.set_xticks(range(len(keys)), [".".join(k) for k in keys],
                  rotation=30, ha="right")
axis.set_title("Compiler optimization levels and hand-written RV32I")
axis.legend(ncol=5)
figure.tight_layout()
figure.savefig(OUT / "cycles-per-call.svg")
figure.savefig(OUT / "cycles-per-call-preview.png", dpi=120)
plt.close(figure)

ratios = []
for key in keys:
    best = min(VARIANTS[:4], key=lambda v: data[key][v])
    comparison = [r for r in rows if (r["prog"], r["fn"]) == key and r["opt"] == best]
    c = next(float(r["per_call"]) for r in comparison if r["impl"] == "c")
    assembly = next(float(r["per_call"]) for r in comparison if r["impl"] == "asm")
    ratios.append(c / assembly)
figure, axis = plt.subplots(figsize=(11, 4.5))
axis.bar(range(len(keys)), ratios, color="#c55a11")
axis.axhline(1, color="black", linewidth=0.8)
axis.set_xticks(range(len(keys)), [".".join(k) for k in keys],
                  rotation=30, ha="right")
axis.set_ylabel("Best C instructions / assembly instructions")
axis.set_title("Assembly speedup")
figure.tight_layout()
figure.savefig(OUT / "speedup.svg")
figure.savefig(OUT / "speedup-preview.png", dpi=120)
plt.close(figure)

with (OUT / "footprint.csv").open(newline="") as file:
    footprint = list(csv.DictReader(file))
programs = ["rubik", "bf16", "gcd", "fixed"]
figure, axis = plt.subplots(figsize=(10, 4.5))
for i, variant in enumerate(VARIANTS):
    points = []
    for program in programs:
        row = next(r for r in footprint if r["prog"] == program
                   and r["variant"] == ("asm" if variant == "asm" else "-" + variant))
        points.append(sum(int(row[s]) for s in (".text", ".rodata", ".data", ".bss")))
    axis.bar([j + (i - 2) * 0.16 for j in range(len(programs))],
             points, 0.16, label=variant)
axis.set_yscale("log")
axis.set_xticks(range(len(programs)), programs)
axis.set_ylabel("Allocated implementation bytes (harness excluded)")
axis.set_title("Implementation footprint; exact sections are in footprint.csv")
axis.legend(ncol=5)
figure.tight_layout()
figure.savefig(OUT / "footprint.svg")
figure.savefig(OUT / "footprint-preview.png", dpi=120)
plt.close(figure)
