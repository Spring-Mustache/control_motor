# S15.4 RIGHT Voice5–40 passive fine alignment capture — 2026-11-05

## Status and purpose
User physically observed S15.4 common-progress travel and good LEFT finish, but RIGHT had extra reversing short-pulse fine corrections for Voice 5°–40°. The cause is not yet established. This is SOURCE-STAGED diagnostic only; do NOT change target speed, pulse PWM, direction, NC or fine tolerance before target Build, safe hardware tests and returned log.

## Branch and known-good rollback
Trial branch `feature/m8-s154-right-fine-passive-diag` forked from exact baseline `1f9334088f4d06349abc862316f5e5ef8800ee05`. The old branch is preserved. No changes to `Steering_VoiceS154Sync`, the LEFT or R1/R2 servo tuning, Voice45 physical NC completion, STOP/PS2, current limits, global timeout/stall, or AS5600 sampling.

## Boot enabled without manual writes
Existing `steer_voice_s154_enable=1`, `steer_angle_wp_enable=1`, `voice_pos_enable=1`, `voice_angle_mapping_enable=1` remain on. New `steer_voice_s154_right_fine_diag_enable=1` is ON at boot. It is not an actuator enable and does not cause motion. Existing guarded auto-ARM preflight still applies.

## Diagnostic format
After a Voice command4 (5°–40°) completes or stops, snapshot is taken before clearing the Voice owner. `M8S154_FINE` includes built-ELF SHA, sequence, angle/turn, totalMs/fineMs, RIGHT RAW start/goal/first fine entry/first quiet/end, signed error, pulse count, count of fine direction reversals, weak/good/backtrack pulses, last settled pulse move, maximum pulse duration, resultL/resultR/finish code, peak S15.4 progress gap and arrival skew. `dropped` counts later records not overwritten while one is pending.

UART3 uses existing M8S11 IRQ transmit buffer, not HAL blocking TX. Sends a single line only with no concurrent M8S11 stream/dump, UART ready, both steering owners stopped, wheel-stop guard and both steer CCR=0. It may wait for a safe window if auto-ARM remains active; no user flag updates or button press are needed. This is not continuous RAW trace or proof of motor current/coast.

## Test on the vehicle
Check exact SHA GitHub CI and then update only the clean local main project at `D:\STM32 task\control_motor`. Clean Build/Flash in CubeIDE and inspect matched ELF. Check NC, PS2 STOP and AS5600 with motor power isolated; proceed only when mechanical cutoffs, secured chassis and independent supply cutoff are verified.

Start native PowerShell capture *before* testing, from repository root (ST-LINK VCP autodetected when unique):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s154_right_fine.ps1 -TimeoutSeconds 1200 -MaxEvents 6 -OutputDirectory .\logs
```

For example try both directions at 15°, 30°, 40°, increasing only if previous angles safely finish. No requirement to fill six lines if movement is abnormal. Do not use Live Expressions, START/USER dump triggers or a serial terminal; this script reads data automatically and never sends motor commands. Preserve partial TXT and send privately. Do not post raw hardware logs on public GitHub without permission.

## Acceptance and next step
Exact-SHA HOST test is not a CubeIDE build or motor PASS. Analyze result codes (2 for Voice 5–40 encoder completion), ±5 RAW settled error, fine pulses/reversals, time, geometry and missing/dropped records. Only after identifying which phase is responsible should a separate boot-OFF actuation-tuning branch be proposed. Keep baseline S15.4 behavior and safety immutable until evidence exists.
