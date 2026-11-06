#!/usr/bin/env python3
"""M9-H1: exact-C GCC PD shadow + wiring and no-actuation invariants.

This verifier runs a synthetic abrupt load-release example. Passing it does
NOT prove motor damping, real-current limits or STM32 target compilation.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
import shutil

ROOT = Path(__file__).resolve().parents[1]
SRC = (ROOT / "Core/Src/main.c").read_text(encoding="utf-8")
H1 = (ROOT / "Core/Src/m9_h1_shadow.c").read_text(encoding="utf-8")
HDR = (ROOT / "Core/Inc/m9_h1_shadow.h").read_text(encoding="utf-8")
CI = (ROOT / ".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
PS = (ROOT / "tools/capture_m9_h1_shadow.ps1").read_text(encoding="utf-8")

def func(signature):
    a = SRC.index(signature)
    b = SRC.index("{", a)
    depth = 0
    for i in range(b, len(SRC)):
        if SRC[i] == "{":
            depth += 1
        elif SRC[i] == "}":
            depth -= 1
            if depth == 0:
                return SRC[a:i+1]
    raise AssertionError(signature)

init = func("static void M9H0_StartAfterDone(")
finish = func("static void M9H0_Stop(")
read = func("static void M9H0_IdleUpdate(")
pos_finish = func("static void Steering_PositionFinish(")
stream = func("static void M8S11_StreamUpdate(void)\n{")

assert '#include "m9_h1_shadow.h"' in SRC
assert "volatile uint8_t m9_h1_shadow_enable = 1U;" in SRC
assert "M9H1_Begin(&m9_h1_shadow" in init
assert "m9_h1_pending == 0U" in init
assert "m9_h1_dropped++" in init
assert "result != 2U" in init and "steer_pos_limit_trip_mask != 0U" in init
assert "steer_pos_left_result != 2U" in init
assert "steer_pos_right_result != 2U" in init
assert "steer_pos_command != 1U" in init and "steer_pos_command != 4U" in init
assert (pos_finish.index("Steering_PositionLeftOff();") <
        pos_finish.index("Steering_PositionRightOff();") <
        pos_finish.index("M9H0_StartAfterDone(result);") <
        pos_finish.index("steer_pos_active = 0U;"))
assert "M9H1_Finish(&m9_h1_shadow, reason);" in finish
assert read.count("M9H1_Feed(&m9_h1_shadow") == 4
assert "M9H1_Tick(&m9_h1_shadow, now)" in read
assert "steer_safety_trip_latched != 0U" in read
assert "control_stop_latched != 0U" in read
assert "Steering_PositionWheelsStopped() == 0U" in read
assert "m9_h1_shadow_enable != 1U" in read
assert "M9H1_SHADOW,sha=%s" in stream
assert "shadowOnly=1" in stream
assert "m9_h0_pending == 0U" in stream
assert "m9_h1_pending = 0U" in stream
assert "m8_s11_stream_state == 0U" in stream
assert "huart3.gState == HAL_UART_STATE_READY" in stream
assert "Steering_PositionWheelsStopped() != 0U" in stream
assert "__HAL_TIM_GET_COMPARE" in stream
assert "M8S11_SendLine(n)" in stream
assert "M9H1_WINDOW_MS" in HDR and "M9H1_STALE_MS" in HDR
assert "M9H1_DEADBAND_RAW" in HDR
assert "M9H1_MAX_STEP_RAW" in H1
assert "M9H1_Candidate" in H1
for code in (init, finish, read, H1):
    for bad in ("__HAL_TIM_SET_COMPARE", "HAL_GPIO_WritePin",
                "AS5600_ReadRawAndStatus", "HAL_UART_Transmit(",
                "HAL_UART_Transmit_IT(", "Turn_Motors_Stop(",
                "Wheel_EmergencyStop("):
        assert bad not in code, bad
assert "capture_m9_h1_shadow.ps1" in CI
assert "verify_m9_h1_shadow.py" in CI
assert "M9H1_SHADOW," in PS and "M9H0_DRIFT," in PS
gcc = shutil.which("gcc")
assert gcc, "gcc needed for exact-C HOST unit tests"
with TemporaryDirectory() as temp:
    exe = str(Path(temp)/"test_m9_h1")
    subprocess.run([gcc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-I"+str(ROOT/"Core/Inc"),
                    str(ROOT/"Core/Src/m9_h1_shadow.c"),
                    str(ROOT/"tools/m9_h1_host_test.c"),
                    "-o", exe], check=True)
    subprocess.run([exe], check=True)
print("PASS M9-H1 strict PD shadow C HOST + motor-output exclusions; NOT STM32 HOLD PASS")
