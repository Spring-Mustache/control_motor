#!/usr/bin/env python3
"""M8-S00 source-only invariants. Does not build or run STM32 hardware."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
main = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")
logger = (root / "Services" / "Src" / "ram_logger.c").read_text(encoding="utf-8")
header = (root / "Services" / "Inc" / "ram_logger.h").read_text(encoding="utf-8")
wheel = (root / "Core" / "Src" / "wheel_control.c").read_text(encoding="utf-8")
powershell = (root / "tools" / "capture_m8_s00.ps1").read_text(encoding="utf-8")

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
    ("fine R1 subreason captured at stop time", all(k in main for k in (
        "v.fine_fault_left = steer_pos_left_fine_fault;",
        "v.fine_fault_right = steer_pos_right_fine_fault;",
        "v.fine_pulses_left = steer_pos_left_fine_pulses;",
        "v.fine_pulses_right = steer_pos_right_fine_pulses;",
        "v.fine_error_left = steer_pos_left_fine_error;",
        "v.fine_error_right = steer_pos_right_fine_error;"))),
    ("fine R1 STOP/FAULT serial evidence for both sides", all(k in main for k in (
        "M8S00_FINE,%s,reasonL=%u,reasonR=%u",
        "p->fine_fault_left", "p->fine_fault_right",
        "p->fine_pulses_left", "p->fine_pulses_right",
        "p->fine_pwm_left", "p->fine_pwm_right",
        "p->fine_pulse_ms_left", "p->fine_pulse_ms_right"))),
    ("PS2 owner flags retained", "v.guard_flags |= 1UL << 15;" in main),
    ("wheel-stop gate not removed", "Motor_LeftUp.current_rpm > 2.0f" in main and
        "Motor_RightDown.current_rpm < -2.0f" in main),
    ("manual stall protection retained", "#define STEER_MANUAL_STALL_MS          650U" in main),
    ("manual timeout protection retained", "#define STEER_MANUAL_RUN_MAX_MS       5000U" in main),
    ("position stall protection retained", "#define STEER_POS_STALL_MS         650U" in main),
    ("NC interrupt PWM cutoff retained", "static void Steering_SafetyOnLimitInterrupt(uint8_t bit)" in main and
        "__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);" in main and
        "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);" in main),
    ("explicit VCP export request", "volatile uint8_t m8_s00_dump_request = 0U;" in main and
        "m8_s00_dump_status = 1U;" in main),
    ("full stop and fault export", 'M8S00_DumpOne("STOP", &m8_s00_first_stop)' in main and
        'M8S00_DumpOne("FAULT", &m8_s00_first_fault)' in main),
    ("distinct framed dump", "#M8S00_BEGIN,1" in main and
        "#M8S00_END" in main),
    ("diagnostic dump after wheel dump", main.find("M8S00_DumpUpdate(); /* explicit VCP export") >
        main.find("RamLogger_DumpCsvUpdate();")),
    ("one-shot export (no delayed unsafe dump)", re.search(
        r"static void M8S00_DumpUpdate\(void\)[\s\S]*?m8_s00_dump_request = 0U;",
        main) is not None),
    ("dump refuses steering motion", all(k in main for k in
        ("steer_safety_motion_permit != 0U", "steer_manual_active != 0U",
         "steer_pos_active != 0U", "steer_cal_active != 0U"))),
    ("dump refuses non-zero steering CCR", all(k in main for k in
        ("__HAL_TIM_GET_COMPARE(&htim12, TIM_CHANNEL_2) != 0U",
         "__HAL_TIM_GET_COMPARE(&htim14, TIM_CHANNEL_1) != 0U"))),
    ("dump refuses wheel and logger activity", all(k in main for k in
        ("Steering_PositionWheelsStopped() == 0U",
         "wheel_log_active != 0U", "wheel_log_dump_active != 0U"))),
    ("no extra sensor read inside UART dump", re.search(
        r"static void M8S00_DumpUpdate\(void\)\n\{([\s\S]*?)\n\}",
        main) is not None),
    ("USER diagnostic vars defined", all(k in logger for k in (
        "volatile uint8_t wheel_log_m8_s00_ready = 0U;",
        "volatile uint8_t wheel_log_m8_s00_button_dump_request = 0U;"))),
    ("USER routing priority", re.search(
        r"static void WC_LogHandleShortUserPress\(void\)[\s\S]*?"
        r"wheel_log_m8_s00_ready[\s\S]*?wheel_log_active != 0U", logger) is not None),
    ("USER diagnostic vars declared", all(k in header for k in (
        "extern volatile uint8_t wheel_log_m8_s00_ready;",
        "extern volatile uint8_t wheel_log_m8_s00_button_dump_request;"))),
    ("USER main loop wired", "m8_s00_dump_request = 1U;" in main and
        "wheel_log_m8_s00_button_dump_request = 0U;" in main),
    ("boot-on wheel PID", "volatile uint8_t wheel_pid_enable = 1U;" in wheel and
        "wheel_pid_enable = 1U;" in wheel),
    ("PID snapshot exported", all(k in main for k in (
        "v.wheel_pid_enable = wheel_pid_enable;",
        "v.wheel_pid_active = wheel_pid_active;",
        "v.wheel_pid_update_count = wheel_pid_update_count;",
        'M8S00_PID,%s,enable=%u,active=%u'))),
    ("PowerShell native COM reader", "[System.IO.Ports.SerialPort]::GetPortNames()" in powershell and
        "System.IO.Ports.SerialPort" in powershell and
        "$serial.ReadLine().Trim()" in powershell),
    ("PowerShell exact frame boundaries", "'#M8S00_BEGIN,1'" in powershell and
        "'#M8S00_END'" in powershell and "WriteAllLines(" in powershell),
    ("PowerShell partial capture warns", "CHI NHAN DUOC MOT PHAN" in powershell),
]

failed = []
for label, ok in checks:
    print(("[PASS] " if ok else "[FAIL] ") + label)
    if not ok:
        failed.append(label)
print(f"\nM8-S00 static checks: {len(checks) - len(failed)}/{len(checks)} PASS")
print("No target build, host-C scenario or motor hardware test has been performed.")
raise SystemExit(1 if failed else 0)
