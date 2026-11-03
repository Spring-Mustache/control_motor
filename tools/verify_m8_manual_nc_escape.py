#!/usr/bin/env python3
"""Static + C HOST test of ACTUAL manual NC helper functions. Not hardware proof."""
from pathlib import Path
from tempfile import TemporaryDirectory
import re,shutil,subprocess
s=(Path(__file__).resolve().parents[1]/"Core/Src/main.c").read_text(encoding="utf-8")
def f(name):
 m=re.search(r"static\s+(?:uint8_t|void)\s+"+re.escape(name)+r"\s*\([^;]*?\)\s*\{",s,re.S)
 assert m,name
 d=0
 for i in range(m.end()-1,len(s)):
  if s[i]=="{":d+=1
  elif s[i]=="}":
   d-=1
   if d==0:return s[m.start():i+1]
 raise AssertionError("unbalanced")
h=f("Steering_ManualOwnEscapeIrq");r=f("Steering_ManualEscapeReleased")
m=f("Steering_ManualUpdate");a=f("Steering_ManualAxisUpdate")
assert "STEER_MANUAL_SETTLING" not in r and "Steering_ManualLeftOff" not in r and "Steering_ManualRightOff" not in r
assert "STEER_MANUAL_MOVING" in r and "STEER_MANUAL_ESCAPE_RELEASE_QUIET_MS" in r
assert m.count("Steering_ManualOwnEscapeIrq(1U,pending,left_direction)")==2
assert m.count("Steering_ManualOwnEscapeIrq(0U,pending,right_direction)")==2
assert "if(left_escape_irq_hold!=0U)la=0U;" in m and "if(right_escape_irq_hold!=0U)ra=0U;" in m
assert "Steering_ManualEscapeReleased(left, raw, now);" in a
assert "axis->escape_release_pending = 0U;" in a
assert "Steering_ManualClampPWM(pwm);" in a
assert "Steering_TestRawLimitMask()" in m and "__disable_irq()" in m and "steer_manual_neutral_required != 0U" in m
assert "STEER_MANUAL_STALL_MS          650U" in s and "STEER_POS_ESCAPE_RELEASE_QUIET_MS" in f("Steering_PositionEscapeReleased")
assert "Steering_ManualLeftOff()" in f("Steering_ManualOnLimitInterrupt") and "Steering_ManualRightOff()" in f("Steering_ManualOnLimitInterrupt")
print("PASS source directional NC, STOP/atomic/stall, R1 unchanged")
cc=shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
assert cc,"Host compiler required"
pre=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_LIMIT_L_MIN_MASK 4U
#define STEER_LIMIT_L_MAX_MASK 8U
#define STEER_LIMIT_R_MIN_MASK 2U
#define STEER_LIMIT_R_MAX_MASK 1U
#define STEER_MANUAL_ESCAPE 2U
#define STEER_MANUAL_MOVING 1U
#define STEER_MANUAL_ESCAPE_RELEASE_QUIET_MS 120U
typedef struct {
 uint16_t previous_raw,best_distance;
 uint32_t progress_ms,escape_start_ms,escape_release_since_ms;
 uint8_t escape_release_pending;
 uint32_t settle_begin_ms,settle_quiet_since_ms;
 uint16_t settle_quiet_raw;
 uint32_t speed_sample_ms;uint16_t speed_filtered_raw_s;uint8_t speed_valid,fast_ready;
 uint32_t fast_reference_sample_ms;uint16_t phase_start_raw;uint8_t phase_start_valid;
} SteeringManualAxis_t;
static uint8_t steer_manual_active=1U,steer_manual_left_state=2U,steer_manual_right_state=2U;
static SteeringManualAxis_t steer_manual_left_axis={0},steer_manual_right_axis={0};
static uint32_t steer_pos_left_last_sample_ms=50U,steer_pos_right_last_sample_ms=70U;
static uint32_t offL=0U,offR=0U;
static void Steering_ManualLeftOff(void){offL++;}
static void Steering_ManualRightOff(void){offR++;}
'''
test=r'''
int main(void){
 Steering_ManualEscapeReleased(0U,200U,500U);
 Steering_ManualEscapeReleased(0U,201U,619U);
 assert(steer_manual_right_state==STEER_MANUAL_ESCAPE);
 Steering_ManualEscapeReleased(0U,205U,620U);
 assert(steer_manual_right_state==STEER_MANUAL_MOVING&&offR==0U);
 assert(steer_manual_right_axis.phase_start_valid==1U);
 Steering_ManualEscapeReleased(1U,1400U,800U);
 Steering_ManualEscapeReleased(1U,1390U,920U);
 assert(steer_manual_left_state==STEER_MANUAL_MOVING&&offL==0U);
 steer_manual_right_state=STEER_MANUAL_ESCAPE;
 steer_manual_right_axis.escape_release_pending=1U;
 assert(Steering_ManualOwnEscapeIrq(0U,STEER_LIMIT_R_MAX_MASK,-1)==1U);
 assert(offR==1U&&offL==0U&&steer_manual_right_axis.escape_release_pending==0U);
 assert(Steering_ManualOwnEscapeIrq(0U,STEER_LIMIT_R_MIN_MASK,-1)==0U);
 assert(Steering_ManualOwnEscapeIrq(0U,STEER_LIMIT_R_MAX_MASK|STEER_LIMIT_R_MIN_MASK,-1)==0U);
 steer_manual_left_state=STEER_MANUAL_ESCAPE;
 assert(Steering_ManualOwnEscapeIrq(1U,STEER_LIMIT_L_MIN_MASK,1)==1U);
 assert(offL==1U);
 assert(Steering_ManualOwnEscapeIrq(1U,STEER_LIMIT_L_MAX_MASK,1)==0U);
 steer_manual_right_state=STEER_MANUAL_ESCAPE;
 Steering_ManualEscapeReleased(0U,960U,1000U);
 assert(Steering_ManualOwnEscapeIrq(0U,STEER_LIMIT_R_MAX_MASK,-1)==1U);
 Steering_ManualEscapeReleased(0U,956U,1090U);
 Steering_ManualEscapeReleased(0U,950U,1209U);
 assert(steer_manual_right_state==STEER_MANUAL_ESCAPE);
 Steering_ManualEscapeReleased(0U,948U,1210U);
 assert(steer_manual_right_state==STEER_MANUAL_MOVING);
 steer_manual_right_state=STEER_MANUAL_ESCAPE;steer_manual_active=0U;
 assert(Steering_ManualOwnEscapeIrq(0U,STEER_LIMIT_R_MAX_MASK,-1)==0U);
 puts("PASS C HOST both axes, contact bounce, origin-only, one-held-command release");
 return 0;
}
'''
with TemporaryDirectory() as d:
 p=Path(d)/"nc.c";exe=Path(d)/"nc"
 p.write_text(pre+"\n"+h+"\n"+r+"\n"+test)
 subprocess.run([cc,"-std=c99","-Wall","-Wextra","-Werror",str(p),"-o",str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print("HOST only: no CubeIDE, flash or powered hardware acceptance")
