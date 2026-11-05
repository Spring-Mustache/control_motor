#!/usr/bin/env python3
"""Verify production NC-selection/RAW-bound C and static ISR/servo integration.
HOST ONLY: NOT a CubeIDE target build, real NC, PWM, or physical safety PASS.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
ROUTE=(ROOT/"Core/Inc/voice_s154_route.h").read_text(encoding="utf-8")

def body(sig):
    p=SRC.index(sig)
    a=SRC.index("{",p)
    n=0
    for i in range(a,len(SRC)):
        if SRC[i]=="{":n+=1
        elif SRC[i]=="}":
            n-=1
            if n==0:return SRC[p:i+1]
    raise RuntimeError("Missing function end: "+sig)

select=body("static uint8_t Steering_PositionExpectedNc(")
arrived=body("static uint8_t Steering_PositionAtExpectedNc(")
guard=body("static uint8_t Steering_Voice45RawBoundaryReached(")
axis=body("static void Steering_PositionAxisUpdate(")
update=body("static void Steering_PositionUpdate(")
expected_mask=body("static uint8_t Steering_SafetyExpectedNcMask(void)\n{")
stage=body("static void Voice_S154StageSteerAngle(")
# Static wiring of REAL control paths. A pure helper test is not sufficient.
assert "STEER_POS_VOICE45_NC 5U" in SRC
assert "STEER_POS_LEFT_MAX_RAW,STEER_POS_RIGHT_MIN_RAW" in stage
assert "STEER_POS_LEFT_MIN_RAW,STEER_POS_RIGHT_MAX_RAW" in stage
assert "((angle==45U)?STEER_POS_VOICE45_NC:4U)" in stage
assert "Steering_PositionAtExpectedNc(left, limit_mask)" in axis
assert "Steering_Voice45RawBoundaryReached(left, raw, goal)" in axis
assert "Steering_PositionAxisFault(left, 8U)" in axis
assert "Steering_PositionFineUpdate(left, axis, raw, err" in axis
assert "if ((steer_pos_command == 1U) || (steer_pos_command == 4U))" in axis
assert "Steering_PositionAtExpectedNc(1U, pending)" in update
assert "Steering_PositionAtExpectedNc(0U, pending)" in update
assert "Steering_PositionAtExpectedNc(1U, late_pending)" in update
assert "Steering_PositionAtExpectedNc(0U, late_pending)" in update
assert "Steering_PositionAtExpectedNc(1U, raw_mask)" in update
assert "Steering_PositionAtExpectedNc(0U, raw_mask)" in update
assert "steer_pos_left_result != 3U" in update
assert "steer_pos_right_result != 3U" in update
assert "buttons!=STEER_POS_VOICE45_NC" in update
assert "Steering_PositionWheelsStopped()!=0U" in update
assert "Steering_PositionExpectedNc(1U)" in expected_mask
assert "Steering_PositionExpectedNc(0U)" in expected_mask
assert "(steer_pos_command != STEER_POS_VOICE45_NC)" in update
assert "Steering_AngleWaypointApply(steer_pos_command" in update
assert "Steering_PhaseLimit(steer_pos_command" in update
assert "Steering_PositionOnLimitInterrupt" in SRC
assert "Steering_PositionNcDirectionAllowed" in update
assert "steer_safety_trip_latched==0U" in update
assert "STEER_POS_TIMEOUT_MS" in update
assert "steer_pos_irq_pending_mask = 0U;" in update

stub=r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_LIMIT_L_MIN_MASK 0x04U
#define STEER_LIMIT_R_MIN_MASK 0x02U
#define STEER_LIMIT_L_MAX_MASK 0x08U
#define STEER_LIMIT_R_MAX_MASK 0x01U
#define STEER_POS_VOICE45_NC 5U
static volatile uint8_t steer_pos_command=0,voice_pos_owned=0;
static volatile uint8_t voice_pos_last_angle_deg=0;
static volatile int8_t voice_pos_last_turn=0;
"""
checks=r"""
static void turn45(int dir,uint8_t left_goal,uint8_t right_goal,
                  uint16_t lraw,uint16_t rraw){
 steer_pos_command=STEER_POS_VOICE45_NC;
 voice_pos_owned=1;
 voice_pos_last_angle_deg=45U;
 voice_pos_last_turn=(int8_t)dir;
 assert(Steering_PositionExpectedNc(1U)==left_goal);
 assert(Steering_PositionExpectedNc(0U)==right_goal);
 assert(Steering_PositionAtExpectedNc(1U,left_goal));
 assert(Steering_PositionAtExpectedNc(0U,right_goal));
 assert(!Steering_PositionAtExpectedNc(1U,(uint8_t)(
    left_goal==STEER_LIMIT_L_MAX_MASK?STEER_LIMIT_L_MIN_MASK:STEER_LIMIT_L_MAX_MASK)));
 assert(!Steering_PositionAtExpectedNc(0U,(uint8_t)(
    right_goal==STEER_LIMIT_R_MIN_MASK?STEER_LIMIT_R_MAX_MASK:STEER_LIMIT_R_MIN_MASK)));
 assert(!Steering_PositionAtExpectedNc(1U,
    (uint8_t)(STEER_LIMIT_L_MAX_MASK|STEER_LIMIT_L_MIN_MASK)));
 assert(!Steering_PositionAtExpectedNc(0U,
    (uint8_t)(STEER_LIMIT_R_MAX_MASK|STEER_LIMIT_R_MIN_MASK)));
 assert(!Steering_PositionAtExpectedNc(1U,0));
 assert(!Steering_PositionAtExpectedNc(0U,0));
 /* Reaching a calibrated encoder end without its NC MUST fail closed. */
 assert(Steering_Voice45RawBoundaryReached(1U,lraw,lraw));
 assert(Steering_Voice45RawBoundaryReached(0U,rraw,rraw));
 if(dir<0) {
  assert(!Steering_Voice45RawBoundaryReached(1U,lraw+1U,lraw));
  assert(!Steering_Voice45RawBoundaryReached(0U,rraw+1U,rraw));
 }else {
  assert(!Steering_Voice45RawBoundaryReached(1U,lraw-1U,lraw));
  assert(!Steering_Voice45RawBoundaryReached(0U,rraw-1U,rraw));
 }
}
int main(void){
 turn45(-1,STEER_LIMIT_L_MAX_MASK,STEER_LIMIT_R_MIN_MASK,1374,195);
 turn45(+1,STEER_LIMIT_L_MIN_MASK,STEER_LIMIT_R_MAX_MASK,2297,1029);
 /* A stale 45 debug variable MUST NOT make ordinary 5..40 touch NC. */
 steer_pos_command=4U;
 assert(Steering_PositionExpectedNc(1U)==0);
 assert(Steering_PositionExpectedNc(0U)==0);
 assert(!Steering_PositionAtExpectedNc(1U,STEER_LIMIT_L_MAX_MASK));
 assert(!Steering_PositionAtExpectedNc(0U,STEER_LIMIT_R_MIN_MASK));
 /* R2 still accepts exactly MIN NC, never MAX. */
 steer_pos_command=2U;voice_pos_owned=0U;
 assert(Steering_PositionAtExpectedNc(1U,STEER_LIMIT_L_MIN_MASK));
 assert(Steering_PositionAtExpectedNc(0U,STEER_LIMIT_R_MIN_MASK));
 assert(!Steering_PositionAtExpectedNc(1U,STEER_LIMIT_L_MAX_MASK));
 assert(!Steering_PositionAtExpectedNc(0U,STEER_LIMIT_R_MAX_MASK));
 /* Non-Voice mode 5 and malformed/stale 45 cannot declare arrival. */
 steer_pos_command=5U;
 voice_pos_owned=0U;
 assert(!Steering_PositionAtExpectedNc(1U,STEER_LIMIT_L_MIN_MASK));
 voice_pos_owned=1U;voice_pos_last_angle_deg=30U;
 assert(!Steering_PositionAtExpectedNc(0U,STEER_LIMIT_R_MAX_MASK));
 voice_pos_last_angle_deg=45U;voice_pos_last_turn=0;
 assert(!Steering_PositionAtExpectedNc(0U,STEER_LIMIT_R_MAX_MASK));
 puts("PASS actual NC C: left/right 45 expected switches, wrong/dual NC, independent R2, RAW boundary fail-closed");
 return 0;
}
"""
cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("Missing HOST gcc/cc")
with TemporaryDirectory() as d:
    p=Path(d)/"test.c"
    p.write_text(stub+"\n"+select+"\n"+arrived+"\n"+guard+"\n"+checks,encoding="utf-8")
    exe=Path(d)/"test"
    built=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",
                          str(p),"-o",str(exe)],text=True,capture_output=True)
    if built.returncode:raise SystemExit("HOST COMPILE FAIL:\n"+built.stdout+built.stderr)
    run=subprocess.run([str(exe)],text=True,capture_output=True)
    if run.returncode:raise SystemExit("HOST RUN FAIL:\n"+run.stdout+run.stderr)
    print(run.stdout.strip())
print("PASS source wiring: all NC event/atomic/initial checks + no RAW-only 45 success; HOST simulation only.")
