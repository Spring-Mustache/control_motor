#!/usr/bin/env python3
"""M8-S13 read-only regression: manual equal-angle A/B mode.

This checks the CURRENT main.c structure and geometry invariants.
It cannot cross-compile STM32 or prove mechanical safety/synchronization.
"""
from pathlib import Path
import re

src = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text("utf-8")

def body(name):
    m = re.search(r"\bstatic\s+(?:void|uint8_t|uint16_t)\s+" +
                  re.escape(name) + r"\s*\([^;]*?\)\s*\{", src, re.S)
    assert m, f"Missing function {name}"
    start, depth = m.end()-1, 0
    for i in range(start, len(src)):
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
            if depth == 0:
                return src[start:i+1]
    raise AssertionError(f"Unbalanced C function {name}")

checks = []
def guard(name, valid):
    checks.append(bool(valid))
    print(("PASS" if valid else "FAIL") + ": " + name)

wp = body("Steering_AngleWaypointApply")
manual = body("Steering_ManualUpdate")
position = body("Steering_PositionUpdate")
fine = body("Steering_PositionFineUpdate")
angle = body("Steering_AngleFromRaw")

guard("A/B defaults to OFF; existing controller unchanged on boot",
      "volatile uint8_t steer_angle_wp_manual_equal_angle_enable = 0U;" in src)
guard("Manual command latches mode; mid-stroke edit cannot switch",
      "steer_angle_wp_manual_equal_angle_latched =" in manual and
      "(steer_angle_wp_manual_equal_angle_enable == 1U) ? 1U : 0U;" in manual and
      "steer_manual_active == 0U" in manual and
      "steer_angle_wp_manual_equal_angle_latched == 1U" in wp)
guard("Equal-angle option limited to manual mode=3 only",
      "if ((mode == 3U) &&" in wp and
      "steer_angle_wp_manual_equal_angle_latched == 1U" in wp and
      "planned_gl = gl;" in wp and "planned_gr = gr;" in wp)
guard("Use shortest safe calibrated stroke, retaining sign",
      "common = (travel_l < travel_r) ? travel_l : travel_r;" in wp and
      "if ((common >= 500U) && (common <= 9000U))" in wp and
      "((delta_l < 0) ? -(int32_t)common : (int32_t)common)" in wp and
      "((delta_r < 0) ? -(int32_t)common : (int32_t)common)" in wp)
guard("No extra I2C or motor write within angle coordinator",
      all(forbidden not in wp for forbidden in
          ("AS5600_Read", "HAL_I2C_", "__HAL_TIM_SET_COMPARE(",
           "HAL_GPIO_WritePin(", "HAL_UART_Transmit(")))
guard("R1/R2 position ownership & fine correction preserved",
      "Steering_AngleWaypointApply(steer_pos_command" in position and
      "steer_pos_command == 1U" in position and
      "STEER_POS_FINE_MAX_PULSES" in fine and
      "Steering_PositionAxisDone(left, 2U)" in fine and
      "Steering_PositionOwnEscapeIrq(1U, pending)" in position)
guard("Manual NC and STOP guards retained after waypoint planner",
      "Steering_AngleWaypointApply(3U," in manual and
      "Steering_TestRawLimitMask();" in manual and
      "__disable_irq();" in manual and
      "Steering_ManualAxisFault(0U, 9U)" in manual and
      "Steering_SafetyOnLimitInterrupt(uint8_t bit)" in src and
      "Wheel_EmergencyStop(void)" in src)
guard("Existing PWM caps and opposite-direction mapping preserved",
      "if (cap_l > 300U) cap_l = 300U;" in wp and
      "if (cap_r > 100U) cap_r = 100U;" in wp and
      "int8_t left_direction = (int8_t)(-command);" in manual and
      "int8_t right_direction = command;" in manual)
guard("R1 RIGHT stable baseline remains 15 RAW / PWM85 / 12ms",
      "volatile uint16_t steer_pos_right_fine_enter_raw = STEER_POS_FINE_RIGHT_MIN_RAW;" in src and
      "volatile uint16_t steer_pos_right_fine_pwm = 85U;" in src and
      "volatile uint16_t steer_pos_right_fine_pulse_ms = 12U;" in src)

# Pure host math tests: equal cdeg displacement must yield equal progress;
# opposite-sign movement must never masquerade as forward progress.
def progress(start, goal, actual):
    span = goal - start
    travel = actual - start
    if span < 0:
        span, travel = -span, -travel
    if not 500 <= span <= 9000:
        return None
    return max(0, min(1000, travel * 1000 // span))

def common_goals(start_l, goal_l, start_r, goal_r):
    dl, dr = goal_l-start_l, goal_r-start_r
    common = min(abs(dl), abs(dr))
    if not 500 <= common <= 9000:
        return goal_l, goal_r
    return (start_l + (-common if dl < 0 else common),
            start_r + (-common if dr < 0 else common))

gl, gr = common_goals(0, 3780, 0, -4050)
guard("Common angular displacement normalized equally, including sign",
      gl == 3780 and gr == -3780 and
      progress(0, gl, 1890) == progress(0, gr, -1890) == 500 and
      progress(0, gl, -600) == 0 and progress(0, gr, 600) == 0)
guard("Near-target (<5 degrees) uses original goal rather than new normalization",
      common_goals(0, 300, 0, -450) == (300, -450))
guard("Angle source retains geometry-dependent mapping",
      "Steering_PositionGeometryValid" in angle and
      "STEER_POS_LEFT_CENTER_RAW" in angle and
      "STEER_POS_RIGHT_CENTER_RAW" in angle)

print(f"\nM8-S13 static/host-geometry checks: {sum(checks)}/{len(checks)} PASS")
print("No STM32 build, AS5600 electrical test or physical motion tested.")
raise SystemExit(0 if all(checks) else 1)
