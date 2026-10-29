# M8-S11: six-motor live CSV regression (staged; hardware unverified)

## Historical logger recovered

- Old M6-S03 logger: Services/Src/ram_logger.c and Services/Inc/ram_logger.h.
- Storage: .noinit.wheel_log linked into SRAM (NOLOAD); NOT a Flash sector.
- Capacity: 6000 samples x 72 bytes = ~432 KB, about 20 ms per sample.
- USER short press sequence: START, STOP, UART3 CSV DUMP after motor stop.
- A completed capture can survive warm reset while SRAM remains powered,
  but not board power loss. Native PC helper in tool_archive/ground_log_capture.py.
- M8-S00 exports and tools/capture_m8_s00.ps1 remain available.

This branch adds a SEPARATE opt-in USART3 live serial stream. It stores
CSV rows directly to the PC drive, not into a new MCU Flash partition or
a large RAM history. Only one static 640-byte UART TX staging buffer is added.
If the PC is absent or power/USB is lost, no on-device backup exists for
this new stream.

## Risk boundaries

- Base commit 54d3bbf (corrected LEFT D-pad physical DIR and NC guards).
- Default m8_s11_stream_enable=0; user must explicitly opt in.
- UART3 115200 8N1, nonblocking HAL_UART_Transmit_IT, priority 8 (lower
  than NC EXTI and VC02 USART6 priority 0).
- Period 50..250 ms, initial trial recommended at 100 ms.
- All 60 telemetry fields are reads of existing state/cached sensors.
  The logger neither calls a motor command nor performs new I2C accesses.
- Legacy blocking CSV/M8S00 exports are deferred while live UART TX owns
  the port; do NOT press USER during live capture.
- Asynchronous TX IRQs and formatting still have some CPU cost.
  This branch is NOT CubeIDE-built or hardware-qualified yet.

## Safe Windows workflow

From existing project folder (preserves uncommitted local files):

    git fetch origin
    git worktree add --detach "..\control_motor_m8_s11" origin/feature/m8-s11-six-motor-live-csv

Import the worktree from a different CubeIDE workspace to avoid duplicate
project-name collisions. Clean Build, flash, check all PWM zero/STOP ready
before continuing. Open only one program on the ST-LINK VCP COM port.

From the M8-S11 worktree:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s11.ps1 -Port COM5 -TimeoutSeconds 240 -OutputDirectory .\logs

Replace COM5 with the correct port. Start the script BEFORE the following
Live Expressions writes, while all motors are stopped:

    m8_s11_stream_period_ms = 100
    m8_s11_stream_enable = 1

Run a short suspended-chassis test. Do not halt CPU with breakpoints while
motors are powered. Stop all motors, then set:

    m8_s11_stream_enable = 0

Wait for #M8S11_END. The script creates a CSV file and a metadata text
file on the PC. Submit BOTH files; reject incomplete files, dropped
sequences or MCU TX errors as conclusive evidence.

## Main captured measurements

- seq, ms: MCU timestamp and sequence; detect timing gaps.
- PS2: mode, ly, rx, dleft, dright, r1, r2.
- Wheels: pid_en, pid_on, pid_ticks, signed left/right target RPM*10,
  LU/LD/RU/RD measured RPM*10, signed PWM, signed encoder deltas,
  and four filtered RPM feedback values ACTUALLY passed into PID.
- Steering: cached AS5600 left/right RAW, age since I2C sample, soft
  goals, requested physical direction, actual DIR pins, actual CCR
  outputs, manual/position state and result, fine phase and position error.
- Safety: sampled NC mask, trip, stop and owner-active flags. A brief
  IRQ event between sample instants might not appear: keep the existing
  M8S00 fault/arrival snapshot for that evidence.

## Regression test A: four wheels, separate CSV

1. Chassis fully supported, steering stationary, STOP available.
2. With PI disabled ONLY while stopped: small forward and reverse
   wheel commands. All four physical wheels must match new direction;
   normalized encoder deltas and signed measured RPM must match target
   sign. If not, stop and do not enable PID.
3. With all wheels stopped, re-enable PI. Low target, then steady
   20-30 RPM forward/reverse separated by full stops. Confirm pid_ticks
   increases and all four filtered feedback values track correct
   signed targets without sustained saturated PWM or runaway.
4. Compare overshoot and steady-state error with the old M6 CSV at
   the same demand. Wheel RD has separate reverse friction FF offset,
   so previous physical calibration may need revalidation.
5. L1/L2 spin requires both MIN NC switches physically asserted and
   must be tested separately after steering interlock passes.

## Regression test B: two independent steering axes, separate CSV

From CENTER left 1873 and right 662:
- D-pad LEFT short pulse: req_l=+1, req_r=-1, actual DIR L=1 R=0,
  both raw angles should decrease (L toward MAX, R toward MIN).
  Then ONLY if this is verified allow a longer soft-stop trial:
  target L_MAX soft ~1454, R_MIN soft ~216.
- D-pad RIGHT: req_l=-1, req_r=+1, actual DIR L=0 R=1,
  both raw angles increase (L toward MIN soft ~2225,
  R toward MAX soft ~999).
- R1 from several starting angles: LEFT CENTER 1873 +/-5 RAW;
  RIGHT CENTER 662 +/-5 RAW.
- R2 ends only with own physical MIN NC, not on a soft angle.
- Unexpected NC, wrong physical motion, bad sensor age or STOP
  failure: stop immediately; do NOT drive to the mechanical limit.

## Source preflight

    python tools/verify_m8_s11_csv.py

This is a STATIC protocol/safety check. It cannot certify MCU compilation
or physical safety. Testing initially at 100 ms reduces serial TX load;
a follow-up 50ms test should specifically compare PID timing and RPM.
