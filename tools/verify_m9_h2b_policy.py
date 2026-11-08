#!/usr/bin/env python3
"""M9-H2B0 physical-drive boundary and actual-C near-goal policy.

STRICTLY SOURCE/HOST validation. No motor energizing and no H2-B bench
evidence is created here; that would require current, thermal, driver
PWM0/brake characterization and independently verified cutoff.
"""
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import shutil
ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
HDR=(ROOT/"Core/Inc/m9_h2b_hold_policy.h").read_text(encoding="utf-8")
SRC=(ROOT/"Core/Src/m9_h2b_hold_policy.c").read_text(encoding="utf-8")
GUARD=(ROOT/"Core/Inc/m9_h2_hold_guard.h").read_text(encoding="utf-8")
W=(ROOT/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
DOC=(ROOT/"docs/M9_H2B_POWER_GATE_2026_11_08.md").read_text(encoding="utf-8")
def method(signature):
  i=MAIN.index(signature)
  start=MAIN.index("{",i)
  depth=0
  for j in range(start,len(MAIN)):
    if MAIN[j]=="{":depth+=1
    elif MAIN[j]=="}":
      depth-=1
      if depth==0:return MAIN[i:j+1]
  raise AssertionError(signature)
begin=method("static void M9H2_StartAfterDone(uint8_t result)")
tick=method("static void M9H2_IdleUpdate(void)")
finish=method("static void Steering_PositionFinish(uint8_t result)")
serial=method("static void M8S11_StreamUpdate(void)\n{")
assert '#include "m9_h2b_hold_policy.h"' in MAIN
assert '#define M9H2B_POWER_STAGE_ENABLED       0U' in HDR
assert '#define M9H2B_VERIFIED_MOTOR_PWM_LIMIT  0U' in HDR
assert '#define M9H2B_VERIFIED_ON_TIME_MS       0U' in HDR
assert "#error \"H2-B power must not be enabled" in HDR
assert "#error \"H2-B calibration values" in HDR
assert "M9H2_POWER_PERMITTED        0U" in GUARD
assert "M9H2B_Begin(&m9_h2b_policy, &m9_h2_guard);" in begin
assert begin.index("M9H2_Begin(&m9_h2_guard") < begin.index("M9H2B_Begin(&m9_h2b_policy")
assert "M9H2B_Step(&m9_h2b_policy, &m9_h2_guard" in tick
assert tick.index("M9H2_Step(&m9_h2_guard, &in);") < tick.index("M9H2B_Step(&m9_h2b_policy")
assert "M9H2_StartAfterDone(result);" in finish
assert "Steering_PositionLeftOff();" in finish and "Steering_PositionRightOff();" in finish
assert finish.index("Steering_PositionRightOff();") < finish.index("M9H2_StartAfterDone(result);")
assert 'M9H2_GUARD,sha=%s' in serial
assert "bState=%u,bReason=%u" in serial and "bCcrL=%u,bCcrR=%u" in serial
assert "powered=0,shadowOnly=1" in serial
assert "m9_h0_pending == 0U" in serial and "m9_h1_pending == 0U" in serial
assert "Steering_PositionWheelsStopped() != 0U" in serial
assert "huart3.gState == HAL_UART_STATE_READY" in serial
assert "M8S11_SendLine(n)" in serial
assert "m9_h2b_policy.motor_ccr_l" in serial
assert "m9_h2b_policy.motor_ccr_r" in serial
assert "zero_motor(p);" in SRC
assert "return 0;" in SRC
assert "M9H2B_SCORE_ENTER_RAW" in SRC and "M9H2B_SCORE_EXIT_RAW" in SRC
assert "M9H2B_GUARD_REVOKED" in SRC
assert "M9H2B_EXPIRED" in SRC
assert "M9H2B_WINDOW_DONE" in SRC
assert "verify_m9_h2b_policy.py" in W
assert "H2-B0" in DOC and "H2-B1" in DOC
for code in (begin,tick,SRC):
  for forbidden in ("__HAL_TIM_SET_COMPARE","HAL_GPIO_WritePin",
     "AS5600_ReadRawAndStatus","HAL_I2C_Master",
     "HAL_UART_Transmit","Wheel_EmergencyStop","Turn_Motors_Stop"):
    assert forbidden not in code,(forbidden,"H2B0 must never actuate")
assert "HAL_" not in SRC
gcc=shutil.which("gcc")
assert gcc,"GCC required for H2-B actual C regression test"
with TemporaryDirectory() as tmp:
  exe=str(Path(tmp)/"m9h2b_host")
  subprocess.run([gcc,"-std=c11","-Wall","-Wextra","-Werror",
    "-I"+str(ROOT/"Core/Inc"),
    str(ROOT/"Core/Src/m9_h2b_hold_policy.c"),
    str(ROOT/"tools/m9_h2b_policy_host_test.c"),
    "-o",exe],check=True)
  subprocess.run([exe],check=True)
print("PASS M9-H2B0 guarded candidate C and hard motor-output lock")
