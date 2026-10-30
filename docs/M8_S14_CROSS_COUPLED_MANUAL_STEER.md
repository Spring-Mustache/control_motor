# M8-S14 — Cross-coupled calibrated-angle/velocity manual steering

**Date:** 2026-10-30. **Branch:** `feature/m7-s15-ps2-controls-restoration-staged`.
**Basis:** M8-S12 restored in commit `f33ca72d4`.
**Status:** implementation staged; **NOT** target-build/flash or hardware PASS.

## Purpose and A/B isolation

M8-S13 compared both axes using a common *virtual* angular stroke and
improved RIGHT but regressed LEFT; the experiment was removed. S14 does
not restore the equal-angle flag or change R1/R2. A new independent
cross-coupled P+D **taper-only** supervisor is opt-in:

- `steer_s14_manual_enable=0` at boot: legacy M8-S12 remains unchanged.
- With both steering motors STOPPED and PS2 neutral, set flag to 1;
  the mode is **latched on the next valid D-pad/RX command**.
- `Steering_S14ManualApply` runs only when **both** independent manual
  axes are MOVING and their initial AS5600 references are valid. It
  bypasses old angle-waypoint and RIGHT-master PI on those frames;
  the old PI remains available for non-S14 single-axis finish.
- R1 CENTER coarse/fine, R2 physical MIN NC, L1/L2, SELECT, wheel
  PID, all per-axis NC IRQ/STOP/geometry/stall and late atomic CCR
  rechecks are unchanged.

## Algorithm and units

Use existing piecewise `Steering_AngleFromRaw`:
LEFT 2297/1873/1374 and RIGHT 166/662/1029 RAW map each axis
to −45/0/+45 calibrated **centidegrees**. This is not an independent
physical wheel-angle measurement.

For each axis, measure positive travel along the commanded direction
since its own settle-complete start. On distinct, validated sensor
timestamps, estimate speed in centidegrees/second using a bounded EMA.
Reject stale/invalid, backward or discontinuous samples; do not add I2C.

```text
gap_cdeg = travel_LEFT - travel_RIGHT
predict_cdeg = gap_cdeg + 0.120*(speed_LEFT - speed_RIGHT)
if predict > +0.5 deg: taper LEFT only
if predict < -0.5 deg: taper RIGHT only
otherwise: gradually release trim
```

Initial candidate: 25 CCR per degree beyond deadband, bounded to
LEFT 96 / RIGHT 48 CCR of **reduction**, slew <=8 CCR on each
new sensor sample and progressively fade taper over the last
200→100 RAW before the independent SOFT target. The owner-provided
per-axis minimum PWM floor is enforced; S14 never creates a
zero-PWM synchronization hold. All numbers are preliminary, not
validated control gains. The shorter target still finishes independently.

**No PWM ceilings raised.** Existing D-pad 300 LEFT / 100 RIGHT
and R2 conditional 330/110 headroom are separate. A prior +30%
operator trial is not documented as a safely verified source default;
`steer_speed_trial_pct` in main.c is capped to 120% and the
manual target is clamped to 300/100. S14 **only lowers** the already
authorized PWM, so it cannot apply a +30% increase.

## Telemetry and checks

Read-only Live Expressions:
`steer_s14_manual_latched`, `steer_s14_last_mode`,
`steer_s14_last_gap_cdeg`, `steer_s14_last_predict_cdeg`,
`steer_s14_speed_l_cdeg_s`, `steer_s14_speed_r_cdeg_s`,
`steer_s14_trim_l_pwm`, `steer_s14_trim_r_pwm`,
`steer_s14_valid_frames`, `steer_s14_skipped_frames`,
`steer_s14_taper_l_frames`, `steer_s14_taper_r_frames`.
On safe DISARM export `M8S00_S14`; counters run since boot,
`last` is the last started manual command. Old `M8S00_TRACK`
records old controller-only metrics and must NOT be compared as
though they use the same S14 metric. M8-S11 raw/CCR CSV is unchanged.

Host/static preflight:
```powershell
python tools/verify_m8_s14_sync.py
python tools/verify_m8_concurrent_drive_steer.py
python tools/verify_m8_r1_center_concurrent.py
python tools/verify_m8_s11_csv.py
python tools/verify_m8_s00_capture.py
```
The S14 verifier compiles and runs `tools/test_m8_s14_math.c` against
the **same** `Core/Inc/steer_s14_math.h` used by the firmware when a
host C compiler is available. Compiler missing = SKIP, not PASS.
CubeIDE clean Debug Build is still required.

## Controlled commissioning / acceptance

1. Safeguard local uncommitted changes; fast-forward same workspace,
   never hard reset or force-clean. Clean Build and flash selected SHA.
2. **With motor power isolated**, check pin directions, NC logical
   levels, output CCR=0, STOP/PS2 disarm and an independently tested
   motor-supply disconnect. Secure vehicle on stand before motion.
3. Baseline A: `steer_s14_manual_enable=0`; D-pad LEFT/RIGHT and RX
   LEFT/RIGHT with matched starting positions and RX magnitude.
   Stop/disarm between trials. Save complete 50-ms CSV.
4. S14 B: with PS2 neutral and all six motors stopped, set flag 1;
   issue a **fresh** manual command. Repeat at least five trials
   per direction with the same starting conditions. Never switch
   while motors are active. Record complete CSV and M8-S00 TXT.
5. Evaluate mean/peak absolute **signed-along-command** calibrated
   angular gap in degrees, speeds, PWM saturation, delayed arrival,
   jerk, false STALL, stale AS5600, NC and unexpected STOP.
   One direction improving at the expense of the other is **not PASS**.
   Optional engineering trial target: >=20% mean-gap reduction per
   direction with no increase in peak, jerk or stop faults; final
   mechanical tolerance requires independent wheel-angle mapping.
6. Regress R2→R1 and separately low LY + D-pad/RX six-motor overlap
   only after steering-alone acceptance. Verify independent electrical
   power cutoff; CSV CCR=0 does NOT prove motor supply isolation.
   Ground-load Gate A is a later, separate decision.

**No motor-powered test or hardware acceptance is claimed in this commit.**
