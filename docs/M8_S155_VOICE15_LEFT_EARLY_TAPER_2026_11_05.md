# M8 S15.5 — Voice LEFT 15° earlier nonzero leader taper experiment (2026-11-05)

## Baseline evidence and objective
New isolated trial branch `feature/m8-s155-voice15-left-early-taper` forks **exact physical 6/6 pass** SHA `9cd6067cf9e7a9885ee4c36121e3912e80956860`; the old branch is preserved. Operator's private third-fourth hardware logs remain PRIVATE and are not committed. Latest run peakGap permille for Voice LEFT15 / RIGHT15 / LEFT30 / RIGHT30 / LEFT40 / RIGHT40 = 214/52/40/25/49/62; translating to worst-point (not mean) trajectory match 78.6/94.8/96.0/97.5/95.1/93.8%. Only LEFT15 is below >90% target; the threshold requires **peakGap <100 permille** on every run. Completion: every 6/6 resultL=resultR=finish=2, RIGHT fine pulse count0 and ±5 RAW error. S15.4 actually ran (syncFrames75/108/139/170/185/209). All NC/STOP/Voice45 functions not claimed newly tested.

## One bounded experiment
New `steer_voice_s155_left15_taper_enable=1U` boots ON; zero operator debug variable editing. Only when voice position is owned, mode4 (5..40 position), angle EXACTLY 15, turn LEFT, and both axes in S15.4 healthy moving coarse, the currently *leading* motor's already-existing phase taper reaches its pre-tested **nonzero floor** at a normalized gap of 100 permille instead of 150: existing deadband lead30, existing ramp120 -> trial ramp70. No changes to PWM rise ceilings, floor left175/right50, per-axis max left300/right100, motor direction, follower boost and integral law, steering angle mapping or common-progress goal. This does not force PWM0; both coarse motors must already be permitted by the same NC/IRQ and fresh AS5600 guards. When no 15LEFT eligibility, extracted actual C verifies fallback tapered PWM **bit-for-bit** across thousands of command/sensor inputs.

Existing RIGHT 10RAW fine-entry gates in *both* directions, ≤8 fine pulses, 85CCR/12ms RIGHT fine, ±5 RAW, 200ms off/100ms quiet/120ms capture, Voice45 expected OWN physical NC, R1/R2, STOP/PS2, travel timeouts and S15.4 post-I2C fresh tick stay untouched. Trial does not auto-ARM or grant motor power. More aggressive leader taper can slow motion and possibly worsen 5s timeout or arrival skew; **>90% is a physical acceptance target, not a guarantee**.

## Auto diagnostics
Extend the existing safe-idle UART3 single-line `M8S154_FINE` log with:
- `peakLeader`: 1 means LEFT was ahead at largest gap, 2 means RIGHT;
- `peakAtMs`: milliseconds into command at first largest gap;
- `earlyTaperFrames`: controller frames where S15.5 was eligible and actively reduced a leader's proposed PWM.
Old peakGap, resultL/resultR/finish, skew, individual done times, RIGHT pulses, entries, syncFrames and skip counters unchanged. The existing Windows PowerShell collector still auto captures max6 lines, no manual debug writes and no extra serial tools.

## Test procedure and rollback
Check exact-SHA GitHub full context HOST, compiled-C helper, and Windows PowerShell syntax SUCCESS. These are not target BUILD/FLASH/HARDWARE PASS. Operator should preserve local changes then fetch/switch trial branch, STM32CubeIDE Clean Build/Flash exact SHA and check ELF stamp. Preflight NC/STOP/AS5600 with motor supply isolated, then secure vehicle on rig with independent motor power cut. Start unchanged `tools/capture_m8_s154_right_fine.ps1` first. Begin with LEFT15, monitor sudden slowing, stall, timeout, overshoot, NC or unexpected motor behavior; immediately stop if abnormal. If safe, repeat the same six Voice 15/30/40 left/right commands (initial positions can differ) and send private TXT. Check all six peakGap <100, completion result=2, ±5 RIGHT RAW final error, no new pulses/reversals, syncFrames>0, NC/STOP healthy and compare skew/time against baseline. Do NOT edit speed further before this log.

Rollback by rebuilding and flashing the physically tested SHA `9cd6067cf9e7a9885ee4c36121e3912e80956860`, without destructive git operations to the user's tree. The trial branch has no physical pass yet.
