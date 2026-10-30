# M8-S13 — Optional synchronized calibrated-angle travel (D-pad / RX)

**Date:** 2026-10-30. **Workspace:** existing `D:\STM32 task\control_motor`.
**Branch:** `feature/m7-s15-ps2-controls-restoration-staged`.
**Status:** SOURCE STAGED; no target CubeIDE build/flash or motor-powered A/B validation for M8-S13 yet.

## Why the existing phase metric needs an A/B check

M8-S12 CSV `M8S11_six_motor_20261030_174837.csv` contains 694
complete samples, stream period 50 ms, no reported missing rows or UART
errors. Its four ordinary manual dual-CCR intervals were:

| Test | Time series, mean absolute difference as % of each SOFT target | Replay: same calibrated angular travel |
| --- | ---: | ---: |
| D-pad LEFT | 45.6 permille | 13.9 permille |
| D-pad RIGHT | 20.7 permille | 47.4 permille |
| RX LEFT | 44.6 permille | 24.1 permille |
| RX RIGHT | 19.5 permille | 53.7 permille |

The replays are **measurements of the OLD physical runs with two
different metrics**, not a performance claim for the new controller.

The two calibrated soft stops do not correspond to equal degrees:
M8-S12 steering geometry is LEFT MIN/CENTER/MAX=2297/1873/1374,
RIGHT MIN/CENTER/MAX=166/662/1029; manual limits in this log are
LEFT MAX=1454 / MIN=2225, RIGHT MIN=216 / MAX=999.
The existing `Steering_AngleWaypointApply()` scales each side by its
own remaining calibrated *soft-stop* angle, so 40 degrees travelled
on one side can be counted as a different progress fraction to
40 degrees on the other. This changes which side is classified as
leading, especially in D-pad RIGHT and RX RIGHT.

These angles are **derived from AS5600 calibrated endstops**, not a
separate direct measurement of wheel/linkage physical degrees. Any
persistent unequal gear/linkage relationship must eventually be
measured mechanically on the chassis.

## M8-S13 source change

- A single opt-in `volatile uint8_t
  steer_angle_wp_manual_equal_angle_enable = 0U` is OFF at boot.
  Thus the current motor behavior remains unchanged after flashing.
- When enabled, and **only** when `mode==3` (D-pad and RX) with
  both axes in the ordinary MOVING phase, the waypoint planner
  calculates a *shared calibrated angular stroke* as the **smaller**
  of the two signed soft-target angular travel magnitudes.
- It constructs equal-magnitude, separately signed virtual goals
  for relative progress and waypoint telemetry. Each motor's
  original independent soft stop, own NC limit, AS5600 hard envelope,
  PWM limits, timeout, stall and STOP continue to decide commands.
- A wrong-direction angle cannot count as forward progress; the
  existing `Steering_AngleStrokeProgress()` logic clamps it to 0.
  A too-short common stroke (< 5 degrees) falls back to legacy
  progress rather than inventing a new motion command.
- The opt-in flag is **latched only at the start of a new, valid
  manual command**; changing Live Expressions during a stroke
  cannot switch the planner midway through that stroke.
- R1/R2 are unaffected by the new progress metric, including
  RIGHT R1 effective baseline 15 RAW / PWM85 / 12ms.
- No additional I2C transactions; no new UART output; no adjustment
  of `steer_angle_wp_lead_permille=30`, steering PI gains or
  manual/position PWM ceilings. Already-tested NC/STOP and
  per-axis final completion remain authoritative.

## In-place, low-risk A/B on suspended chassis

1. In the **existing** project, check `git status --short`; safeguard
   any uncommitted edits; `git fetch origin` and
   `git merge --ff-only origin/feature/m7-s15-ps2-controls-restoration-staged`.
   Run `python tools/verify_m8_s13_manual_angle.py`,
   `python tools/verify_m8_concurrent_drive_steer.py`,
   `python tools/verify_m8_r1_center_concurrent.py`,
   `python tools/verify_m8_s11_csv.py` and any other configured
   safety checks. Then Clean Build and flash **once**.
2. Verify all motor CCRs zero, STOP/NC available, supply cutoff
   independently reachable, PS2 arm controlled. With the vehicle
   fully on a rated stand, start M8-S11 CSV on the PC.
3. **A = existing baseline:** with all motors stopped, write
   `steer_angle_wp_manual_equal_angle_enable=0`. On a fresh PS2
   neutral->direction press, test short D-pad LEFT/RIGHT then RX
   LEFT/RIGHT. Log all four runs, keeping steering distance and
   initial positions comparable.
4. After all six motors stop and manual owner is neutral,
   **B = new metric:** write
   `steer_angle_wp_manual_equal_angle_enable=1`. Repeat the four
   short runs. No rebuild/reflash/new workspace is required.
   If a direction behaves incorrectly, release PS2 command and
   use the independent motor cutoff if necessary; restore flag 0
   while stopped, then require a fresh neutral input.
5. End M8-S11 stream via `m8_s11_stream_enable=0` and wait for
   complete `#M8S11_END`. After disarming and releasing the COM
   port, dump M8-S00 TXT if the same MCU session remains powered.
   Do not press USER during live CSV capture.
6. Compare mean/peak signed **calibrated-angle difference**
   (on same initial angle stroke), `M8S00_TRACK`
   mean/peak/over30, PWM saturations, any unexpected PWM=0,
   AS5600 sample ages, physical jitter, independent stops, NC
   trips and STOP behavior. Compare A vs B on matching directions;
   do not use mixed metric `M8S00_TRACK` values without labeling.
7. After normal manual tests, repeat one safe R1 CENTER and one
   position-only R2/NC test where previously validated; R1 must
   remain LEFT 1873 +/-5 RAW and RIGHT 662 +/-5 RAW. No newly
   unexpected NC, runaway, manual blocking or altered direction.
   Ground-load validation is a separate safety gate.

## Acceptance boundary

No M8-S13 PASS yet. Hardware A/B, STOP, AS5600 validity and
mechanical wheel-angle mapping are **NOT** certified by static or
CSV replay inspection. If B does not consistently improve true
angle tracking without new stops/jerk, keep option 0 and review
geometry or output control separately. Never raise PWM to force
a result without a new test-backed limit.
