#!/usr/bin/env python3
"""Compiled production S15.6 helper and integration. HOST-only."""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess, shutil
R=Path(__file__).resolve().parents[1]
C=(R/"Core/Src/main.c").read_text(encoding="utf-8")
W=(R/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
def fn(sig):
 a=C.index(sig);b=C.index("{",a);d=0
 for k in range(b,len(C)):
  if C[k]=="{":d+=1
  elif C[k]=="}":
   d-=1
   if d==0:return C[a:k+1]
 raise AssertionError(sig)
h=fn("static uint16_t Steering_VoiceS156StartupCap(")
sync=fn("static void Steering_VoiceS154Sync(")
pos=fn("static void Steering_PositionUpdate(void)\n{")
cap=fn("static void Steering_VoiceRightFineCapture(")
uart=fn("static void M8S11_StreamUpdate(void)\n{")
assert "#define STEER_VOICE_S156_START_CAP_MS 350U" in C
for x in ("steer_voice_s156_start_cap_enable = 1U;",
"steer_voice_s155_left15_taper_enable = 1U;",
"steer_voice_s154_right_late_fine_enable = 1U;",
"steer_voice_s154_right_fall_late_fine_enable = 1U;",
"steer_voice_s154_enable = 1U;","steer_angle_wp_enable = 1U;"):
 assert x in C
assert sync.index("Steering_AngleWaypointApply(mode,")<sync.index("Steering_VoiceS156StartupCap(mode,")<sync.index("steer_voice_s154_pwm_r = *pwm_r;")
assert "Steering_VoiceS154Sync(HAL_GetTick(), raw_mask, &left_cmd, &right_cmd);" in pos
assert pos.index("Steering_PositionAxisUpdate(0U")<pos.index("Steering_VoiceS154Sync(")<pos.index("late_pending = steer_pos_irq_pending_mask;")
assert "steer_voice_s156_start_cap_frames = 0U;" in pos
assert "d->s156_cap_frames = steer_voice_s156_start_cap_frames;" in cap
assert "startCapFrames=%lu" in uart
assert "Steering_PositionWheelsStopped() != 0U" in uart
assert "huart3.gState == HAL_UART_STATE_READY" in uart
assert "HAL_UART_Transmit(" not in uart
assert "verify_m8_s156_start_cap.py" in W
cc=shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
if not cc:raise SystemExit("no HOST C compiler")
head=r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_VOICE_S156_START_CAP_MS 350U
static uint8_t steer_voice_s156_start_cap_enable=1U;
static uint8_t voice_pos_owned=1U;
static uint8_t voice_pos_last_angle_deg=15U;
static int8_t voice_pos_last_turn=-1;
"""
tail=r"""
int main(void) {
 assert(Steering_VoiceS156StartupCap(4,0,0,0,100,50)==50);
 assert(Steering_VoiceS156StartupCap(4,245,10,211,100,50)==50);
 assert(Steering_VoiceS156StartupCap(4,350,50,60,95,50)==50);
 assert(Steering_VoiceS156StartupCap(4,351,50,60,95,50)==95);
 assert(Steering_VoiceS156StartupCap(4,245,211,10,100,50)==100);
 for(unsigned a=0;a<=1000;a+=25)
 for(unsigned b=0;b<=1000;b+=25)
 for(unsigned pwm=50;pwm<=100;pwm+=5){
  unsigned v=Steering_VoiceS156StartupCap(4,200,a,b,pwm,50);
  assert(v>=50 && v<=pwm);
  assert(v==(b>=a?50:pwm));
 }
 voice_pos_last_angle_deg=30;
 assert(Steering_VoiceS156StartupCap(4,245,0,100,100,50)==100);
 voice_pos_last_angle_deg=15;voice_pos_last_turn=1;
 assert(Steering_VoiceS156StartupCap(4,245,0,100,100,50)==100);
 voice_pos_last_turn=-1;
 for(unsigned m=0;m<6;m++)if(m!=4)
  assert(Steering_VoiceS156StartupCap(m,245,0,100,100,50)==100);
 voice_pos_owned=0;
 assert(Steering_VoiceS156StartupCap(4,245,0,100,100,50)==100);
 voice_pos_owned=1;steer_voice_s156_start_cap_enable=0;
 assert(Steering_VoiceS156StartupCap(4,245,0,100,100,50)==100);
 puts("PASS actual C S15.6 startup cap, bounded floor and exclusions");
 return 0;
}
"""
with TemporaryDirectory() as tmp:
 p=Path(tmp)/"v.c";p.write_text(head+h+"\n"+tail,encoding="utf-8")
 ex=Path(tmp)/"v"
 b=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(p),"-o",str(ex)],capture_output=True,text=True)
 if b.returncode:raise SystemExit("C compile FAIL\n"+b.stderr)
 v=subprocess.run([str(ex)],capture_output=True,text=True)
 if v.returncode:raise SystemExit("C runtime FAIL\n"+v.stdout+v.stderr)
 print(v.stdout.strip())
print("PASS static NC/late IRQ/serial guards, not CubeIDE nor powered motor")
