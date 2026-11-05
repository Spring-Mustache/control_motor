#!/usr/bin/env python3
"""S15.5: actual production C helper, unchanged fallback arithmetic and guards.

A HOST unit/regression check, NOT a simulation of chassis dynamics and
NOT proof that physical peakGap will be <= 100permille.
"""
from pathlib import Path
import tempfile,subprocess,shutil
R=Path(__file__).resolve().parents[1]
source=(R/"Core/Src/main.c").read_text(encoding="utf-8")
workflow=(R/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
def extract(sig):
 start=source.index(sig);begin=source.index("{",start);depth=0
 for i in range(begin,len(source)):
  if source[i]=="{":depth+=1
  elif source[i]=="}":
   depth-=1
   if depth==0:return source[start:i+1]
 raise AssertionError(sig)
active=extract("static uint8_t Steering_VoiceS155Left15Active(")
taper=extract("static uint16_t Steering_VoiceS155LeaderTaper(")
wp=extract("static void Steering_AngleWaypointApply(")
sync=extract("static void Steering_VoiceS154Sync(")
pos=extract("static void Steering_PositionUpdate(void)\n{")
finish=extract("static void Steering_PositionFinish(")
uart=extract("static void M8S11_StreamUpdate(void)\n{")
cap=extract("static void Steering_VoiceRightFineCapture(")
assert "#define STEER_VOICE_S155_LEFT15_TAPER_RAMP_PERMILLE 70U" in source
assert "steer_voice_s155_left15_taper_enable = 1U;" in source
for t in ("steer_voice_s154_enable = 1U;","steer_angle_wp_enable = 1U;",
          "steer_voice_s154_right_late_fine_enable = 1U;",
          "steer_voice_s154_right_fall_late_fine_enable = 1U;",
          "steer_voice_s154_right_fine_diag_enable = 1U;"):
 assert t in source
assert "Steering_VoiceS155Left15Active(mode)" in wp
assert wp.count("Steering_VoiceS155LeaderTaper(")==2
assert "steer_voice_s155_left15_taper_frames++" in wp
assert "Steering_VoiceS154Sync(HAL_GetTick(), raw_mask, &left_cmd, &right_cmd);" in pos
assert pos.index("Steering_PositionAxisUpdate(0U") < pos.index("Steering_VoiceS154Sync(") < pos.index("late_pending = steer_pos_irq_pending_mask;")
for t in ("Steering_PositionNcDirectionAllowed", "Steering_PositionFineUpdate", "Steering_Voice45RawBoundaryReached"):
 assert t in source
assert "Steering_VoiceRightFineCapture(result);" in finish
assert "steer_voice_s155_left15_taper_frames = 0U;" in pos
assert "steer_voice_s155_peak_leader = 0U;" in pos
assert "steer_voice_s155_peak_at_ms = 0U;" in pos
assert "d->peak_leader = steer_voice_s155_peak_leader;" in cap
assert "d->peak_at_ms = steer_voice_s155_peak_at_ms;" in cap
assert "d->s155_taper_frames = steer_voice_s155_left15_taper_frames;" in cap
for x in ("peakLeader=%u","peakAtMs=%lu","earlyTaperFrames=%lu"):
 assert x in uart
assert "Steering_PositionWheelsStopped() != 0U" in uart
assert "huart3.gState == HAL_UART_STATE_READY" in uart
assert "HAL_UART_Transmit(" not in uart
assert "verify_m8_s155_voice15_left_taper.py" in workflow
assert "steer_voice_s154_peak_gap_permille = gap;" in sync
assert "steer_voice_s155_peak_leader" in sync and "steer_voice_s155_peak_at_ms" in sync
cc=shutil.which("gcc") or shutil.which("clang") or shutil.which("cc")
if not cc:raise SystemExit("gcc/clang missing")
top=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_VOICE_S155_LEFT15_TAPER_RAMP_PERMILLE 70U
static uint8_t steer_voice_s155_left15_taper_enable=1U;
static uint8_t voice_pos_owned=1U;
static uint8_t voice_pos_last_angle_deg=15U;
static int8_t voice_pos_last_turn=-1;
'''
tail=r'''
static uint16_t old(uint16_t duty,uint16_t floor,uint16_t gap,uint16_t lead){
  uint16_t excess;
  uint32_t trim;
  if(duty<=floor||gap<=lead)return duty;
  excess=(uint16_t)(gap-lead);
  if(excess>120U)excess=120U;
  trim=((uint32_t)(duty-floor)*excess+60U)/120U;
  return (uint16_t)(duty-trim);
}
int main(void){
 assert(Steering_VoiceS155Left15Active(4U)==1U);
 for(int m=0;m<=5;m++){
  uint8_t mode=(uint8_t)m;
  assert(Steering_VoiceS155Left15Active(mode)==(mode==4U));
 }
 voice_pos_last_turn=1;
 assert(!Steering_VoiceS155Left15Active(4U));
 voice_pos_last_turn=-1;voice_pos_last_angle_deg=30;
 assert(!Steering_VoiceS155Left15Active(4U));
 voice_pos_last_angle_deg=15;voice_pos_owned=0U;
 assert(!Steering_VoiceS155Left15Active(4U));
 voice_pos_owned=1U;steer_voice_s155_left15_taper_enable=0U;
 assert(!Steering_VoiceS155Left15Active(4U));
 steer_voice_s155_left15_taper_enable=1U;
 // Exact fallback behavior over relevant duty, floor, gap and lead grid.
 for(uint16_t floor=50;floor<=175;floor+=125)
 for(uint16_t duty=floor;duty<=300;duty+=7)
 for(uint16_t gap=0;gap<=1000;gap+=7)
 for(uint16_t lead=15;lead<=80;lead+=5) {
   uint16_t legacy=old(duty,floor,gap,lead);
   uint16_t fallback=Steering_VoiceS155LeaderTaper(duty,floor,gap,lead,0U);
   uint16_t tuned=Steering_VoiceS155LeaderTaper(duty,floor,gap,lead,1U);
   assert(fallback==legacy);
   assert(tuned>=floor && tuned<=duty);
   assert(tuned<=fallback);
   if(gap<=lead) assert(tuned==duty);
 }
 assert(Steering_VoiceS155LeaderTaper(250U,175U,99U,30U,1U)>175U);
 assert(Steering_VoiceS155LeaderTaper(250U,175U,100U,30U,1U)==175U);
 assert(Steering_VoiceS155LeaderTaper(100U,50U,100U,30U,1U)==50U);
 puts("PASS compiled C 15LEFT-only gate; fallback bit-exact; bounded early floor");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
 f=Path(t)/"v.c";f.write_text(top+active+"\n"+taper+"\n"+tail,encoding="utf-8")
 exe=Path(t)/"v"
 r=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(f),"-o",str(exe)],text=True,capture_output=True)
 if r.returncode:raise SystemExit("HOST C COMPILE FAIL\n"+r.stderr)
 r=subprocess.run([str(exe)],text=True,capture_output=True)
 if r.returncode:raise SystemExit("HOST C RUN FAIL\n"+r.stdout+r.stderr)
 print(r.stdout.strip())
print("PASS S15.4 actual source/NC ordering unchanged; actual chassis >90% pending")
