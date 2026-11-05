#!/usr/bin/env python3
"""M8 R1/Voice B0 RIGHT CENTER fine-entry compiled actual-C HOST test.

No mechanical coasting, CubeIDE target compilation or physical NC evidence.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess,shutil
root=Path(__file__).resolve().parents[1]
C=(root/"Core/Src/main.c").read_text(encoding="utf-8")
W=(root/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
PS=(root/"tools/capture_m8_r1_center.ps1").read_text(encoding="utf-8")
def extract(signature):
 a=C.index(signature);b=C.index("{",a);depth=0
 for i in range(b,len(C)):
  if C[i]=="{":depth+=1
  elif C[i]=="}":
   depth-=1
   if depth==0:return C[a:i+1]
 raise AssertionError(signature)
geo=extract("static uint8_t Steering_PositionGeometryValid(")
helper=extract("static uint16_t Steering_R1RightLateFineEntry(")
axis=extract("static void Steering_PositionAxisUpdate(")
fine=extract("static void Steering_PositionFineUpdate(")
update=extract("static void Steering_PositionUpdate(void)\n{")
capture=extract("static void Steering_R1RightFineCapture(")
finish=extract("static void Steering_PositionFinish(")
done=extract("static void Steering_PositionAxisDone(")
uart=extract("static void M8S11_StreamUpdate(void)\n{")
assert "#define STEER_R1_RIGHT_FINE_ENTRY_RAW 10U" in C
assert "#define STEER_R1_RIGHT_CENTER_NC_CLEARANCE_RAW 30U" in C
assert "steer_r1_right_late_fine_enable = 1U;" in C
assert "Steering_R1RightLateFineEntry(" in axis
assert axis.index("fine_enter = Steering_VoiceRightLateFineEntry(") < axis.index("fine_enter = Steering_R1RightLateFineEntry(") < axis.index("Steering_PositionFineUpdate(left, axis,")
assert "if ((steer_pos_command == 1U) || (steer_pos_command == 4U))" in axis
assert "Steering_PositionAtExpectedNc(left, limit_mask)" in axis
assert "Steering_PositionNcDirectionAllowed" in C
assert "STEER_POS_FINE_OFF_MIN_MS" in fine
assert "STEER_POS_FINE_MAX_PULSES" in fine
assert "STEER_POS_FINE_CAPTURE_MS" in fine
assert "STEER_POS_R1_TIMEOUT_MS   9000U" in C
assert "steer_pos_right_fine_pwm = 85U;" in C
assert "steer_pos_right_fine_pulse_ms = 12U;" in C
assert "steer_pos_right_center_tol_raw = 5U;" in C
assert "steer_r1_right_entry_used = STEER_POS_FINE_RIGHT_MIN_RAW;" in update
assert "steer_r1_right_reversals = 0U;" in update
assert "steer_r1_left_done_ms = HAL_GetTick();" in done
assert "steer_r1_right_done_ms = HAL_GetTick();" in done
assert "Steering_R1RightFineCapture(result);" in finish
assert finish.index("Steering_R1RightFineCapture(result);") < finish.index("steer_pos_active = 0U;")
assert "m8_r1_right_pending = 1U;" in capture
assert "d->source = (voice_pos_owned != 0U) ? 2U : 1U;" in capture
assert "d->entry_limit = steer_r1_right_entry_used;" in capture
assert "m8_s154_right_fine_pending != 0U" in uart
assert "m8_r1_right_pending != 0U" in uart
for key in ("M8R1_FINE,sha=%s","entryLimit=%u","pulses=%u","reversals=%u","rightFineMs=%lu","leftDoneMs=%lu","rightDoneMs=%lu"):
 assert key in uart
assert "Steering_PositionWheelsStopped() != 0U" in uart
assert "huart3.gState == HAL_UART_STATE_READY" in uart
assert "HAL_UART_Transmit(" not in uart
assert "M8R1_FINE," in PS
assert "verify_m8_r1_right_center_late_fine.py" in W
assert "capture_m8_r1_center.ps1" in W
cc=shutil.which("gcc") or shutil.which("clang") or shutil.which("cc")
if not cc:raise SystemExit("HOST compiler unavailable")
head=r"""
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define STEER_POS_LEFT_CENTER_RAW 1886U
#define STEER_POS_RIGHT_CENTER_RAW 662U
#define STEER_R1_RIGHT_FINE_ENTRY_RAW 10U
#define STEER_R1_RIGHT_CENTER_NC_CLEARANCE_RAW 30U
static uint16_t steer_cal_right_min_runtime=195U,steer_cal_right_max_runtime=1029U;
#define STEER_POS_RIGHT_MIN_RAW (steer_cal_right_min_runtime)
#define STEER_POS_RIGHT_MAX_RAW (steer_cal_right_max_runtime)
static uint8_t steer_r1_right_late_fine_enable=1U;
"""
tail=r"""
int main(void){
 // Physical default right center is 662; 195 < 662 < 1029.
 assert(Steering_R1RightLateFineEntry(0U,1U,15U)==10U);
 assert(Steering_R1RightLateFineEntry(1U,1U,15U)==15U);
 assert(Steering_R1RightLateFineEntry(0U,4U,15U)==15U);
 assert(Steering_R1RightLateFineEntry(0U,2U,15U)==15U);
 assert(Steering_R1RightLateFineEntry(0U,5U,15U)==15U);
 steer_r1_right_late_fine_enable=0U;
 assert(Steering_R1RightLateFineEntry(0U,1U,15U)==15U);
 steer_r1_right_late_fine_enable=1U;
 // An invalid physical min/max calibration forces unmodified threshold.
 steer_cal_right_min_runtime=650U;
 assert(Steering_R1RightLateFineEntry(0U,1U,15U)==15U);
 steer_cal_right_min_runtime=195U;
 steer_cal_right_max_runtime=700U;
 assert(Steering_R1RightLateFineEntry(0U,1U,15U)==15U);
 steer_cal_right_max_runtime=1029U;
 assert(Steering_R1RightLateFineEntry(0U,1U,15U)==10U);
 puts("PASS compiled actual C RIGHT CENTER 10RAW gating, geometry and legacy fallback");
 return 0;
}
"""
with TemporaryDirectory() as tmp:
 p=Path(tmp)/"main.c";p.write_text(head+"\n"+geo+"\n"+helper+"\n"+tail,encoding="utf-8")
 exe=Path(tmp)/"main"
 b=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(p),"-o",str(exe)],text=True,capture_output=True)
 if b.returncode:raise SystemExit("HOST compilation FAILED\n"+b.stderr)
 r=subprocess.run([str(exe)],text=True,capture_output=True)
 if r.returncode:raise SystemExit("HOST runtime FAILED\n"+r.stdout+r.stderr)
 print(r.stdout.strip())
print("PASS actual-C source wiring and safe-idle UART, NOT target/hardware PASS")
