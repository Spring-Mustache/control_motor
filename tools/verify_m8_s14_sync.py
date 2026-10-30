#!/usr/bin/env python3
"""M8-S14 static routing/safety checks + host C planner tests.

Static checks do not establish STM32 build, motor behavior or NC electrical safety.
Missing host compiler is SKIP, never reported as C test PASS.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = (root / "Core/Src/main.c").read_text(encoding="utf-8")
math = (root / "Core/Inc/steer_s14_math.h").read_text(encoding="utf-8")
checks = {
    "optional OFF at boot": "volatile uint8_t steer_s14_manual_enable = 0U;" in src,
    "mode latched at fresh manual start": "steer_s14_manual_latched = (steer_s14_manual_enable == 1U) ? 1U : 0U;" in src,
    "S14 exclusive versus S12 waypoint": bool(re.search(
        r"if \(steer_s14_manual_latched == 1U\)\s+Steering_S14ManualApply\(", src)),
    "old PI bypass for simultaneous S14 moving": "((steer_angle_wp_enable == 1U) ||\n       (steer_s14_manual_latched == 1U))" in src,
    "stop resets S14 dynamic state": "Steering_S14ManualReset();\n  Steering_ManualLeftOff();" in src,
    "uses already validated AS5600 mapping": src.count("Steering_AngleFromRaw(") >= 10,
    "no S13 equal-angle flag restored": "steer_angle_wp_manual_equal_angle_enable" not in src,
    "R1 fine baseline preserved": "volatile uint16_t steer_pos_right_fine_pwm = 85U;" in src and
        "volatile uint16_t steer_pos_right_fine_pulse_ms = 12U;" in src,
    "R2 NC-authoritative handling preserved": "M8S00_R2EncOnMin(left, result);" in src,
    "actual NC atomic recheck preserved": "primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();" in src,
    "legacy PWM caps retained": "if (cap_l > 300U) cap_l = 300U;" in src and
        "if (cap_r > 100U) cap_r = 100U;" in src,
    "S14 only requests taper": "M8S14_Plan" in math and
        "trim_left_pwm" in math and "trim_right_pwm" in math and
        "__HAL_TIM_SET_COMPARE" not in math and "HAL_GPIO_WritePin" not in math,
    "S14 export after disarm": '"M8S00_S14,boot=%u,last=%u,frames=%lu,"' in src,
    "S14 UART line CRLF framing": "\"trimL=%u,trimR=%u\\r\\n\"," in src,
}
for label, okay in checks.items():
    print(("PASS" if okay else "FAIL") + ": " + label)
if not all(checks.values()):
    raise SystemExit(1)

cc = next((p for n in ("gcc", "clang", "cc") if (p := shutil.which(n))), None)
if cc is None:
    print("SKIP: host C planner execution (no C compiler)")
else:
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / ("test_s14.exe" if __import__("os").name == "nt" else "test_s14")
        build = subprocess.run([cc, "-std=c11", "-Wall", "-Wextra",
                                "-Werror", "-I", str(root / "Core/Inc"),
                                str(root / "tools/test_m8_s14_math.c"),
                                "-o", str(exe)], capture_output=True, text=True)
        if build.returncode:
            print(build.stdout + build.stderr)
            raise SystemExit("FAIL: host C test compilation")
        run = subprocess.run([str(exe)], capture_output=True, text=True)
        print(run.stdout or run.stderr)
        if run.returncode:
            raise SystemExit("FAIL: host C planner")
print("M8-S14 static checks PASS; STM32 target + hardware acceptance still pending")
