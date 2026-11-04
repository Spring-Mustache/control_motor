#!/usr/bin/env python3
"""Compile actual production auto-ARM and PS2 input C using inert HOST mocks.
Tests automatic retry of temporary faults, START neutral/no toggle,
sensor/NC gating, RF disarm and latched fault isolation. Not hardware proof.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess,shutil
ROOT=Path(__file__).resolve().parents[1]
SRC=(ROOT/"Core/Src/main.c").read_text(encoding="utf-8")
def extract(signature):
 i=SRC.index(signature); p=SRC.index("{",i); d=0
 for j in range(p,len(SRC)):
  if SRC[j]=="{":d+=1
  elif SRC[j]=="}":
   d-=1
   if d==0:return SRC[i:j+1]
 raise ValueError(signature)
names=["static uint8_t Steering_CalInputsNeutral(",
       "static void Steering_PS2ManualDisarm(",
       "static void Steering_PS2ManualArmUpdate(",
       "static uint8_t PS2_IsManualControlActive("]
actual="\n\n".join(extract(s) for s in names)
assert "start_now" not in extract("static void Steering_PS2ManualArmUpdate(")
assert "Steering_PositionReadOne(1U" in extract("static void Steering_PS2ManualArmUpdate(")
assert "Steering_PositionReadOne(0U" in extract("static void Steering_PS2ManualArmUpdate(")
assert "Steering_PositionWheelsStopped()" in extract("static void Steering_PS2ManualArmUpdate(")
assert "(PS2_Pad.Start != 0U)" not in extract("static uint8_t PS2_IsManualControlActive(")
assert "RamLogger_ButtonUpdate();" in SRC
assert "TODO(next commit): map PS2 START to logging" in SRC

before=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define PS2_LY_FORWARD_LIMIT 118U
#define PS2_LY_REVERSE_LIMIT 138U
#define STEER_PS2_MANUAL_ARM_HOLD_MS 1500U
#define VOICE_MOVE_ROTATE 3U
#define VOICE_MOVE_ROTATE_RIGHT 4U
typedef struct {
 uint8_t Connected,Mode,Select,L3,R3,Triangle,Circle,Cross,Square;
 uint8_t R1,R2,L1,L2,Start,Up,Down,Left,Right,LY,LX,RY,RX;
} PS2PadMock_t;
static PS2PadMock_t PS2_Pad={0};
typedef struct{uint8_t initialized;} LimitMock_t;
static LimitMock_t debug_limit_r_max={1},debug_limit_r_min={1};
static LimitMock_t debug_limit_l_max={1},debug_limit_l_min={1};
static volatile uint8_t debug_limit_active_mask=0,debug_limit_raw_mask=0;
static volatile uint8_t steer_safety_motion_permit=0,steer_safety_trip_latched=0;
static volatile uint8_t steer_manual_enable=0,steer_pos_enable=0;
static volatile uint8_t steer_safety_neutral_required=1,steer_manual_neutral_required=1;
static volatile uint8_t steer_manual_active=0,steer_pos_active=0,steer_cal_active=0;
static volatile uint8_t steer_test_running=0,steer_test_arm=0,steer_test_request=0;
static volatile uint8_t debug_as5600_poll_enable=0;
static volatile uint8_t control_stop_latched=0,ps2_cancel_hold=0;
static volatile uint8_t ps2_spin_rf_latched=0,voice_execution_active=0,voice_command_ready=0;
static volatile uint8_t voice_pos_owned=0,voice_rotate_stage=0;
static volatile uint8_t steer_ps2_manual_armed=0,steer_ps2_manual_arm_status=0;
static volatile uint32_t steer_ps2_manual_arm_hold_elapsed_ms=0;
static volatile uint32_t steer_ps2_manual_arm_success_count=0;
static volatile uint32_t steer_ps2_manual_disarm_count=0;
static volatile uint32_t steer_ps2_manual_arm_reject_count=0;
static volatile uint8_t steer_ps2_arm_sensor_fail_mask=0;
static volatile uint8_t steer_ps2_arm_left_magnet_status=0,steer_ps2_arm_right_magnet_status=0;
static volatile uint8_t steer_ps2_arm_left_read_status=0,steer_ps2_arm_right_read_status=0;
static volatile uint16_t steer_ps2_arm_left_raw=0,steer_ps2_arm_right_raw=0;
static volatile uint32_t steer_ps2_arm_sensor_fail_count=0;
static volatile uint8_t debug_as5600_left_read_status=0,debug_as5600_right_read_status=0;
static volatile uint8_t debug_as5600_left_status=0x20,debug_as5600_right_status=0x20;
static volatile uint8_t debug_as5600_left_comm_ok=1,debug_as5600_right_comm_ok=1;
static volatile uint16_t debug_as5600_left_raw=1800,debug_as5600_right_raw=700;
static uint8_t left_good=1,right_good=1,left_env=1,right_env=1,wheels_stopped=1;
static uint8_t spin_active=0;
static uint32_t ticks=0,steer_stops=0,manual_stops=0,pos_stops=0;
static uint8_t raw_mask=0;
static uint32_t HAL_GetTick(void){return ticks;}
static uint8_t Steering_TestRawLimitMask(void){return raw_mask;}
static uint8_t Steering_PositionWheelsStopped(void){return wheels_stopped;}
static void Steering_ManualStop(void){manual_stops++;steer_manual_active=0;}
static void Steering_PositionFinish(uint8_t code){
 assert(code==9U);pos_stops++;steer_pos_active=0;
}
static void Turn_Motors_Stop(void){steer_stops++;}
static uint8_t Steering_PositionReadOne(uint8_t left,uint16_t *raw){
 if(left){
  debug_as5600_left_comm_ok=left_good;
  debug_as5600_left_read_status=left_good?0:1;
  debug_as5600_left_status=left_good?0x20:0;
  if(left_good){*raw=1800;debug_as5600_left_raw=*raw;}
  return left_good;
 }
 debug_as5600_right_comm_ok=right_good;
 debug_as5600_right_read_status=right_good?0:1;
 debug_as5600_right_status=right_good?0x20:0;
 if(right_good){*raw=700;debug_as5600_right_raw=*raw;}
 return right_good;
}
static uint8_t AS5600_DebugReadingValid(uint8_t strict,uint8_t status){
 assert(strict==1U);return status==0x20;
}
static uint8_t Steering_SafetyEnvelopeWithNcDiscovery(uint8_t left,uint16_t raw,uint8_t nc){
 (void)raw;(void)nc;return left?left_env:right_env;
}
static int Steering_ManualRequestDirection(void){return 0;}
static uint8_t WheelControl_SpinIsActive(void){return spin_active;}
"""
after=r"""
static void neutral(void){
 PS2_Pad.Connected=1;PS2_Pad.Mode=0x73;
 PS2_Pad.Select=0;PS2_Pad.L3=0;PS2_Pad.R3=0;
 PS2_Pad.Triangle=0;PS2_Pad.Circle=0;PS2_Pad.Cross=0;PS2_Pad.Square=0;
 PS2_Pad.R1=0;PS2_Pad.R2=0;PS2_Pad.L1=0;PS2_Pad.L2=0;
 PS2_Pad.Start=0;PS2_Pad.Up=0;PS2_Pad.Down=0;
 PS2_Pad.Left=0;PS2_Pad.Right=0;PS2_Pad.LY=128;
 PS2_Pad.LX=128;PS2_Pad.RY=128;PS2_Pad.RX=128;
}
static void reset_flags(void){
 steer_safety_motion_permit=0;steer_safety_trip_latched=0;
 steer_manual_enable=0;steer_pos_enable=0;
 steer_safety_neutral_required=1;steer_manual_neutral_required=1;
 steer_manual_active=0;steer_pos_active=0;steer_cal_active=0;
 steer_test_running=0;steer_test_arm=0;steer_test_request=0;
 debug_as5600_poll_enable=0;control_stop_latched=0;ps2_cancel_hold=0;
 ps2_spin_rf_latched=0;voice_execution_active=0;voice_command_ready=0;
 steer_ps2_manual_armed=0;steer_ps2_manual_arm_status=0;
 steer_ps2_manual_arm_hold_elapsed_ms=0;steer_ps2_manual_arm_reject_count=0;
 steer_ps2_arm_sensor_fail_mask=0;steer_ps2_arm_sensor_fail_count=0;
 left_good=1;right_good=1;left_env=1;right_env=1;
 wheels_stopped=1;spin_active=0;raw_mask=0;
 debug_limit_active_mask=0;debug_limit_raw_mask=0;
 debug_limit_r_max.initialized=1;debug_limit_r_min.initialized=1;
 debug_limit_l_max.initialized=1;debug_limit_l_min.initialized=1;
 neutral();
}
static void tick(void){ticks+=500U;Steering_PS2ManualArmUpdate();}
int main(void){
 /* Boot requires no START press and never auto-starts an actuator. */
 reset_flags();tick();
 assert(steer_ps2_manual_armed==1&&steer_safety_motion_permit==1&&
        steer_manual_enable==1&&steer_pos_enable==1&&
        steer_ps2_manual_arm_status==2&&steer_stops==0&&
        PS2_IsManualControlActive()==0);
 /* START no longer toggles ARM or classifies as Voice takeover. */
 PS2_Pad.Start=1;
 Steering_PS2ManualArmUpdate();
 assert(steer_ps2_manual_armed==1&&steer_safety_motion_permit==1&&
        PS2_IsManualControlActive()==0);
 PS2_Pad.Start=0;

 /* START alone cannot acquire ARM while not neutral; release is enough
    to permit automatic retry without START hold or other button. */
 reset_flags();PS2_Pad.Start=1;tick();
 assert(steer_ps2_manual_armed==0&&steer_ps2_manual_arm_status==5);
 PS2_Pad.Start=0;tick();assert(steer_ps2_manual_armed==1);

 /* Transient left AS5600 bus error auto-recovers after 500ms. */
 reset_flags();left_good=0;tick();
 assert(steer_ps2_manual_armed==0&&steer_ps2_manual_arm_status==9);
 left_good=1;tick();
 assert(steer_ps2_manual_armed==1&&steer_safety_motion_permit==1);

 /* Physical angle envelope is also required before grant. */
 reset_flags();right_env=0;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_arm_sensor_fail_mask!=0);
 right_env=1;tick();assert(steer_ps2_manual_armed==1);

 /* Contradictory NC and wheel RPM block; recovery needs no key. */
 reset_flags();raw_mask=0x03U;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_manual_arm_status==6);
 raw_mask=0;wheels_stopped=0;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_manual_arm_status==8);
 wheels_stopped=1;tick();assert(steer_ps2_manual_armed==1);

 /* Missing PS2 and reconnect with NEW neutral frame: auto-arm. */
 reset_flags();PS2_Pad.Connected=0;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_manual_arm_status==4);
 neutral();tick();assert(steer_ps2_manual_armed==1);
 PS2_Pad.Connected=0;tick();
 assert(steer_ps2_manual_armed==0&&steer_safety_motion_permit==0&&
        steer_manual_enable==0&&steer_pos_enable==0);
 neutral();tick();assert(steer_ps2_manual_armed==1);

 /* No auto grant during STOP or a currently executing Voice command. */
 reset_flags();control_stop_latched=1;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_manual_arm_status==7);
 control_stop_latched=0;voice_execution_active=1;tick();
 assert(!steer_ps2_manual_armed&&steer_ps2_manual_arm_status==7);
 voice_execution_active=0;tick();assert(steer_ps2_manual_armed==1);

 /* Hard safety-trip latch remains faulted, even when sensors look OK;
    this is not a transient preflight reject and must not be bypassed. */
 reset_flags();steer_safety_trip_latched=1;tick();
 assert(!steer_ps2_manual_armed&&steer_safety_motion_permit==0);
 tick();assert(!steer_ps2_manual_armed);
 reset_flags();tick();assert(steer_ps2_manual_armed==1);

 puts("PASS actual production C: automatic ARM without START, self-retry on transient NC/AS5600/neutral/wheels/PS2 recovery, static safety-trip preserved");
 return 0;
}
"""
cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("HOST GCC unavailable")
with TemporaryDirectory() as d:
 p=Path(d)/"test.c"
 p.write_text(before+"\n\n"+actual+"\n\n"+after,encoding="utf-8")
 out=Path(d)/"auto_arm"
 cmd=[cc,"-std=c11","-Wall","-Wextra","-Werror","-Wno-unused-function",str(p),"-o",str(out)]
 built=subprocess.run(cmd,capture_output=True,text=True)
 if built.returncode:raise SystemExit("HOST compile failed:\n"+built.stderr)
 run=subprocess.run([str(out)],capture_output=True,text=True)
 if run.returncode:raise SystemExit("HOST runtime failed:\n"+run.stderr+run.stdout)
 print(run.stdout.strip())
print("HOST C only. STM32 target build/flash/physical motor test not proven.")
