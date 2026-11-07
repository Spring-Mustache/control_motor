#!/usr/bin/env python3
"""Verify M9-H1 REAL flight recorder is read-only, safe-idle and actual-C.
It instruments REAL sensors/motor outputs in firmware. HOST test is only a
regression harness and is NOT used as measured tuning data.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
import shutil

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
REC=(ROOT/"Core/Src/m9_h1_real_trace.c").read_text(encoding="utf-8")
HDR=(ROOT/"Core/Inc/m9_h1_real_trace.h").read_text(encoding="utf-8")
W=(ROOT/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
PS=(ROOT/"tools/capture_m9_h1_real.ps1").read_text(encoding="utf-8")

def method(sig):
  a=MAIN.index(sig)
  b=MAIN.index("{",a)
  depth=0
  for i in range(b,len(MAIN)):
    if MAIN[i]=="{":depth+=1
    elif MAIN[i]=="}":
      depth-=1
      if depth==0:return MAIN[a:i+1]
  raise AssertionError(sig)

start=method("static void M9REAL_StartForPosition(void)")
observe=method("static void M9REAL_ObserveForPosition(void)")
finish=method("static void M9REAL_FinishForPosition(uint8_t result)")
position=method("static void Steering_PositionFinish(uint8_t result)")
motion=method("static void Steering_PositionUpdate(void)")
stream=method("static void M8S11_StreamUpdate(void)\n{")
assert '#include "m9_h1_real_trace.h"' in MAIN
assert "M9REAL_StartForPosition();" in motion
assert "steer_pos_run_count++;" in motion
assert motion.index("steer_pos_run_count++;") < motion.index("M9REAL_StartForPosition();")
assert "M9REAL_FinishForPosition(result);" in position
assert position.index("M9REAL_FinishForPosition(result);") < position.index("Steering_PositionLeftOff();")
assert "M9REAL_ObserveForPosition(); /* capture REAL M8 motion in RAM only */" in MAIN
assert "M9H0_IdleUpdate();" in MAIN
assert "M9H1_REAL_BEGIN,sha=%s" in stream
assert "M9H1_REAL_FRAME,sha=%s" in stream
assert "M9H1_REAL_END,sha=%s" in stream
assert "m9_h0_pending == 0U" in stream
assert "m9_h1_pending == 0U" in stream
assert "Steering_PositionWheelsStopped() != 0U" in stream
assert "huart3.gState == HAL_UART_STATE_READY" in stream
assert "m8_s154_right_fine_pending == 0U" in stream
assert "__HAL_TIM_GET_COMPARE" in stream
assert "M8S11_SendLine(n)" in stream
assert "m9_real_pending = 0U" in stream
assert "m9_real_tx_next" in stream
assert "steer_pos_command != 1U" in start
assert "steer_pos_command != 4U" in start
assert "voice_pos_owned == 0U" in start
assert "steer_safety_trip_latched != 0U" in start
assert "steer_pos_left_new_sample_count" in observe
assert "steer_pos_right_new_sample_count" in observe
assert "steer_pos_left_last_sample_ms" in observe
assert "steer_pos_right_last_sample_ms" in observe
assert "AS5600_DebugReadingValid" in observe
assert "M9REAL_Observe(&m9_real_trace" in observe
assert "M9REAL_Finish(&m9_real_trace" in finish
assert "M9REAL_MAX_FRAMES 80U" in HDR
assert "M9REAL_FRAME_PERIOD_MS 80U" in HDR
assert "M9H1_Feed(&d->pd" in REC
assert "M9H1_Candidate" not in REC
assert "verify_m9_h1_real.py" in W
assert "capture_m9_h1_real.ps1" in W
assert "M9H1_REAL_FRAME," in PS and "M9H1_REAL_END," in PS
for block in (start,observe,finish,REC):
  for forbidden in ("__HAL_TIM_SET_COMPARE","HAL_GPIO_WritePin",
                    "AS5600_ReadRawAndStatus","HAL_I2C_Master",
                    "HAL_UART_Transmit", "Turn_Motors_Stop",
                    "Wheel_EmergencyStop", "HAL_Delay("):
    assert forbidden not in block,(forbidden,"read-only invariant")
gcc=shutil.which("gcc")
assert gcc,"GCC needed for H1 REAL HOST regression"
with TemporaryDirectory() as temp:
  exe=str(Path(temp)/"m9realtest")
  subprocess.run([gcc,"-std=c11","-Wall","-Wextra","-Werror",
                  "-I"+str(ROOT/"Core/Inc"),
                  str(ROOT/"Core/Src/m9_h1_real_trace.c"),
                  str(ROOT/"Core/Src/m9_h1_shadow.c"),
                  str(ROOT/"tools/m9_h1_real_host_test.c"),
                  "-o",exe],check=True)
  subprocess.run([exe],check=True)
print("PASS M9-H1 REAL recorder no new actuator writes, exact-C HOST invariants")
