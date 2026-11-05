# M8-S15.4 — VC-02 synchronized dual steering, staged 2026-11-05

## Basis / operator-proven prerequisite
The operator confirmed that Voice 45 steering moves as intended when VC-02 gets a **short spoken pause** between direction `A5/A6` and angle `B9`. Do not change the strict two-event parser or automatically assume one utterance is decoded as two events. Verified practical behavior was on the predecessor `feature/m8-vc02-45deg-nc-endstops` at commit `66053b61ea3890ed7a56c80a98eced758908d911`. This confirms the prior operator's steering motion, not new S15.4 simultaneous-arrival performance.

## New staged branch
`feature/m8-s154-voice-dual-steer-common-progress`, forked at exact `66053b61ea3890ed7a56c80a98eced758908d911`. Parent branch remains unchanged. New code changes only Voice position owner command 4 (angles 5–40) and command 5 (45 physical NC) when `steer_voice_s154_enable==1`; `steer_voice_s154_enable=0` restores the previous independent Voice position control without reflashing.

## Design
- Start / end of each servo axis are independently read from its existing AS5600 (`steer_left_axis.escape_start_raw` / `steer_right_axis.escape_start_raw` and `voice_pos_goal_l/r`). No assumption that the two sides have identical RAW stroke, motor speed, gearing or PWM.
- Reuse the already-present M8-S00 `Steering_AngleWaypointApply` / `Steering_AngleStrokeProgress` / `Steering_AngleSpeedObserve` for normalized 0..1000 stroke progress and AS5600 sample-time normalized velocity, with per-side bounded speed feedback. This gives a *common progress/rate reference*, not an unverified hard simultaneous-arrival guarantee or a second PID stacked on the existing position servo.
- The coordinator gets proposals **after** independent axis sensor read, direction/RAW validation, progress/stall/timeout/NC checks, **before** atomic late NC/CCR writes. It is eligible only when BOTH are in coarse `STEER_AXIS_MOVING`, neither has fine-phase active, both own NC masks are clear, both samples are fresh (age <=40 ms), both independent owner duty proposals are >0 and at least the existing per-axis minimum. No increase beyond current tested cap (LEFT <=300, RIGHT <=100) and no synchronization-induced PWM=0; if a side has no headroom or is near terminal 100 RAW, feedback falls back to the original independent owner. It cannot bypass physical NC, safety trip, motion permit, or direction guard.
- 5–40: targets remain calibrated CENTER to physical MIN/MAX interpolation and servo finish remains encoder-based independent fine tolerance. The coordinator does not declare DONE.
- 45: LEFT expects `L_MAX+R_MIN`, RIGHT expects `L_MIN+R_MAX` and *both* physical NC completions (per-axis result 3), unchanged. Once either axis has done it stays PWM=0; the partner continues under its own conditions. RAW hard landmark before NC -> fault (no force-past). NC interrupt remains independent and immediate.
- Existing global 5s Voice position timeout, 650ms per-axis stall guard, guarded auto-ARM, STOP, PS2 emergency takeover, wheel time/speed logic, R1/R2/SELECT, and physical hard-overrun envelope are unchanged.
- The existing legacy M8-S00 R1/R2/manual tracking accumulators are not polluted by Voice mode4; S15.4 gets separate observations.
- A read-only `steer_voice_s154_common_eta_ms` is the slower measured remaining normalized fraction's ETA, when both observed speeds are valid. It does **not** command more time or change the 5s watchdog. Where a common arrival is not physically feasible, independently accurate position/NC completion and existing fault handling take precedence.

## New CubeIDE Live Expressions
- `steer_voice_s154_enable` — trial active boot = 1; 0 immediate opt-out to independent position commands (do not toggle while an active stroke).
- `steer_voice_s154_active` — this control frame applied Voice sync logic.
- `steer_voice_s154_last_mode` — 4 angle RAW, 5 NC.
- `steer_voice_s154_status` — 0 idle, 1 paced, 2 disabled, 3 outside shared coarse phase / invalid short stroke, 4 stale/missing sample or PWM, 5 NC asserted.
- `steer_voice_s154_start_l/r` — saved starting RAW for the command.
- `steer_voice_s154_progress_l/r` — normalized traveled fraction, per mille.
- `steer_voice_s154_gap_permille`, `steer_voice_s154_peak_gap_permille` — current and peak normalized phase difference.
- `steer_speed_measured_l_pms/r_pms`, `steer_speed_reference_pms` — existing normalized rate feedback.
- `steer_voice_s154_pwm_l/r` — post-coordinator proposal (before final atomic NC validation).
- `steer_voice_s154_l_eta_ms/r_eta_ms/common_eta_ms` — **diagnostic only**; invalid/zero-speed shows 0.
- `steer_voice_s154_done_l_ms/r_ms`, `steer_voice_s154_arrival_skew_ms` — terminal servo completion tick and difference; may differ by physical NC/coast and fine correction. A low skew cannot be assumed from HOST tests.
- `voice_pos_last_result`, `steer_pos_left_result`, `steer_pos_right_result` — authoritative position/NC outcome.

## Validation / not yet proven on target
- `tools/verify_m8_s154_voice_sync.py` builds and tests the extracted real C functions that calculate normalized progress, velocity, and call the coordinator with correct Voice-only geometry; statically verifies ordering around NC/ISR. Existing R1/R2, VC-02 parser, NC-only 45, STOP/PS2 and safety regressions remain in GitHub Actions, exact commit SHA.
- HOST CI is **not** a target STM32CubeIDE build or validation of real motors/NC. A new common-time 45° run is NOT physically confirmed. The operator has only confirmed the prior, unsynchronized VC02 45° function.
- First do CubeIDE Clean Build, flash intended commit and reconnect the *matching* ELF (past CubeIDE variables were wrong until refresh/rebuild). With motor supply disconnected, inspect NC masks and command handling, PS2/FF/limit safety gates. Next, on a secured test rig with independent power cutoff, test initial distances and both turns at a small, safe intermediate angle and then progressive angles. Do **not** bypass stop or increase PWM to force NC. Compare normalized progress, actual PWM, terminal result and `arrival_skew_ms` with S15.4 enabled vs `steer_voice_s154_enable=0`, using the same starting positions.
- Existing 45° 5-second timeout remains; speed sync cannot promise a forced common time. Disable S15.4 with the flag if either servo fails, oscillates, or has larger stopping error; do not leave test motors unattended.
