#!/usr/bin/env python3
"""
logic_view.py — plot Digilent WaveForms logic analyzer CSV exports.

Usage:
  python logic_view.py /path/to/data0001.csv
  python logic_view.py /path/to/data0001.csv --t0 -5e-6 --t1 5e-6
  python logic_view.py /path/to/data0001.csv --sharey
"""

# Logic Analyzer Use: https://digilent.com/reference/test-and-measurement/guides/waveforms-logic-analyzer

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def find_time_column(columns: list[str]) -> str:
    # Common variants: "Time (s)", "Time(s)", "Time", etc.
    for c in columns:
        cl = c.strip().lower()
        if cl.startswith("time"):
            return c
    raise ValueError(f"Could not find a time column. Columns: {columns}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path, help="Logic analyzer CSV file")
    ap.add_argument("--t0", type=float, default=None, help="Start time (seconds) for zoom")
    ap.add_argument("--t1", type=float, default=None, help="End time (seconds) for zoom")
    ap.add_argument("--sharey", action="store_true", default=True, help="Share y-axis across subplots")
    args = ap.parse_args()

    if not args.csv.exists():
        print(f"File not found: {args.csv}", file=sys.stderr)
        return 2

    # Digilent WaveForms exports metadata lines starting with '#'
    df = pd.read_csv(args.csv, comment="#")

    if df.empty:
        print("Parsed dataframe is empty. Is this a valid WaveForms CSV?", file=sys.stderr)
        return 2

    time_col = find_time_column(list(df.columns))
    t = df[time_col].to_numpy()

    # All other columns are treated as digital channels (0/1)
    chan_cols = [c for c in df.columns if c != time_col]

    if not chan_cols:
        print("No channel columns found (only time column present).", file=sys.stderr)
        return 2

    # Optional time-window zoom
    if args.t0 is not None or args.t1 is not None:
        t0 = -np.inf if args.t0 is None else args.t0
        t1 =  np.inf if args.t1 is None else args.t1
        mask = (t >= t0) & (t <= t1)
        df = df.loc[mask].reset_index(drop=True)
        t = df[time_col].to_numpy()

    # Plot: one subplot per channel (best readability)
    n = len(chan_cols)
    fig, axes = plt.subplots(
        nrows=n, ncols=1, figsize=(12, max(3, 1.6 * n)),
        sharex=True, sharey=args.sharey
    )
    if n == 1:
        axes = [axes]

    for ax, col in zip(axes, chan_cols):
        y = df[col].to_numpy(dtype=float)

        # Ensure 0/1-like values; if already ints it's fine.
        # Use step plot (digital waveform).
        ax.step(t, y, where="post")
        ax.set_ylim(-0.25, 1.25)
        ax.set_yticks([0, 1])
        ax.set_ylabel(col)
        ax.grid(True, which="both", axis="both", alpha=0.3)

    axes[-1].set_xlabel("Time (s)")
    fig.suptitle(f"Logic Analyzer: {args.csv.name}", y=0.98)
    fig.tight_layout()
    plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
