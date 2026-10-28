# M8 — independent wheel drive and steering (2026-10-28)

**Status:** source change committed on feature branch. No fresh CubeIDE target build, flash, powered bench validation, or ground-load acceptance was performed after this change. This is **not** hardware PASS.

## Why the architecture changed

A captured ground-load M8-S00 sample recorded PS2 LEFT held, no wheel drive PWM, RIGHT UP wheel encoder +2.8 RPM, `manualBlock=4`, no NC trip and valid steering progress. The operator independently observed passive wheel displacement when the offset steering pivot scrubs against the ground. The old universal 2-RPM wheel-stopped gate caused a manual steering stop and forced a new D-pad press. This does **not** imply an encoder electrical fault.

Normal PS2 driving requires the 4 traction motors and 2 steering actuators to be independently commandable at the same time. The old code could erase steering CCR when LY/Up/Down was active, zero wheel target while R1/R2 was active, and reject D-pad/RX and R1/R2 due to nonzero wheel RPM.

## Changes in main.c

- `PS2_ApplyManualControl`: LY/Up/Down continues through `WheelControl_SetTargetPWM(pwm_duty)` even when D-pad/RX or R1/R2 steering is active. Remove the old throttle-induced `Turn_Motors_Stop`. Idle steering CCR is zeroed only without an active/pending manual or position steering owner.
- `Steering_ManualUpdate`: no LY/Up/Down neutral gate and no `Steering_PositionWheelsStopped()` gate. Normal wheel encoder motion no longer triggers `manualBlock=4` when steering. L1/L2 in-place spin, other steering owners, STOP, voice, debug conflicts and required steering permits still block.
- `Steering_PositionUpdate`: R1/R2 steering is allowed with LY/Up/Down traction; L1/L2 active/decaying spin remains exclusive. Other mode ownership, NC, AS5600 and position bounds remain.
- `Steering_ManualDirectionNeutral`: a steering-direction release resets the manual steering rearm latch even while LY/Up/Down stays active. The original `Steering_ManualInputsNeutral` is unchanged for START arm and global steering safety rearm.
- `PS2_RestoreRouteUpdate`: normal wheel+steering is not reported as a prohibited combination. SELECT calibration and L1/L2 spin are still incompatible; the wheel-stop observer bit applies only to SELECT.
- `ps2_drive_steer_request_frames`: read-only counter of concurrent wheel+steer input request frames; not proof of actual physical actuation or safe operation.

## Safety boundaries retained

- START held ~1.5 s with all controls neutral and wheels stopped still authorizes steering; fresh START press disarms.
- Four NC switches cut corresponding steering PWM in IRQ and are checked before further PWM. AS5600 live validity, calibrated hard envelope, soft clearances, stalls/timeouts, anti-reverse and per-axis completion still apply.
- Global STOP, steering safety trip, RF/PS2 loss, Voice cancellation priority remain.
- L1/L2 in-place spin and SELECT calibration are **not** part of normal concurrent drive+steer. SELECT still requires full neutral and wheels stopped.
- Blocking M8 snapshot UART export continues to require complete disarm and standstill. It is **never** permitted while motor control is active.
- Zero PWM is not an independent motor power disconnect. The RIGHT driver has previously exhibited unintended movement at zero request; confirm a working physical motor supply cutoff before powered testing.
- The code does **not yet implement** chassis speed compensation/steering inverse kinematics for the four rolling wheel contacts. Concurrent mode might scrub tires, pull against linkages or increase steering motor load.

## Static checks (run in PowerShell at repository root)

```powershell
python tools/verify_m8_concurrent_drive_steer.py
python tools/verify_m8_s00_capture.py
python tools/verify_ps2_restore.py
python tools/test_s16_reassembly_geometry.py
```

Next run CubeIDE **Debug Build**; do not regenerate from the currently divergent .ioc file. Build success verifies only compilation, not physical safety.

## Staged commissioning (not yet performed)

1. Keep motor supply independently disconnectable. Test NC, driver OFF behavior, PS2 radio STOP/disarm and steering sensor validity in a controlled setup before powering any wheel or steering movement. If any motor energizes without command, cut external supply, do not continue.
2. Secure the chassis on a rated stand with all wheels clear of the ground and no person within actuator sweep. Start each test with neutral inputs and no motor motion. Hold START 1.5 s then release, return to neutral and let the steering neutral rearm settle.
3. Test separately: low LY wheel drive alone; low-angle D-pad/RX steering alone. Observe correct direction, limits, stops and no uncontrolled motion. Then test low LY **while** D-pad or RX is held briefly, return steering input to neutral **while keeping LY**, then release LY. Expected: traction continues after steering releases; steering can move while traction is commanded. Repeat with one controlled direction reversal only after neutral. Do **not** test L1/L2 or SELECT concurrently.
4. Only after manual combo is verified, check R1/R2 with a low LY input, under the same suspended conditions, away from endpoints. Observe both axes' AS5600 position, PWM and timeout/limit behavior; do not run into physical NC on an unvalidated coast distance.
5. Disarm with a fresh START press, release all inputs, wait for all motion to stop. Collect one M8-S00 TXT via `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1` then a **short USER press**, without resetting MCU first. Send entire file.
6. Ground-load validation is a separate stage after stand tests, with a clear test zone, proven independent motor cutoff and restrained low-speed operation; no full-speed racing or uncontrolled turn in this phase.

M8-S00 and concurrent control remain **UNVERIFIED** until the rebuilt image and staged actual motor tests pass.
