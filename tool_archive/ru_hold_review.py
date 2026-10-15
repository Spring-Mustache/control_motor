#!/usr/bin/env python3
"""Evaluate FIXED +/-40 RPM wheel HOLD capture and RU hold-settle trim.

Usage (PowerShell):
    python .\tool_archive\ru_hold_review.py analog_bench_20.csv
    python .\tool_archive\ru_hold_review.py analog_bench_19_again2.csv analog_bench_20.csv

Outputs one CSV per input and a compact console summary. Requires pandas, numpy.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import numpy as np
import pandas as pd

WHEELS = ('lu','ld','ru','rd')


def first_hold_time(g: pd.DataFrame, wheel: str, min_dur_ms: int) -> float:
    vals = (g[f'hunt_{wheel}'].to_numpy() == 1)
    ts = g['time_ms'].to_numpy()
    starts = np.flatnonzero(vals & ~np.r_[False, vals[:-1]])
    for start in starts:
        if not min_dur_ms:
            return (ts[start] - ts[0]) / 1000.0
        # Require continuous >= min_dur_ms using timestamps, not count*nominal dt.
        last = start
        while last + 1 < len(vals) and vals[last + 1]:
            last += 1
        if ts[last] - ts[start] >= min_dur_ms:
            return (ts[start] - ts[0]) / 1000.0
    return float('nan')


def analyze(file: Path) -> pd.DataFrame:
    data = pd.read_csv(file, comment='#', on_bad_lines='skip')
    required = {'time_ms','control_mode','pid_target_mrpm','integral_hold', 'ru_mrpm', 'ru_pwm'}
    required |= {f'hunt_{w}' for w in WHEELS}
    if not required.issubset(data.columns):
        raise ValueError('missing fields: ' + ', '.join(sorted(required - set(data.columns))))
    # fixed-speed plateau: target has reached full +/-40 rpm and ramp/integral hold is OFF.
    good = (data.control_mode == 2) & (data.pid_target_mrpm.abs() >= 40000) & (data.integral_hold == 0)
    islands = good.ne(good.shift(fill_value=False)).cumsum()
    results = []
    for _, q in data[good].groupby(islands[good]):
        if len(q) < 60:  # ignore fragments shorter than ~1.5 s
            continue
        q = q.reset_index(drop=True)
        target = q.pid_target_mrpm.iloc[0] / 1000.0
        if abs(target) > 41 or abs(target) < 39:
            continue
        d = {'plateau': f'{target:+.2f} #{1 + sum(np.sign(r["target_rpm"]) == np.sign(target) for r in results)}',
             't_start_ms': int(q.time_ms.iloc[0]), 'target_rpm': round(target,3)}
        for w in WHEELS:
            d[w + '_first_s'] = first_hold_time(q, w, 0)
            d[w + '_stable_s'] = first_hold_time(q, w, 1000)
            d[w + '_pass_first'] = bool(d[w + '_first_s'] < 0.5)
            d[w + '_pass_stable'] = bool(d[w + '_stable_s'] < 0.5)
            tail = q.loc[q.time_ms >= q.time_ms.iloc[-1]-5000]
            if w + '_mrpm' in q.columns:
                errors = (tail.pid_target_mrpm - tail[w + '_mrpm']).abs() / 1000.0
                d[w + '_last5_mae'] = round(float(errors.mean()), 3)
            if w + '_pwm' in q.columns:
                d[w + '_last5_pwm_std'] = round(float(tail[w + '_pwm'].std(ddof=0)), 3)
        for key in ['capture_ru_active','ru_settle_active']:
            if key in q:
                d[key + '_samples'] = int((q[key] == 1).sum())
        if 'ru_settle_pwm_x1000' in q.columns:
            arr = q['ru_settle_pwm_x1000'].astype(float)/1000.0
            d['ru_settle_pwm_peak_abs'] = round(float(arr.abs().max()), 2)
            d['ru_settle_pwm_peak_pos'] = round(float(arr.max()), 2)
            d['ru_settle_pwm_peak_neg'] = round(float(arr.min()), 2)
        results.append(d)
    return pd.DataFrame(results)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('logs', nargs='+', type=Path, help='STM32 WHEEL_ANALOG CSV files')
    p.add_argument('--out-dir', type=Path, default=None, help='CSV summary destination (default: beside logs)')
    args = p.parse_args()
    for file in args.logs:
        stats = analyze(file)
        if stats.empty:
            print(f'{file.name}: no eligible fixed +/-40 plateaus')
            continue
        outfile = ((args.out_dir or file.parent) / (file.stem + '_hold_review.csv'))
        outfile.parent.mkdir(parents=True, exist_ok=True)
        stats.to_csv(outfile, index=False)
        print('\n===', file.name, '===')
        print(stats[['plateau','lu_first_s','ld_first_s','ru_first_s','rd_first_s',
                     'ru_stable_s', 'rd_stable_s']].round(3).to_string(index=False))
        for w in WHEELS:
            first = int(stats[w+'_pass_first'].sum())
            stable = int(stats[w+'_pass_stable'].sum())
            print(f'{w.upper()}: first <0.5s {first}/{len(stats)}, stable >=1s from <0.5s {stable}/{len(stats)}')
        if 'ru_settle_pwm_peak_abs' in stats:
            print('RU settle trim peak:', stats.ru_settle_pwm_peak_abs.max(), 'PWM')
            print('RU settle active samples:', int(stats.ru_settle_active_samples.sum()))
        print('summary:', outfile)

if __name__ == '__main__':
    main()
