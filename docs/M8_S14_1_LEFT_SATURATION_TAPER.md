# M8-S14.1 — Saturation-aware LEFT steering correction

**Date:** 2026-10-30. **Base:** M8-S14 at 20189e36.
**Status:** SOURCE-STAGED; NOT CubeIDE built / flashed / hardware PASS.

## Evidence and problem

Two 50-ms UART CSV recordings (one per controller variant):
- D-pad LEFT mean calibrated travel gap: S12 0.64°, S14 1.03°.
- RX LEFT: S12 0.89°, S14 1.42°.
- For S14 LEFT runs RIGHT at 100 CCR in 86% / 84% of
  both-powered frames, but LEFT remains ahead despite S14 taper.
- RIGHT-direction S14 improved, so do not modify its gain.

Those are calibrated travel (AS5600 -45 to +45°), NOT independently
measured physical wheel angles. Different starts/joystick magnitude
and single trials do not prove a unique root cause.

## Control change

Existing S14 remains intact. Two mutually comparable variants:
- A: steer_s14_manual_enable=1, steer_s141_left_enable=0 (old S14).
- B: steer_s14_manual_enable=1, steer_s141_left_enable=1 (new S14.1).

Both values must be set only while stopped and PS2 neutral. The
S14.1 flag latches only at the start of a fresh D-pad/RX LEFT
command (command < 0). D-pad/RX RIGHT are unchanged.

Original projected calibrated travel error:
  e_pred_cdeg = travelL - travelR + 0.120 * (omegaL - omegaR).

When S14.1 LEFT is latched, the independent RIGHT owner PWM is
>=98 CCR (near its 100 CCR verified cap), and e_pred >30 cdeg:
  left_trim_candidate = 60 CCR/degree * (e_pred - 0.30°)
  left_trim = max(previous S14 trim, new candidate)
  left_trim <=96 CCR
  final 200->100 RAW fade; <=8 CCR per valid AS5600 sample slew.

Any other case follows the original S14 planner unchanged.
Always clamp to existing LEFT floor/authorized base PWM, never
boost/reverse/hold at zero, never alter NC, STOP, R1 fine, R2 MIN,
wheel PI, actuator GPIO direction or 300/100 PWM caps.
These gains are hypothetical trial settings, not proven stable.

## Diagnostics and acceptance

New M8-S00 line:
  M8S00_S141,enable=...,last=...,rightNearCap=...,
  extraFrames=...,baseTrimL=...,planTrimL=...

last=1 proves most recent manual command latched S14.1.
extraFrames>0 proves the new planner requested more LEFT taper
than S14 on some frames; does not prove the actual motor slowed
or alignment improved. All counters are since boot.
The original 60-column M8-S11 CSV is unchanged.

Run 'python tools/verify_m8_s14_sync.py', CubeIDE Clean Build,
Flash chosen SHA, then a suspended-chassis A/B with power-cutoff
already independently verified. Recommend >=5 matched repeats
D-pad LEFT and RX LEFT, plus D-pad RIGHT/RX RIGHT regression.
Export separate CSV/meta and after DISARM M8-S00 TXT; compare
mean/peak signed-along-command angular travel error, stall,
NC trip, PWM jitter, coast clearance and real wheel angle if
physical measurement is available.

One earlier test logged RIGHT R2 error 9, although the later
R2 completed: this fault is NOT conclusively resolved. No
ground-loaded or six-motor trial until independent protective
checks have passed. Do not treat SW CCR=0 as power isolation.

**No real motor run, MCU build, or S14.1 hardware pass was done here.**
