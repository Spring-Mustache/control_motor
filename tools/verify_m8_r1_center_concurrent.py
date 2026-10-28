#!/usr/bin/env python3
"""Source-only M8 R1 coarse/fine and six-motor concurrent PWM checks.

This does NOT compile STM32 or verify powered hardware. Do not mistake
an MCU PWM command observation for a physical motor movement test.
"""
from pathlib import Path
import re

src = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text("utf-8")


def body(name: str) -> str:
    pat = r"\bstatic\s+(?:void|uint8_t|uint16_t)\s+" + re.escape(name)
    pat += r"\s*\([^;]*?\)\s*\{"
    m = re.search(pat, src, re.S)
    if m is None:
        raise AssertionError(f"{name}: function definition not found")
    start = m.end() - 1
    dep = 0
    for pos in range(start, len(src)):
        ch = src[pos]
        if ch == "{":
            dep += 1
        if ch == "}":
            dep -= 1
            if dep == 0:
                return src[m.start():pos + 1]
    raise AssertionError(f"{name}: braces missing")


checks = []
def verify(msg, test):
    result = bool(test)
    print(("PASS" if result else "FAIL") + ": " + msg)
    checks.append(result)

axis = body("Steering_PositionAxisUpdate")
fine = body("Steering_PositionFineUpdate")
position = body("Steering_PositionUpdate")
ps2 = body("PS2_ApplyManualControl")
obs = body("M8S00_ObserveConcurrent")
dump = body("M8S00_DumpUpdate")
output = body("M8S00_DumpConcurrent")
cal = body("Steering_CalUpdate")
emergency = body("Wheel_EmergencyStop")

verify("R1 LEFT fine entry 15 RAW, RIGHT remains 35",
       re.search(r"#define\s+STEER_POS_FINE_LEFT_ENTER_RAW\s+15U\b", src) and
       re.search(r"#define\s+STEER_POS_FINE_ENTER_RAW\s+35U\b", src) and
       "left != 0U" in axis and
       "STEER_POS_FINE_LEFT_ENTER_RAW : STEER_POS_FINE_ENTER_RAW" in axis)
verify("R1 fine retains +/-5 RAW, 8-pulse, 9s bounds",
       "Steering_CenterFineTolClamp(" in fine and
       "#define STEER_POS_FINE_MAX_PULSES       8U" in src and
       "#define STEER_POS_R1_TIMEOUT_MS   9000U" in src and
       "STEER_POS_FINE_MAX_PULSES" in fine)
verify("fine controller does not increase PWM or pulse cap",
       "if (pwm > 300U) pwm = 300U;" in fine and
       "if (pwm > 100U) pwm = 100U;" in fine and
       "if (v > 60U) ? 60U : v" in src and
       "steer_pos_left_fine_pwm = 175U" in src)
verify("R1 stays active through NC departure, with 120ms quiet",
       "Steering_PositionOwnEscapeIrq(1U, pending)" in position and
       "Steering_PositionOwnEscapeIrq(0U, pending)" in position and
       "Steering_PositionEscapeReleased(left, now)" in src and
       "#define STEER_POS_ESCAPE_RELEASE_QUIET_MS 120U" in src)
verify("R2 remains distinct from NC endpoint calibration",
       "steer_pos_left_r2_target_raw" in axis and
       "steer_pos_right_r2_target_raw" in axis and
       "STEER_POS_LEFT_MIN_RAW" in axis and
       "STEER_POS_RIGHT_MIN_RAW" in axis)
verify("SELECT remains isolated and all NC discovery protections remain",
       "#define STEER_CAL_PROBE_MAX_EXTENSION_RAW 64U" in src and
       "Steering_CalInputsNeutral() == 0U" in cal and
       "Steering_PositionWheelsStopped() == 0U" in cal and
       "Steering_CalCommitAxisSafe(" in src)
verify("PS2 LY/Up/Down continue to issue wheel targets with R1/R2",
       "WheelControl_SetTargetPWM(pwm_duty);" in ps2 and
       "if ((PS2_Pad.Select != 0U) || (steer_cal_active != 0U))" in ps2)
verify("PWM overlap checks real four wheel commands and both steer CCRs",
       all(x in obs for x in ("wheel_pwm_lu == 0", "wheel_pwm_ld == 0",
                             "wheel_pwm_ru == 0", "wheel_pwm_rd == 0",
                             "htim12, TIM_CHANNEL_2", "htim14, TIM_CHANNEL_1")))
verify("PWM overlap blocked in fault, disarm, spin, calibration",
       all(x in obs for x in ("control_mode != CONTROL_MODE_PS2",
                             "steer_safety_motion_permit != 1U",
                             "steer_safety_trip_latched != 0U",
                             "steer_cal_active != 0U",
                             "WheelControl_SpinIsActive() != 0U")))
verify("observer runs after all steering owners and wheel updates",
       src.index("WheelControl_Update();", src.index("int main(void)")) <
       src.index("Steering_PositionUpdate(); /* R1/R2 */", src.index("int main(void)")) <
       src.index("M8S00_ObserveConcurrent();", src.index("int main(void)")))
verify("serial overlap evidence is printed only after safety gate",
       "M8S00_DumpConcurrent() == 0U" in dump and
       "M8S00_CONCURRENT,scope=since_boot" in output and
       "(steer_safety_motion_permit != 0U)" in dump and
       "(Steering_PositionWheelsStopped() == 0U)" in dump)
verify("R1 fine reason remains in captured STOP/FAULT",
       "M8S00_FINE,%s,reasonL=%u,reasonR=%u" in src and
       "v.fine_fault_left = steer_pos_left_fine_fault;" in src)
verify("STOP still shuts off wheel and steering commands",
       "WheelControl_EmergencyStop();" in emergency and
       "Turn_Motors_Stop();" in emergency)

print(f"\nM8 R1/concurrent regression: {sum(checks)}/{len(checks)} PASS")
print("No STM32 build, mechanical NC, driver power, wheel load or motor PWM was tested.")
raise SystemExit(0 if all(checks) else 1)
