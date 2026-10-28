#!/usr/bin/env python3
"""M8 source invariants: simultaneous normal wheel drive + PS2 steering.

Usage: python tools/verify_m8_concurrent_drive_steer.py
Static inspection only; NOT a CubeIDE build or physical motion test.
"""
from pathlib import Path
import re

main = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text(encoding="utf-8")


def function(name):
    # Match the actual definition, not a forward declaration followed
    # much later by some unrelated function's opening brace.
    pattern = r"\bstatic\s+" + re.escape(name) + r"\s*\(\s*void\s*\)\s*\{"
    match = re.search(pattern, main)
    if match is None:
        raise AssertionError("Missing definition: " + name)
    opening = match.end() - 1
    # Skip comments and strings: braces in comments cannot end a function.
    depth = 0
    state = "code"
    i = opening
    while i < len(main):
        a, b = main[i], main[i + 1] if i + 1 < len(main) else ""
        if state == "line_comment":
            if a == "\n":
                state = "code"
        elif state == "block_comment":
            if a == "*" and b == "/":
                state, i = "code", i + 1
        elif state == "string":
            if a == "\\":
                i += 1
            elif a == '"':
                state = "code"
        elif state == "char":
            if a == "\\":
                i += 1
            elif a == "'":
                state = "code"
        elif a == "/" and b == "/":
            state, i = "line_comment", i + 1
        elif a == "/" and b == "*":
            state, i = "block_comment", i + 1
        elif a == '"':
            state = "string"
        elif a == "'":
            state = "char"
        elif a == "{":
            depth += 1
        elif a == "}":
            depth -= 1
            if depth == 0:
                return main[opening:i + 1]
        i += 1
    raise AssertionError("Unbalanced body: " + name)


def check(label, ok):
    print(("[PASS] " if ok else "[FAIL] ") + label)
    if not ok:
        errors.append(label)


errors = []
ps2 = function("void PS2_ApplyManualControl")
manual = function("void Steering_ManualUpdate")
position = function("void Steering_PositionUpdate")
full_neutral = function("uint8_t Steering_ManualInputsNeutral")
direction_neutral = function("uint8_t Steering_ManualDirectionNeutral")
arm = function("void Steering_PS2ManualArmUpdate")
calibration = function("void Steering_CalUpdate")
dump = function("void M8S00_DumpUpdate")
route = function("void PS2_RestoreRouteUpdate")
stop = function("void Wheel_EmergencyStop")

# Normal propulsion and steering are independent; simultaneous inputs
# should not be suppressed by other subsystem's normal activity.
check("wheel target derived from LY/Up/Down",
      "WheelControl_SetTargetPWM(pwm_duty);" in ps2 and
      "PS2_Convert_LY_To_PWM(PS2_Pad.LY)" in ps2)
check("R1/R2 never force wheel command to zero",
      "if ((PS2_Pad.Select != 0U) || (steer_cal_active != 0U))" in ps2 and
      "if ((PS2_Pad.R1 != 0U) || (PS2_Pad.R2 != 0U)" not in ps2)
check("LY throttle cannot blank active steering CCR",
      "if ((pwm_duty != 0) || (PS2_Pad.Up != 0U)" not in ps2 and
      "steer_pos_active == 0U" in ps2 and
      "steer_manual_active == 0U" in ps2)
check("manual steering independent of wheel RPM/PWM and LY",
      "Steering_PositionWheelsStopped()" not in manual and
      "PS2_Pad.LY < PS2_LY_FORWARD_LIMIT" not in manual and
      "PS2_Pad.Up != 0U" not in manual and
      "PS2_Pad.Down != 0U" not in manual)
check("R1/R2 position independent of wheel RPM/PWM and LY",
      "Steering_PositionWheelsStopped()" not in position and
      "PS2_Pad.LY < PS2_LY_FORWARD_LIMIT" not in position and
      "PS2_Pad.Up == 0U" not in position and
      "(WheelControl_SpinIsActive() == 0U)" in position)
check("manual direction latch can rearm while wheel moving",
      "Steering_ManualDirectionNeutral()" in manual and
      "PS2_Pad.LY" not in direction_neutral and
      "PS2_Pad.Up" not in direction_neutral)
check("START still needs full PS2 neutral AND stopped wheels",
      "Steering_ManualInputsNeutral() == 0U" in arm and
      "Steering_PositionWheelsStopped() == 0U" in arm and
      "PS2_Pad.LY >= PS2_LY_FORWARD_LIMIT" in full_neutral and
      "PS2_Pad.Up == 0U" in full_neutral)
check("L1/L2 spin exclusive with steering",
      "WheelControl_SpinIsActive() != 0U" in manual and
      "WheelControl_SpinIsActive() == 0U" in position and
      "WheelControl_SetSpinDirection(" in ps2 and
      "Turn_Motors_Stop();" in ps2)
check("SELECT calibration still stationary",
      "Steering_PositionWheelsStopped() == 0U" in calibration and
      "Steering_CalInputsNeutral() == 0U" in calibration and
      "WheelControl_SetTargetPWM(0);" in ps2)
check("PS2 mapping observer does not falsely block normal LY+steer",
      "(action == 13U) && (Steering_PositionWheelsStopped() == 0U)" in route and
      "PS2_Pad.L1 != 0U" in route and
      "PS2_Pad.Select != 0U" in route)
check("STOP still stops wheels AND steering",
      "WheelControl_EmergencyStop();" in stop and
      "Turn_Motors_Stop();" in stop)
check("NC, raw AS5600, stall and permit retained",
      all(token in main for token in (
          "Steering_TestRawLimitMask()", "Steering_SafetyOnLimitInterrupt(",
          "Steering_SafetyEnvelopeWithNcDiscovery(", "STEER_MANUAL_STALL_MS",
          "STEER_POS_TIMEOUT_MS", "steer_safety_motion_permit = 0U;")))
check("M8 snapshot UART still requires fully stopped and disarmed",
      "Steering_PositionWheelsStopped() == 0U" in dump and
      "steer_safety_motion_permit != 0U" in dump and
      "M8S00_DumpOne(\"STOP\"" in dump and
      "M8S00_DumpOne(\"FAULT\"" in dump)
check("combined PS2 request debug present",
      "ps2_drive_steer_request_frames++" in ps2 and
      "volatile uint32_t ps2_drive_steer_request_frames = 0U;" in main)

print(f"\nM8 simultaneous-control static checks: {14-len(errors)}/14 PASS")
print("No MCU build, PWM timing, motor power, load, braking or real PS2 actuation tested.")
raise SystemExit(1 if errors else 0)
