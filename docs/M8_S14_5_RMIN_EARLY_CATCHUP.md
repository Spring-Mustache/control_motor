# M8-S14.5 — RIGHT R_MIN early catch-up, Level 1 opt-in

Status 2026-10-31: **CODE STAGED, not STM32 CubeIDE-built/flashed or physically tested.**
Branch `feature/m7-s15-ps2-controls-restoration-staged`, parent `0b2c452c1a7b8dc24261ed00b498a25b4d5c3d80`.

## Measured problem

User M8-S11 2026-10-31 15:11:30, 1722 valid rows in 92.13 s, zero missing rows/UART errors. Paired D-pad LEFT near-full-range strokes: RIGHT reaches R_MIN software boundary about 460–558ms after LEFT, around 0.5s late. At RIGHT remaining ~500 to ~450 RAW, its command was commonly 125 CCR despite available Level-1 cap 150 CCR; higher PWM near 320 RAW occurred only briefly. Current Level-1 control first trims the faster leader or boosts the follower using measured angle+angular velocity, but its objective is **equal instantaneous angular travel**, which does not anticipate RIGHT's much slower last 100-RAW approach. The last 100 RAW commanded RIGHT duty falls to ~50–100 CCR under the safety coast taper, while LEFT finishes first.

Other same-boot diagnostic `M8_S00_20261031_151417.txt`: S14.2 level1, S14.3 enabled grants24 fastAxis46 fallback0; no FAULT registered in this snapshot. Recorded RAW extrema from CSV include L_MIN ~1430 vs soft1454, R_MIN ~209 vs soft216, R_MAX ~1020 vs soft999. Soft targets are **not mechanical NC triggers**. These values mean actual coast/physical clearance remains unvalidated; a higher PWM in final approach is not an acceptable unverified shortcut.

## Change and engineering choice

Introduce boot-OFF experiment `steer_s144_rmin_catchup_enable=0`. **Only** S14.2 Level1 D-pad LEFT moving RIGHT toward R_MIN, with both S14.2 owner PWM nonzero, fresh AS5600 and known calibrated trajectory, may ask the existing S14.2 P+D planner to target a modest early RIGHT lead. It does this by adding a 0–200 centidegree (0–2.0°) calibrated angle *priority bias* to the tracking gap; the actual measured gap remains unchanged and is still used for un-biased `M8S00_S142_SYNC.predict`. It is not a new speed loop, does not directly write PWM and cannot increase per-axis cap 375/150.

- Only eligible if RIGHT is not already leading by more than 2.5° and BOTH axes remain farther than 320 RAW from their respective SOFT goals. When minimum remaining is >=380 RAW, bias is 2.0°, then linearly fades over 380→320 RAW to zero. At or inside 320 RAW the original unmodified S14.2 plan is used; the full 320→180 and 100→0 RAW braking/clearance zones remain untouched.
- Existing S14.2 P+D selects whether to boost RIGHT or LEFT and when to taper leader. The original 12-CCR rise slew, independently calculated `M8S142_Envelope`, fresh samples, wrong-direction/stall, NC/STOP/EXTI atomic CCR gate and early independent SOFT completion are unchanged. R1/R2, RX, D-pad RIGHT, Level0, Level2, 4-wheel PI and START authorization unchanged.
- No change to boot defaults for proven settings: `steer_s142_trial_level=1`, `steer_s143_fast_start_enable=1`, wheel PID enabled, UART streaming disabled by default.
- Diagnostics in safe after-disarm M8-S00 dump: `M8S00_S144,enable,biasFrames,rightPriority,lastBias,peakBias`. `rightPriority` counts planner frames with more RIGHT boost than original unbiased S14.2; this counter does not prove faster physical completion.

## Tests / safeguards

`tools/test_m8_s144_catchup.c` exercises the exact helper and original S14.2 planner using host GCC; `tools/verify_m8_s142_speed.py` runs it alongside prior S14.2 tests, and GitHub Actions runs the verifier before issuing the context release. Target CubeIDE Clean Build and Flash are still pending; C host tests alone cannot validate motion or current/clearance.

Commission with **independent motor supply cutoff** and NC/STOP tested before powering on, robot on a rigid suspended stand, mechanical clearance confirmed:
1. On the new firmware keep Level1/FastStart at boot and `steer_s144_rmin_catchup_enable=0`. Record 5+ full D-pad LEFT strokes toward R_MIN *only if existing stopping clearance is verified*, alternating with known safe R1 or opposite return. Keep center-to-mid-range short tests first. Record 50ms M8-S11 + M8-S00.
2. Fully stop, enable `steer_s144_rmin_catchup_enable=1` (do not change during powered command), and repeat comparable starts on stand. Compare RIGHT vs LEFT arrival timestamps, AS5600 angle gap, actual PWM, NC/STOP, coast beyond SOFT, mechanical clearance and current/driver temperature. If NC actuates unexpectedly, temperature/current abnormal, or coast worsens, disable while stopped and halt high-power tests.
3. Only if A/B shows better timing **without worse coast or faults** consider keeping the feature, later Level2/current/load work remains separately gated. **Never enable S14.5 by default without these tests.**

A 0.5s gap may **not** disappear: the algorithm only anticipates the right's slow final approach using existing early headroom, not add energy in the last 320 RAW. If a ~0.5s lag remains, reassess mechanical drag, friction, sensor calibration, physical dynamic braking and clearance before changing power nearer the limit.
