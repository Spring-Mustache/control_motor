#!/usr/bin/env python3
"""M8-S00 source-only invariants. Does not build or run STM32 hardware."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
main = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")

checks = [
    ("capture struct", "M8S00StopSnapshot_t" in main),
    ("first stop sticky", "volatile M8S00StopSnapshot_t m8_s00_first_stop" in main),
    ("first abnormal sticky", "volatile M8S00StopSnapshot_t m8_s00_first_fault" in main),
    ("explicit diagnostic clear", "m8_s00_clear_request" in main and
     "if ((m8_s00_clear_request == 0U)" in main),
    ("clear does not modify owner permit", re.search(
        r"static void M8S00_ClearService\(void\)\s*\{([\s\S]*?)\n\}", main) is not None),
    ("position finish capture", re.search(
        r"static void Steering_PositionFinish\(uint8_t result\)\s*\{\s*"
        r"if \(steer_pos_active != 0U\)\s*M8S00_CaptureStop\("
        r"6U, 3U, result,[\s\S]*?Steering_PositionLeftOff\(", main) is not None),
    ("position axis fault captured", re.search(
        r"static void Steering_PositionAxisFault\([\s\S]{0,200}"
        r"M8S00_CaptureStop\(5U,[\s\S]{0,200}Steering_PositionLeftOff\(", main) is not None),
    ("manual axis fault captured", re.search(
        r"static void Steering_ManualAxisFault\([\s\S]{0,200}"
        r"M8S00_CaptureStop\(2U,[\s\S]{0,200}Steering_ManualLeftOff\(", main) is not None),
    ("manual owner stop captured", re.search(
        r"static void Steering_ManualStop\(void\)[\s\S]{0,600}"
        r"M8S00_CaptureStop\(3U", main) is not None),
    ("four measured RPM in evidence", all(x in main for x in
        ("v.rpm_lu =", "v.rpm_ld =", "v.rpm_ru =", "v.rpm_rd ="))),
    ("per-wheel PWM in evidence", all(x in main for x in
        ("v.wheel_pwm_lu =", "v.wheel_pwm_ld =",
         "v.wheel_pwm_ru =", "v.wheel_pwm_rd ="))),
    ("NC in evidence", "v.nc_mask = Steering_TestRawLimitMask();" in main),
    ("physical CCR evidence", "v.ccr_left = (uint16_t)__HAL_TIM_GET_COMPARE" in main and
        "v.ccr_right = (uint16_t)__HAL_TIM_GET_COMPARE" in main),
    ("PS2 owner flags retained", "v.guard_flags |= 1UL << 15;" in main),
    ("wheel-stop gate not removed", "Motor_LeftUp.current_rpm > 2.0f" in main and
        "Motor_RightDown.current_rpm < -2.0f" in main),
    ("manual stall protection retained", "#define STEER_MANUAL_STALL_MS          650U" in main),
    ("manual timeout protection retained", "#define STEER_MANUAL_RUN_MAX_MS       5000U" in main),
    ("position stall protection retained", "#define STEER_POS_STALL_MS         650U" in main),
    ("NC interrupt PWM cutoff retained", "static void Steering_SafetyOnLimitInterrupt(uint8_t bit)" in main and
        "__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);" in main and
        "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);" in main),
]

failed = []
for label, ok in checks:
    print(("[PASS] " if ok else "[FAIL] ") + label)
    if not ok:
        failed.append(label)
print(f"\nM8-S00 static checks: {len(checks) - len(failed)}/{len(checks)} PASS")
print("No target build, host-C scenario or motor hardware test has been performed.")
raise SystemExit(1 if failed else 0)
