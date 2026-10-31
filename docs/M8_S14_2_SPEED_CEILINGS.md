# M8-S14.2 Manual steering speed-ceiling experiment

Status: SOURCE-STAGED. Not CubeIDE-built, flashed or hardware validated.

Manual D-pad/RX speed level in Live Expressions:
- steer_s142_trial_level=0: baseline LEFT 300, RIGHT 100 CCR (default).
- steer_s142_trial_level=1: LEFT 375, RIGHT 150 CCR.
- steer_s142_trial_level=2: LEFT 450, RIGHT 200 CCR.
Invalid values are treated as zero. Level latches only when a new
manual command begins after neutral, never while moving.

Speed-priority mode bypasses old manual PI, waypoint/phase coordination
and S14/S14.1 leader-reducing taper; the two independent manual owners
remain authoritative. The manually driven MOVING path bypasses old
300/100 caps and 400 hard limit only where it explicitly checks the
new experimental cap. Position R1/R2, ESCAPE, calibration and wheel
PWM retain their old limits.

Extra PWM over 300/100 is available only beyond 180 RAW to the per-axis
SOFT target, fades from peak down to old caps between 320 and 180 RAW,
and rises no more than 12 CCR per control pass. Within 180 RAW old
caps are restored; within 100 RAW existing soft-approach PWM taper
is untouched. NC, EXTI/STOP, AS5600, stall and late atomic CCR
interlocks remain; these checks DO NOT certify real electrical
power cutoff or motor stopping distance.

M8S00_S142 diagnostic includes selected level, last latched
level, frames requesting >legacy PWM, soft-envelope events and
peak owner-demand CCR. These are NOT physical speed measurements.

Commissioning: verify protected workspace and clean CubeIDE build;
with motor power disconnected test STOP/NC and independently verified
power isolation, then chassis securely lifted. Only then attempt
very short level=1 pulses away from ends; record M8-S11 CSV and M8-S00.
Advance to level=2 only if measured coast clearance, current and
hardware protection support it. Measure actual angular speed and
arrival error. Do not conduct ground-load or simultaneous LY
trials before isolated steering-level acceptance.

CAUTION: prior RIGHT R2 NC fault and the earlier FIRST-STOP
electrical power anomaly are not resolved by increasing PWM.
No motor-powered validation is claimed.

## S14.2 cross-coupled synchronization revision

Speed mode must NOT run open-loop. The default-OFF selection of
S14.2 level=1/2 now activates cross-coupled angle+velocity tracking
in addition to higher PWM, replacing the earlier speed-only draft.
Do not call this a verified high-speed controller until physical tests.

Keep 300/100 legacy baseline with level=0. At level=1 (375/150 cap)
start from D-pad base LEFT 330 / RIGHT 125; at level=2 (450/200 cap)
base LEFT 405 / RIGHT 175. Both motor bases exceed the old baseline
but reserve headroom for lagger. RX scales from legacy minimum
(175/50) up to these new base values. The old +20% feedforward
speed-trial multiplier is disabled only in level=1/2, because it
would consume reserved headroom before feedback could act.

For each new AS5600 sample, direction-align travel in calibrated
centidegrees, compute filtered cdeg/s per sensor and predict error:
  e_pred = travelL - travelR + 0.120*(speedL - speedR).
If LEFT leads (>0.40deg), boost RIGHT up to its available
dynamic ceiling. RIGHT leads => boost LEFT. Only when the follower
has saturated, the predicted gap exceeds 1.50deg AND follower
is not already catching up sufficiently, optionally taper leader,
bounded to 48 CCR and never below its prior minimum drive floor.
No extra PWM boost or leader taper when either axis is <=180 RAW
from own SOFT. Other independent safety guards remain authoritative.
This is a P+D candidate with no integral windup. The output
PWM increase after correction also obeys the 12-CCR/per-pass
S14.2 rise limiter; decreases are immediate.

Live M8S00 diagnostic M8S00_S142_SYNC provides frames, skipped,
boostL/R frames, trimL/R frames, saturation flags counts and
most recent angle gap/predicted error (centidegrees). The existing
M8-S11 CSV remains 60 columns; CCR shows actual command requests.
The old S14/S14.1 run only when level=0.

Validation must compare three states with separate captures:
(1) level0 baseline; (2) level1 new sync; (3) level2 ONLY after
level1 physical SOFT clearance/current/electrical STOP checks.
Run at least five repeats per D-pad/RX direction and verify
six-motor concurrent LY only after steering-alone acceptance.

## Command continuity clarification

Controller output is recomputed and applied at every manual-owner
service tick using the most recent *fresh, bounded* AS5600 rate
estimate. This avoids returning to the unsynchronized owner PWM
between distinct I2C samples. The 12 CCR per-pass upward slew
still applies to final commanded PWM; a drop in SOFT cap is immediate.

## S14.3 fast-start follow-up

Related boot-OFF prequalified-neutral AS5600 fast-start trial is in [M8-S14.3](M8_S14_3_FAST_START.md). Level2 is operator-liked but not independently safe-approved or boot default. No high-speed physical acceptance claimed.

## M8-S14.4 boot defaults after Level-1 bench feedback — 2026-10-31

CURRENT (supersedes earlier lines in this historical experiment document): `steer_s142_trial_level` now initializes to **1** (375/150 maximum) because Level-1 steering and four-wheel concurrent control were logged in the user's M8S11 14:57:37 run. `steer_s143_fast_start_enable` now initializes to **1** but never arms motors on its own. `steer_manual_enable=0` and motion permit=0 at reset, START arming/neutral checks and STOP/NC/SOFT remain. `wheel_pid_enable=1` and `m8_s11_stream_period_ms=50` were already defaults; `m8_s11_stream_enable=0` remains operator-controlled. Level2 (450/200) is NOT the boot default despite operator preference for its speed, because level2 physical stopping clearance/current remains unverified.

The 14:57:37 M8S11 logged RIGHT taking longer near R_MIN than LEFT: near RIGHT distance 180/80/40/17 RAW, commanded right PWM approximately 100/90/67/58 CCR. The existing S14.2 envelope decays to legacy right 100 within 180 RAW, and the independent last-100-RAW taper reduces toward right RX floor 50 CCR; LEFT can finish before RIGHT. Raw RIGHT MIN ~211 vs SOFT 216, and LEFT MIN ~1432 vs SOFT 1454 were also seen, so don't raise near-SOFT duty or remove taper without independently measuring stop/coast/clearance and physical power cutoff. High-speed acceptance under load is NOT implied by enabling Level1 as a software default.
