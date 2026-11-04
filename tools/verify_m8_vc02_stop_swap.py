#!/usr/bin/env python3
"""Compile ACTUAL main.c stop/arbitration functions against inert HOST mocks.
Confirms FF soft wheel ramp vs PS2 immediate emergency cut.
Not a CubeIDE build/flash, physical brake or safety certification.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil, subprocess

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
WHEEL=(ROOT/"Core/Src/wheel_control.c").read_text(encoding="utf-8")
def extract(src, signature):
    i=src.index(signature)
    brace=src.index("{",i)
    depth=0
    for j in range(brace,len(src)):
        if src[j]=="{":depth+=1
        elif src[j]=="}":
            depth-=1
            if depth==0:return src[i:j+1]
    raise RuntimeError("Unbalanced "+signature)

names=[
    "static void Voice_S154CancelAll(",
    "static void Voice_CancelByPS2(",
    "static void Control_RequestStop(",
    "static void Control_Arbitrate("
]
actual="\n\n".join(extract(MAIN,n) for n in names)
assert "Wheel_EmergencyStop();" not in extract(MAIN,"static void Control_RequestStop(")
assert "WheelControl_SetTargetPWM(0);" in extract(MAIN,"static void Voice_S154CancelAll(")
assert "Wheel_EmergencyStop();" in extract(MAIN,"static void Control_Arbitrate(")
assert "WheelControl_EmergencyStop();" in extract(MAIN,"static void Wheel_EmergencyStop(")
assert "WC_FixedSCurveReset();" in extract(WHEEL,"void WheelControl_EmergencyStop(")
assert "steer_safety_trip_latched != 0U" in MAIN

before=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef enum {
 CONTROL_MODE_IDLE=0, CONTROL_MODE_VOICE=1,
 CONTROL_MODE_PS2=2, CONTROL_MODE_STOP=3
} ControlMode_t;
typedef enum {VOICE_BUILD_RESULT_NONE=0} VoiceBuildResult_t;
typedef struct { uint8_t Connected,Mode; } PS2Mock_t;
static PS2Mock_t PS2_Pad={1U,0x73U};
static volatile ControlMode_t control_mode=CONTROL_MODE_IDLE;
static volatile uint8_t ps2_manual_active=0,ps2_spin_rf_latched=0;
static volatile uint8_t ps2_cancel_hold=0,control_stop_latched=0;
static volatile uint8_t voice_execution_active=0,voice_command_ready=0;
static volatile uint8_t voice_pos_request=0,voice_pos_owned=0;
static volatile uint8_t voice_command_complete=0,voice_rotate_stage=0;
static volatile int8_t voice_rotate_direction=0;
static volatile uint8_t steer_pos_active=0,steer_manual_active=0,steer_cal_active=0;
static volatile VoiceBuildResult_t voice_last_build_result=VOICE_BUILD_RESULT_NONE;
static volatile int16_t voice_execution_pwm=0;
static volatile uint32_t voice_cancel_by_ps2_count=0;
static volatile uint32_t ps2_cancel_stop_count=0,control_stop_count=0;
static uint8_t ps2_input=0,valid_neutral=1,wheel_stopped=0,spin_active=0;
static int applied_pwm=500,target_pwm=500;
static unsigned emergency_calls=0,manual_calls=0,steer_stop_calls=0;
static unsigned pending_clear_count=0,steer_finish_count=0;
static int8_t spin_direction=0;
static uint8_t pending_data=0;
static uint8_t PS2_IsManualControlActive(void){return ps2_input;}
static uint8_t Voice_HasPendingData(void){return pending_data;}
static void Voice_ResetPendingCommand(void){pending_data=0;pending_clear_count++;}
static void Steering_PositionFinish(uint8_t code){
 assert(code==9U);steer_finish_count++;steer_pos_active=0;voice_pos_owned=0;
}
static void WheelControl_SetSpinDirection(int8_t dir,int16_t pwm){
 (void)pwm;spin_direction=dir;
}
static void WheelControl_SetTargetPWM(int16_t pwm){target_pwm=pwm;}
static void Turn_Motors_Stop(void){steer_stop_calls++;}
static void Wheel_EmergencyStop(void){
 emergency_calls++;applied_pwm=0;target_pwm=0;spin_active=0;spin_direction=0;
 Turn_Motors_Stop();
}
static uint8_t Steering_PositionWheelsStopped(void){return wheel_stopped;}
static uint8_t Steering_CalInputsNeutral(void){return valid_neutral;}
static uint8_t WheelControl_SpinIsActive(void){return spin_active;}
static void PS2_ApplyManualControl(void){manual_calls++;}
static void Voice_StartExecution(void){voice_execution_active=1;voice_command_ready=0;}
static void Voice_ExecuteMotorControl(void){}
"""
after=r"""
static void reset(void){
 control_mode=CONTROL_MODE_IDLE;ps2_manual_active=0;ps2_spin_rf_latched=0;
 ps2_cancel_hold=0;control_stop_latched=0;voice_execution_active=0;
 voice_command_ready=0;voice_pos_request=0;voice_pos_owned=0;
 voice_command_complete=0;voice_rotate_stage=0;voice_rotate_direction=0;
 steer_pos_active=0;steer_manual_active=0;steer_cal_active=0;
 voice_execution_pwm=0;voice_cancel_by_ps2_count=0;
 ps2_cancel_stop_count=0;control_stop_count=0;
 ps2_input=0;valid_neutral=1;wheel_stopped=0;spin_active=0;
 applied_pwm=500;target_pwm=500;emergency_calls=0;manual_calls=0;
 steer_stop_calls=0;pending_clear_count=0;steer_finish_count=0;
 spin_direction=0;pending_data=0;PS2_Pad.Connected=1;PS2_Pad.Mode=0x73U;
}
int main(void){
 /* Voice FF cancels both groups, requests a SOFT wheel zero target. */
 reset();voice_execution_active=1;voice_pos_owned=1;steer_pos_active=1;
 Control_RequestStop();
 assert(control_stop_latched==1&&control_stop_count==1&&
        voice_execution_active==0&&voice_pos_owned==0&&
        steer_finish_count==1&&target_pwm==0&&applied_pwm==500&&
        emergency_calls==0);
 Control_Arbitrate();
 assert(control_mode==CONTROL_MODE_STOP&&control_stop_latched==1&&
        emergency_calls==0&&applied_pwm==500&&target_pwm==0);
 /* Keep STOP priority until wheel-stop sensor checks pass. */
 Control_Arbitrate();assert(control_stop_latched==1&&emergency_calls==0);
 wheel_stopped=1;Control_Arbitrate();
 assert(control_stop_latched==0&&emergency_calls==0);

 /* PS2 first input with active Voice is a hard cut; no manual movement.
  * Holding the same button cannot drive the wheels.
  */
 reset();voice_execution_active=1;voice_pos_owned=1;steer_pos_active=1;
 ps2_input=1;valid_neutral=0;Control_Arbitrate();
 assert(voice_execution_active==0&&voice_pos_owned==0&&
        ps2_cancel_hold==1&&ps2_cancel_stop_count==1&&
        emergency_calls==1&&applied_pwm==0&&manual_calls==0);
 Control_Arbitrate();assert(ps2_cancel_hold==1&&manual_calls==0);
 ps2_input=0;valid_neutral=0;Control_Arbitrate();
 assert(ps2_cancel_hold==1&&manual_calls==0);
 valid_neutral=1;Control_Arbitrate();
 assert(ps2_cancel_hold==0&&manual_calls==0);
 ps2_input=1;Control_Arbitrate();assert(manual_calls==1);

 /* PS2 during voice FF soft coasting escalates to immediate hard cut. */
 reset();voice_execution_active=1;Control_RequestStop();
 assert(control_stop_latched==1&&applied_pwm==500);
 ps2_input=1;valid_neutral=0;Control_Arbitrate();
 assert(emergency_calls==1&&applied_pwm==0&&
        control_stop_latched==0&&ps2_cancel_hold==1&&manual_calls==0);
 Control_Arbitrate();assert(manual_calls==0&&ps2_cancel_hold==1);

 /* Spin owner receives zero-speed request; its controlled braking is
  * preserved, but PS2 RF loss during coast triggers emergency cut.
  */
 reset();spin_active=1;spin_direction=-1;voice_execution_active=1;
 Control_RequestStop();Control_Arbitrate();
 assert(emergency_calls==0&&spin_direction==0&&target_pwm==0&&
        applied_pwm==500);
 PS2_Pad.Connected=0;Control_Arbitrate();
 assert(emergency_calls==1&&applied_pwm==0&&ps2_spin_rf_latched==1);

 /* Real hardware fault path stays wired to the existing emergency API. */
 reset();Wheel_EmergencyStop();
 assert(emergency_calls==1&&applied_pwm==0);
 puts("PASS actual C: Voice FF soft brake, PS2 hard cut, PS2 after FF coast, RF hard cut, neutral rearm");
 return 0;
}
"""
cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("Missing HOST gcc/cc")
with TemporaryDirectory() as d:
    p=Path(d)/"stop_swap.c"
    p.write_text(before+"\n"+actual+"\n"+after,encoding="utf-8")
    out=Path(d)/"stop_swap"
    cmd=[cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",
         "-Wno-unused-function",str(p),"-o",str(out)]
    built=subprocess.run(cmd,encoding="utf-8",text=True,capture_output=True)
    if built.returncode:raise SystemExit("COMPILE FAIL:\n"+built.stdout+built.stderr)
    ran=subprocess.run([str(out)],encoding="utf-8",text=True,capture_output=True)
    if ran.returncode:raise SystemExit("RUNTIME FAIL:\n"+ran.stdout+ran.stderr)
    print(ran.stdout.strip())
print("HOST C only. No MCU build, no flash, no physical stop validation.")
