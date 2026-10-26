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
    "R1/R2 position owner": r"Steering_PositionUpdate\(\)",
    "SELECT calibration owner": r"Steering_CalUpdate\(\)",
    "safety actuator gate": r"steer_safety_motion_permit\s*!=\s*1U",
    "NC safety latch": r"steer_safety_trip_latched\s*=\s*1U",
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

if fail:
    print("FAIL: " + ", ".join(fail))
    raise SystemExit(1)
print("PASS static PS2 restored source control paths/legacy feature checks.")
print("NOT verified: STM32CubeIDE build, real key actuation, hardware safety.")
