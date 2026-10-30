# M8-S12 — Baseline R1 RIGHT and next dual-axis steering control

**Date:** 2026-10-30. **Branch:** `feature/m7-s15-ps2-controls-restoration-staged`.

## Operator-observed R1 commissioning baseline

Operator reports that R1 RIGHT responds best with Live Expressions
`steer_pos_right_fine_enter_raw=10`, `steer_pos_right_fine_pwm=85`,
`steer_pos_right_fine_pulse_ms=10`. LEFT nearly always reaches CENTER
smoothly; RIGHT now usually needs only one small final trim, but still
sometimes makes a slight jerk. Do **not** call this zero-jerk/10-of-10
acceptance. No new CSV was supplied after the last parameter comparison.

**IMPORTANT: the firmware does not actually execute 10 RAW / 10 ms.**
`Steering_PositionAxisUpdate()` clamps RIGHT fine entry to **15..35 RAW**;
`Steering_PositionFinePulseMs()` clamps pulse length to **12..60 ms**.
Therefore the effective, repeat-tested settings are:
- RIGHT fine entry: **15 RAW**;
- RIGHT fine PWM: **85 / ARR=999**;
- RIGHT fine pulse: **12 ms**;
- CENTER LEFT: **1873 ±5 RAW**; CENTER RIGHT: **662 ±5 RAW**.

M8-S12 makes the **effective** settings the boot defaults in existing
`Core/Src/main.c`; they remain mutable with Live Expressions when all
actuators have stopped. Historical RIGHT handoff 35 RAW and fine pulse
40 ms are still available as an immediate parameter-only A/B rollback.
No clamp, PWM safety ceiling, motor DIR/encoder mapping, PID wheel
controller, NC switch IRQ, sensor validation, STOP, fine 8-pulse/9s
timeout or ±5 acceptance condition is weakened.

The source change and regression guard are committed; CubeIDE
build/flash using these *new defaults* and physical behavior after
restart have **not** yet been independently confirmed.

## Architecture already present: do not rewrite from scratch

1. **Independent axis protection:** each steering motor has its
   own AS5600, requested direction, soft/hard limits, own NC IRQ
   cutoff, stall/timeout and position state. A normal completion of
   one axis must not erase the partner's ability to finish.
2. **Manual D-pad/PS2 RX:** LEFT and RIGHT move in opposite physical
   directions after M8-S09/S10 remapping. The preferred active
   coordinator is `steer_angle_wp_enable=1` using normalized
   position progress plus measured AS5600 speed for two MOVING axes.
   The historical RIGHT-master/LEFT-follower
   `Steering_SyncApply()` PI is bypassed for those simultaneous
   MOVING frames when waypoint mode is enabled; do not tune its
   gains expecting them to change that ordinary manual path.
3. **R1 CENTER:** both independent position loops target 1873/662.
   The coordinator may pace the coarse stage, but once either axis
   enters fine trim it must not hold the still-coarse partner or
   override local zero-PWM settling. RIGHT's residual small jerk is
   a known *quality* limitation, not license to increase PWM.
4. **R2:** success is determined by each motor's own physical MIN
   NC; a raw/soft target alone must never signal spin-ready.
5. **Wheel drive+steer:** LY/Up/Down may command four traction wheels
   while ordinary manual steering is active; L1/L2 spin and SELECT
   remain exclusive cases. STOP and motor supply isolation remain
   independently essential.

## Next milestone — synchronized dual-axis travel on a supported chassis

Keep the existing project/workspace. Do **not** create a new worktree
for parameter tuning. Do not refactor motor control before gathering
a new baseline with the stored R1 defaults.

1. Clean Build + flash the current project once (only to persist the
   new defaults across reset). With motor power isolated verify no
   accidental PWM/arming; then conduct suspended-chassis tests with
   accessible independent motor-supply cut-off.
2. Run D-pad LEFT and RIGHT from CENTER and separately from offset
   positions; then short right-stick RX left/right tests. Save the
   M8-S11 six-motor 50-ms CSV and M8-S00 TXT only **after**
   stopping/disarming and closing the live stream. The telemetry
   must be associated with the same firmware commit and trial.
3. Inspect `M8S00_TRACK` for each mode: normalized progress
   `meanGap_permille`, `peakGap_permille`, `over30Frames`.
   Inspect `M8S00_PACING` and `M8S00_TIMING` to distinguish
   coarse travel, fine trim and finishing skew. A phase gap of
   30 permille = 3% is the *existing trial controller threshold*,
   not yet a certified mechanical tolerance.
4. Look for stops/restarts, unexpected direction changes,
   unequal time to reach opposite soft stops, incorrect NC/stop
   behavior, and loss of reference/stale AS5600 samples. Evaluate
   smoothness independently from final CENTER tolerance.
5. Only if measured mismatches are persistent, adjust a **single**
   bounded coordinator parameter while all motors are stopped
   (`steer_angle_wp_lead_permille` is currently 30); compare with
   a baseline, not arbitrary repeated tuning. Never loosen PWM/NC
   or bypass another owner's STOP to improve synchronization.
6. R1 small final jerk remains a separate follow-up, not a reason
   to compromise the shared two-axis phase controller. Repeat the
   R1 test after any later controller changes to catch regressions.

**Acceptance is pending:** need both-direction synchronization evidence,
safe independent stopping, stable R1/R2 behavior and eventually a
separate ground-load commissioning gate. Observed R1 improvement alone
does not close M8-S00 or Gate A.
