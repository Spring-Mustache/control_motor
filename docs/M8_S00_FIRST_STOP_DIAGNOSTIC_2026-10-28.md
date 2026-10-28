# M8-S00 — first-stop diagnostic for ground-load steering (2026-10-28)

**Status:** source instrumentation implemented; NOT a fix or hardware PASS. The firmware has NOT been built/flashed or tested on the vehicle in this step. **Branch only:** \`feature/m7-s15-ps2-controls-restoration-staged\`.

## Scope and invariant

The problem reported by the operator: on supported stand D-pad and R1/R2 steering move comparatively well; on the ground they move in segments and require repeated presses. Root cause remains **unknown**. The instrumentation preserves the earliest steering-stop event and separately the earliest abnormal event to avoid Live Expressions losing evidence after owner cleanup. No PWM, stall/timeout, soft clearance, NC, hard-angle protection, START arming, or wheel-stop arbitration logic has been relaxed.

Both records are in ordinary RAM and are **not persistent across reset or power removal**. They are meant to be inspected after the motors have stopped, before reset.

## One-shot USB/VCP text export (new 2026-10-28)

To avoid screenshots of 50+ struct fields, an explicit **UART3 / ST-Link Virtual COM** exporter now prints BOTH complete sticky snapshots as a short, labeled key/value log. The old USER-button wheel logger is **not** used; its CSV does not include these new steering snapshots, and its default start configuration may enable wheel PID.

1. Only after a successful CubeIDE Debug build and verified flash, open the ST-Link VCP COM port in a terminal at **115200 baud, 8N1**, no flow control (e.g. Tera Term); enable text capture or copy the text later. This USART3 connection must actually exist on the tested hardware.
2. On a supported stand with safe power isolation available, clear old evidence with `m8_s00_clear_request=1` while steering is inactive; check the clear count increased and `first_stop.valid=0`. Perform **one** authorized D-pad or R1/R2 test under the approved safety conditions, release the controls, and **disarm START**. Wait for all wheel/steer motion and feedback to settle.
3. In CubeIDE Live Expressions, write `m8_s00_dump_request=1`. It is **one-shot**: firmware resets it to 0 whether export succeeds or is refused. The COM terminal should show `#M8S00_BEGIN,1` through `#M8S00_END`. Copy and send **the entire text block**; there is no need to expand `first_stop` and `first_fault` in Live Expressions.
4. Check `m8_s00_dump_status`: **1** successful UART transmission; **2** unsafe/busy (steering still armed, any owner active, nonneutral PS2, wheel feedback/targets or logger busy); **3** no captured event; **4** UART/write error; **0** no attempt. `m8_s00_dump_count` increments for every completed export. `status=1` means bytes were transmitted to UART, **not** proof a PC terminal received or saved them.
5. DO NOT reset or power off before exporting; the M8-S00 snapshots are ordinary RAM. If status 2, do not bypass safety gates: make the system safely inactive, then request a new export. Do not hold the USER button to collect M8-S00 evidence.

Each available snapshot exports five groups `M8S00_META`, `M8S00_PS2`, `M8S00_STEER`, `M8S00_STATE`, `M8S00_WHEEL`, labeled `STOP` or `FAULT`; an empty fault record emits `M8S00_EMPTY,FAULT`. Wheel RPM fields with prefix `rpm10` and `targetRpm10` are **tenths of RPM** (divide integers by 10). Raw angular values are **AS5600 counts**, not wheel angle degrees. Output occurs in the main loop **after** normal wheel logger service; no extra I2C sampling is invoked.

**Safe transport limitation:** UART transmission blocks the main loop briefly; the exporter therefore refuses to run while steering permits/enables remain active or wheel/steering motion or telemetry exceeds the stopped checks. An idle MCU cannot substitute for independent physical driver power isolation, and existing MCU PWM=0 is not an electrical emergency disconnect. The exporter has not yet been CubeIDE cross-built or tested with an attached terminal. This feature is pending build and bench verification.

## Debug variables (Live Expressions)

- \`m8_s00_first_stop\`: first event of any kind since explicit clear/boot.
- \`m8_s00_first_fault\`: first abnormal event since explicit clear/boot (may be empty when the stop was normal).
- \`m8_s00_event_count\`: total captured stop-path invocations (not limited to first two).
- \`m8_s00_clear_request\`: set to **1** only while both steering owners are inactive; firmware clears both records and resets the request, without changing actuator state.
- \`m8_s00_clear_count\`: confirms a clear was consumed.
- \`*.valid = 1\` means recorded, \`0\` means empty.
- \`*.tick_ms\`, \`*.command_age_ms\`, \`*.manual_left_progress_age_ms\` / \`*.manual_right_progress_age_ms\`.
- \`*.event\`, \`*.axis\`, \`*.code\`, \`*.manual_*_state/result\`, \`*.position_*_state/result\`, \`*.manual_block_reason\`, \`*.position_block_reason\`.
- \`*.raw_left/right\`, \`*.ccr_left/right\`, \`*.manual_pwm_left/right\`, \`*.position_pwm_left/right\`, \`*.nc_mask\`, \`*.safety_trip_latched\`, \`*.safety_trip_reason\`.
- \`*.wheel_stop_gate\` (1=all wheel-stop conditions passed; 0=one or more failed), \`*.rpm_lu/ld/ru/rd\`, \`*.wheel_target_pwm\`, \`*.wheel_current_pwm\`, \`*.wheel_pid_target_rpm\`, \`*.wheel_pwm_lu/ld/ru/rd\`, \`*.sync_left_speed/right_speed\`, \`*.sync_reference_hold_count\`.
- \`*.ps2_*\`, \`*.control_mode\`, \`*.control_stop_latched\`, \`*.guard_flags\` for owner/input conflicts.

**Event codes:** 1 manual per-axis DONE; 2 manual per-axis FAULT; 3 manual owner STOP; 4 position per-axis DONE; 5 position per-axis FAULT; 6 position owner FINISH. **Axis:** 1 LEFT, 2 RIGHT, 3 both. **Code:** the **existing** firmware result/reason (no remapping).

**Guard mask** (\`guard_flags\`, bit set when named condition is true):

| Bit | Condition | Bit | Condition |
|---:|---|---:|---|
| 0 | \`ps2_cancel_hold\` | 8 | \`debug_as5600_poll_enable\` |
| 1 | \`ps2_spin_rf_latched\` | 9 | \`steer_manual_enable\` |
| 2 | \`voice_execution_active\` | 10 | \`steer_pos_enable\` |
| 3 | \`voice_command_ready\` | 11 | \`steer_safety_motion_permit\` |
| 4 | \`steer_cal_active\` | 12 | \`wheel_spin_active\` |
| 5 | \`steer_test_running\` | 13 | \`wheel_ff_test_active\` |
| 6 | \`steer_test_arm\` | 14 | PS2 L1 |
| 7 | \`steer_test_request\` | 15 | PS2 L2 |

## Static checks and commissioning

1. On Windows, preserve local \`.cproject\`, \`.settings\`, startup and untracked files. Inspect \`git status --short\` and \`git diff\` before fast-forwarding the **same** branch. Never \`reset --hard\`, \`clean\`, force or \`stash pop\`. If the pull conflicts, stop and reconcile the specific files; do not overwrite local work.
2. Run \`python tools/verify_m8_s00_capture.py\` and existing \`python tools/verify_ps2_restore.py\` plus \`python tools/test_s16_reassembly_geometry.py\`. Tests requiring a missing host C compiler are SKIP, not PASS.
3. Build **Debug** in STM32CubeIDE, confirm \`Services\` is included in Debug project Source Location for the RAM Logger, inspect build log, then flash the verified ELF. Source static PASS does **not** imply CubeIDE build PASS. Record Git commit, build time and ELF identity.
4. **No unattended ground run**: M7 Gate A is still open. First observe with motor drive unpowered where appropriate; verify all NC GPIO levels and usable **total-vehicle supply cutoff** with the electrical setup known. Driver power is NOT isolated by \`CCR=0\`; do not unplug powered motor terminals. Keep SELECT disabled until the L_MIN mechanical switch actuator is corrected.
5. When stand-supported and safe for a motor test, with all commands neutral and all required guards satisfied: after observed stop, write \`m8_s00_clear_request=1\`; wait for \`m8_s00_clear_count\` to increment, verify \`first_stop.valid=0\`. Use existing START arming procedure. Test one D-pad left/right hold **or** one R1/R2 press; release/stop normally. Inspect both snapshots **before reset/power off**. Capture a debugger screenshot of each populated record.
6. If both snapshots stay empty, report \`m8_s00_event_count\`, owner \`*_active\`, PS2 connected/mode, current output CCRs; do not infer the mechanism.
7. Move to any under-load test only **after** an acceptable Gate A and a verified emergency supply cutoff. Do not increase PWM or alter driver current potentiometers to force completion.

## Interpretation (hypotheses; NOT root-cause proof)

- Manual \`event=2, code=13\`: stall/no progress 650 ms; \`code=14\`: unsettled; \`code=5\`: 5-s manual timeout; \`code=4\`: AS5600/geometry read path; \`code=9\`: unexpected/contradictory NC. \`code=11\` is a normal soft stop when \`event=1\`.
- Manual \`event=3, code=4\` and \`wheel_stop_gate=0\`: wheel-stop interlock canceled the held input. Determine exactly which wheel control PWM, target or measured RPM failed from fields, including passive wheel rotation (measured RPM beyond ±2 on any wheel). Do not remove the interlock without a coordinated owner design.
- Position \`event=6, code=9\`: position owner canceled; \`wheel_stop_gate=0\` and guard/input values help distinguish wheel/owner/PS2 conditions. Position \`event=5\` with per-axis \`code=8\` can reflect no progress or direction issue.
- MCU \`CCR>0\` but AS5600 RAW does not advance: firmware output did not cut at that instant; investigate physical load, driver ERR/OC/iLIMIT/undervoltage and mechanical binding **safely**, without assuming any one is proven.
- NC EXTI may set a channel CCR to zero **before** the main-loop snapshot is captured. Check \`nc_mask\`, \`manual_irq_trip_mask\` and \`position_limit_trip_mask\`; the captured CCR is the value at capture time, not necessarily the value immediately before an ISR edge.
- \`m8_s00_first_stop\` may record a normal arrival before a later fault; \`m8_s00_first_fault\` is deliberately separate. No snapshot is an oscillogram or an electrical proof of driver current.

## Exit criteria

S00 closes only after the actual first stopping path has been classified, necessary safe code/hardware corrections completed, CubeIDE build and safety regression verified, and D-pad hold plus R1/R2 one-press behavior accepted on stand and then under controlled load. **Current status: diagnosis prepared; S00 NOT PASS.**
