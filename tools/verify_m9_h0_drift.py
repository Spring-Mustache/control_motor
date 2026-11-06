#!/usr/bin/env python3
"""M9-H0 source wiring + real pure-C drift accounting (HOST ONLY).

No target CubeIDE compile, no extra AS5600 polls, no powered HOLD.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil
import subprocess

R = Path(__file__).resolve().parents[1]
main = (R / "Core/Src/main.c").read_text(encoding="utf-8")
impl = (R / "Core/Src/m9_h0_drift.c").read_text(encoding="utf-8")
header = (R / "Core/Inc/m9_h0_drift.h").read_text(encoding="utf-8")
workflow = (R / ".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
ps = (R / "tools/capture_m9_h0_drift.ps1").read_text(encoding="utf-8")

def function(signature):
    start = main.index(signature)
    begin = main.index("{", start)
    depth = 0
    for i in range(begin, len(main)):
        if main[i] == "{":
            depth += 1
        elif main[i] == "}":
            depth -= 1
            if depth == 0:
                return main[start:i+1]
    raise AssertionError(signature)

start = function("static void M9H0_StartAfterDone(")
stop = function("static void M9H0_Stop(")
idle = function("static void M9H0_IdleUpdate(")
finish = function("static void Steering_PositionFinish(")
stream = function("static void M8S11_StreamUpdate(void)\n{")
loop = main[main.index("Steering_PositionTelemetryIdleUpdate();", main.index("int main(void)")):]

assert '#include "m9_h0_drift.h"' in main
assert "volatile uint8_t m9_h0_monitor_enable = 1U;" in main
assert "result != 2U" in start
assert "steer_pos_left_state != STEER_AXIS_DONE" in start
assert "steer_pos_right_state != STEER_AXIS_DONE" in start
assert "steer_pos_left_result != 2U" in start
assert "steer_pos_right_result != 2U" in start
assert "steer_pos_limit_trip_mask != 0U" in start
assert "steer_pos_command != 1U" in start
assert "steer_pos_command != 4U" in start
assert "m9_h0_pending != 0U" in start
assert "M9H0_Begin(&m9_h0_drift" in start
assert (finish.index("Steering_PositionLeftOff();") <
        finish.index("Steering_PositionRightOff();") <
        finish.index("M9H0_StartAfterDone(result);") <
        finish.index("steer_pos_active = 0U;"))
assert "M9H0_Finish(&m9_h0_drift" in stop
assert "Steering_PositionWheelsStopped() == 0U" in idle
assert "steer_safety_trip_latched != 0U" in idle
assert "control_stop_latched != 0U" in idle
assert "steer_pos_active != 0U" in idle
assert "__HAL_TIM_GET_COMPARE" in idle
assert "steer_pos_left_new_sample_count" in idle
assert "steer_pos_right_new_sample_count" in idle
assert "debug_as5600_left_read_fail_count" in idle
assert "debug_as5600_right_read_fail_count" in idle
assert "AS5600_DebugReadingValid" in idle
assert "M9H0_WINDOW_MS" in idle
assert (loop.index("Steering_PositionTelemetryIdleUpdate();") <
        loop.index("M9H0_IdleUpdate();") <
        loop.index("M8S11_StreamUpdate();"))
for section in [start, stop, idle, impl]:
    for forbidden in ("__HAL_TIM_SET_COMPARE", "HAL_GPIO_WritePin(",
                      "AS5600_ReadRawAndStatus(", "HAL_UART_Transmit(",
                      "HAL_UART_Transmit_IT(", "Turn_Motors_Stop("):
        assert forbidden not in section, forbidden
assert "M9H0_DRIFT,sha=%s" in stream
assert "m9_h0_pending = 0U" in stream
assert "huart3.gState == HAL_UART_STATE_READY" in stream
assert "Steering_PositionWheelsStopped() != 0U" in stream
assert "M8S11_SendLine(n)" in stream
assert "M9H0_DRIFT," in ps
assert "m9_h0_drift.c" in header or "M9H0_AbsDelta" in header
assert "python3 tools/verify_m9_h0_drift.py" in workflow
assert 'capture_m9_h0_drift.ps1' in workflow

gcc = shutil.which("gcc")
assert gcc is not None, "GCC required for M9-H0 exact C host test"
with TemporaryDirectory() as tmp:
    out = str(Path(tmp)/"m9_h0")
    subprocess.run([gcc, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-I"+str(R/"Core/Inc"),
                    str(R/"Core/Src/m9_h0_drift.c"),
                    str(R/"tools/m9_h0_host_test.c"),
                    "-o", out], check=True)
    subprocess.run([out], check=True)
print("PASS M9-H0 source routing, passive constraints and GCC C HOST; NOT target/hardware PASS")
