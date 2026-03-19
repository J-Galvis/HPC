from __future__ import annotations

import os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.figure import Figure

MATRIX_SIZES = [500, 1000, 1300, 1600, 2000, 2300, 2600, 3000, 3300, 3600, 4000]
COLORS = [
    "#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf",
]


def _load_csv(path: str) -> list[list[float]]:
    rows = []
    with open(path, "r") as f:
        for line in f:
            stripped = line.strip()
            if not stripped:
                continue
            values = [float(x) for x in stripped.rstrip(",").split(",") if x.strip()]
            if values:
                rows.append(values)
    return rows


def plot_csv_comparison(
    csv_paths: list[str],
    labels: list[str],
    title: str = "Matrix Multiplication Performance",
    xlabel: str = "Matrix Size",
    ylabel: str = "Time (s)",
    output_path: str | None = None,
) -> Figure:
    if len(csv_paths) != len(labels):
        raise ValueError(f"csv_paths ({len(csv_paths)}) and labels ({len(labels)}) must have the same length")

    for path in csv_paths:
        if not os.path.exists(path):
            raise FileNotFoundError(f"CSV file not found: {path}")

    if len(csv_paths) > len(COLORS):
        raise ValueError(f"Maximum {len(COLORS)} datasets supported")

    fig, ax = plt.subplots(figsize=(10, 6))

    for idx, (path, label) in enumerate(zip(csv_paths, labels)):
        rows = _load_csv(path)
        if not rows:
            print(f"Warning: empty CSV file '{path}', skipping")
            continue

        for row in rows:
            if len(row) != len(MATRIX_SIZES):
                print(f"Warning: row length {len(row)} != expected {len(MATRIX_SIZES)} in '{path}', skipping row")
                continue
            ax.plot(MATRIX_SIZES, row, marker="o", linestyle="-",
                    color=COLORS[idx], linewidth=1.2, markersize=3, alpha=0.7)

        mean_values = np.mean(rows, axis=0)
        ax.plot(MATRIX_SIZES, mean_values, marker="o", linestyle="-",
                color=COLORS[idx], linewidth=2.2, markersize=5,
                label=label, zorder=10)

    ax.set_xlabel(xlabel, fontsize=12)
    ax.set_ylabel(ylabel, fontsize=12)
    ax.set_title(title, fontsize=14, fontweight="bold")
    ax.set_xticks(MATRIX_SIZES)
    ax.set_xticklabels([str(s) for s in MATRIX_SIZES], rotation=45, ha="right")
    ax.grid(which="major", linestyle="--", alpha=0.5)
    ax.legend(loc="upper left", fontsize=10, framealpha=0.9)
    fig.tight_layout()

    if output_path:
        fig.savefig(output_path, dpi=150, bbox_inches="tight")

    return fig


if __name__ == "__main__":
    fig = plot_csv_comparison(
        csv_paths=[
            "../stats/kali/20260317_225936.done/secuential.csv",
            "../stats/kali/20260317_225936.done/memory.csv",
            "../stats/kali/20260317_225936.done/threads4.csv",
            "../stats/kali/20260317_225936.done/mutiprocessing4.csv",
        ],
        labels=["Sequential", "Memory-Optimized", "Threads (4)", "Multiprocessing (4)"],
        title="Matrix Multiplication Performance (kali)",
        output_path="output.png",
    )
    print("Saved to Graphics/output.png")



