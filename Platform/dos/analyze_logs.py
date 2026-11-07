#!/usr/bin/env python3
"""Summarize DOS Space Cadet gameplay/CI logs, including legacy flight formats.

Run on a PC, NOT on the Tandy, to keep game diagnostics fast. No dependencies.
Typical: python Platform/dos/analyze_logs.py --directory C:\CADET
"""
import argparse
from collections import Counter
from pathlib import Path
import re

INPUT_BITS = ((1, "Z"), (2, "/"), (4, "SPACE"), (8, "X"),
              (16, "A"), (32, "D"))
EVENTS = {0: "none", 1: "bumper", 2: "launch",
          3: "drain", 4: "kickback", 5: "tilt"}
LEFT = (78, 113, 145, 180)
RIGHT = (119, 155, 145, 180)


def parse_summary(text):
    """Parse INPUT.TXT / SMOKE.TXT without confusing 'input=6' with live keys."""
    first = next((line.strip() for line in text.splitlines() if line.strip()), "")
    if not first.startswith("PASS "):
        raise ValueError("Missing PASS completion marker")
    pairs = re.findall(r"\b([a-z_]+)=(\d+)\b", first)
    if len(pairs) != len(set(key for key, _ in pairs)):
        raise ValueError("Duplicate keys in completion marker")
    return {key: int(value) for key, value in pairs}


def parse_flight(text):
    """Return schema, ordered columns and numeric samples; handles old recordings."""
    schema = 1
    columns = None
    rows = []
    for number, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if line.startswith("#"):
            match = re.search(r"\bschema=(\d+)", line)
            if match:
                schema = int(match.group(1))
            continue
        if line.startswith("FRAME "):
            columns = line.split()
            if len(columns) != len(set(columns)):
                raise ValueError("Duplicate FLIGHT column on line " + str(number))
            continue
        if columns is None:
            # Older flight logs have an explanatory first line.
            if line.startswith("last "):
                continue
            raise ValueError("Missing flight header before line " + str(number))
        pieces = line.split()
        if len(pieces) != len(columns):
            raise ValueError("Flight column count mismatch at line " + str(number))
        try:
            row = dict(zip(columns, map(int, pieces)))
        except ValueError as exc:
            raise ValueError("Non-numeric flight row " + str(number)) from exc
        rows.append(row)
    if not columns or not rows:
        raise ValueError("No complete FLIGHT samples")
    if "XQ6" not in columns or "YQ6" not in columns:
        raise ValueError("Missing ball position fields")
    if any(b["FRAME"] < a["FRAME"] for a, b in zip(rows, rows[1:])):
        raise ValueError("Flight frames out of order (rollover or corrupt data)")
    return schema, columns, rows


def keys(mask):
    return "+".join(label for bit, label in INPUT_BITS if bit & mask) or "none"


def coords(row):
    return (row["XQ6"] / 64, row["YQ6"] / 64)


def summary_lines(rows, columns, maximum=15):
    lines = []
    first, last = rows[0], rows[-1]
    count = len(rows)
    elapsed = (last["FRAME"] - first["FRAME"]) / 18.2
    lines.append(f"Flight: {count} samples, frames {first['FRAME']}..{last['FRAME']}, "
                 f"{elapsed:.1f} seconds of visible history (only the last 256 frames).")
    if "INPUT" in columns:
        lines.append("Per-frame INPUT and flipper contact counters: AVAILABLE")
    else:
        lines.append("Per-frame INPUT and contact counters: NOT IN THIS LEGACY LOG. "
                     "INPUT.TXT's input= is an OR of ALL keys seen, not key state at drain.")
    transitions = []
    previous = None
    for row in rows:
        event = row.get("EVENT", 0)
        if event or (previous and row.get("BALLS") != previous.get("BALLS")):
            transitions.append(row)
        previous = row
    if transitions:
        lines.append("Events / ball changes:")
        for row in transitions[-maximum:]:
            x, y = coords(row)
            extra = (" keys=" + keys(row["INPUT"])) if "INPUT" in row else ""
            lines.append(f"  frame {row['FRAME']:>5} ({x:6.1f},{y:6.1f}) "
                         f"event={EVENTS.get(row.get('EVENT'), 'unknown')} "
                         f"balls={row.get('BALLS', '?')}{extra}")
    else:
        lines.append("No events recorded in this window.")
    if "LEFT_HIT" in columns and "RIGHT_HIT" in columns:
        before = rows[0]
        after = rows[-1]
        for side in ("LEFT", "RIGHT"):
            delta = after.get(side + "_HIT", 0) - before.get(side + "_HIT", 0)
            powered = after.get(side + "_POWER", 0) - before.get(side + "_POWER", 0)
            lines.append(f"{side} flipper contacts in retained window: "
                         f"{delta} total, {powered} powered")
    if "BORDER" in columns:
        delta = last.get("BORDER", 0) - first.get("BORDER", 0)
        lines.append(f"Top/side safety contacts in window: {delta}")
    if "LATE" in columns:
        lines.append(f"Late frames in window: {last['LATE'] - first['LATE']}")
    # Show the real end of the trajectory, not just an abstract event count.
    lines.append("Last 14 frames (x/y in pixels; vx/vy in Q6 units per step):")
    for row in rows[-14:]:
        x, y = coords(row)
        key_text = keys(row["INPUT"]) if "INPUT" in row else "unknown"
        extra = ""
        if "RIGHT_HIT" in row:
            extra = (" L/R contacts=" + str(row["LEFT_HIT"]) + "/" + str(row["RIGHT_HIT"]) +
                     " L/R powered=" + str(row["LEFT_POWER"]) + "/" + str(row["RIGHT_POWER"]))
        lines.append(f"  {row['FRAME']:>5} x={x:6.1f} y={y:6.1f} "
                     f"vx={row.get('VXQ6', 0):>4} vy={row.get('VYQ6', 0):>4} "
                     f"keys={key_text}{extra}")
    for region, name in ((LEFT, "left"), (RIGHT, "right")):
        found = [row for row in rows
                 if region[0] <= coords(row)[0] <= region[1] and
                 region[2] <= coords(row)[1] <= region[3] and
                 row.get("VYQ6", 0) > 0]
        if found:
            line = f"Descending ball visited {name} flipper region on {len(found)} recorded frames."
            if "INPUT" in columns:
                bit = 1 if name == "left" else 2
                pressed = sum(bool(row["INPUT"] & bit) for row in found)
                line += f" {pressed} of those frames reported that flipper key held."
            lines.append(line)
    return lines


def summarize(directory):
    lines = ["Space Cadet DOS logging diagnostic (PC-side report)",
             "INFO: TEST.LOG is DOS unit-test stdout; CORE.LOG and VIDEO.LOG "
             "are DOSBox-X startup/process logs, not gameplay traces.",
             "INFO: INPUT.TXT and SMOKE.TXT are end-of-run counters, "
             "not frame-by-frame evidence."]
    for filename in ("INPUT.TXT", "SMOKE.TXT"):
        path = directory / filename
        if path.exists():
            try:
                data = parse_summary(path.read_text(errors="replace"))
                pairs = " ".join(f"{k}={v}" for k, v in data.items())
                lines.append(f"{filename}: {pairs}")
                if filename == "INPUT.TXT" and "input" in data:
                    lines.append("  keys ever seen: " + keys(data["input"]))
            except ValueError as exc:
                lines.append(f"{filename}: INVALID ({exc})")
    flight = directory / "FLIGHT.TXT"
    if flight.exists():
        try:
            schema, columns, rows = parse_flight(flight.read_text(errors="replace"))
            lines.append(f"FLIGHT.TXT: schema={schema}, columns={len(columns)}")
            lines.extend(summary_lines(rows, columns))
        except ValueError as exc:
            lines.append(f"FLIGHT.TXT: INVALID ({exc})")
    else:
        lines.append("FLIGHT.TXT: not present; use CADET /record on physical hardware.")
    test = directory / "TEST.LOG"
    if test.exists():
        body = test.read_text(errors="replace")
        lines.append("TEST.LOG: " + ("all core tests passed" if "All DOS core tests passed."
                                     in body else "no core success marker; inspect file"))
        stages = [x for x in body.splitlines() if x.startswith("STAGE:")]
        lines.append(f"  {len(stages)} stage messages")
    for filename in ("CORE.LOG", "VIDEO.LOG"):
        path = directory / filename
        if not path.exists():
            continue
        body = path.read_text(errors="replace")
        warnings = [x for x in body.splitlines()
                    if "LOG: WARNING:" in x or "LOG: CPU warning:" in x]
        errors = [x for x in body.splitlines()
                  if "##[error]" in x or "Traceback" in x or
                  "fatal error" in x.lower()]
        lines.append(f"{filename}: DOSBox startup warnings={len(warnings)}, "
                     f"fatal/error indicators={len(errors)}")
        # Do not infer a game failure from optional MIDI/SDL/8086 warnings.
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, default=Path("."),
                        help="Directory containing FLIGHT.TXT, INPUT.TXT, etc.")
    parser.add_argument("--output", type=Path, help="Optional report file")
    args = parser.parse_args()
    result = summarize(args.directory)
    if args.output:
        args.output.write_text(result, encoding="utf-8")
    print(result, end="")


if __name__ == "__main__":
    main()
