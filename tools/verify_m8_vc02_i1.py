#!/usr/bin/env python3
"""VC02-I1: REAL decoder C, real pure route C, and source integration guards.
HOST ONLY, not a STM32CubeIDE target build, flash or powered S15.4 PASS.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil, subprocess

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"Core/Src/main.c").read_text(encoding="utf8")
H=(ROOT/"Core/Inc/vc02.h").read_text(encoding="utf8")
DEC=(ROOT/"Core/Src/vc02.c").read_text(encoding="utf8")
ROUTE=(ROOT/"Core/Inc/voice_s154_route.h").read_text(encoding="utf8")
def check(label, ok):
 print(("PASS" if ok else "FAIL")+": "+label)
 if not ok: raise SystemExit(1)
def body(src, name):
 start=src.index(name)
 first=src.index("{",start)
 depth=0
 for i in range(first,len(src)):
  if src[i]=="{":depth+=1
  elif src[i]=="}":
   depth-=1
   if depth==0:return src[start:i+1]
 raise RuntimeError("unbalanced function "+name)

checks={
 "A4 remains left event, A7 parsed as right with old enum ids stable":
  "VC02_EVENT_ROTATE_IN_PLACE" in DEC and "case 0xA7U" in DEC and
  "VC02_EVENT_ROTATE_RIGHT" in DEC and
  H.index("VC02_EVENT_STOP") < H.index("VC02_EVENT_ROTATE_RIGHT"),
 "Voice command paths enabled, master steering motion permit remains boot OFF":
  all(x in MAIN for x in ("voice_pos_enable = 1U;",
      "voice_angle_mapping_enable = 1U;","voice_pivot_enable = 1U;",
      "steer_safety_motion_permit = 0U;")) and
  "steer_safety_motion_permit==1U" in
    body(MAIN,"static void Steering_PositionUpdate("),
 "Steering angle route is command 4, not PS2 1|2 == 3 conflict":
  "voice_pos_request=(angle==0U)?1U:4U;" in MAIN and
  "steer_pos_command == 4U" in MAIN and
  "(steer_pos_command != 4U)" in MAIN,
 "Duration and movement both required before wheel motion":
  "VoiceS154_DurationValid(duration_sec)" in
   body(MAIN,"static uint8_t Voice_FinalizeCommand(") and
  "voice_pending_command.movement_valid==0U" in
   body(MAIN,"static uint8_t Voice_FinalizeCommand(") and
  "voice_pending_command.duration_valid!=0U" in
   body(MAIN,"static void VC02_M2_ProcessEvent("),
 "Latest partial wins without movement/steering conflict":
  "voice_pending_command.movement_conflict = 0U;" in
   body(MAIN,"static void Voice_SetMovement(") and
  "voice_pending_command.steering_conflict = 0U;" in
   body(MAIN,"static void Voice_SetTurn("),
 "F0 clears pending only, never accepted Voice steering request":
  "voice_pos_request=0U;" not in
   body(MAIN,"static void VC02_M2_ProcessEvent("),
 "A5/A6+B angle is committed independently with no TIME":
  "Voice_S154StageSteerAngle(event->value)" in
   body(MAIN,"static void VC02_M2_ProcessEvent(") and
  "VoiceS154_AngleGoals" in
   body(MAIN,"static void Voice_S154StageSteerAngle("),
 "C1..C5 updates standalone speed without starting wheel":
  "voice_speed_mode=event->value;" in
   body(MAIN,"static void VC02_M2_ProcessEvent(") and
  "Voice_StartExecution" not in
   body(MAIN,"static void VC02_M2_ProcessEvent("),
 "New physical steering goes through protected position owner":
  "voice_pos_owned=from_voice;" in
   body(MAIN,"static void Steering_PositionUpdate(") and
  "Steering_PositionAxisUpdate" in
   body(MAIN,"static void Steering_PositionUpdate(") and
  "Steering_PositionNcDirectionAllowed" in
   body(MAIN,"static void Steering_PositionUpdate(") and
  "steer_safety_motion_permit==1U" in
   body(MAIN,"static void Steering_PositionUpdate("),
 "R2 requested before voice wheel pivot and never fake PS2 fields":
  "voice_pos_request=2U;" in
   body(MAIN,"static void Voice_StartExecution(") and
  "voice_rotate_stage=1U" in
   body(MAIN,"static void Voice_StartExecution(") and
  "VoiceS154_PivotNcReady" in
   body(MAIN,"static void Voice_ExecuteMotorControl(") and
  "Steering_PositionWheelsStopped()" in
   body(MAIN,"static void Voice_StartExecution(") and
  "PS2_Pad." not in
   body(MAIN,"static void Voice_StartExecution("),
 "A4 -> L2 (-1), A7 -> L1 (+1), bounded to PS2 spin ceiling":
  "voice_rotate_direction=(kind==VOICE_MOVE_ROTATE)?-1:1;" in MAIN and
  "MOTOR_DPAD_PWM" in
   body(MAIN,"static void Voice_ExecuteMotorControl("),
 "Physical NC release during Voice spin aborts with emergency STOP":
  "Wheel_EmergencyStop();" in
   body(MAIN,"static void Voice_ExecuteMotorControl(") and
  "voice_rotate_nc_cut_count++;" in
   body(MAIN,"static void Voice_ExecuteMotorControl("),
 "Wheel executor never kills valid concurrent steering owner":
  "Turn_Motors_Stop(" not in
   body(MAIN,"static void Voice_ExecuteMotorControl(") and
  "voice_pos_request != 0U" in
   body(MAIN,"static void Control_Arbitrate("),
 "STOP cancels both active voice streams; PS2 cancel has neutral rearm":
  "Steering_PositionFinish(9U);" in
   body(MAIN,"static void Voice_S154CancelAll(") and
  "voice_pos_request=0U;" in
   body(MAIN,"static void Voice_S154CancelAll(") and
  "Voice_S154CancelAll();" in
   body(MAIN,"static void Control_RequestStop(") and
  "Steering_CalInputsNeutral()" in
   body(MAIN,"static void Control_Arbitrate(") and
  "ps2_cancel_hold = 1U;" in
   body(MAIN,"static void Control_Arbitrate("),
 "RF-loss PS2 guard excludes voice-owned R2 and voice spin":
  "(voice_rotate_stage != 2U)" in
    body(MAIN,"static void Control_Arbitrate(") and
  "(voice_pos_owned == 0U)" in
    body(MAIN,"static void Control_Arbitrate("),
 "Preserve M8 legacy RIGHT/LEFT safety, no S15.3 PWM or PS2 key changes":
  "STEER_MANUAL_STALL_MS" in MAIN and
  "steer_s153_trim_only_enable=1U;" in MAIN and
  "#define STEER_SAFETY_INCIDENT_LOCKOUT 0U" in MAIN and
  "steer_pos_left_pwm_min = 175U;" in MAIN and
  "steer_pos_right_pwm_min = 50U;" in MAIN,
}
for k,v in checks.items():check(k,v)
CC=shutil.which("gcc") or shutil.which("cc")
if not CC:raise SystemExit("HOST GCC compiler not installed")
# Compile the actual vc02.c and vc02.h, replacing HAL main.h with
# tiny host declarations. Do not rewrite/replace production parser code.
stub=r"""
#ifndef MAIN_H
#define MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int id; } UART_HandleTypeDef;
typedef enum { HAL_OK=0, HAL_ERROR=1 } HAL_StatusTypeDef;
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*,uint8_t*,uint16_t);
#define UART_FLAG_ORE (1U)
#define RESET 0
#define __HAL_UART_GET_FLAG(a,b) (0)
#define __HAL_UART_CLEAR_OREFLAG(a) ((void)0)
static inline uint32_t __get_PRIMASK(void){return 0U;}
static inline void __disable_irq(void){}
static inline void __enable_irq(void){}
#endif
"""
test=r"""
#include <assert.h>
#include <stdio.h>
#include "vc02.h"
#include "voice_s154_route.h"
static uint8_t *wire=0;
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *u,uint8_t *p,uint16_t n)
{assert(u!=0&&p!=0&&n==1);wire=p;return HAL_OK;}
static void push(UART_HandleTypeDef *u,uint8_t b)
{assert(wire!=0);*wire=b;VC02_UART_RxCpltCallback(u);}
static VC02_Event_t pop(VC02_EventType_t type,uint8_t value,uint8_t raw0)
{VC02_Event_t e={0};assert(VC02_GetEvent(&e));assert(e.type==type);
 assert(e.value==value&&e.raw0==raw0);return e;}
int main(void) {
 UART_HandleTypeDef u={1};VC02_Event_t e;
 assert(VC02_Begin(&u)==HAL_OK);
 push(&u,0xA4);pop(VC02_EVENT_ROTATE_IN_PLACE,0,0xA4);
 push(&u,0xA7);pop(VC02_EVENT_ROTATE_RIGHT,0,0xA7);
 push(&u,0xA5);pop(VC02_EVENT_TURN_LEFT,0,0xA5);
 push(&u,0xA6);pop(VC02_EVENT_TURN_RIGHT,0,0xA6);
 push(&u,0xB0);pop(VC02_EVENT_ANGLE,0,0xB0);
 push(&u,0xB6);pop(VC02_EVENT_ANGLE,30,0xB6);
 push(&u,0xB9);pop(VC02_EVENT_ANGLE,45,0xB9);
 push(&u,0xC3);pop(VC02_EVENT_SPEED,3,0xC3);
 push(&u,0xD0);assert(!VC02_GetEvent(&e));
 push(&u,0x0A);e=pop(VC02_EVENT_DURATION,10,0xD0);assert(e.raw1==0x0A);
 push(&u,0xD0);assert(!VC02_GetEvent(&e));
 push(&u,0xFF);pop(VC02_EVENT_STOP,0,0xFF); /* invalid duration fallback */
 push(&u,0xF0);pop(VC02_EVENT_CLEAR,0,0xF0);
 push(&u,0xD0);assert(!VC02_GetEvent(&e));
 push(&u,0x07);assert(!VC02_GetEvent(&e));
 push(&u,0xA2);pop(VC02_EVENT_FORWARD,0,0xA2);
 assert(vc02_uart_rx_byte_count>=14U&&vc02_uart_last_byte==0xA2);
 assert(vc02_uart_begin_status==0U&&vc02_uart_rearm_error_count==0U);
 VC02_UART_ErrorCallback(&u);assert(vc02_uart_error_count==1U);
 for(int sec=5;sec<=120;sec+=5)
   assert(VoiceS154_DurationValid((uint8_t)sec));
 assert(!VoiceS154_DurationValid(0) &&
        !VoiceS154_DurationValid(7) &&
        !VoiceS154_DurationValid(121));
 assert(VoiceS154_PivotDirection(0xA4)==-1);
 assert(VoiceS154_PivotDirection(0xA7)==1);
 assert(VoiceS154_PivotDirection(0xA5)==0);
 assert(VoiceS154_PivotNcReady(0x06,0x04,0x02,0x08,0x01));
 assert(!VoiceS154_PivotNcReady(0x02,0x04,0x02,0x08,0x01));
 assert(!VoiceS154_PivotNcReady(0x07,0x04,0x02,0x08,0x01));
 assert(!VoiceS154_PivotNcReady(0x0E,0x04,0x02,0x08,0x01));
 for(int d=5;d<=45;d+=5)for(int dir=-1;dir<=1;dir+=2) {
  VoiceS154Goals_t g=VoiceS154_AngleGoals((int8_t)dir,
   (uint8_t)d,1886,662,1454,245,2225,970);
  assert(g.valid && g.left_raw>=1454&&g.left_raw<=2225);
  assert(g.right_raw>=245&&g.right_raw<=970);
  if(dir<0){assert(g.left_raw<1886&&g.right_raw<662);}
  else {assert(g.left_raw>1886&&g.right_raw>662);}
  if(d==45){assert(g.left_raw==(dir<0?1454:2225));
            assert(g.right_raw==(dir<0?245:970));}
 }
 assert(!VoiceS154_AngleGoals(0,30,1886,662,1454,245,2225,970).valid);
 assert(!VoiceS154_AngleGoals(-1,0,1886,662,1454,245,2225,970).valid);
 assert(!VoiceS154_AngleGoals(1,30,1886,662,1900,245,2225,970).valid);
 puts("PASS actual UART decoder, full TIME set, A4/A7, B0/B9, speed, physical NC gate and independent angle goal C kernels");
 return 0;
}
"""
with TemporaryDirectory() as tmp:
 d=Path(tmp)
 (d/"vc02.h").write_text(H,encoding="utf8")
 (d/"main.h").write_text(stub,encoding="utf8")
 (d/"test.c").write_text(test,encoding="utf8")
 out=d/"vc02_host"
 cmd=[CC,"-std=c11","-O2","-Wall","-Wextra","-Werror",
      "-I",str(d),"-I",str(ROOT/"Core/Inc"),
      str(d/"test.c"),str(ROOT/"Core/Src/vc02.c"),"-o",str(out)]
 p=subprocess.run(cmd,text=True,capture_output=True)
 if p.returncode:raise SystemExit("HOST compile FAILED\n"+p.stdout+p.stderr)
 p=subprocess.run([str(out)],text=True,capture_output=True)
 if p.returncode:raise SystemExit("HOST runtime FAILED\n"+p.stdout+p.stderr)
 print(p.stdout.strip())
print("PASS VC02-I1 HOST integration. CubeIDE/STM32 flash/powered hardware still UNTESTED.")
