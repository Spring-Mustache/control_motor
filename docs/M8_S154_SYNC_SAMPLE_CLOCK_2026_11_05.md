# M8 S15.4 sample clock and per-run skip diagnostics — 2026-11-05

## Evidence and distinct issue

Operator supplied a private six-line hardware log stamped `699932abbe09e875a776f53403a18d7743171b9f` (file remains private, NOT uploaded to GitHub). The trial RIGHT RAW-rise 10-RAW handoff clearly worked in the three measured rightward Voice 15/30/40 strokes: pulse counts from prior log 1/6/6 to 0/0/0, reverse counts 0/1/1 to zero, rightFineMs=205 for those three, settled errors +4/+1/+2 RAW (within +/-5). Total command ms 3159/3710/4226 before -> 1563/2319/2835 after, but starts differ and no repeated randomized trials. Leftward command 15/30/40 has RIGHT pulse counts 1/2/3, terminal right errors -4/-5/-5, so it still merits analysis; do not alter LEFT or further widen control envelope on this branch.

**Critical discovery**: `syncFrames=0,peakGap=0` across all six trials; thus the S15.4 active pace function is *not evidenced* during these Voice4 runs, despite boot flags `steer_voice_s154_enable=1`, `steer_angle_wp_enable=1`. Coarse appearance alone does not prove coordinator involvement.

Source inspection identifies a plausible software time-reference mismatch: `Steering_PositionUpdate()` captures `now = HAL_GetTick()` **before** two `Steering_PositionReadOne` AS5600 calls. Each read independently stores `steer_pos_left_last_sample_ms` or `steer_pos_right_last_sample_ms` using a later `HAL_GetTick()`. Old `Steering_VoiceS154Sync(now,...)` then subtracts those later stamps from earlier `now` as unsigned ages, making a fresh sample look ~2^32 ms old and reject coordinator pacing (status=4) if a SysTick boundary is crossed. This is a supported software hypothesis, not yet proven as the exclusive physical reason for 0 frames.

## Isolated source change

New branch `feature/m8-s154-fresh-tick-sync-telemetry` forks verified trial SHA `699932abbe09e875a776f53403a18d7743171b9f`; existing successful RIGHT fine branch is untouched. Change ONLY the Voice S15.4 call site to pass `HAL_GetTick()` evaluated after both per-axis AS5600 checks. Leave the existing `now` untouched for all other control/safety/escape/stall/timeout uses, and leave PWM/NC/STOP/PS2/RIGHT10 handoff unchanged. This may activate pacing that previously remained ineligible, so real motor behavior can change and requires cautious small-angle stand test.

Add resettable **read-only** per-Voice-run skip counters recorded in the existing automatic `M8S154_FINE` row: `syncDisabled` (enable flag not 1), `syncPhase` (axes not jointly moving coarse), `syncStale` (zero duty/future/stale timestamp/floor), `syncNc` (physical NC asserted), `syncWaypoint` (waypoint geometry or eligible sample rejected). `syncFrames` counts successfully paced frames per run; only when >0 may operator say coordinator actually ran. `peakGap=0` with `syncFrames=0` is not evidence of perfect synchronization.

All existing test flags boot 1: `steer_voice_s154_enable`, `steer_angle_wp_enable`, `steer_voice_s154_right_fine_diag_enable`, `steer_voice_s154_right_late_fine_enable`. No manual debugger edits; no new motor enable or baud/protocol change. Existing native PowerShell `tools/capture_m8_s154_right_fine.ps1` still captures rows automatically.

## Test/acceptance

Static production ordering, previous S15.4 extracted actual-C HOST tests, RIGHT-fine compiled fixtures, source safety checks, wrap-safe age unit test and Windows PowerShell parser all must PASS on new **exact SHA**. This is NOT an STM32CubeIDE build, flashed ELF or powered motor proof.

Only after Clean Build/Flash exact new SHA, nonpowered NC/STOP checks, secured chassis with independent motor power cut: begin with a small Voice 5° or 15° in each direction, monitor abnormal sync movements, timeouts, unexpected NC or wrong behavior, and stop immediately if needed. Because this newly enables an earlier inactive coordinator, do not assume RIGHT 10-RAW results will stay constant. Keep private TXT, do not commit operator log. Then, only if safe, repeat six Voice 15/30/40 tests and send fresh TXT. Compare active `syncFrames`, reason counters, per-axis done times, pulses, reversals, final errors, total time and zero faults. Original SHA 699932abbe09e875a776f53403a18d7743171b9f is rollback point if new pacing degrades.

Five-tier status: SOURCE-STAGED, HOST PASS, CubeIDE BUILD, FLASH, HARDWARE PASS separate.
