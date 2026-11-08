#!/usr/bin/env python3
"""M9-H2A: power-locked supervisor, exact-C GCC, main wiring invariants.

Do not infer bench active HOLD PASS: M9H2_POWER_PERMITTED=0U and NO motor
writes in any H2 main function. Data are H0/H1 cached AS5600 only.
"""
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import shutil

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
SRC=(ROOT/"Core/Src/m9_h2_hold_guard.c").read_text(encoding="utf-8")
HDR=(ROOT/"Core/Inc/m9_h2_hold_guard.h").read_text(encoding="utf-8")
W=(ROOT/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
PS=(ROOT/"tools/capture_m9_h2_guard.ps1").read_text(encoding="utf-8")
def method(signature):
  start=MAIN.index(signature)
  a=MAIN.index("{",start)
  n=0
  for j in range(a,len(MAIN)):
    if MAIN[j]=="{":n+=1
    elif MAIN[j]=="}":
      n-=1
      if n==0:return MAIN[start:j+1]
  raise AssertionError(signature)
begin=method("static void M9H2_StartAfterDone(uint8_t result)")
tick=method("static void M9H2_IdleUpdate(void)")
finish=method("static void Steering_PositionFinish(uint8_t result)")
stream=method("static void M8S11_StreamUpdate(void)\n{")
assert '#include "m9_h2_hold_guard.h"' in MAIN
assert "M9H2_POWER_PERMITTED        0U" in HDR
assert "volatile uint8_t m9_h2_monitor_enable = 1U" in MAIN
assert "volatile uint8_t m9_h2_bench_request = 0U" in MAIN
assert "M9H2_Begin(&m9_h2_guard" in begin
for condition in ("result != 2U", "steer_pos_left_state != STEER_AXIS_DONE",
    "steer_pos_right_state != STEER_AXIS_DONE", "steer_pos_left_result != 2U",
    "steer_pos_right_result != 2U", "steer_pos_limit_trip_mask != 0U",
    "steer_safety_trip_latched != 0U", "control_stop_latched != 0U",
    "steer_pos_command != 1U", "steer_pos_command != 4U",
    "voice_pos_owned == 0U","m9_h2_pending != 0U"):
  assert condition in begin,condition
assert finish.index("Steering_PositionLeftOff();") < finish.index("M9H2_StartAfterDone(result);")
assert finish.index("Steering_PositionRightOff();") < finish.index("M9H2_StartAfterDone(result);")
assert finish.index("M9H2_StartAfterDone(result);") < finish.index("steer_pos_active = 0U;")
assert "M9H2_IdleUpdate(); /* H2A: independent safe supervisor; no motor output */" in MAIN
assert "M9H2_Step(&m9_h2_guard, &in)" in tick
assert "M9H2_ModeAllowed(" in tick and "m9_h2_mode_ok != 0U" in tick
assert "m9_h2_mode_ok == 0U" in tick
assert "Voice_HasPendingData()" in tick
assert "voice_execution_active != 0U" in tick
assert "voice_command_ready != 0U" in tick
assert "voice_pos_request != 0U" in tick
assert "voice_pos_owned != 0U" in tick
assert "ps2_manual_active, ps2_cancel_hold" in tick
assert "M9H2_ModeAllowed(" in SRC
assert "M9H2_CONTROL_VOICE 1U" in HDR
assert "M9H2_CONTROL_IDLE  0U" in HDR
assert "M9H2_CONTROL_PS2   2U" in HDR
assert "M9H2_CONTROL_STOP  3U" in HDR
assert "CONTROL_MODE_IDLE = 0" in MAIN
assert "CONTROL_MODE_VOICE," in MAIN
assert "CONTROL_MODE_PS2," in MAIN
assert "CONTROL_MODE_STOP" in MAIN
for condition in ("Steering_TestRawLimitMask()", "steer_safety_trip_latched",
 "control_stop_latched", "steer_safety_neutral_required",
 "steer_safety_motion_permit", "STEER_SAFETY_INCIDENT_LOCKOUT",
 "Steering_PositionWheelsStopped()", "debug_as5600_poll_enable",
 "steer_pos_left_new_sample_count", "steer_pos_right_new_sample_count",
 "AS5600_DebugReadingValid", "steer_pos_left_last_sample_ms",
 "steer_pos_right_last_sample_ms", "m9_h2_block_bits",
 "m9_h2_owner_mode", "m9_h2_bench_request"):
  assert condition in tick,condition
assert "M9H2_GUARD,sha=%s" in stream
assert "powered=0,shadowOnly=1" in stream
assert "modeStart=%u,modeNow=%u,modeOK=%u,voiceBusy=%u" in stream
assert "dutyL=%u,dutyR=%u" in stream
assert "m9_h0_pending == 0U" in stream
assert "m9_h1_pending == 0U" in stream
assert "steer_pos_active == 0U" in stream
assert "Steering_PositionWheelsStopped() != 0U" in stream
assert "huart3.gState == HAL_UART_STATE_READY" in stream
assert "M8S11_SendLine(n)" in stream
assert "m9_h2_pending = 0U" in stream
assert "M9H2_Cancel" in SRC
assert "zero_output(g)" in SRC
assert "M9H2_POWER_PERMITTED != 0U" in SRC
assert "M9H2_MAX_HOLD_ERROR_RAW" in SRC
assert "M9H2_SAMPLE_STALE_MS" in SRC
assert "M9H2_KD_Q8" in SRC
for code in (begin,tick,SRC):
  for forbidden in ("__HAL_TIM_SET_COMPARE","HAL_GPIO_WritePin",
      "AS5600_ReadRawAndStatus","HAL_I2C_Master",
      "HAL_UART_Transmit", "Wheel_EmergencyStop", "Turn_Motors_Stop"):
    assert forbidden not in code,(forbidden,"H2A no actuation invariant")
assert "verify_m9_h2_guard.py" in W
assert "capture_m9_h2_guard.ps1" in W
assert "M9H2_GUARD," in PS
assert "powered=0" in PS and "dutyL=0,dutyR=0" in PS
gcc=shutil.which("gcc")
assert gcc,"GCC needed for H2-A real-C unit tests"
with TemporaryDirectory() as d:
  exe=str(Path(d)/"h2a")
  subprocess.run([gcc,"-std=c11","-Wall","-Wextra","-Werror",
      "-I"+str(ROOT/"Core/Inc"),
      str(ROOT/"Core/Src/m9_h2_hold_guard.c"),
      str(ROOT/"Core/Src/m9_h1_shadow.c"),
      str(ROOT/"tools/m9_h2_host_test.c"),"-o",exe],check=True)
  subprocess.run([exe],check=True)
print("PASS M9-H2A source guard, hard-locked output, C host and HAL exclusions")
