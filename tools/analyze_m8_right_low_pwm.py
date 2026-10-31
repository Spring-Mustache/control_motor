#!/usr/bin/env python3
"""Passive M8-S14.5 RIGHT R_MIN low-PWM analysis from existing M8S11 CSV.

No motor commands, firmware changes, hardware PASS or current/stop guarantee.
Speeds are sampled AS5600 RAW/s, not independently measured wheel or shaft RPM.
"""
import argparse
import csv
import json
import statistics as st
from pathlib import Path

FIELDS = ('ms','dleft','manual_active','soft_goal_l','soft_goal_r',
          'raw_l','raw_r','ccr_l','ccr_r','nc','trip')
ZONES = ((0,10),(10,20),(20,40),(40,60),(60,80),(80,100),
         (100,180),(180,320),(320,380),(380,800))

def read_csv(path):
    with Path(path).open(encoding='utf-8-sig',newline='') as fd:
        reader=csv.DictReader(fd)
        missing=set(FIELDS)-set(reader.fieldnames or ())
        if missing:
            raise ValueError(f'{path}: missing columns {sorted(missing)}')
        rows=[]
        for line,record in enumerate(reader,2):
            try:
                rows.append({key:int(record[key]) for key in FIELDS})
            except (ValueError,TypeError) as exc:
                raise ValueError(f'{path}:{line}: noninteger signal') from exc
    if not rows:
        raise ValueError(f'{path}: empty CSV')
    return rows

def read_metadata(path,count):
    meta=Path(str(path)[:-4]+'.meta.txt')
    if not meta.is_file():
        return 'NO META: transmission must be checked separately'
    data={}
    for line in meta.read_text(encoding='utf-8-sig').splitlines():
        if '=' in line:
            a,b=line.split('=',1)
            data[a.strip()]=b.strip()
    if (data.get('complete')!='True' or
        int(data.get('captured_rows','-1'))!=count or
        any(data.get(k)!='0' for k in ('invalid_lines','sequence_missing')) or
        'missed=0' not in data.get('end','') or
        'tx_errors=0' not in data.get('end','')):
        raise ValueError(f'{meta}: incomplete/lost UART or rows differ')
    return 'complete, no loss/errors'

def first_time(rows,predicate):
    return next((r['ms'] for r in rows if predicate(r)),None)

def summarise(rows,left_soft=1454,right_soft=216):
    runs=[]
    strokes=[]
    current=[]
    for row in rows:
        if row['dleft']==1:
            current.append(row)
        elif current:
            strokes.append(current)
            current=[]
    if current:
        strokes.append(current)
    zones={f'{lo}-{hi}':[] for lo,hi in ZONES}
    for stroke in strokes:
        # Keep post-arrival records while D-pad is STILL held. manual_active
        # becomes 0 exactly when BOTH have reached; dropping that final row
        # would falsely classify a completed stroke as incomplete.
        active=[r for r in stroke if r['soft_goal_l']==left_soft and
                r['soft_goal_r']==right_soft]
        if len(active)<4:
            continue
        first=active[0]
        full=first['raw_l']>=2100 and first['raw_r']>=900
        lt=first_time(active,lambda r:r['raw_l']<=left_soft)
        rt=first_time(active,lambda r:r['raw_r']<=right_soft)
        l100=first_time(active,lambda r:r['raw_l']<=left_soft+100)
        r100=first_time(active,lambda r:r['raw_r']<=right_soft+100)
        runs.append({
            't0_ms':first['ms'],
            'full_range':full,
            'both_reached':lt is not None and rt is not None,
            'l_goal_ms':None if lt is None else lt-first['ms'],
            'r_goal_ms':None if rt is None else rt-first['ms'],
            'right_lag_ms':None if lt is None or rt is None else rt-lt,
            'l_last100_ms':None if lt is None or l100 is None else lt-l100,
            'r_last100_ms':None if rt is None or r100 is None else rt-r100,
            'min_l_raw':min(r['raw_l'] for r in active),
            'min_r_raw':min(r['raw_r'] for r in active),
            'trip_or_nc':any(r['trip']!=0 or r['nc']!=0 for r in active)
        })
        # A 50-61ms sample is NOT a perfect synchronous AS5600 reading.
        # Use real board dt; discard reversals, extreme jumps, zero duty.
        for a,b in zip(active,active[1:]):
            dt=b['ms']-a['ms']
            delta=a['raw_r']-b['raw_r']
            if not 0<dt<=150 or not 0<=delta<=150 or b['ccr_r']<=0:
                continue
            remaining=b['raw_r']-right_soft
            speed=delta*1000.0/dt
            for lo,hi in ZONES:
                if lo<remaining<=hi:
                    zones[f'{lo}-{hi}'].append((b['ccr_r'],speed))
                    break
    full=[r for r in runs if r['full_range'] and r['both_reached']]
    def average(k):
        return round(st.mean(r[k] for r in full),1) if full else None
    return {
        'manual_left_strokes':len(runs),
        'full_range_completed':len(full),
        'full_range_not_completed':sum(r['full_range'] and
                                       not r['both_reached'] for r in runs),
        'mean_right_lag_ms':average('right_lag_ms'),
        'mean_left_last100_ms':average('l_last100_ms'),
        'mean_right_last100_ms':average('r_last100_ms'),
        'minimum_raw_l':min(r['raw_l'] for r in rows),
        'minimum_raw_r':min(r['raw_r'] for r in rows),
        'soft_cross_l_raw':max(0,left_soft-min(r['raw_l'] for r in rows)),
        'soft_cross_r_raw':max(0,right_soft-min(r['raw_r'] for r in rows)),
        'any_trip_or_nc':any(r['trip']!=0 or r['nc']!=0 for r in rows),
        'zones':{key:{
            'n':len(samples),
            'median_pwm_ccr':st.median(q[0] for q in samples) if samples else None,
            'median_as5600_raw_per_s':round(st.median(q[1] for q in samples),1)
                                       if samples else None
        } for key,samples in zones.items()},
        'runs':runs
    }

def self_test():
    def row(ms,l,r,button=1,ccr=100):
        return dict(ms=ms,raw_l=l,raw_r=r,dleft=button,
                    manual_active=int(button==1),soft_goal_l=1454,
                    soft_goal_r=216,ccr_l=250,ccr_r=ccr,nc=0,trip=0)
    data=[row(0,2230,1000),row(60,1830,450),row(120,1570,340),
          row(180,1550,305),row(240,1454,280),row(300,1454,216),
          row(360,1450,210),row(420,1450,210,0),
          row(480,2230,1000),row(540,2200,960),row(600,2150,900),
          row(630,2100,850),row(660,2150,900,0)]
    d=summarise(data)
    assert d['full_range_completed']==1 and d['full_range_not_completed']==1
    assert d['mean_right_lag_ms']==60
    assert d['mean_left_last100_ms']==60 and d['mean_right_last100_ms']==120
    assert d['soft_cross_l_raw']==4 and d['soft_cross_r_raw']==6
    print('PASS: completed/truncated detection, lag, 100RAW, SOFT crossing')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--off',type=Path)
    p.add_argument('--on',type=Path)
    p.add_argument('--output-json',type=Path)
    p.add_argument('--self-test',action='store_true')
    args=p.parse_args()
    if args.self_test:
        self_test()
        return
    if not args.off or not args.on:
        p.error('need --off and --on CSV')
    result={}
    for mode,path in (('off',args.off),('on',args.on)):
        rows=read_csv(path)
        result[mode]={'path':path.name,'metadata':read_metadata(path,len(rows)),
                      **summarise(rows)}
    print('M8-S14.5 passive A/B — AS5600 RAW/s (not current/stop certification)')
    for mode in ('off','on'):
        q=result[mode]
        print(f"{mode.upper()}: full={q['full_range_completed']} "
              f"partial={q['full_range_not_completed']} "
              f"RIGHT-lag={q['mean_right_lag_ms']}ms "
              f"last100-L={q['mean_left_last100_ms']}ms "
              f"last100-R={q['mean_right_last100_ms']}ms "
              f"soft-cross-L/R={q['soft_cross_l_raw']}/{q['soft_cross_r_raw']}RAW "
              f"NC-or-trip={q['any_trip_or_nc']}")
    a=result['off']['mean_right_lag_ms']
    b=result['on']['mean_right_lag_ms']
    if a is not None and b is not None:
        print(f'ON minus OFF = {b-a:+.1f}ms; small N, no causal claim')
    print('Distance zone: OFF vs ON (median CCR and sampled AS5600 speed)')
    for zone in result['off']['zones']:
        x=result['off']['zones'][zone]
        y=result['on']['zones'][zone]
        print(f" {zone:>7} RAW | OFF n={x['n']:>2} CCR={str(x['median_pwm_ccr']):>5} "
              f"RAW/s={str(x['median_as5600_raw_per_s']):>6} | "
              f"ON n={y['n']:>2} CCR={str(y['median_pwm_ccr']):>5} "
              f"RAW/s={str(y['median_as5600_raw_per_s']):>6}")
    if args.output_json:
        args.output_json.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print('SAFETY: no motor current, hardware braking or clearance inference.')

if __name__=='__main__':
    main()
