#!/usr/bin/env python3
"""Run real STM32 Voice executor C with safe HAL/owner stubs on HOST.
R2 and NC are simulated booleans. DOES NOT verify powered hardware.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
def extract(name):
 i=SRC.index(name);p=SRC.index("{",i);d=0
 for n in range(p,len(SRC)):
  if SRC[n]=="{":d+=1
  elif SRC[n]=="}":
   d-=1
   if d==0:return SRC[i:n+1]
 raise ValueError(name)
funcs=["static void Voice_ResetPendingCommand(",
       "static void Voice_S154CancelAll(",
       "static uint16_t Voice_SpeedLevelToPWM(",
       "static uint8_t Voice_IsOpenLoopCommandSupported(",
       "static void Voice_StartExecution(",
       "static void Voice_ExecuteMotorControl("]
actual="\n\n".join(extract(i) for i in funcs)
before=r"""
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "voice_s154_route.h"
typedef enum { VOICE_MOVE_NONE=0,VOICE_MOVE_FORWARD,VOICE_MOVE_BACKWARD,
 VOICE_MOVE_ROTATE,VOICE_MOVE_ROTATE_RIGHT } VoiceMovement_t;
typedef enum { VOICE_TURN_NONE=0,VOICE_TURN_LEFT,VOICE_TURN_RIGHT } VoiceTurn_t;
typedef enum { VOICE_BUILD_RESULT_NONE=0,VOICE_BUILD_RESULT_OK } VoiceBuildResult_t;
typedef struct {
 VoiceMovement_t movement;VoiceTurn_t turn;uint8_t angle_deg;
 uint8_t movement_valid,turn_valid,angle_valid,movement_conflict,steering_conflict;
 uint8_t duration_valid,duration_sec;
 uint32_t wheel_pending_last_ms,steer_pending_last_ms;
} VoicePendingCommand_t;
typedef struct {
 VoiceMovement_t movement;VoiceTurn_t turn;uint8_t angle_deg,speed_level,duration_sec;
} VoiceVehicleCommand_t;
enum {VC02_SPEED_VERY_SLOW=1,VC02_SPEED_SLOW=2,
 VC02_SPEED_MEDIUM=3,VC02_SPEED_FAST=4,VC02_SPEED_FULL=5};
#define VOICE_PWM_VERY_SLOW 250
#define VOICE_PWM_SLOW 400
#define VOICE_PWM_MEDIUM 550
#define VOICE_PWM_FAST 750
#define VOICE_PWM_FULL 999
#define MOTOR_DPAD_PWM 400
#define WHEEL_COMMAND_PROFILE_FIXED 1
#define STEER_LIMIT_L_MIN_MASK 0x04U
#define STEER_LIMIT_R_MIN_MASK 0x02U
#define STEER_LIMIT_L_MAX_MASK 0x08U
#define STEER_LIMIT_R_MAX_MASK 0x01U
#define VOICE_ROTATE_R2_WAIT_LIMIT_MS 6500U
static VoicePendingCommand_t voice_pending_command={0};
static volatile VoiceVehicleCommand_t voice_completed_command={0},voice_active_command={0};
static volatile VoiceBuildResult_t voice_last_build_result=VOICE_BUILD_RESULT_NONE;
static volatile uint8_t voice_command_ready=0,voice_command_complete=0;
static volatile uint8_t voice_execution_active=0;
static volatile int16_t voice_execution_pwm=0;
static volatile uint8_t voice_speed_mode=VC02_SPEED_MEDIUM;
static volatile uint32_t voice_speed_active_update_count=0;
static volatile uint32_t voice_execution_start_tick=0;
static volatile uint32_t voice_execution_duration_ms=0;
static volatile uint32_t voice_execution_elapsed_ms=0;
static volatile uint32_t voice_execution_finish_count=0;
static volatile uint32_t voice_unsupported_command_count=0;
static volatile uint8_t voice_rotate_stage=0;
static volatile int8_t voice_rotate_direction=0;
static volatile uint32_t voice_rotate_wait_start_ms=0;
static volatile uint32_t voice_rotate_reject_count=0;
static volatile uint32_t voice_rotate_nc_cut_count=0;
static volatile uint8_t voice_pos_enable=0,voice_pivot_enable=0;
static volatile uint8_t voice_pos_owned=0,voice_pos_request=0,voice_pos_rejected=0;
static volatile uint8_t voice_pos_last_result=0;
static volatile uint32_t voice_pos_request_count=0;
static volatile uint8_t steer_pos_active=0,steer_manual_active=0,steer_cal_active=0;
static volatile uint8_t steer_pos_result=0,steer_pos_left_result=0,steer_pos_right_result=0;
static volatile uint8_t steer_safety_motion_permit=1,steer_safety_trip_latched=0;
static volatile uint8_t control_stop_latched=0;
static volatile uint32_t fake_ms=0;
static volatile uint8_t fake_nc=0;
static volatile uint8_t fake_wheels_stopped=1;
static volatile int16_t fake_wheel_pwm=0, fake_spin_duty=0;
static volatile int8_t fake_spin_dir=0;
static volatile uint8_t fake_spin_active=0;
static volatile uint32_t fake_emergency=0,fake_steer_stop=0,fake_steer_finish=0;
static uint32_t HAL_GetTick(void){return fake_ms;}
static uint8_t Steering_TestRawLimitMask(void){return fake_nc;}
static uint8_t Steering_PositionWheelsStopped(void){return fake_wheels_stopped;}
static void Steering_PositionFinish(uint8_t code) {
 fake_steer_finish++;steer_pos_active=0;voice_pos_owned=0;steer_pos_result=code;
}
static void Turn_Motors_Stop(void){fake_steer_stop++;}
static void WheelControl_SetTargetPWM(int16_t pwm){fake_wheel_pwm=pwm;}
static void WheelControl_SetCommandProfile(int mode){assert(mode==WHEEL_COMMAND_PROFILE_FIXED);}
static void WheelControl_SetSpinDirection(int8_t dir,int16_t pwm){
 fake_spin_dir=dir;fake_spin_duty=pwm;
 if(dir==0){fake_spin_active=0;fake_wheel_pwm=0;}
 else {fake_spin_active=1;fake_wheel_pwm=(int16_t)(dir*pwm);}
}
static uint8_t WheelControl_SpinIsActive(void){return fake_spin_active;}
static void Wheel_EmergencyStop(void){
 fake_emergency++;fake_spin_active=0;fake_spin_dir=0;fake_wheel_pwm=0;
}
"""
after=r"""
static void reset(void) {
 voice_command_ready=0;voice_command_complete=0;
 voice_execution_active=0;voice_rotate_stage=0;voice_rotate_direction=0;
 voice_pos_request=0;voice_pos_owned=0;voice_pos_rejected=0;
 steer_pos_active=0;steer_safety_motion_permit=1;steer_safety_trip_latched=0;
 control_stop_latched=0;steer_pos_result=0;steer_pos_left_result=0;
 steer_pos_right_result=0;voice_execution_pwm=0;fake_ms=1000;
 fake_nc=0;fake_wheels_stopped=1;fake_wheel_pwm=0;
 fake_spin_active=0;fake_spin_dir=0;fake_spin_duty=0;
 voice_pos_enable=0;voice_pivot_enable=0;
 voice_speed_mode=VC02_SPEED_MEDIUM;Voice_ResetPendingCommand();
}
static void request(VoiceMovement_t move,uint8_t speed,uint8_t duration){
 voice_completed_command.movement=move;voice_completed_command.turn=VOICE_TURN_NONE;
 voice_completed_command.angle_deg=0;voice_completed_command.speed_level=speed;
 voice_speed_mode=speed;
 voice_completed_command.duration_sec=duration;voice_command_ready=1;
 Voice_StartExecution();
}
int main(void){
 reset();
 request(VOICE_MOVE_ROTATE,5,10);
 assert(!voice_execution_active&&!voice_pos_request&&!fake_wheel_pwm &&
        voice_rotate_reject_count>=1); /* boot OFF */
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE,5,10);
 assert(voice_execution_active&&voice_rotate_stage==1&&
        voice_rotate_direction==-1&&voice_pos_request==2&&
        !fake_wheel_pwm&&!fake_spin_active);
 Voice_ExecuteMotorControl();
 assert(!fake_wheel_pwm&&!fake_spin_active); /* waiting for R2 */
 voice_pos_request=0;voice_pos_owned=1;steer_pos_active=1;steer_pos_result=1;
 fake_ms=2500;Voice_ExecuteMotorControl();
 assert(!fake_wheel_pwm&&!fake_spin_active);
 voice_pos_owned=0;steer_pos_active=0;steer_pos_result=2;
 steer_pos_left_result=3;steer_pos_right_result=3;
 fake_nc=0x06;fake_ms=3000;Voice_ExecuteMotorControl();
 assert(voice_rotate_stage==2&&voice_execution_start_tick==3000&&
        !fake_spin_active);
 fake_ms=3050;Voice_ExecuteMotorControl();
 assert(fake_spin_active&&fake_spin_dir==-1&&fake_spin_duty==400);
 fake_ms=13000;Voice_ExecuteMotorControl();
 assert(!voice_execution_active&&!voice_rotate_stage&&
        fake_spin_dir==0&&fake_wheel_pwm==0);
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE_RIGHT,4,5);
 assert(voice_rotate_direction==1&&voice_rotate_stage==1);
 voice_pos_request=0;voice_pos_owned=0;steer_pos_result=2;
 steer_pos_left_result=3;steer_pos_right_result=3;fake_nc=0x06;
 Voice_ExecuteMotorControl();assert(voice_rotate_stage==2);
 fake_ms+=20;Voice_ExecuteMotorControl();
 assert(fake_spin_dir==1&&fake_spin_duty==400);
 /* Losing physical NC contact while spinning is an emergency stop. */
 fake_nc=0x02;fake_ms+=20;Voice_ExecuteMotorControl();
 assert(!voice_execution_active&&!voice_rotate_stage&&fake_emergency>0);
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE_RIGHT,2,5);
 fake_ms+=6501;Voice_ExecuteMotorControl();
 assert(!voice_execution_active&&!voice_rotate_stage&&!fake_spin_active &&
        !voice_pos_request);
 /* No raw NC or owner success == no wheel spin */
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE_RIGHT,2,5);
 voice_pos_request=0;voice_pos_owned=0;steer_pos_result=2;
 steer_pos_left_result=3;steer_pos_right_result=3;fake_nc=0x04;
 Voice_ExecuteMotorControl();
 assert(!voice_execution_active&&!fake_spin_active&&!fake_wheel_pwm);
 /* Four wheels may run independently of an ongoing voice steering route. */
 reset();voice_pos_owned=1;steer_pos_active=1;
 request(VOICE_MOVE_FORWARD,3,5);
 assert(voice_execution_active);
 fake_ms+=100;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==550);
 /* STOP cancellation is never just a wheel-only clear */
 Voice_S154CancelAll();
 assert(!voice_execution_active&&!voice_pos_owned&&
        fake_steer_finish>0&&!fake_wheel_pwm);
 reset();request(VOICE_MOVE_BACKWARD,2,5);
 fake_ms+=100;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==-400);
 /* Live C1..C5 updates a running forward wheel command. No TIME restart. */
 reset();request(VOICE_MOVE_FORWARD,3,10);
 uint32_t start_tick=voice_execution_start_tick;
 uint32_t duration=voice_execution_duration_ms;
 fake_ms+=50;Voice_ExecuteMotorControl();assert(fake_wheel_pwm==550);
 voice_speed_mode=VC02_SPEED_VERY_SLOW;
 fake_ms+=50;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==250&&voice_execution_pwm==250&&
        voice_active_command.speed_level==1&&
        voice_execution_start_tick==start_tick&&
        voice_execution_duration_ms==duration&&
        voice_speed_active_update_count>0);
 voice_speed_mode=VC02_SPEED_FULL;
 fake_ms+=50;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==999&&voice_execution_pwm==999&&
        voice_execution_start_tick==start_tick);
 /* Invalid speed cannot turn an active wheel command off or up. */
 voice_speed_mode=0;
 fake_ms+=50;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==999&&voice_active_command.speed_level==5);
 /* Live change preserves the negative sign of BACKWARD. */
 reset();request(VOICE_MOVE_BACKWARD,2,10);
 fake_ms+=50;Voice_ExecuteMotorControl();assert(fake_wheel_pwm==-400);
 uint32_t reverse_start=voice_execution_start_tick;
 voice_speed_mode=VC02_SPEED_FAST;
 fake_ms+=50;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==-750&&voice_execution_pwm==-750&&
        voice_execution_start_tick==reverse_start);
 /* Both pivot directions use the new speed live while retaining NC and
    the immutable <=400CCR spin ceiling. Waiting for R2 NEVER spins. */
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE,5,10);
 voice_speed_mode=VC02_SPEED_VERY_SLOW;
 fake_ms+=50;Voice_ExecuteMotorControl();
 assert(voice_rotate_stage==1&&voice_active_command.speed_level==1&&
        !fake_spin_active&&!fake_wheel_pwm);
 voice_pos_request=0;voice_pos_owned=0;steer_pos_result=2;
 steer_pos_left_result=3;steer_pos_right_result=3;fake_nc=0x06;
 fake_ms+=20;Voice_ExecuteMotorControl();assert(voice_rotate_stage==2);
 fake_ms+=20;Voice_ExecuteMotorControl();
 assert(fake_spin_active&&fake_spin_dir==-1&&fake_spin_duty==250);
 uint32_t spin_start=voice_execution_start_tick;
 voice_speed_mode=VC02_SPEED_FULL;
 fake_ms+=20;Voice_ExecuteMotorControl();
 assert(fake_spin_active&&fake_spin_dir==-1&&fake_spin_duty==400&&
        voice_execution_start_tick==spin_start);
 /* NC release remains an immediate emergency stop after speed change. */
 fake_nc=0x02;fake_ms+=20;Voice_ExecuteMotorControl();
 assert(!voice_execution_active&&!fake_spin_active&&fake_emergency>0);
 reset();voice_pos_enable=1;voice_pivot_enable=1;
 request(VOICE_MOVE_ROTATE_RIGHT,5,10);
 voice_pos_request=0;steer_pos_result=2;
 steer_pos_left_result=3;steer_pos_right_result=3;fake_nc=0x06;
 Voice_ExecuteMotorControl();assert(voice_rotate_stage==2);
 fake_ms+=20;Voice_ExecuteMotorControl();assert(fake_spin_dir==1&&fake_spin_duty==400);
 voice_speed_mode=VC02_SPEED_VERY_SLOW;
 fake_ms+=20;Voice_ExecuteMotorControl();
 assert(fake_spin_dir==1&&fake_spin_duty==250&&voice_rotate_stage==2);
 /* No active Voice => speed configuration alone cannot start wheels. */
 Voice_S154CancelAll();fake_wheel_pwm=0;
 voice_speed_mode=VC02_SPEED_FULL;fake_ms+=20;Voice_ExecuteMotorControl();
 assert(fake_wheel_pwm==0&&!voice_execution_active);
 puts("PASS real VC02 Voice executor: LIVE C1..C5 updates forward/backward/left+right spin targets without TIME reset; R2/NC/STOP and cap preserved");
}
"""
CC=shutil.which("gcc") or shutil.which("cc")
if not CC:raise SystemExit("Missing HOST GCC")
with TemporaryDirectory() as td:
 f=Path(td)/"exec_test.c";f.write_text(before+"\n\n"+actual+"\n\n"+after,encoding="utf8")
 out=Path(td)/"exec_test"
 cmd=[CC,"-std=c11","-Wall","-Wextra","-Werror","-Wno-unused-function",
     "-I",str(ROOT/"Core/Inc"),str(f),"-o",str(out)]
 p=subprocess.run(cmd,capture_output=True,text=True)
 if p.returncode:raise SystemExit("HOST compile failed\n"+p.stdout+p.stderr)
 p=subprocess.run([str(out)],capture_output=True,text=True)
 if p.returncode:raise SystemExit("HOST runtime failed\n"+p.stdout+p.stderr)
 print(p.stdout.strip())
print("HOST executor C only. MCU ISR, CubeIDE build/flash and powered spin NOT proven.")
