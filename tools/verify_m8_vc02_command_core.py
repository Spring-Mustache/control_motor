#!/usr/bin/env python3
"""Compile actual VC02-I1 main.c event/command functions with bounded HOST stubs.
This tests interpreter semantics, not target HAL, motor PWM or electronics.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess,shutil
ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
def extract(sig):
 i=SRC.index(sig);start=SRC.index("{",i)
 d=0
 for k in range(start,len(SRC)):
  if SRC[k]=="{":d+=1
  elif SRC[k]=="}":
   d-=1
   if d==0:return SRC[i:k+1]
 raise RuntimeError(sig)
sigs=[
"static void VC02_M1_RecordEvent(",
"static void Voice_ResetPendingCommand(",
"static void Voice_PendingExpire(",
"static void Voice_SetMovement(",
"static void Voice_SetTurn(",
"static void Voice_SetAngle(",
"static void Voice_SetStraight(",
"static void Voice_S154StageSteerAngle(",
"static uint8_t Voice_FinalizeCommand(",
"static void VC02_M2_ProcessEvent("
]
actual="\n\n".join(extract(sig) for sig in sigs)
top=r"""
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include "voice_s154_route.h"
typedef enum { VOICE_MOVE_NONE=0, VOICE_MOVE_FORWARD, VOICE_MOVE_BACKWARD,
 VOICE_MOVE_ROTATE, VOICE_MOVE_ROTATE_RIGHT } VoiceMovement_t;
typedef enum { VOICE_TURN_NONE=0, VOICE_TURN_LEFT, VOICE_TURN_RIGHT } VoiceTurn_t;
typedef enum { VOICE_BUILD_RESULT_NONE=0,VOICE_BUILD_RESULT_OK,
 VOICE_BUILD_ERROR_NO_MOVEMENT,VOICE_BUILD_ERROR_MOVEMENT_CONFLICT,
 VOICE_BUILD_ERROR_STEERING_CONFLICT,VOICE_BUILD_ERROR_STEERING_INCOMPLETE,
 VOICE_BUILD_ERROR_ROTATE_WITH_STEERING } VoiceBuildResult_t;
typedef enum { VC02_EVENT_NONE=0,VC02_EVENT_WAKEUP,VC02_EVENT_FORWARD,
 VC02_EVENT_BACKWARD,VC02_EVENT_ROTATE_IN_PLACE,VC02_EVENT_TURN_LEFT,
 VC02_EVENT_TURN_RIGHT,VC02_EVENT_ANGLE,VC02_EVENT_SPEED,VC02_EVENT_DURATION,
 VC02_EVENT_CLEAR,VC02_EVENT_STOP,VC02_EVENT_ROTATE_RIGHT } VC02_EventType_t;
enum { VC02_SPEED_VERY_SLOW=1,VC02_SPEED_FULL=5,VC02_SPEED_MEDIUM=3 };
typedef struct { VC02_EventType_t type; uint8_t value,raw0,raw1; } VC02_Event_t;
typedef struct {
 VoiceMovement_t movement;VoiceTurn_t turn;uint8_t angle_deg;
 uint8_t movement_valid,turn_valid,angle_valid;
 uint8_t movement_conflict,steering_conflict;
 uint8_t duration_valid,duration_sec;
 uint32_t wheel_pending_last_ms,steer_pending_last_ms;
} VoicePendingCommand_t;
typedef struct {
 VoiceMovement_t movement;VoiceTurn_t turn;
 uint8_t angle_deg,speed_level,duration_sec;
} VoiceVehicleCommand_t;
static VoicePendingCommand_t voice_pending_command={0};
static VoiceVehicleCommand_t voice_completed_command={0};
static volatile uint8_t voice_command_complete=0, voice_command_ready=0;
static volatile uint8_t voice_execution_active=0;
static volatile uint8_t voice_speed_mode=3;
#define VOICE_PENDING_TIMEOUT_MS 20000U
static volatile uint32_t fake_tick=0;
static uint32_t HAL_GetTick(void){return fake_tick;}
static volatile uint32_t voice_wheel_partial_replace_count=0,voice_steer_partial_replace_count=0;
static volatile uint32_t voice_wheel_pending_expire_count=0,voice_steer_pending_expire_count=0;
static volatile uint32_t voice_orphan_duration_discard_count=0,voice_orphan_angle_discard_count=0;
static volatile uint8_t voice_angle_mapping_enable=1;
static volatile uint8_t voice_pos_request=0,voice_pos_owned=0,voice_rotate_stage=0;
static volatile uint8_t voice_pos_rejected=0,voice_pos_last_result=0;
static volatile uint8_t voice_pos_last_angle_deg=0;
static volatile int8_t voice_pos_last_turn=0;
static volatile uint16_t voice_pos_goal_l=0,voice_pos_goal_r=0;
static volatile uint8_t steer_pos_active=0,steer_manual_active=0,steer_cal_active=0;
static volatile uint32_t voice_pos_request_count=0;
static volatile uint32_t voice_completed_count=0,voice_invalid_count=0;
static volatile uint32_t voice_ignored_during_execution_count=0;
static volatile uint32_t voice_clear_count=0,voice_stop_count=0;
static volatile VoiceBuildResult_t voice_last_build_result=VOICE_BUILD_RESULT_NONE;
static volatile uint32_t vc02_event_count=0;
static volatile VC02_EventType_t vc02_last_event_type=0;
static volatile uint8_t vc02_last_value=0,vc02_last_raw0=0,vc02_last_raw1=0;
enum { CONTROL_MODE_IDLE=0,CONTROL_MODE_PS2=3 };
static volatile int control_mode=CONTROL_MODE_IDLE;
static volatile uint8_t control_stop_latched=0,ps2_cancel_hold=0,ps2_spin_rf_latched=0;
#define STEER_POS_LEFT_CENTER_RAW 1886U
#define STEER_POS_RIGHT_CENTER_RAW 662U
#define STEER_MANUAL_LEFT_SOFT_MAX 1454U
#define STEER_MANUAL_RIGHT_SOFT_MIN 245U
#define STEER_MANUAL_LEFT_SOFT_MIN 2225U
#define STEER_MANUAL_RIGHT_SOFT_MAX 970U
static void Voice_ResetPendingCommand(void);
static void Control_RequestStop(void)
{
 Voice_ResetPendingCommand();voice_pos_request=0;voice_pos_owned=0;
 voice_command_ready=0;voice_execution_active=0;voice_rotate_stage=0;
 control_stop_latched=1;
}
"""
bottom=r"""
static void send(VC02_EventType_t type,uint8_t val) {
 VC02_Event_t e={type,val,0,0};VC02_M2_ProcessEvent(&e);
}
static void neutral(void) {
 Voice_ResetPendingCommand();voice_command_ready=0;voice_command_complete=0;
 voice_execution_active=0;voice_pos_request=0;voice_pos_owned=0;
 voice_rotate_stage=0;voice_speed_mode=VC02_SPEED_MEDIUM;
 fake_tick=0;
 voice_last_build_result=VOICE_BUILD_RESULT_NONE;
 control_stop_latched=0;control_mode=CONTROL_MODE_IDLE;
 ps2_cancel_hold=0;ps2_spin_rf_latched=0;
}
int main(void) {
 neutral();
 send(VC02_EVENT_FORWARD,0);
 assert(voice_pending_command.movement_valid&&
        !voice_command_ready&&!voice_execution_active);
 send(VC02_EVENT_DURATION,10);
 assert(voice_command_ready&&voice_completed_command.duration_sec==10 &&
        voice_completed_command.movement==VOICE_MOVE_FORWARD);
 /* Two Voice command groups can be simultaneously READY. */
 send(VC02_EVENT_TURN_LEFT,0);
 assert(voice_pending_command.turn_valid&&!voice_pos_request);
 send(VC02_EVENT_ANGLE,30);
 assert(voice_pos_request==4&&voice_command_ready==1&&
        voice_pos_goal_l<1886&&voice_pos_goal_r<662);
 neutral();
 send(VC02_EVENT_TURN_RIGHT,0);
 send(VC02_EVENT_ANGLE,45);
 assert(voice_pos_request==4&&voice_pos_goal_l==2225&&
        voice_pos_goal_r==970);
 neutral();
 send(VC02_EVENT_ANGLE,0);
 assert(voice_pos_request==1&&voice_pos_goal_l==1886&&
        voice_pos_goal_r==662);
 neutral();
 send(VC02_EVENT_SPEED,5);
 assert(voice_speed_mode==5&&!voice_command_ready&&!voice_pos_request);
 /* Live speed input is accepted even with an active wheel owner;
  * the executor consumes the new setting on its next pass.
  */
 voice_execution_active=1;
 send(VC02_EVENT_SPEED,1);
 assert(voice_speed_mode==1&&voice_execution_active==1&&
        !voice_command_ready&&!voice_pos_request);
 neutral();
 send(VC02_EVENT_ROTATE_IN_PLACE,0);
 assert(voice_pending_command.movement==VOICE_MOVE_ROTATE&&
        !voice_command_ready&&!voice_pos_request);
 send(VC02_EVENT_DURATION,5);
 assert(voice_command_ready&&
        voice_completed_command.movement==VOICE_MOVE_ROTATE);
 neutral();
 send(VC02_EVENT_ROTATE_RIGHT,0);
 send(VC02_EVENT_DURATION,20);
 assert(voice_command_ready&&
        voice_completed_command.movement==VOICE_MOVE_ROTATE_RIGHT&&
        voice_completed_command.duration_sec==20);
 neutral();
 /* Orphan angle must NEVER be cached or replayed by later A5/A6. */
 send(VC02_EVENT_ANGLE,45);
 assert(!voice_pos_request&&!voice_pending_command.angle_valid&&
        voice_orphan_angle_discard_count>0);
 send(VC02_EVENT_TURN_LEFT,0);
 assert(!voice_pos_request&&voice_pending_command.turn_valid&&
        !voice_pending_command.angle_valid);
 send(VC02_EVENT_ANGLE,15);
 assert(voice_pos_request==4&&voice_pos_last_angle_deg==15);
 neutral();
 send(VC02_EVENT_FORWARD,0);
 send(VC02_EVENT_CLEAR,0);
 assert(!voice_pending_command.movement_valid&&!voice_command_ready);
 neutral();
 send(VC02_EVENT_BACKWARD,0);
 send(VC02_EVENT_DURATION,10);
 send(VC02_EVENT_STOP,0);
 assert(control_stop_latched&&voice_command_ready==0&&
        voice_pos_request==0&&voice_execution_active==0);
 neutral();
 /* Latest directional input overwrites the unfinished previous one. */
 send(VC02_EVENT_TURN_LEFT,0);
 send(VC02_EVENT_TURN_RIGHT,0);
 send(VC02_EVENT_ANGLE,15);
 assert(voice_pos_request==4&&voice_pos_last_turn==1&&
        voice_steer_partial_replace_count>=1);
 neutral();
 send(VC02_EVENT_FORWARD,0);send(VC02_EVENT_BACKWARD,0);
 send(VC02_EVENT_DURATION,10);
 assert(voice_last_build_result==VOICE_BUILD_RESULT_OK&&
        voice_completed_command.movement==VOICE_MOVE_BACKWARD&&
        voice_command_ready==1&&voice_wheel_partial_replace_count>=1);
 neutral();
 send(VC02_EVENT_ROTATE_IN_PLACE,0);send(VC02_EVENT_ROTATE_RIGHT,0);
 send(VC02_EVENT_DURATION,5);
 assert(voice_command_ready&&
        voice_completed_command.movement==VOICE_MOVE_ROTATE_RIGHT);
 neutral();
 /* Orphan duration must NEVER be cached or replayed by later A2..A7. */
 send(VC02_EVENT_DURATION,5);
 assert(!voice_command_ready&&!voice_pending_command.duration_valid&&
        voice_orphan_duration_discard_count>0);
 send(VC02_EVENT_FORWARD,0);
 assert(!voice_command_ready&&voice_pending_command.movement_valid&&
        !voice_pending_command.duration_valid);
 send(VC02_EVENT_DURATION,10);
 assert(voice_command_ready&&
        voice_completed_command.movement==VOICE_MOVE_FORWARD&&
        voice_completed_command.duration_sec==10);
 neutral();
 /* Two pending owner registers can overlap without stealing each other. */
 send(VC02_EVENT_FORWARD,0);
 send(VC02_EVENT_TURN_LEFT,0);
 send(VC02_EVENT_ANGLE,30);
 assert(voice_pos_request==4&&voice_pending_command.movement_valid);
 send(VC02_EVENT_DURATION,10);
 assert(voice_command_ready&&
        voice_completed_command.movement==VOICE_MOVE_FORWARD);
 neutral();
 /* Incomplete phrases expire independently. Late parameters are orphaned,
  * and do not remain pending for the next primary command.
  */
 send(VC02_EVENT_TURN_LEFT,0);
 fake_tick=20001;send(VC02_EVENT_ANGLE,30);
 assert(!voice_pos_request&&!voice_pending_command.angle_valid&&
        !voice_pending_command.turn_valid&&voice_steer_pending_expire_count>0);
 send(VC02_EVENT_TURN_RIGHT,0);
 assert(!voice_pos_request&&!voice_pending_command.angle_valid);
 neutral();
 send(VC02_EVENT_FORWARD,0);
 fake_tick=20001;send(VC02_EVENT_DURATION,5);
 assert(!voice_command_ready&&!voice_pending_command.duration_valid&&
        !voice_pending_command.movement_valid&&voice_wheel_pending_expire_count>0);
 send(VC02_EVENT_BACKWARD,0);
 assert(!voice_command_ready&&!voice_pending_command.duration_valid);
 neutral();
 /* CLEAR only discards uncommitted registers, never accepted owners. */
 send(VC02_EVENT_TURN_LEFT,0);send(VC02_EVENT_CLEAR,0);
 assert(!voice_pending_command.turn_valid&&!voice_pos_request);
 neutral();
 send(VC02_EVENT_TURN_LEFT,0);send(VC02_EVENT_ANGLE,5);
 assert(voice_pos_request==4);
 send(VC02_EVENT_CLEAR,0);
 assert(voice_pos_request==4&&!voice_pending_command.turn_valid);
 neutral();
 send(VC02_EVENT_FORWARD,0);send(VC02_EVENT_DURATION,5);
 assert(voice_command_ready==1);
 send(VC02_EVENT_CLEAR,0);
 assert(voice_command_ready==1);
 neutral();
 /* The next primary word must clear the ENTIRE incomplete group
  * including a stale parameter even if one leaked into storage.
  */
 neutral();
 send(VC02_EVENT_FORWARD,0);
 voice_pending_command.duration_sec=30;
 voice_pending_command.duration_valid=1;
 send(VC02_EVENT_BACKWARD,0);
 assert(!voice_pending_command.duration_valid&&
        voice_pending_command.duration_sec==0&&!voice_command_ready&&
        voice_pending_command.movement==VOICE_MOVE_BACKWARD);
 send(VC02_EVENT_DURATION,5);
 assert(voice_command_ready&&voice_completed_command.duration_sec==5&&
        voice_completed_command.movement==VOICE_MOVE_BACKWARD);
 neutral();
 send(VC02_EVENT_TURN_LEFT,0);
 voice_pending_command.angle_deg=40;
 voice_pending_command.angle_valid=1;
 send(VC02_EVENT_TURN_RIGHT,0);
 assert(!voice_pending_command.angle_valid&&
        voice_pending_command.angle_deg==0&&!voice_pos_request);
 send(VC02_EVENT_ANGLE,10);
 assert(voice_pos_request==4&&voice_pos_last_turn==1&&
        voice_pos_last_angle_deg==10);
 /* Duplicate primary commands also start a fresh phrase, not inheriting
  * an old parameter. Wheel and steering groups remain independent.
  */
 neutral();
 send(VC02_EVENT_FORWARD,0);
 voice_pending_command.duration_sec=15;
 voice_pending_command.duration_valid=1;
 send(VC02_EVENT_FORWARD,0);
 assert(!voice_pending_command.duration_valid&&
        voice_pending_command.duration_sec==0&&!voice_command_ready);
 send(VC02_EVENT_TURN_LEFT,0);
 assert(voice_pending_command.movement_valid);
 send(VC02_EVENT_ANGLE,25);
 assert(voice_pos_request==4&&voice_pending_command.movement_valid);
 neutral();
 send(VC02_EVENT_FORWARD,0);send(VC02_EVENT_DURATION,0);
 assert(!voice_command_ready&&voice_invalid_count);
 puts("PASS actual VC02 PLC C: strict primary->parameter, orphan TIME/ANGLE discarded, replacement clears group, independent owners, timeout, F0/FF");
}
"""
CC=shutil.which("gcc") or shutil.which("cc")
if not CC:raise SystemExit("Missing gcc")
with TemporaryDirectory() as td:
 src=Path(td)/"voice.c"
 src.write_text(top+"\n"+actual+"\n"+bottom,encoding="utf-8")
 out=Path(td)/"voice_test"
 p=subprocess.run([CC,"-std=c11","-O2","-Wall","-Wextra","-Werror",
    "-Wno-unused-function","-I",str(ROOT/"Core/Inc"),str(src),"-o",str(out)],
    capture_output=True,text=True)
 if p.returncode:raise SystemExit("HOST command compile FAILED:\n"+p.stdout+p.stderr)
 p=subprocess.run([str(out)],capture_output=True,text=True)
 if p.returncode:raise SystemExit("HOST command runtime FAILED:\n"+p.stdout+p.stderr)
 print(p.stdout.strip())
print("VC02-I1 actual event C TESTED on HOST. No powered motor or target toolchain tests.")
