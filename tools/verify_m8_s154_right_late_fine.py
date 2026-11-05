#!/usr/bin/env python3
"""M8-S15.4 RIGHT Voice4 RAW-increase fine handoff — actual-C HOST tests.
No CubeIDE target compile, physical coast, NC or motor-speed validation.
"""
from pathlib import Path
import subprocess,tempfile,shutil
R=Path(__file__).resolve().parents[1]
S=(R/"Core/Src/main.c").read_text(encoding="utf-8")
W=(R/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
def extract(signature):
 start=S.index(signature);b=S.index("{",start);depth=0
 for i in range(b,len(S)):
  if S[i]=="{":depth+=1
  elif S[i]=="}":
   depth-=1
   if depth==0:return S[start:i+1]
 raise AssertionError(signature)
helper=extract("static uint16_t Steering_VoiceRightLateFineEntry(")
axis=extract("static void Steering_PositionAxisUpdate(")
position=extract("static void Steering_PositionUpdate(")
fine=extract("static void Steering_PositionFineUpdate(")
sync=extract("static void Steering_VoiceS154Sync(")
finish=extract("static void Steering_PositionFinish(")
cap=extract("static void Steering_VoiceRightFineCapture(")
uart=extract("static void M8S11_StreamUpdate(void)\n{")
assert "steer_voice_s154_right_late_fine_enable = 1U;" in S
assert "steer_voice_s154_right_fine_diag_enable = 1U;" in S
assert "steer_voice_s154_enable = 1U;" in S
assert "voice_pos_enable = 1U;" in S
assert "steer_angle_wp_enable = 1U;" in S
assert "#define STEER_VOICE_S154_RIGHT_RISE_FINE_ENTRY_RAW 10U" in S
assert "#define STEER_VOICE_S154_RIGHT_MAX_CLEARANCE_RAW 30U" in S
assert "Steering_VoiceRightLateFineEntry(left, err, fine_enter);" in axis
assert "(axis->fine_phase == STEER_POS_FINE_PHASE_IDLE) &&" in axis
assert "(abs_error <= fine_enter))" in axis
assert axis.index("steer_voice_s154_right_fine_entry_used = fine_enter;") < axis.index(
 "Steering_PositionFineUpdate(left, axis, raw, err, step, now,")
assert axis.index("Steering_VoiceRightLateFineEntry(left, err, fine_enter);") < axis.index(
 "Steering_PositionFineUpdate(left, axis, raw, err, step, now,")
assert "if ((steer_pos_command == 1U) || (steer_pos_command == 4U))" in axis
assert "Steering_PositionAtExpectedNc(left, limit_mask)" in axis
assert "Steering_PositionNcDirectionAllowed" in S
assert "STEER_POS_FINE_OFF_MIN_MS" in fine
assert "STEER_POS_FINE_MAX_PULSES" in fine
assert "STEER_POS_FINE_CAPTURE_MS" in fine
assert "Steering_VoiceS154Sync(HAL_GetTick(), raw_mask, &left_cmd, &right_cmd);" in position
assert position.index("Steering_PositionAxisUpdate(0U") < position.index(
 "Steering_VoiceS154Sync(") < position.index("late_pending = steer_pos_irq_pending_mask;")
assert "steer_voice_s154_run_sync_frames = 0U;" in position
assert "steer_voice_s154_run_sync_frames++;" in sync
assert "Steering_VoiceRightFineCapture(result);" in finish
for f in ("d->left_done_ms", "d->right_done_ms", "d->right_fine_ms",
          "d->entry_limit", "d->sync_frames"):
 assert f in cap
for f in ("entryLimit=%u", "syncFrames=%lu","leftDoneMs=%lu",
          "rightDoneMs=%lu","rightFineMs=%lu"):
 assert f in uart
assert "HAL_UART_Transmit(" not in uart
assert "Steering_PositionWheelsStopped() != 0U" in uart
assert "huart3.gState == HAL_UART_STATE_READY" in uart
assert "verify_m8_s154_right_late_fine.py" in W
cc=shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
if not cc:raise SystemExit("No compiler available")
top=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_VOICE_S154_RIGHT_RISE_FINE_ENTRY_RAW 10U
#define STEER_VOICE_S154_RIGHT_MAX_CLEARANCE_RAW 30U
static uint8_t steer_voice_s154_right_late_fine_enable=1U;
static uint8_t voice_pos_owned=1U;
static uint8_t steer_pos_command=4U;
static uint16_t steer_cal_right_max_runtime=1029U;
static uint16_t voice_pos_goal_r=988U;
#define STEER_POS_RIGHT_MAX_RAW (steer_cal_right_max_runtime)
'''
tail=r'''
int main(void) {
 // actual trial angle40 RIGHT RAW rise: MAX1029, goal988, 41 RAW NC clearance.
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==10U);
 assert(Steering_VoiceRightLateFineEntry(0U,5,15U)==10U);
 // LEFT, RIGHT RAW decreasing, R1, Voice45, disabled trial all unchanged.
 assert(Steering_VoiceRightLateFineEntry(1U,14,15U)==15U);
 assert(Steering_VoiceRightLateFineEntry(0U,-14,15U)==15U);
 steer_pos_command=1U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==15U);
 steer_pos_command=5U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==15U);
 steer_pos_command=4U; voice_pos_owned=0U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==15U);
 voice_pos_owned=1U;
 steer_voice_s154_right_late_fine_enable=0U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==15U);
 steer_voice_s154_right_late_fine_enable=1U;
 // More than 30 RAW clearance guard: <=29 falls back to 15.
 voice_pos_goal_r=1000U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==15U);
 voice_pos_goal_r=999U;
 assert(Steering_VoiceRightLateFineEntry(0U,14,15U)==10U);
 puts("PASS actual C Voice4 RIGHT RAW-rise 10-RAW handoff; fallback guards");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/"trial.c";p.write_text(top+helper+"\n"+tail,encoding="utf-8")
 exe=Path(t)/"trial"
 run=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(p),"-o",str(exe)],capture_output=True,text=True)
 if run.returncode:raise SystemExit("GCC compile failed\n"+run.stdout+run.stderr)
 run=subprocess.run([str(exe)],capture_output=True,text=True)
 if run.returncode:raise SystemExit("HOST C runtime failed\n"+run.stdout+run.stderr)
 print(run.stdout.strip())
print("PASS unchanged S15.4 coordinator, active NC/late IRQ guards and passive timing diagnostics (static)")
