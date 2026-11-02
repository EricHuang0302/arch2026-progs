#!/usr/bin/env python3
"""Plot the verified contract's two square-root cases and normal products."""
import csv
import math
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

OUT = Path(__file__).resolve().parents[1] / "measurements/report-final"
plt.rcParams["svg.fonttype"] = "none"
records = []
figure, axes = plt.subplots(1, 2, figsize=(12, 4.5))
for odd in (True, False):
    errors = []
    for significand in range(128, 256):
        mantissa = significand if odd else 2 * significand
        root = math.isqrt(128 * (mantissa + 1) - 1)
        error = root / math.sqrt(128 * mantissa) - 1
        errors.append(100 * error)
        records.append(dict(operation="sqrt", a=significand, b="",
                            exponent_parity=int(odd), relative_error=error))
    axes[0].plot(range(128, 256), errors,
                 label="E odd" if odd else "E even", linewidth=1)
axes[0].axhline(0, color="black", linewidth=0.6)
axes[0].set_xlabel("Input significand integer M = 128 + fraction")
axes[0].set_ylabel("Signed relative error (%)")
axes[0].set_title("Square root: both biased-exponent parities")
axes[0].legend()

grid = []
for a in range(128, 256):
    row = []
    for b in range(128, 256):
        product = a * b
        shift = 8 if product >= 32768 else 7
        kept = (product >> shift) << shift
        error = kept / product - 1
        row.append(100 * error)
        records.append(dict(operation="multiply", a=a, b=b,
                            exponent_parity="", relative_error=error))
    grid.append(row)
heat = axes[1].imshow(grid, origin="lower", interpolation="nearest",
                      extent=(127.5, 255.5, 127.5, 255.5), aspect="auto",
                      cmap="viridis")
axes[1].set_xlabel("Significand integer of b")
axes[1].set_ylabel("Significand integer of a")
axes[1].set_title("Multiply: all 16,384 normal significand pairs")
figure.colorbar(heat, ax=axes[1], label="Signed relative error (%)")
figure.tight_layout()
figure.savefig(OUT / "bf16-error.svg")
figure.savefig(OUT / "bf16-error-preview.png", dpi=120)
plt.close(figure)
with (OUT / "bf16-error.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=list(records[0]))
    writer.writeheader()
    writer.writerows(records)
for operation in ("sqrt", "multiply"):
    maximum = max(abs(r["relative_error"]) for r in records
                  if r["operation"] == operation)
    print(f"{operation} maximum relative error: {100 * maximum:.9f}%")
