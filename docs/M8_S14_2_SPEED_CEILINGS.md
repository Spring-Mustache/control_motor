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
