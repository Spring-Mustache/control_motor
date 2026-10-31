# M8-S14.3 — Prequalified-neutral low-latency steering
Status 2026-10-31: SOURCE-STAGED; no new target build/flash/hardware validation.

Operator reports S14.2 Level2 (450/200 CCR) speed pleasant but ~0.5s steering onset lag during concurrent six-motor stand test. M8S11 13:21 meta: 1461 rows, 0 dropped/UART errors. Prior assistant derived ~122ms LEFT and 183ms RIGHT from decoded PS2 request to CCR; ~366ms to >=8 RAW. Not independent oscilloscope evidence. Baseline manual safety requires LEFT 100ms, RIGHT 160ms startup minimum +100ms measured quiet, max850ms fail-closed.

A boot-OFF flag `steer_s143_fast_start_enable` optionally reuses AS5600 STILLNESS gathered during prior neutral. Only new S14.2 level 1/2 D-pad/RX commands: at least three DISTINCT sensor samples over >=100ms, each drift <=1 RAW, cumulative <=2 RAW, ages <=140ms and per-axis gap <=180ms (corrected for alternating idle polling). No extra I2C; background safety reads cache while armed. Both sensors must remain valid, in bounds, NC=0 and newly re-read on command; require an additional fresh sample >=10ms after command with no drift to enter MOVING, with PWM zero that transition cycle. If either axis unqualified or coasting, preserve original full SETTLING. No direct hard reverse; escape, NC/STOP/EXTI, AS5600, SOFT, S14.2 synchronizer/rise and R1/R2 preserved; no automatic Level2 boot selection.

New `M8S00_S143`: fast grants, per-axis starts, fallback attempts, quiet frames. Counters are not timing measurements. Helper C/GCC cases cover stale/drift/sensor gap/RAW wrap, but never certify target behavior.

Commissioning: preserve dirty Windows CubeIDE and logs; `git fetch origin`, `git merge --ff-only origin/feature/m7-s15-ps2-controls-restoration-staged`, verifier `python tools/verify_m8_s143_faststart.py`, CubeIDE Clean Debug build + flash SHA. Check actual independent electrical E-stop, limit NC, arm/STOP with motor supply isolated. On secure suspended chassis, compare Level1 fast=0 vs1 over >=5 short neutral-to-D-pad/RX each direction, monitor current/coast, record 50ms M8-S11 CSV and M8-S00 TXT. Only then consider operator-preferred Level2 if current/coasting/mechanical clearance have been established. Never claim physically near-zero delay until measured. No six-motor/ground-load PASS for new firmware.

## S14.3: correction for alternating idle AS5600 samples (2026-10-31)

[USER IMAGE] Level 2 last used; Fast Start enabled but `fast_grants=0`, `fast_starts=0`, `fallback_starts=41`, so no fast starts were authorized. The 39 (=0x27) AS5600 status indicates magnet detected with neither high nor low field warning; it is not by itself the blocker. This is not evidence Level2 stopped safely.

[SOURCE] Idle telemetry performs one AS5600 read on alternate sides at intervals >=50ms; a single axis normally receives a sample >=100ms apart. Old S14.3 helper rejected samples >80ms apart and cache >65ms old, repeatedly resetting neutral quiet history. Direct software defect found independently of the hardware latency estimate.

[PATCH] Adjust cache windows only: MAX_GAP_MS=180 and MAX_AGE_MS=140. Call `Steering_S143NeutralObserve()` after (not before) the existing one-sensor idle poll; there are no new I2C reads. Distinct-sample, measured quiet >=100ms, <=2 RAW cumulative drift, post-command NEW read >=10ms, NC/STOP/EXTI, SOFT, R1/R2 and old coast/fallback logic stay intact. PWM cap/Level2 selection remains untouched. If a sample is stale or either shaft drifts, Fast Start is refused.

[TEST] Pure C regression simulates two AS5600s polled alternately each 50ms (each axis sampled each 100ms), stale-data refusal and coast cancellation. Static verifier additionally checks polling order and timing constants. Target STM32 Clean Build, Flash and any real latency reduction are still PENDING.

[SAFETY] Suspend robot and verify independent power cutoff and stop/NC before powered trials; Level 1 A/B with Fast Start OFF then ON first, >=5 repetitions each side, capture M8-S11 50ms and disarmed M8-S00; check grants/starts and coast/current. Do not resume ground-level Level2 trials based on this source patch alone.

## S14.3 timestamp-order fix and gate diagnosis (2026-10-31)

[USER M8-S00] `M8_S00_20261031_142133.txt` after the first cadence fix reports S143 `enable=1,grants=0,fastAxis=0,fallback=10,quietFrames=4877`. These counters are from this boot/session, distinct from prior image showing fallback=3. Quiet proof succeeds during neutral, but grant is never earned at fresh manual stroke; the exact condition of each previous attempt is not recorded.

[SOURCE] `Steering_ManualUpdate()` took `now=HAL_GetTick()` BEFORE new blocking `Steering_PositionReadOne()` calls. Each AS5600 read updates `steer_pos_*_last_sample_ms` with its own later timestamp. Passing the pre-read `now` to `M8S143_Ready()` caused unsigned `(now-sample_ms)` underflow if I2C crossed a millisecond, invalidating otherwise-fresh cache. The same pre-I2C clock was used when an axis tried to activate `fast_now` after a further sensor read in `Steering_ManualAxisUpdate()`.

[PATCH] Obtain a fresh HAL tick AFTER both baseline sensor reads for the grant validity check and a fresh tick AFTER the axis sensor read for `fast_now` age. Preserve existing sensor freshness, >10ms post-command new sample, <=2RAW drift, STOP/NC/EXTI/SOFT, fallback settling and 850ms abort. Add read-only `steer_s143_last_gate_missing_mask`: 0 means all prerequisites passed; bit2=LEFT not SETTLING; bit3=RIGHT not SETTLING; bit4=raw NC; bit5=active NC; bit6=LEFT neutral cache unavailable; bit7=RIGHT neutral cache unavailable. `M8S00_S143_GATE` exports mask. Masks are from the LAST fresh S14.2 Level1/2 command while fast enabled; they are not direct safety-trip indicators.

[TEST] Add pure C regression demonstrating a cached sample timestamp 2ms newer than pre-I2C clock fails and the corrected check at the new tick succeeds. Extend verifier to require both fresh ticks and diagnostics. Source staged only. The previous log's cause is a strongly supported clock bug, but hardware validation of faster response and missing-mask values is still needed.

[NEXT] Windows sync (preserve dirty local files) / verify / CubeIDE Clean Build+Flash, on suspended stand Level1 fast=1 after >=300ms neutral and no coasting, monitor `steer_s143_quiet_frames`, `grants`, `fast_starts`, `fallback`, `steer_s143_last_gate_missing_mask`. Capture M8S11 50ms and disarmed M8S00. Only after actual physical STOP/coast/current clearance evaluate Level2 or ground operation.
