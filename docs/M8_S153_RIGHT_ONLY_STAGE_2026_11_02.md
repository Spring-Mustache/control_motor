# M8-S15.3 — RIGHT-only opt-in controller (2026-11-02)

**Status: SOURCE/HOST candidate. Defaults OFF. No CubeIDE clean target build, flashed ELF/SHA comparison, current/coast/clearance, or suspended/ground hardware acceptance yet.** This is not a claim the prior 625–649 ms RIGHT terminal lag is solved.

## Evidence / decision

Collected privately by PowerShell: `M8S152_four_tests_20261102_132140.txt` (four strokes, first D-pad LEFT short/invalid) and replacement `M8S152_four_tests_20261102_132923.txt` (valid D-pad LEFT).

| Tested manual command | LEFT reaches SOFT | RIGHT reaches SOFT | RIGHT later |
|---|---:|---:|---:|
| D-pad LEFT replacement | 1,957 ms | 2,606 ms | **649 ms** |
| D-pad RIGHT | 2,173 ms | 2,198 ms | 25 ms |
| RX LEFT | 1,885 ms | 2,510 ms | **625 ms** |
| RX RIGHT | 2,161 ms | 2,294 ms | 133 ms |

All four complete runs reached both respective firmware SOFT goals; S15.2 yielded valid angles, finite predicted data and zero lost trace rows. No fault was recorded during these runs. These are independent **SOFT completion times**, not proof of Ackermann relation or external motor cutoff.

In replacement D-pad LEFT at LEFT completion (~1957ms), RIGHT was **51 RAW** from its SOFT; RIGHT continued another 649ms, owner duty falling ~75→50CCR. S14.2 counters from that run: boost LEFT 89, trim RIGHT 44, boost RIGHT 0 **requests**, not proof all were applied. Hypothesis: baseline S14.2 may worsen terminal arrival by RIGHT trim while its angular-progress predictor sees RIGHT ahead; can only assess experimentally. S15.3 refuses to add PWM once either distance <=180 RAW, so it **cannot directly correct the last 51 RAW**. No invented estimate of improvement.

## Actual source change

- New `Core/Inc/steer_s153_right_only.h`: **pure C**, normalized proxy error from existing S15.2 observer; no motor IO. RIGHT can only receive **0–12 CCR above its own owner request**, **never above existing M8S142 right envelope (maximum 200 CCR)**; LEFT request is never modified, no taper/hold/brake of either motor from this coordinator.
- `steer_s153_right_only_enable=0U` boot default, **latch only at start of a new legitimate PS2 manual D-pad/RX stroke with Level2**. Existing S15 common-T gets priority. When S15.3 is latched, its RIGHT candidate replaces and **discards** S14.2's RIGHT output (including RIGHT trim), S14.7 PI/S14.9 lead/S14.5 bias are excluded. For LEFT only, the existing S14.2 positive correction is still calculated and preserved; an S14.2 LEFT taper is clamped to the untouched LEFT baseline owner. This is explicit split-axis arbitration, NOT additive or two competing RIGHT owners. Existing standard Level2 owner is untouched while S15.3 disabled.
- Reject RIGHT change if either owner command is zero; either axis is <=180 RAW from SOFT; either axis has moved less than **12 RAW** from its valid starting sample; calibrated AS5600 angles/timestamps or speed history are invalid/stale; any NC mask is active; motion permit not granted or STOP/trip active. No new I2C reads. Zero `predValid` is fail-closed. State/timing guards continue to apply for BOTH axes.
- A small `M8S153,...` diagnostic line is sent through the existing fully-disarmed M8S00 UART dump and receives only via native PowerShell `tools/capture_m8_s152.ps1` (no terminal app, Live Expressions or S11 required). The existing 96-row M8S152 samples remain unchanged. Diagnostic `requestFrames` is pre-atomic, `appliedFrames` compares post-atomic RIGHT CCR with its baseline owner; neither is a measurement of physical motion.
- Existing final atomic NC/IRQ+GPIO+PWM arbitration, STOP, SOFT independent owner and motor-driver cutoff remain authoritative. S15.3 cannot energize CCR0, re-start DONE, or override an NC fault.
- The automatic memory/resource costs are one small AS5600 observer and diagnostic counters; no extra per-frame RAM ring, no extra UART in powered control and no EEPROM/Flash state.
- This is an **experimental right-only replacement**, not a tuned Ackermann controller. The S15.3 route now preserves **S14.2's existing LEFT positive boost** or the owner baseline (whichever is higher). It never reduces LEFT below its baseline. The discarded S14.2 RIGHT trim/boost is replaced by S15.3 RIGHT-only. LEFT performance and control-loop latency still require actual hardware A/B measurement; source arbitration alone does not prove physical equivalence. Do not assert parity without measured MCU data.

## Release gates

1. Source/static/GCC HOST regressions including nested S14/S15, unchanged S15.2 observer, disarmed PowerShell protocol: GitHub Actions against exact SHA.
2. CubeIDE clean compile/link and ELF identity with final SHA, without blind force-update or untracked-file loss. Only then flash if physical gate allows.
3. **Before enabling S15.3 (default OFF):** independent physical motor supply cutoff, safe supported chassis, verified NC/STOP/soft, current/temperature measurement and measured coast/clearance at RIGHT R_MIN; historical measured ~3mm R_MIN→NC is **not** adequate assurance that increased PWM is safe. Do not recommend skipping these checks.
4. After hardware gate, first limited **non-terminal mid-stroke** observation at constrained RIGHT owner limit; stop early, compare feedback with baseline, then only if safe consider controlled A/B. Do not turn on S15.3 mid-stroke; enable must be selected before a NEW neutral-to-manual transition and verified in next M8S153 line after disarm.
5. Compare same starting calibrated angles, joystick magnitude and motor supply/load, collect output **only using native PowerShell** (`capture_m8_s152.ps1`), and verify `M8S153,last=1` and `appliedFrames>0` for a genuine active correction. `frames>0` alone is not proof of change. Separately assess normalized angle error over time, RIGHT SOFT arrival, LEFT responsiveness, NC, stop and driver currents. No arbitrary PASS tolerance without measurement.

**Not yet achieved:** CubeIDE build, flash verification, physical safety, power current/coast data, target input-to-CCR latency or demonstrable reduction of the 625–649ms RIGHT terminal lag. S15.4 R1/R2 and VC02 phases unchanged.

## S15.3 actual STM32CubeIDE build failure and fix (2026-11-02)

Operator's **real CubeIDE build** of SHA `0cb9df9` failed in `Core/Src/main.c`: S15.3 function appeared at line 3422 **before** declaration of `Steering_PositionRawInRange(uint8_t,uint16_t)` at line 4071 and **before** `#define STEER_MANUAL_MOVING 1U` at line 5553. GCC reported 3 errors, 1 warning. Previous HOST/CI success did not compile full STM32 `main.c`: **do not call 0cb9df9 target build PASS**.

**Fixed source:** move the entire unchanged `Steering_S153RightApply` function down to immediately before `Steering_ManualUpdate` (after state macros and Range function). No motor-control algorithm, gating, PWM, logging or PS2 owner edits. Add explicit static source ordering check to `tools/verify_m8_s153_right_only.py`. Verify CI exact updated SHA, then operator re-runs STM32CubeIDE Clean Build; cannot claim target PASS until this is reported. Physical safety/current/coast gates remain pending; keep `steer_s153_right_only_enable=0U`.
