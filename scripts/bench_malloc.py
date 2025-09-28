#!/usr/bin/env python3
import argparse, csv, subprocess, shlex, sys, os
from collections import OrderedDict, defaultdict

SUBJECT_CUSTOM_ONLY = {"test3", "test4", "test5"}  # per subject, run only via wrapper

def run_cmd(cmd_str):
    p = subprocess.Popen(cmd_str, shell=True,
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         text=True)
    out, err = p.communicate()
    return p.returncode, out, err

def parse_time_v(stderr_text):
    """Parse `/usr/bin/time -v` output into {metric: value} with a few guards."""
    metrics = OrderedDict()
    for line in stderr_text.splitlines():
        if ':' not in line:
            continue
        # ignore ld/loader messages like "/path/to/exe: error while loading shared libraries: ..."
        # crude but effective: lines that start with '/' or './' before the first colon are not metrics
        head = line.split(':', 1)[0].strip()
        if head.startswith('/') or head.startswith('./'):
            continue
        # split on the LAST colon, since some keys contain colons in the text
        key, val = line.split(':', 1)
        metrics[key.strip()] = val.strip()
    return metrics

def format_col_name(test, kind):
    return f"{test}_{kind}"

def main():
    ap = argparse.ArgumentParser(description="Run subject tests with system & custom malloc; export /usr/bin/time -v metrics to CSV.")
    ap.add_argument("--time", default="/usr/bin/time -v")
    ap.add_argument("--runner-custom", dest="runner_custom", default="scripts/run_linux.sh")
    ap.add_argument("--out", default="bench.csv")
    ap.add_argument("tests", nargs="+", help="Paths to test executables (e.g., bench_tests/bin/test0 ...)")
    args = ap.parse_args()

    metrics_union = []    # stable order
    seen = set()
    table = defaultdict(dict)

    for test_path in args.tests:
        exe = test_path  # keep as given (can be a path)
        base = os.path.basename(exe)

        if not os.path.exists(exe):
            print(f"[warn] missing executable: {exe} (skipping)", file=sys.stderr)
            continue

        run_sys = base not in SUBJECT_CUSTOM_ONLY

        # ---- system run (test0..test2 only) ----
        if run_sys:
            sys_cmd = f'{args.time} {shlex.quote(exe)}'
            rc, out, err = run_cmd(sys_cmd)
            if rc != 0:
                print(f"[warn] system run failed for {exe} (exit {rc})", file=sys.stderr)
            sys_metrics = parse_time_v(err)
        else:
            sys_metrics = {}

        # ---- custom run (all tests via wrapper) ----
        custom_cmd = f'{shlex.quote(args.runner_custom)} {args.time} {shlex.quote(exe)}'
        rc2, out2, err2 = run_cmd(custom_cmd)
        if rc2 != 0:
            print(f"[warn] custom run failed for {exe} (exit {rc2})", file=sys.stderr)
        custom_metrics = parse_time_v(err2)

        # union of keys
        for k in list(sys_metrics.keys()) + list(custom_metrics.keys()):
            if k not in seen:
                seen.add(k)
                metrics_union.append(k)

        col_sys = format_col_name(exe, "sys")
        col_cus = format_col_name(exe, "custom")
        for m in metrics_union:
            if m in sys_metrics:
                table[m][col_sys] = sys_metrics[m]
            if m in custom_metrics:
                table[m][col_cus] = custom_metrics[m]

    # header: Metric, <path>_sys, <path>_custom, ...
    cols = []
    for t in args.tests:
        cols.append(format_col_name(t, "sys"))
        cols.append(format_col_name(t, "custom"))

    with open(args.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Metric"] + cols)
        for m in metrics_union:
            w.writerow([m] + [table.get(m, {}).get(c, "") for c in cols])

    print(f"[ok] wrote {args.out} with {len(metrics_union)} metrics across {len(args.tests)} tests")

if __name__ == "__main__":
    main()
