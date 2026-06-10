"""Compare benchmark JSON reports against a baseline (CI or local)."""

from __future__ import annotations

import argparse
import json
import sys


def case_key(row):
    return (row.get("name", ""), row.get("name_suffix", ""), row.get("params", ""))


def load_results(path):
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    out = {}
    for row in data.get("results", []):
        out[case_key(row)] = float(row["median_ms"])
    return out


def compare(baseline_path, current_path, threshold):
    baseline = load_results(baseline_path)
    current = load_results(current_path)
    errors = []

    for key, base_ms in sorted(baseline.items()):
        if key not in current:
            errors.append("missing case in current run: %s" % (key,))
            continue
        cur_ms = current[key]
        if base_ms <= 0:
            continue
        ratio = cur_ms / base_ms
        if ratio > threshold:
            name, suffix, params = key
            errors.append(
                "regression %.2fx (>%.2fx): %s%s [%s] baseline=%s ms current=%s ms"
                % (ratio, threshold, name, suffix, params, base_ms, cur_ms)
            )

    for key in sorted(current):
        if key not in baseline:
            errors.append("new case not in baseline (update baseline): %s" % (key,))

    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True)
    parser.add_argument("--current", required=True)
    parser.add_argument("--threshold", type=float, default=2.0)
    args = parser.parse_args()

    errors = compare(args.baseline, args.current, args.threshold)
    if errors:
        for line in errors:
            sys.stderr.write("tgen benchmark_check: %s\n" % line)
        return 1
    print("tgen benchmark_check: ok (%d cases)" % len(load_results(args.baseline)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
