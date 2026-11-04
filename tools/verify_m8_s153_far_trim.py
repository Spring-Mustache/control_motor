#!/usr/bin/env python3
"""S15.3 far trim: actual portable C host regression + source safety check.
This does NOT establish STM32 target build, physical coast/current or motor PASS.
"""
from pathlib import Path
import shutil
import subprocess
from tempfile import TemporaryDirectory

root=Path(__file__).resolve().parents[1]
src=(root/"Core/Src/main.c").read_text(encoding="utf8")
head=(root/"Core/Inc/steer_s153_far_trim.h").read_text(encoding="utf8")
b=src.index("    else if(steer_s153_trim_only_latched!=0U)")
e=src.index("    else if (steer_s142_latched != 0U)",b)
mode=src[b:e]
# Extract the exact C function, not all following helper functions.
# New S15.3 LEFT pacing intentionally writes LEFT, whereas this RIGHT
# controller remains LEFT-read-only; old text-marker slicing conflated them.
b=src.index("static void Steering_S153FarTrimObserve(")
open_brace=src.index("{",b)
depth=0
end=None
for i in range(open_brace,len(src)):
    if src[i]=="{":depth+=1
    elif src[i]=="}":
        depth-=1
        if depth==0:
            end=i+1
            break
if end is None:raise SystemExit("Cannot extract far trim C function")
pilot=src[b:end]
checks={
 "fe4c58f mode1 boot ON": "steer_s153_trim_only_enable=1U;" in src,
 "far phase proposal SHADOW ON": "steer_s153_far_shadow_enable=1U;" in src,
 "pilot actuation boot OFF": "steer_s153_far_trim_enable=0U;" in src,
 "new stroke only, inside mode1": 
     "((steer_s153_trim_only_latched==1U) &&" in src and
     "(steer_s153_far_trim_enable==1U))?1U:0U;" in src,
 "original RIGHT owner retained in baseline mode1":
     "uint16_t baseline_l=la,discarded_right=ra;" in mode and
     "ra=discarded_right" not in mode and
     "Steering_S153FarTrimObserve(" in mode and
     "if(steer_s153_far_trim_latched==1U)" in pilot and
     "*pwm_r=plan.right_pwm;" in pilot,
 "LEFT command untouched by pilot": "*pwm_l=" not in pilot,
 "no writes to CCR/GPIO or fresh I2C in pilot":
     all(t not in pilot for t in (
      "__HAL_TIM_SET_COMPARE","HAL_GPIO_WritePin",
      "HAL_I2C_","HAL_SPI_","Steering_PositionReadOne(")),
 "both shafts release far before terminal":
     "remain_l<=M8S153_FAR_RELEASE_RAW" in head and
     "remain_r<=M8S153_FAR_RELEASE_RAW" in head and
     "M8S153_FAR_RELEASE_RAW 420U" in head,
 "all existing STOP/NC/geometry/STALL guards intact":
     "primask = __get_PRIMASK();\n  __disable_irq();" in src and
     "steer_manual_right_rx_min_pwm = 50U;" in src and
     "steer_cal_right_min_runtime = 195U;" in src and
     "#define STEER_MANUAL_STALL_MS          650U" in src,
 "ETA separate, default OFF": "steer_s153_right_only_enable=0U;" in src,
 "dump gate and new mode1 companion telemetry":
     "M8S153_MODE,mode=%u" in src and
     "M8S153_FAR,shadowEnable=%u" in src and
     "(M8S153_Dump() == 0U)" in src
}
for name,yes in checks.items():
 print(("PASS" if yes else "FAIL")+": "+name)
if not all(checks.values()):raise SystemExit("FAIL S15.3 far-zone guard")
cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("Missing portable C compiler")
test_c=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "steer_s153_far_trim.h"
static M8S152Plan_t lead(int16_t delta,int16_t predicted,
                        int16_t initial,unsigned valid) {
 M8S152Plan_t p={0};
 p.angle_valid=1U;p.pred_valid=(uint8_t)valid;
 p.delta=delta;p.predicted=predicted;p.initial=initial;return p;
}
int main(void) {
 M8S152Plan_t o=lead(-350,-270,100,1);
 M8S153FarPlan_t p=M8S153_FarTrim(450,175,50,650,650,1,1,&o);
 assert(p.eligible==1&&p.trim_ccr==16&&p.right_pwm==159);
 p=M8S153_FarTrim(450,175,50,500,500,1,1,&o);
 assert(p.trim_ccr==10&&p.right_pwm==165);
 p=M8S153_FarTrim(450,175,50,428,490,1,1,&o);
 assert(p.trim_ccr==1&&p.right_pwm==174);
 p=M8S153_FarTrim(450,175,50,420,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 p=M8S153_FarTrim(450,175,50,800,180,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 p=M8S153_FarTrim(450,175,50,800,800,1,0,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 p=M8S153_FarTrim(450,175,50,800,800,0,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 p=M8S153_FarTrim(0,175,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 p=M8S153_FarTrim(450,0,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==0);
 p=M8S153_FarTrim(450,50,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==50);
 p=M8S153_FarTrim(450,175,50,800,800,1,1,0);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 o=lead(-350,-270,100,0);
 p=M8S153_FarTrim(450,175,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 o=lead(-350,-10,100,1);
 p=M8S153_FarTrim(450,175,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 o=lead(-100,-270,100,1);
 p=M8S153_FarTrim(450,175,50,800,800,1,1,&o);
 assert(p.trim_ccr==0&&p.right_pwm==175);
 o=lead(-32768,-32768,32767,1);
 for(unsigned rl=0;rl<=900;rl+=5)
  for(unsigned rr=0;rr<=900;rr+=5)
   for(unsigned duty=0;duty<=200;duty+=5) {
    p=M8S153_FarTrim(450,duty,50,(uint16_t)rl,(uint16_t)rr,1,1,&o);
    assert(p.right_pwm<=duty);
    assert(p.trim_ccr<=16);
    assert((uint16_t)(duty-p.right_pwm)==p.trim_ccr);
    if(duty<=50||rl<=420||rr<=420)assert(p.trim_ccr==0);
    else assert(p.right_pwm>=50);
   }
 puts("PASS real C kernel: <=16CCR, 420RAW release, invalid/NC veto and 1.3M property cases.");
 return 0;
}
"""
with TemporaryDirectory() as td:
 d=Path(td)
 (d/"test.c").write_text(test_c,encoding="utf8")
 compiled=subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror",
   "-I",str(root/"Core/Inc"),str(d/"test.c"),"-o",str(d/"test")],
   capture_output=True,text=True)
 if compiled.returncode:
  raise SystemExit(compiled.stdout+compiled.stderr)
 check=subprocess.run([str(d/"test")],capture_output=True,text=True)
 if check.returncode:raise SystemExit(check.stdout+check.stderr)
 print(check.stdout.strip())
print("Source/HOST PASS only, target Build/Flash and hardware trial not verified.")
