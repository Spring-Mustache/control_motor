#!/usr/bin/env python3
"""S15.3 ETA LEFT-pace: static integration/safety + actual C kernel regression.
Does NOT prove CubeIDE target build, coast/current/cutoff or <=2% finishes.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil,subprocess

root=Path(__file__).resolve().parents[1]
src=(root/"Core/Src/main.c").read_text(encoding="utf8")
hd=(root/"Core/Inc/steer_s153_left_pace.h").read_text(encoding="utf8")
a=src.index("static void Steering_S153LeftPaceObserve(")
b=src.index("static void Steering_ManualUpdate(void)",a)
observe=src[a:b]
c=src.index("    else if(steer_s153_trim_only_latched!=0U)")
d=src.index("    else if (steer_s142_latched != 0U)",c)
mode=src[c:d]
checks={
 "original fe4c58f mode1 boot ON, ETA RIGHT OFF":
  "steer_s153_trim_only_enable=1U;" in src and
  "steer_s153_right_only_enable=0U;" in src,
 "new LEFT ETA shadow ON and actuation OFF at boot":
  "steer_s153_left_pace_shadow_enable=1U;" in src and
  "steer_s153_left_pace_enable=0U;" in src,
 "only a new right-RMIN mode1 stroke can opt into LEFT pacing":
  "((steer_s153_left_pace_enable==1U) &&" in src and
  "(steer_s153_trim_only_latched==1U)" in src and
  "(steer_s153_far_trim_latched==0U))?1U:0U;" in src,
 "no double RIGHT pilot, no pilot S15 common T or PI":
  "(steer_s153_far_trim_latched==0U)" in src and
  "(steer_s153_latched==0U) &&" in src and
  "(steer_s15_latched==0U) &&" in src and
  "(steer_s147_latched==0U) &&" in src,
 "no actual LEFT change without pilot latch":
  "if(steer_s153_left_pace_latched==1U)" in observe and
  "*pwm_l=plan.left_pwm;" in observe,
 "no RIGHT change/CCR/GPIO/I2C/SPI in new observer":
  all(x not in observe for x in (
   "*pwm_r=","__HAL_TIM_SET_COMPARE","HAL_GPIO_WritePin",
   "HAL_I2C_","HAL_SPI_","Steering_PositionReadOne(")),
 "valid BOTH sensors, both MOVING, NC0 and STOP veto":
  "Steering_TestRawLimitMask()==0U" in observe and
  "steer_manual_left_state==STEER_MANUAL_MOVING" in observe and
  "steer_manual_right_state==STEER_MANUAL_MOVING" in observe and
  "steer_safety_trip_latched==0U" in observe and
  "control_stop_latched==0U" in observe and
  "M8S152_Observe(" in observe,
 "only reduce LEFT outside both 320RAW, up to72CCR and >=owner floor":
  "M8S153_PACE_RELEASE_RAW           320U" in hd and
  "M8S153_PACE_MAX_REDUCTION_CCR      72U" in hd and
  "remain_l<=M8S153_PACE_RELEASE_RAW" in hd and
  "remain_r<=M8S153_PACE_RELEASE_RAW" in hd and
  "room=(uint16_t)(owner_l-floor_l);" in hd and
  "p.left_pwm=(uint16_t)(owner_l-p.reduction_ccr);" in hd,
 "last stage remains f2137556 original RIGHT owner":
  "uint16_t baseline_l=la,discarded_right=ra;" in mode and
  "ra=discarded_right" not in mode and
  "Steering_S153FarTrimObserve(" in mode,
 "original independent guards and 650ms STALL unchanged":
  "primask = __get_PRIMASK();\n  __disable_irq();" in src and
  "steer_cal_right_min_runtime = 195U;" in src and
  "steer_manual_right_rx_min_pwm = 50U;" in src and
  "#define STEER_MANUAL_STALL_MS          650U" in src and
  "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ra);" in src,
 "fully disarmed UART has separate PACE and old mode1 labels":
  "M8S153_PACE,shadowEnable=%u" in src and
  "M8S153_MODE,mode=%u" in src and
  "(M8S153_Dump() == 0U)" in src,
}
for name,good in checks.items():print(("PASS" if good else "FAIL")+": "+name)
if not all(checks.values()):raise SystemExit("FAIL source integration/safety checks")

cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("Missing host C compiler")
test=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "steer_s153_left_pace.h"
static M8S152Plan_t speed(int16_t l,int16_t r,int valid) {
 M8S152Plan_t p={0};
 p.angle_valid=1U;p.pred_valid=(uint8_t)valid;
 p.velL=l;p.velR=r;
 return p;
}
int main(void) {
 M8S152Plan_t s=speed(1455,1672,1);
 M8S153PacePlan_t p=M8S153_LeftPacePlan(
   450,175,300,617,536,779,761,1,1,1,&s);
 assert(p.eligible==1&&p.eta_valid==1);
 assert(p.forecast_right_late_ms>=140&&p.forecast_right_late_ms<=220);
 assert(p.reduction_ccr>0&&p.reduction_ccr<=72&&p.left_pwm<450);
 s=speed(1533,1424,1);
 p=M8S153_LeftPacePlan(450,175,300,469,390,779,761,1,1,1,&s);
 assert(p.eligible==1&&p.reduction_ccr<=35&&p.reduction_ccr>0);
 p=M8S153_LeftPacePlan(450,175,300,469,320,779,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&p.left_pwm==450);
 s=speed(-1322,-1433,1);
 p=M8S153_LeftPacePlan(450,175,300,507,475,789,755,-1,1,1,&s);
 assert(p.eligible==1&&p.reduction_ccr==0);
 s=speed(1455,1672,1);
 p=M8S153_LeftPacePlan(450,175,300,617,536,779,761,1,1,0,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 p=M8S153_LeftPacePlan(450,175,300,617,536,779,761,1,0,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 p=M8S153_LeftPacePlan(450,175,300,617,536,779,761,0,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 p=M8S153_LeftPacePlan(450,175,300,617,536,600,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 p=M8S153_LeftPacePlan(300,175,300,617,536,779,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 p=M8S153_LeftPacePlan(450,0,300,617,536,779,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 s=speed(-1455,-1672,1);
 p=M8S153_LeftPacePlan(450,175,300,617,536,779,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 s=speed(1455,1672,0);
 p=M8S153_LeftPacePlan(450,175,300,617,536,779,761,1,1,1,&s);
 assert(p.reduction_ccr==0&&!p.eligible);
 s=speed(1000,2000,1);
 for(unsigned rl=0;rl<=1000;rl+=10)
  for(unsigned rr=0;rr<=1000;rr+=10)
   for(unsigned duty=0;duty<=450;duty+=10) {
    p=M8S153_LeftPacePlan((uint16_t)duty,175,300,
       (uint16_t)rl,(uint16_t)rr,779,761,1,1,1,&s);
    assert(p.left_pwm<=duty);
    assert(p.reduction_ccr<=72);
    assert((uint16_t)(duty-p.left_pwm)==p.reduction_ccr);
    if(duty<=300||rl<=320||rr<=320)assert(p.reduction_ccr==0);
    else assert(p.left_pwm>=300);
   }
 puts("PASS actual C: ETA cases from log, 320RAW release, 72CCR limit, >400K bounded cases.");
}
"""
with TemporaryDirectory() as td:
 d=Path(td);(d/"test.c").write_text(test,encoding="utf8")
 cmd=[cc,"-std=c11","-Wall","-Wextra","-Werror",
  "-I",str(root/"Core/Inc"),str(d/"test.c"),"-o",str(d/"test")]
 build=subprocess.run(cmd,capture_output=True,text=True)
 if build.returncode:raise SystemExit(build.stdout+build.stderr)
 run=subprocess.run([str(d/"test")],capture_output=True,text=True)
 if run.returncode:raise SystemExit(run.stdout+run.stderr)
 print(run.stdout.strip())
print("SOURCE+HOST only; physical % skew change NOT established.")
