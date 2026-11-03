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

## 2026-11-03 — RIGHT R_MIN last-100RAW floor 50-to-60 CCR trial staged OFF

[USER] Current a85438e full four-run log shows RIGHT lag 396ms D-pad LEFT, 517ms RX LEFT. Operator suspects PWM 50 near SOFT. The value arises from `steer_manual_right_rx_min_pwm = 50U` used for manual taper within the last 100 RAW; it is NOT `steer_pos_right_pwm_min` for R1/R2. Observed lag is real, but the exact physical root cause is not confirmed.

[SOURCE] An independent test flag `steer_right_min_tail60_trial_enable=0U` defaults OFF. If explicitly enabled before a NEW manual RIGHT-to-R_MIN command in Level2 with no S15/S15.3/S147/S149 competitor, and RIGHT MOVING with NC clear, the final 100 RAW tapered candidate uses **60 CCR instead of 50 CCR** only where it increases the baseline and never exceeds pre-taper owner PWM. Weak joystick requests below 60 stay unchanged. This leaves normal default motion untouched, unchanged position mode R1/R2, LEFT/RIGHT_MAX, NC/STOP/SOFT, caps, physical cutoff and late atomic gate. No extra I2C.

[M8S00 UART] Stopped-only `M8S15_TAIL60,enable,last,candidateFrames,floorBase=50,floorTrial=60,windowRaw=100,dirR=-1` records trial. candidateFrames are PRE-atomic suggestions only; actual CCR from M8S152_B must be inspected. The existing four-run PowerShell still saves one TXT.

[SAFETY] R_MIN has historical approximately **3mm between SOFT and physical NC**, with no accepted post-PWM-off coast/current or driver cutoff evidence. Therefore this commit is **SOURCE/STAGED ONLY**; do not enable or run powered near R_MIN until that physical safety gate is satisfied. This pilot is separate from S15.3 and is not an active S15.3 PASS. After gate, compare matched ON/OFF from same positions and joystick input, actual right CCR, arrival skew, current, NC and coast; no claimed improvement in advance.

## 2026-11-03 — Correction: operator requested actual 60-CCR default, not OFF

Previous commit 9418d4a staged a 50-to-60 CCR test but set `steer_right_min_tail60_trial_enable=0U` (boot OFF). The operator explicitly objected: the requested comparison must actually use 60 CCR by default when the test conditions apply. **This commit corrects the source default to `1U` (ON).** The manual RIGHT-to-R_MIN only condition (D-pad LEFT / RX LEFT), new-stroke latch, Level2 requirement, exclusion of S15/S15.3/S147/S149, 100 RAW taper window, initial owner ceiling, zero PWM at SOFT and late STOP/NC veto remain unchanged. No change to R1/R2, RIGHT MAX, LEFT, S15.3 default OFF or Git/PowerShell capture.

**Important operational limit:** ON means **the new firmware will command the modified RIGHT terminal PWM during eligible powered strokes**. The ~3mm historically measured R_MIN SOFT-to-NC physical gap and unmeasured coast/current remain unresolved hazards. HOST CI and source update do not show powered hardware safety. Do not perform powered full-stroke test until cutoff, current, coast and physical clearance have been verified. New firmware is only an operator-requested candidate; do not call 60 CCR safe or proven to improve latency. Its disarmed `M8S15_TAIL60` line plus actual `M8S152_B` and `M8S00_ARRIVAL` are needed for objective A/B comparison with a85438e OFF baseline. The prior OFF documentation describes the **older** 9418d4a commit only.

## 2026-11-03 — CURRENT S15.3: remove tail60 experiment; early-mid RIGHT R_MIN candidate

[BASE] On explicit user request, code returns to last established 50-CCR firmware `a85438ec07d626a872b488cbd30feb95b43989b9`, preserving AS5600 RAW32+UART crosscheck and LEFT CENTER=1886 RIGHT CENTER=662. This was implemented as a new forward-only Git commit: no force reset, no user-stash deletion and no alteration of the original 50-CCR logic. All 60CCR trial variables, command latch, terminal-taper candidate, M8S15_TAIL60 UART and static checks were removed from CURRENT compiled source. Prior descriptions of 60CCR in earlier sections of this document are **historical**, no longer active.

[DATA] Two 2026-11-03 baseline 50CCR four-run captures recorded D-pad LEFT RIGHT lag 396ms and 440ms; RX LEFT 517ms and 561ms. In D-pad LEFT mid-stroke with both axes >320RAW remaining, S15.2 120ms predicted normalized angle proxy is often negative (~-20..-50 permille: RIGHT currently slightly ahead); original S15.3 requires >+20pm to boost and would miss this terminal-delay precursor. RX LEFT already has positive mid-stroke proxy and may request modest boost. Legacy S14.2 logs count trimRf attempts, but these are plans rather than independently measured actuator inhibition.

[OPTIMIZATION] Added pure-C `M8S153_PlanRightMid` helper. Original reactive S15.3 behavior is preserved outside the middle band; specifically only RIGHT-to-R_MIN (D-pad LEFT or RX LEFT), when BOTH axes remaining RAW are **>320 and <=650**, fresh angle/rate, both moving, NC clear, and RIGHT not ahead by >=80 permille, can use a **virtual +80pm forecast offset**. Maximum +12CCR remains enforced above independent RIGHT owner and existing Level2 RIGHT cap 200, no correction when owner=0, no added preview at <=320RAW and original <=180RAW hard veto remains. LEFT motor path, terminal 50CCR taper, soft targets, STOP, NC and atomic CCR safety guards are unmodified. S15.3 **boots OFF** and is only latched on new stroke after explicit opt-in.

[LOG] Existing M8S153 disarmed-only line appends `earlyFrames` (pre-atomic early decisions beyond baseline S15.3) and `avoidedReductionR` (pre-atomic S14.2 RIGHT proposal lower than independent owner but discarded by S15.3). Existing `appliedFrames` remains post-atomic. None of these predicts actual improvement without a controlled on-target A/B test. New HOST regression includes direction/band/freshness/owner-off/NC gates plus >10k bounds combinations. Source/CI success does not prove CubeIDE Clean Build/Flash or powered motor outcome; physical current, coast and independent power cutoff still need validation for S15.3 ON.

## 2026-11-03 19:09 — S15.3 ON only for RIGHT R_MIN after 50CCR OFF baseline

Four completed M8S00 export frames for SHA faf628b (dirty=1) show S15.3 enable=0,last=0,appliedFrames=0 throughout. Therefore no actual S15.3 test occurred. D-pad LEFT LEFT completed in 1777ms but RIGHT still RAW225 vs SOFT216 at end3157ms, reached=1; RIGHT RAW229 held about300ms despite PWM56, no NC or fault. D-pad RIGHT lag73ms, RX LEFT541ms, RX RIGHT109ms. S14.2 cumulative RIGHT trim counter 41/68/125/148 equals proposed trims per stroke 41/27/57/23, not proven physical applied trim.

Operator-requested NEXT test: source now defaults steer_s153_right_only_enable=1U, but only a new Level2 command with right_direction<0 latches S15.3 (D-pad LEFT and RX LEFT). RIGHT-to-MAX (D-pad RIGHT and RX RIGHT) keeps legacy S14.2. Middle forecast 320<remaining BOTH<=650RAW and bounded +12CCR above RIGHT owner; original reactive S15.3 refuses action <=180RAW either axis. Last100RAW production floor remains 50CCR, no trial60 code. LEFT, STOP, NC, SOFT and final atomic veto unchanged. This is source-only proposal, NOT confirmed MCU build/flash or physical improvement. Check motor supply cutoff and STOP/NC before powered testing.

For next after-disarm log, on D-pad LEFT/RX LEFT verify M8S153 enable1,last1 and appliedFrames>0 as postatomic proof; earlyFrames is preatomic. For RIGHT commands enable1,last0 is expected by design. If RIGHT still stalls at 9..13RAW terminal zone, early midstroke alone may not fix. Raw32 AS5600 match12 and dropped counts remain crosschecks.


## 2026-11-03 — RIGHT MIN measured 195–197; choose 195 as new boot default

Operator reports RIGHT physical R_MIN NC reading **197 RAW sometimes drifting to 195 RAW**; explicitly selected **195** as the single corrected reference. Previous historical 166 boot default was no longer representative. Update BOTH `steer_cal_right_min_runtime` and `steer_pos_right_r2_target_raw` from 166 to 195, retaining RIGHT CENTER662 and MAX1029, LEFT CENTER1886 and all other LEFT/MAX values. RIGHT D-pad/RX R_MIN SOFT margin remains50 RAW, computed boot threshold **195+50=245 RAW** (48..50 RAW before reported 195..197 NC). Therefore RIGHT should stop PWM 29 RAW earlier than the older 216 SOFT, without changing taper floor50CCR or adding PWM near NC. No change to S15.3 default ON for eligible fresh Level2 RIGHT->R_MIN strokes, +12CCR max earlier only in configured middle band, and LEFT/STOP/NC/SOFT independent guards. R2 still requires its own physical MIN NC contact to declare success; the R2 raw reference is only a bounded approach reference. A fully valid SELECT scan can replace runtime geometry later; post-DISARM `M8S00_CAL_RUNTIME,minR=195,r2R=195` and `M8S00_STEER,...softR=245` must be verified on the flashed build. Git SHA can be dirty, so confirm these **live numeric fields**, not the stamp alone.

R_MIN mismatch may have biased S15.2 angle-normalized timing and moved actual late taper thresholds, but cannot yet be assigned as sole cause of the RIGHT lag or zero S15.3 boost requests. Earlier four-run 19:19 firmware afc42cd had `enable=1,last=1` for R_MIN, `requestFrames=0,appliedFrames=0,earlyFrames=0`; its completion skews D-pad LEFT +297ms, RX LEFT +374ms, D-pad RIGHT RIGHT ahead165ms, RX RIGHT 0ms. New geometry changes stroke and stop target, so older and revised results are not strictly identical A/B timing comparisons. Preserve original log and remeasure four strokes after target build/flash and independent stop/NC verification. New CI standalone host static guard `tools/verify_m8_right_min_geometry.py` checks both boot references, margin, SOFT245, old PWM60 removal, NC and S15.3 controls, and records HOST/target separation. Source update alone does not establish compiled target or physical test PASS.
