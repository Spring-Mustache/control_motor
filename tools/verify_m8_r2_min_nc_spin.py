#!/usr/bin/env python3
"""M8 R2 physical-NC arrival / L1-L2 in-place spin geometry static checks.

Run at repo root:
  python tools/verify_m8_r2_min_nc_spin.py

Static source inspection only: NOT an STM32 build, NC electrical test, or
proof that the wheels can pivot under load. Verify hardware on a secure
suspended stand with independent motor-power cutoff.
"""
from pathlib import Path
import re

src = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text(encoding="utf-8")


def body(name):
    match = re.search(r"\bstatic\s+(?:void|uint8_t)\s+" + re.escape(name)
                      + r"\s*\([^;]*?\)\s*\{", src, flags=re.S)
    assert match, "Missing function definition: " + name
    start = match.end() - 1
    level = 0
    for i in range(start, len(src)):
        if src[i] == "{":
            level += 1
        elif src[i] == "}":
            level -= 1
            if level == 0:
                return src[start:i + 1]
    raise AssertionError("Unclosed function " + name)


def check(label, result):
    print(("[PASS] " if result else "[FAIL] ") + label)
    tests.append(bool(result))


tests = []
axis = body("Steering_PositionAxisUpdate")
position = body("Steering_PositionUpdate")
ps2 = body("PS2_ApplyManualControl")
isr = body("Steering_SafetyOnLimitInterrupt")
arm = body("Steering_PS2ManualArmUpdate")
stop = body("Wheel_EmergencyStop")
dump = body("M8S00_DumpUpdate")
nc_dump = body("M8S00_DumpSpinNc")

check("R2 targets restored to guarded endstop seek references",
      "steer_pos_left_r2_target_raw = 2297U;" in src
      and "steer_pos_right_r2_target_raw = 166U;" in src)
check("sensor tolerance cannot finish R2",
      "Steering_PositionAxisDone(left, 2U)" not in axis
      and "else if (abs_error <= STEER_POS_TOL_RAW)" not in axis)
check("R2 only finishes on its own MIN NC level",
      "(steer_pos_command == 2U)" in axis
      and "(limit_mask & own_min)" in axis
      and "Steering_PositionAxisDone(left, 3U)" in axis)
check("R2 NC IRQ and late IRQ stop own axis",
      position.count("Steering_PositionAxisDone(1U, 3U)") >= 3
      and position.count("Steering_PositionAxisDone(0U, 3U)") >= 3
      and "Steering_PositionAxisFault(1U, 9U)" in position
      and "Steering_PositionAxisFault(0U, 9U)" in position)
check("R2 without NC stops at known RAW boundary, no hunt",
      "if (((left != 0U) && (raw >= goal)) ||" in axis
      and "((left == 0U) && (raw <= goal)))" in axis
      and "Steering_PositionAxisFault(left, 8U)" in axis
      and "STEER_POS_STALL_MS" in axis)
check("both MIN NCs required before spin is commanded",
      "uint8_t nc = Steering_TestRawLimitMask();" in ps2
      and "uint8_t pivot_nc = (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_R_MIN_MASK);" in ps2
      and "STEER_LIMIT_L_MAX_MASK | STEER_LIMIT_R_MAX_MASK" in ps2
      and ps2.index("if ((nc & (STEER_LIMIT_L_MIN_MASK")
          < ps2.index("WheelControl_SetSpinDirection(1, MOTOR_DPAD_PWM)"))
check("released NC during active spin kills wheel PWM",
      "if (WheelControl_SpinIsActive() != 0U)" in ps2
      and "WheelControl_EmergencyStop();" in ps2
      and "ps2_spin_nc_active_cut_count++;" in ps2)
check("new L1/L2 press without both NCs cannot spin",
      "ps2_spin_nc_block_frames++;" in ps2
      and "WheelControl_SetTargetPWM(0);" in ps2)
check("steering ISR still zeros own PWM immediately",
      "__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);" in isr
      and "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);" in isr)
check("global STOP and START safety remain",
      "WheelControl_EmergencyStop();" in stop
      and "Turn_Motors_Stop();" in stop
      and "Steering_ManualInputsNeutral() == 0U" in arm)
check("R1 remains independently fine-trimmed",
      "Steering_PositionFineUpdate(left, axis" in axis
      and "STEER_POS_R1_TIMEOUT_MS" in position)
check("limits/sensor/SELECT scan bounds still retained",
      "STEER_SAFETY_HARD_RAW_OVERSHOOT 8U" in src
      and "STEER_CAL_PROBE_MAX_EXTENSION_RAW 64U" in src
      and "Steering_SafetyEnvelopeWithNcDiscovery(" in src)
check("diagnostics available only via safe disarmed M8-S00 UART",
      "M8S00_SPIN_NC,scope=since_boot" in nc_dump
      and "M8S00_DumpSpinNc() == 0U" in dump
      and "steer_safety_motion_permit != 0U" in dump
      and "Steering_PositionWheelsStopped() == 0U" in dump)

passed = sum(tests)
print(f"\nM8 R2/NC/spin source checks: {passed}/{len(tests)} PASS")
print("NOT a build or hardware PASS. NEVER bypass a failed NC.")
raise SystemExit(0 if passed == len(tests) else 1)
