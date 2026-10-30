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
