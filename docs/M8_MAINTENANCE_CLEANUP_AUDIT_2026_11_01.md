# M8 code footprint and latency cleanup — evidence audit, pass 1

**Audit base SHA:** `70f304b6463e3555e18068f04e7f227b1eb90dfd` (feature/m8-s15-dual-target-common-time). This pass is staged on a **separate branch**, not released as MCU firmware. The complete history, failed trials and past GitHub Releases are retained in Git; prior source snapshots remain recoverable. Do **not** equate an old GitHub context-release artifact, HOST CI pass, or a source commit with CubeIDE build / MCU flashed SHA / physical motor proof.

## Decision rubric: KEEP, RETIRE or OPEN

| Item | Evidence from branch history and current source | Decision |
|---|---|---|
| **S14/S14.1** equal calibrated-angle, cross-coupled left taper (S14.1 additional taper when RIGHT near cap) | Source opt-in OFF; code asks to **slow LEFT**, contradicts latest manual-angle-relation RIGHT-priority objective. Prior S13 equal-angle experiment was reverted; neither S14/14.1 has confirmed target hardware PASS. S14.2 Level2 (newer separate controller) supersedes this *opt-in taper owner*. | **RETIRE in pass 1:** old S14/14.1 enable/latch/counters, PWM trim planner and obsolete S14/14.1 UART diagnostics. Archive math/old verifier in Git history, not active tree. |
| **M8-S14.2 / S14.3** Level2 LEFT450/RIGHT200 and Fast Start | Active boot defaults; operator demanded retaining tested LEFT responsiveness; current new trial still depends on AS5600 speed observer from S14. **Shared `Steering_S14Observe` MUST remain** despite its name. | **KEEP** intact. |
| **S14.5** early RIGHT bias | A/B operator evidence reports no measured improvement; default off, but optional logic still referenced by S14.7 exclusivity and current host guards. | **OPEN:** do not remove in pass 1 without verifying resulting mode arbitration / exact safety and target tests. |
| **S14.7–S14.9** opt-in PI, hybrid and terminal anticipation | Unfinished pilots (no accepted current/coast/clearance and MCU SHA); S15.3 RIGHT work could reuse feasible parts. Some runs improved early timing, terminal arrival did not meet original goal. | **KEEP pending technical disposition**, never count as hardware PASS. |
| **S14.10** bounded RIGHT terminal read-only shadow | Intended only to measure candidate near SOFT where RIGHT lag persisted; safety evidence still missing, S15.3 work may need it. | **KEEP pending decision/data**, not actuator. |
| **S15 common-T kernel / S15.1** | Common-time pilot did not achieve common *physical* completion; S15.4 R1/R2 conditional common-T remains planned, S15.1 trace may provide relevant legacy pilot evidence. | **KEEP incomplete**, don't use manual common-T by default. |
| **S15.2** manual angle-relation read-only shadow, 96 rows | Current first incomplete MCU/HARDWARE validation; needed for S15.3. PowerShell-only `capture_m8_s152.ps1` used to export each manual stroke. | **KEEP active test**, no actuation/PWM modifications. |
| **S11 streaming** | Real six-motor CSV logs 2026-10-31 used for S14/S15 diagnosis, including CPU period and independent wheel data; default stream OFF. | **KEEP for VC-02 six-motor regression**. Off-state per-loop check remains; no reliable latency comparison exists. |
| **M8S00** first stop/fault, NC, telemetry, after-disarm UART | S15.2 PowerShell capture **directly requires** M8S00 and short USER dispatch. Important post-stop fault/arrival evidence. | **KEEP**. Do not accidentally delete or make it UART-active while motors move. |
| **Original M6 RAM logger / USER 3-press** | `ram_logger.c` stores up to 6000×72-byte samples in `.noinit`; prioritizes diagnostic M8S00 short USER on an eligible steering stop, and retains old 3-press start/stop/dump for wheel trials. Significant **SRAM reserve** but operator explicitly consulted historical mechanism and future VC-02 wheel regression remains pending; cannot currently prove the old logger permanently unused. | **KEEP pending an explicit replacement with equal wheel evidence, memory budget and integration tests.** Arbitrary removal could affect recovery and TEST priorities. |
| **VC-02** parser, priority, executor | User-approved next milestone after S15.4. Parser present, TURN/ANGLE not yet actuator-supported. | **KEEP**, no claimed voice steering. |
| **PS2/STOP/NC/SOFT/stall/atomic guards and START arming** | Independent safety owner. Motor may move unexpectedly even with CCR zero; physical power cutoff remains required. | **KEEP** without relaxation. |
| **S15.5 Ackermann** | Deferred due incomplete actual kinematics. | **DO NOT IMPLEMENT**; historical plans preserved, not a deletion license for shared measurement geometry. |

## Pass 1 actual source change and limits

1. Remove **only S14/S14.1 optional left-taper manual controller** and its runtime variables / UART diagnostic lines. Its **`steer_s14_math.h` helper and old helper-only tests** remain retrievable at base SHA rather than compiled/current test inventory. No other compiler-owned control mode is removed.
2. **Retain `Steering_S14ManualReset`** despite the historical name; it resets the still-active S14.2/S14.7/S14.9/S15 shared observer/owner state. Retain `Steering_S14Observe` and both cached AS5600 observers, reused in Level2 PI and P+D.
3. Preserve exact unrelated code for all atomic NC/CCR writes, STOP, PS2/Voice arbitration, user button, M8S00/S11, all motor caps, S15.2, and firmware startup.
4. Update S14.2 static route guard from "S14.1 only disabled in Level2" to "S14/14.1 no longer exists". Historical test scripts formerly validating obsolete taper routes are **removed from active tree**, but remain in Git history.
5. CI HOST/static (exact final SHA) and Windows PowerShell parse are necessary **but cannot establish STM32 compilation or real-time improvements**. To claim faster response later, require same PS2 input, AS5600 timestamps, same battery/load/firmware comparison, measure PS2-to-CCR latency median/P95/max and scheduling overruns on actual STM32; else status **UNVERIFIED**. Source deletion (about 9.7 kB text in `main.c`) does not directly equal Flash/SRAM saving.

## Planned follow-up pass (NOT implemented)

- Quantify SRAM sections via `arm-none-eabi-size` and linker map on the user's CubeIDE build. Assess whether original M6 logger memory should be replaced with **session-based bounded steering/wheel evidence** before deletion.
- Audit `S14.5` opt-in old bias vs needed S15.3 path with first-stop events, don't remove merely because one A/B failed while integration still references it.
- Map each recurring loop read/format cost using actual target profile. Do not replace data without replacing its safety/diagnostic role.
