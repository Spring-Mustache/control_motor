#!/usr/bin/env python3
"""Static safety and preservation checks for M7-S15 PS2 routing.

Use: python tools/verify_ps2_restore.py
This is NOT a CubeIDE cross-build or a powered functional acceptance test.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
src = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")

must_have = {
    "steering control path compiled": r"#define\s+STEER_SAFETY_INCIDENT_LOCKOUT\s+0U",
    "right pin quarantine still present": r"Steering_QuarantineParkPins\(\)",
    "explicit steering permit off at boot": r"steer_safety_motion_permit\s*=\s*0U",
    "manual position arms off at boot": r"steer_pos_enable\s*=\s*0U",
    "manual direction arms off at boot": r"steer_manual_enable\s*=\s*0U",
    "M7-S10 RX deadzone restored": r"#define\s+PS2_RX_DEADZONE_COUNTS\s+10\b",
    "LY+RX independent request observation": r"ps2_restore_rx_wheel_combined_frames",
    "per-axis NC expected classifier": r"Steering_SafetyExpectedNcMask\(void\)",
    "expected NC mask": r"steer_safety_expected_nc_mask",
    "source PWM enable gating": r"#if\s+STEER_SAFETY_INCIDENT_LOCKOUT\s*==\s*0U",
    "wheel PI+FF execution": r"WheelControl_Update\(\)",
    "wheel target command": r"WheelControl_SetTargetPWM\(pwm_duty\)",
    "voice receive": r"VC02_GetEvent\(&vc02_event\)",
    "RAM logger": r"RamLogger_Update\(PS2_Pad\.LY",
    "PS2 input read": r"PS2_Read_Data\(\)",
    "PS2 16-button mask": r"debug_ps2_button_mask\s*=\s*mask;",
    "live control route": r"PS2_RestoreRouteUpdate\(\);",
    "wheel spin L1/L2": r"WheelControl_SetSpinDirection\(",
    "D-pad/RX motion owner": r"Steering_ManualUpdate\(\)",
    "PS2 manual steering arm gesture": r"Steering_PS2ManualArmUpdate\(\);",
    "PS2 arm via START hold": r"STEER_PS2_MANUAL_ARM_HOLD_MS\s+1500U",
    "separate shorter SELECT 600ms deliberate hold": r"STEER_CAL_PS2_SELECT_HOLD_MS\s+600U",
    "RIGHT calibration slows near NC": r"steer_cal_right_approach_floor\s*=\s*60U",
    "SELECT normalized completion sync": r"Steering_CalEndpointSyncApply\(&lpwm, &rpwm\);",
    "SELECT synchronizes stage only": r"stage\s*!=\s*steer_cal_right_axis\.stage",
    "SELECT NC-arrival skew telemetry": r"steer_cal_min_contact_skew_ms",
    "SELECT MAX-arrival skew telemetry": r"steer_cal_max_contact_skew_ms",
    "SELECT PS2 hold diagnostics": r"steer_cal_select_hold_grant_count",
    "R_MAX own earlier soft margin": r"steer_manual_right_max_soft_margin_raw\s*=\s*100U",
    "LEFT MIN guarded closer default": r"steer_manual_left_soft_margin_raw\s*=\s*60U",
    "LEFT MAX independently trimmed 20": r"steer_manual_left_max_soft_margin_raw\s*=\s*80U",
    "LEFT MAX separate limit": r"STEER_MANUAL_LEFT_SOFT_MAX\s+\(STEER_POS_LEFT_MAX_RAW\s*\+\s*STEER_MANUAL_LEFT_MAX_SOFT_MARGIN_RAW\)",
    "RIGHT MIN calibrated physical NC 166": r"steer_cal_right_min_runtime\s*=\s*166U",
    "RIGHT MIN soft clearance from 166": r"steer_manual_right_soft_margin_raw\s*=\s*50U",
    "hard position overflow guard still 8": r"#define\s+STEER_SAFETY_HARD_RAW_OVERSHOOT\s+8U",
    "RIGHT MAX retained": r"STEER_MANUAL_RIGHT_SOFT_MAX\s+\(STEER_POS_RIGHT_MAX_RAW\s*-\s*STEER_MANUAL_RIGHT_MAX_SOFT_MARGIN_RAW\)",
    "shorter LEFT motion settle minimum": r"STEER_MANUAL_START_LEFT_MIN_MS\s+100U",
    "shorter RIGHT motion settle minimum": r"STEER_MANUAL_START_RIGHT_MIN_MS\s+160U",
    "unchanged AS5600 coast quiet time": r"STEER_MANUAL_START_QUIET_MS\s+100U",
    "unchanged unsafe-settle timeout": r"STEER_MANUAL_START_SETTLE_MAX_MS\s+850U",
    "R1 NC release quiet verification": r"STEER_POS_ESCAPE_RELEASE_QUIET_MS\s+120U",
    "independent normalized speed target": r"steer_sync_left_target_raw_s\s*=\s*\(uint16_t\)target;",
    "PS2 arm state observable": r"steer_ps2_manual_arm_status",
    "PS2 arm needs neutral": r"Steering_ManualInputsNeutral\(\)\s*==\s*0U",
    "PS2 arm denies dual NC on RIGHT": r"\(Steering_TestRawLimitMask\(\)\s*&\s*0x03U\)\s*==\s*0x03U",
    "PS2 arm denies dual NC on LEFT": r"\(Steering_TestRawLimitMask\(\)\s*&\s*0x0CU\)\s*==\s*0x0CU",
    "PS2 arm uses bounded NC discovery envelope": r"Steering_SafetyEnvelopeWithNcDiscovery\(",
    "bounded SELECT probe max 64 RAW": r"STEER_CAL_PROBE_MAX_EXTENSION_RAW\s+64U",
    "strict default still first": r"if\s*\(Steering_SafetyHardEnvelopeValid\(left, raw\)\s*!=\s*0U\)\s*return\s*1U;",
    "no extension without SELECT or own NC": r"else if\s*\(\(\(beyond_min\s*!=\s*0U\)",
    "probe envelope debug reason": r"steer_cal_probe_envelope_last_reason",

    "R1/R2 position owner": r"Steering_PositionUpdate\(\)",
    "R2 retains near-MIN LEFT target": r"steer_pos_left_r2_target_raw\s*=\s*2297U",
    "R2 retains near-MIN RIGHT target": r"steer_pos_right_r2_target_raw\s*=\s*191U",
    "R2 target separated from discovered NC": r"left\s*\?\s*steer_pos_left_r2_target_raw\s*:",
    "R2 never commands past new RIGHT NC": r"goal\s*<\s*STEER_POS_RIGHT_MIN_RAW",
    "SELECT calibration owner": r"Steering_CalUpdate\(\)",
    "safety actuator gate": r"steer_safety_motion_permit\s*!=\s*1U",
    "independent sensor safety latch": r"steer_safety_trip_latched\s*=\s*1U",
    "S14 incident trace": r"Steering_IncidentTraceUpdate\(\);",
}
fail = [label for label, pattern in must_have.items()
        if re.search(pattern, src) is None]
for removed_gate in ("steer_manual_rx_enable", "steer_manual_rx_deadzone_raw",
                     "STEER_MANUAL_RX_DEBOUNCE_MS"):
    if re.search(r"\b" + removed_gate + r"\b", src):
        fail.append(f"old RX gate remains: {removed_gate}")
if not re.search(r"if\s*\(rx\s*<\s*-PS2_RX_DEADZONE_COUNTS\)\s*return\s*-1;", src):
    fail.append("immediate RX LEFT path")
if not re.search(r"if\s*\(rx\s*>\s*PS2_RX_DEADZONE_COUNTS\)\s*return\s*1;", src):
    fail.append("immediate RX RIGHT path")

# Regression: S14 trace runs before the S12 PS2 shadow decoder is defined.
# A C global must be declared before the first function referencing it.
# Previously the compiler failed in Steering_IncidentTraceUpdate() at
# 'debug_steer_shadow_action' undeclared despite its later definition.
shadow_def = "volatile uint8_t debug_steer_shadow_action = 0U;"
shadow_trace = "sample->steer_shadow_action = debug_steer_shadow_action;"
if (src.count(shadow_def) != 1 or src.count(shadow_trace) != 1 or
        src.index(shadow_def) > src.index(shadow_trace)):
    fail.append("PS2 shadow action must be defined before incident trace use")

route = src.split("static void PS2_RestoreRouteUpdate(void)", 1)
if len(route) != 2:
    fail.append("PS2 route function")
else:
    body = route[1].split("static void PS2_DebugObserveRightOutput(void)", 1)[0]
    actions = {
        1: "LY forward", 2: "LY reverse", 3: "D-pad UP", 4: "D-pad DOWN",
        5: "L1 spin", 6: "L2 spin", 7: "D-pad LEFT", 8: "D-pad RIGHT",
        9: "RX LEFT", 10: "RX RIGHT", 11: "R1 CENTER", 12: "R2 MIN",
        13: "SELECT calibration",
    }
    for number, label in actions.items():
        if number in (9, 10):
            # RX is chosen by a ternary expression, not separate assignments.
            if "? 9U : 10U" not in body:
                fail.append(f"RX action {number} ({label})")
        elif re.search(r"action\s*=\s*" + str(number) + r"U", body) is None:
            fail.append(f"action {number} ({label})")
    for forbidden in ("HAL_GPIO_WritePin", "__HAL_TIM_SET_COMPARE",
                      "HAL_TIM_PWM_Start", "WheelControl_SetTargetPWM",
                      "HAL_UART_Transmit"):
        if forbidden in body:
            fail.append(f"output side effect in read-only route: {forbidden}")

# M7-S16 directional clearance and latency tuning are ONLY threshold changes,
# not relaxations of the I2C/NC validity gates, wheel timing or power cut.
# LEFT toward MIN grows; toward MAX falls. RIGHT toward MIN falls.
# These static checks cannot certify actual coast distance on the chassis.
m = {
    "lmin": re.search(r"steer_manual_left_soft_margin_raw\s*=\s*(\d+)U", src),
    "lmax": re.search(r"steer_manual_left_max_soft_margin_raw\s*=\s*(\d+)U", src),
    "rmin": re.search(r"steer_manual_right_soft_margin_raw\s*=\s*(\d+)U", src),
    "rmax": re.search(r"steer_manual_right_max_soft_margin_raw\s*=\s*(\d+)U", src),
}
if all(m.values()):
    stop = {"lmin": 2297 - int(m["lmin"].group(1)),
            "lmax": 1374 + int(m["lmax"].group(1)),
            "rmin": 166 + int(m["rmin"].group(1)),
            "rmax": 982 - int(m["rmax"].group(1))}
    if stop != {"lmin": 2237, "lmax": 1454, "rmin": 216, "rmax": 882}:
        fail.append(f"manual soft-limit staged thresholds altered: {stop}")
else:
    fail.append("missing per-direction manual soft-limit defaults")
if "HAL_Delay(20);  /* Chu kỳ điều khiển khoảng 50 Hz */" not in src:
    fail.append("M6 wheel-control loop cadence altered by latency change")

# START and the global safety loop must use the same bounded physical
# discovery helper. The 8-RAW strict envelope remains the default for
# normal D-pad/R1/R2 when no physical NC is pressed.
if "Steering_SafetyEnvelopeWithNcDiscovery(1U, lraw, nc) == 0U" not in src:
    fail.append("main hard-envelope gate bypassed or not discovery-aware")
if "Steering_SafetyEnvelopeWithNcDiscovery(0U, rraw, nc) == 0U" not in src:
    fail.append("main RIGHT gate bypassed or not discovery-aware")
if "Steering_SafetyHardEnvelopeValid(left, raw) != 0U" not in src:
    fail.append("strict 8-RAW envelope no longer first gate")

# Restored M7-S10 endstop policy: IRQ affects the owning motor only.
# No NC level may latch a vehicle-wide fault. Hard-angle and AS5600
# failures still use their independent global safety latch.
irq = src.split("static void Steering_SafetyOnLimitInterrupt(uint8_t bit)", 1)
if len(irq) != 2:
    fail.append("NC EXTI ownership function")
else:
    local_irq = irq[1].split(
        "static void Steering_ManualOnLimitInterrupt(uint8_t bit)", 1)[0]
    for required in ("STEER_LIMIT_L_MIN_MASK", "STEER_LIMIT_R_MAX_MASK",
                     "__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);",
                     "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);"):
        if required not in local_irq:
            fail.append(f"own-axis interrupt stop: {required}")
    if "steer_safety_trip_latched = 1U;" in local_irq:
        fail.append("NC IRQ still latches the whole vehicle")
loop = src.split("uint8_t nc = Steering_TestRawLimitMask();", 1)
if len(loop) != 2:
    fail.append("NC main-loop sampling")
else:
    level_block = loop[1].split(
        "if ((STEER_SAFETY_INCIDENT_LOCKOUT == 0U)", 1)[0]
    if "steer_safety_trip_latched = 1U;" in level_block:
        fail.append("NC GPIO level still latches all four drive wheels")
if "volatile uint8_t steer_manual_allow_right_max_escape = 1U;" not in src:
    fail.append("bounded RIGHT MAX reverse escape disabled")
if src.count("Steering_ManualEscapeReleased(") < 4:
    fail.append("NC escape must settle then continue held command")
if "steer_pos_enable = 1U;" not in src.split(
        "static void Steering_PS2ManualArmUpdate(void)", 1)[-1].split(
        "static uint8_t PS2_IsManualControlActive(void)", 1)[0]:
    fail.append("START did not enable R1/R2")
if not all(term in src for term in (
        "steer_pos_left_center_tol_raw = 5U;",
        "steer_pos_right_center_tol_raw = 5U;",
        "STEER_POS_TOL_RAW             5")):
    fail.append("legacy R1 +/-5 / R2 MIN tolerance changed")

# START status 9 diagnosis must NOT bypass bad I2C frames, magnet status
# or the calibrated hard-angle guard. Both buses must be sampled so a
# failed LEFT reading does not conceal the RIGHT condition.
arm_diagnostics = src.split("static void Steering_PS2ManualArmUpdate(void)", 1)
if len(arm_diagnostics) != 2:
    fail.append("START steering arm update")
else:
    arm_body = arm_diagnostics[1].split(
        "static uint8_t PS2_IsManualControlActive(void)", 1)[0]
    for token in (
        "left_ok = Steering_PositionReadOne(1U, &left_raw);",
        "right_ok = Steering_PositionReadOne(0U, &right_raw);",
        "AS5600_DebugReadingValid(1U, debug_as5600_left_status)",
        "AS5600_DebugReadingValid(1U, debug_as5600_right_status)",
        "Steering_SafetyEnvelopeWithNcDiscovery(",
        "Steering_TestRawLimitMask()",
        "steer_ps2_arm_sensor_fail_mask = fail;",
        "steer_ps2_manual_arm_status = 9U;",
        "(left_envelope == 0U) || (right_envelope == 0U)",
    ):
        if token not in arm_body:
            fail.append(f"START sensor diagnostic invariant: {token}")

# Manual PS2 arming must happen immediately after an actual PS2 sample,
# never before it; no START hold can secretly clear a latched safety trip.
if "PS2_Read_Data();\n    Steering_PS2ManualArmUpdate();" not in src:
    fail.append("PS2 arm service order")
if "steer_safety_trip_latched = 0U;" in src[
        src.index("static void Steering_PS2ManualArmUpdate(void)"):
        src.index("static uint8_t PS2_IsManualControlActive(void)")]:
    fail.append("PS2 arm must never clear safety latch")

if fail:
    print("FAIL: " + ", ".join(fail))
    raise SystemExit(1)
print("PASS static PS2 restored source control paths/legacy feature checks.")
print("NOT verified: STM32CubeIDE build, real key actuation, hardware safety.")
