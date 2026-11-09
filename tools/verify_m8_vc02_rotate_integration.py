#!/usr/bin/env python3
"""Actual Voice executor + arbiter + PS2 ownership + wheel spin admission C.
HAL, steering completion and measured feedback are simulated; wheel spin
accept/refuse and target APIs are extracted unchanged from wheel_control.c.
This catches the old one-refusal cancellation that an always-accept mock missed.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import ast, shutil, subprocess
ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/'Core/Src/main.c').read_text()
WHEEL=(ROOT/'Core/Src/wheel_control.c').read_text()
def extract(src, name):
    i=src.index(name); b=src.index('{',i); depth=0
    for j in range(b,len(src)):
        if src[j]=='{': depth+=1
        elif src[j]=='}':
            depth-=1
            if depth==0:return src[i:j+1]
    raise ValueError(name)
# Reuse inert platform/type declarations, never execute the other test.
test=ROOT/'tools/verify_m8_vc02_executor.py'
parsed=ast.parse(test.read_text())
before=next(ast.literal_eval(n.value) for n in parsed.body
            if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='before' for t in n.targets))
for name in ['static void WheelControl_SetTargetPWM(',
             'static void WheelControl_SetSpinDirection(',
             'static uint8_t WheelControl_SpinIsActive(']:
    before=before.replace(extract(before,name),'')
extra=r'''
#define WC_PID_ZERO_EPS_RPM 0.01f
static int16_t wc_pwm_max=999;
static uint8_t wheel_pid_enable=1,wheel_ff_test_active=0;
static uint8_t wheel_encoder_available_lu=1,wheel_encoder_available_ld=1;
static uint8_t wheel_encoder_available_ru=1,wheel_encoder_available_rd=1;
static int16_t wheel_target_pwm=0,wheel_pwm_lu=0,wheel_pwm_ld=0,wheel_pwm_ru=0,wheel_pwm_rd=0;
static float wheel_pid_target_rpm=0;
static uint8_t wheel_spin_active=0;
static int8_t wheel_spin_direction=0;
static uint32_t wheel_spin_deferred_count=0,wheel_spin_start_count=0,wc_spin_zero_tick=0;
static int wheel_command_profile=0;
typedef struct {float current_rpm;} MotorFeedback_t;
static MotorFeedback_t motors[4];
static MotorFeedback_t *wc_motor_lu=&motors[0],*wc_motor_ld=&motors[1];
static MotorFeedback_t *wc_motor_ru=&motors[2],*wc_motor_rd=&motors[3];
static void WC_ResetAllPID(void){}
typedef enum {CONTROL_MODE_IDLE=0,CONTROL_MODE_VOICE,CONTROL_MODE_PS2,CONTROL_MODE_STOP} ControlMode_t;
static ControlMode_t control_mode=CONTROL_MODE_IDLE;
static uint8_t ps2_manual_active=0,ps2_cancel_hold=0,ps2_spin_rf_latched=0;
static uint32_t ps2_cancel_stop_count=0,control_stop_count=0,voice_cancel_by_ps2_count=0;
static uint8_t steer_test_running=0;
static unsigned manual_calls=0;
#define PS2_LY_FORWARD_LIMIT 118
#define PS2_LY_REVERSE_LIMIT 138
static struct {uint8_t Connected,Mode,LY,LX,RY,Up,Down,Left,Right,L1,L2,R1,R2,
 Select,L3,R3,Triangle,Circle,Cross,Square;} PS2_Pad;
static int Steering_ManualRequestDirection(void){return 0;}
static uint8_t Steering_CalInputsNeutral(void){return 1;}
static void PS2_ApplyManualControl(void){manual_calls++;}
'''
before=before.replace('static uint32_t HAL_GetTick',extra+'\nstatic uint32_t HAL_GetTick',1)
before=before.replace('fake_emergency++;fake_spin_active=0;fake_spin_dir=0;fake_wheel_pwm=0;',
    'fake_emergency++;wheel_spin_active=0;wheel_spin_direction=0;wheel_target_pwm=0;')
before+=MAIN.split('/* M8_ROTATE_STATE_BEGIN:',1)[1].split('/* M8_ROTATE_STATE_END */',1)[0].split('*/',1)[1]
actual='\n'.join(extract(WHEEL,n) for n in ['static float WC_AbsFloat(',
    'void WheelControl_SetTargetPWM(','void WheelControl_SetSpinDirection(',
    'uint8_t WheelControl_SpinIsActive('])
actual+='\n'+'\n'.join(extract(MAIN,n) for n in [
    'static void Voice_ResetPendingCommand(','static uint8_t Voice_HasPendingData(',
    'static void Voice_RotateCapture(','static void Voice_S154CancelAll(',
    'static void Voice_CancelByPS2(','static void Control_RequestStop(',
    'static uint16_t Voice_SpeedLevelToPWM(',
    'static uint8_t Voice_IsOpenLoopCommandSupported(',
    'static void Voice_StartExecution(','static void Voice_ExecuteMotorControl(',
    'static uint8_t PS2_IsManualControlActive(','static void Control_Arbitrate('])
after=r'''
static void reset(void){
 PS2_Pad=(__typeof__(PS2_Pad)){.Connected=1,.Mode=0x73,.LY=128,.LX=128,.RY=128};
 control_mode=CONTROL_MODE_IDLE;control_stop_latched=ps2_cancel_hold=ps2_spin_rf_latched=0;
 steer_pos_active=steer_manual_active=steer_cal_active=0;
 voice_execution_active=voice_command_ready=voice_pos_owned=voice_pos_request=0;
 voice_rotate_stage=voice_rotate_spin_started=voice_rotate_result=voice_rotate_diag_pending=0;
 voice_pos_rejected=steer_safety_trip_latched=0;steer_safety_motion_permit=1;
 voice_pivot_enable=voice_pos_enable=1;fake_ms=1000;fake_nc=0;fake_wheels_stopped=1;
 voice_speed_mode=3;voice_execution_start_tick=0;
 wheel_spin_active=0;wheel_spin_direction=0;wheel_target_pwm=0;
 wheel_pid_target_rpm=0;wheel_pid_enable=1;wheel_ff_test_active=0;
 wheel_encoder_available_lu=wheel_encoder_available_ld=wheel_encoder_available_ru=wheel_encoder_available_rd=1;
 wheel_pwm_lu=wheel_pwm_ld=wheel_pwm_ru=wheel_pwm_rd=0;
 for(unsigned i=0;i<4;i++)motors[i].current_rpm=0;
 fake_emergency=0;manual_calls=0;Voice_ResetPendingCommand();
}
static void request(int dir,unsigned sec){
 voice_completed_command=(VoiceVehicleCommand_t){
 .movement=(dir<0)?VOICE_MOVE_ROTATE:VOICE_MOVE_ROTATE_RIGHT,
 .speed_level=3,.duration_sec=sec};voice_command_ready=1;Control_Arbitrate();
 assert(voice_rotate_stage==1&&voice_pos_request==2&&wheel_target_pwm==0);
}
static void r2_done(void){
 fake_ms+=1500;voice_pos_request=voice_pos_owned=0;steer_pos_result=2;
 steer_pos_left_result=steer_pos_right_result=3;fake_nc=6;Control_Arbitrate();
 assert(voice_rotate_stage==2&&voice_rotate_spin_started==0);
 assert(voice_execution_start_tick==0&&voice_execution_elapsed_ms==0);
}
int main(void){
 for(int dir=-1;dir<=1;dir+=2)for(unsigned sec=5;sec<=120;sec+=(sec==5?5:110)){
  reset();request(dir,sec);r2_done();
  /* A measured feedback transient AFTER R2: real wheel admission refuses.
   * Old executor canceled here; holding L1/L2 kept retrying this exact API.
   */
  motors[0].current_rpm=3.0f;
  fake_ms+=5;Control_Arbitrate();
  assert(voice_execution_active&&voice_rotate_stage==2&&!wheel_spin_active);
  assert(voice_rotate_deferred_count==1&&voice_execution_elapsed_ms==0);
  assert(voice_execution_start_tick==0);
  fake_ms+=300;Control_Arbitrate();assert(voice_execution_active&&!wheel_spin_active);
  motors[0].current_rpm=0;fake_ms+=5;Control_Arbitrate();
  assert(wheel_spin_active&&wheel_spin_direction==dir&&wheel_target_pwm==dir*400);
  uint32_t start=fake_ms;assert(voice_execution_start_tick==start);
  for(unsigned elapsed=5;elapsed<sec*1000;elapsed+=5){
   fake_ms=start+elapsed;Control_Arbitrate();
   assert(voice_execution_active&&wheel_spin_active&&wheel_spin_direction==dir);
   assert(control_mode==CONTROL_MODE_VOICE&&voice_execution_start_tick==start);
  }
  fake_ms=start+sec*1000;Control_Arbitrate();
  assert(!voice_execution_active&&wheel_target_pwm==0&&wheel_spin_direction==0);
  assert(voice_rotate_diag.result==1&&voice_rotate_diag.spin_ms==sec*1000);
  assert(voice_rotate_diag.r2_ms==1500&&voice_rotate_diag.wait_ms==310);
 }
 /* Real PID setpoint must be <=0.01, even though steering stationary gate
  * accepts <=0.5. Wait until the existing wheel shaper has settled to zero.
  */
 reset();request(-1,5);r2_done();wheel_pid_target_rpm=0.1f;
 fake_ms+=5;Control_Arbitrate();assert(voice_execution_active&&!wheel_spin_active);
 wheel_pid_target_rpm=0;fake_ms+=5;Control_Arbitrate();assert(wheel_spin_active);
 uint32_t start=fake_ms;voice_speed_mode=1;fake_ms+=5;Control_Arbitrate();
 assert(wheel_target_pwm==-250&&voice_execution_start_tick==start);
 voice_speed_mode=5;fake_ms+=5;Control_Arbitrate();assert(wheel_target_pwm==-400);
 /* Neutral connected pad is not a takeover of Voice's own spin. */
 assert(!PS2_IsManualControlActive());
 /* Physical pad input cancels once, no manual move, then neutral rearm. */
 PS2_Pad.L1=1;fake_ms+=5;Control_Arbitrate();
 assert(!voice_execution_active&&ps2_cancel_hold&&fake_emergency==1&&manual_calls==0);
 assert(voice_rotate_diag.result==8);
 Control_Arbitrate();assert(manual_calls==0);
 PS2_Pad.L1=0;Control_Arbitrate();assert(!ps2_cancel_hold);
 PS2_Pad.L1=1;Control_Arbitrate();assert(manual_calls==1);
 /* Permanent refusal is bounded, no falsely counted spin duration. */
 reset();request(1,5);r2_done();wheel_encoder_available_lu=0;
 fake_ms+=5;Control_Arbitrate();assert(voice_execution_active);
 fake_ms+=1995;Control_Arbitrate();
 assert(!voice_execution_active&&!wheel_spin_active&&voice_rotate_diag.result==6);
 assert(voice_rotate_diag.spin_ms==0&&voice_rotate_diag.started==0);
 /* NC release before acceptance and during spin cuts immediately. */
 for(unsigned started=0;started<=1;started++){
  reset();request(-1,5);r2_done();
  if(started){fake_ms+=5;Control_Arbitrate();assert(wheel_spin_active);}
  fake_nc=2;fake_ms+=5;Control_Arbitrate();
  assert(!voice_execution_active&&!wheel_spin_active&&fake_emergency==1);
  assert(voice_rotate_diag.result==5);
 }
 /* Removing the master motion permit cuts pending or active spin. */
 for(unsigned started=0;started<=1;started++){
  reset();request(1,5);r2_done();
  if(started){fake_ms+=5;Control_Arbitrate();}
  steer_safety_motion_permit=0;fake_ms+=5;Control_Arbitrate();
  assert(!voice_execution_active&&!wheel_spin_active&&fake_emergency==1);
  assert(voice_rotate_diag.result==5);
 }
 /* STOP while pending / active never retries or resumes. */
 for(unsigned started=0;started<=1;started++){
  reset();request(1,5);r2_done();
  if(started){fake_ms+=5;Control_Arbitrate();}
  Control_RequestStop();fake_ms+=5;Control_Arbitrate();
  assert(!voice_execution_active&&wheel_target_pwm==0&&wheel_spin_direction==0);
  assert(voice_rotate_diag.result==9);
 }
 /* Lost real spin ownership must cancel; never restart a running timer. */
 reset();request(1,5);r2_done();fake_ms+=5;Control_Arbitrate();
 wheel_spin_active=0;wheel_target_pwm=0;fake_ms+=5;Control_Arbitrate();
 assert(!voice_execution_active&&!wheel_spin_active&&voice_rotate_diag.result==7);
 /* Preserve an unsent prior record; count a dropped later event. */
 VoiceRotateDiag_t saved=voice_rotate_diag;
 voice_rotate_stage=2;Voice_RotateCapture(8);
 assert(voice_rotate_diag.seq==saved.seq&&voice_rotate_diag.result==saved.result);
 assert(voice_rotate_diag_dropped>0);voice_rotate_stage=0;
 /* R2 has its own failure deadline and must never start wheel spin. */
 reset();request(-1,5);fake_ms+=6501;Control_Arbitrate();
 assert(!voice_execution_active&&!wheel_spin_active&&voice_rotate_diag.result==3);
 /* Contradictory MAX while at MIN fails the existing physical gate. */
 reset();request(-1,5);r2_done();fake_nc=14;fake_ms+=5;Control_Arbitrate();
 assert(!voice_execution_active&&!wheel_spin_active&&voice_rotate_diag.result==5);
 /* Unsigned clock rollover: full requested duration still required. */
 reset();fake_ms=0xfffff800U;request(-1,5);r2_done();fake_ms+=5;Control_Arbitrate();
 start=fake_ms;fake_ms=start+4999;Control_Arbitrate();assert(voice_execution_active);
 fake_ms=start+5000;Control_Arbitrate();assert(!voice_execution_active&&voice_rotate_diag.spin_ms==5000);
 puts("PASS actual C integration: LEFT/RIGHT 5/10/120s; real spin deferral; R2/wait excluded; live speed; neutral ownership; PS2/FF/NC; timeout; lost owner; tick rollover");
}
'''
# The automatic result line must remain under the stopped, nonblocking
# UART owner. Capture never prints or writes actuators from the motion path.
emitter=MAIN.split('if ((voice_rotate_diag_pending != 0U)',1)[1].split('/* Automatic Voice RIGHT fine summary',1)[0]
for guard in ['voice_execution_active == 0U','voice_pos_request == 0U',
              'voice_pos_owned == 0U','steer_pos_active == 0U',
              'steer_manual_active == 0U','steer_cal_active == 0U',
              'steer_test_running == 0U','Steering_PositionWheelsStopped() != 0U',
              'huart3.gState == HAL_UART_STATE_READY','M8S11_SendLine(n)']:
    assert guard in emitter,guard
assert emitter.count('__HAL_TIM_GET_COMPARE')==2
assert 'HAL_UART_Transmit(' not in emitter and 'SetTargetPWM' not in emitter
assert 'HAL_UART' not in extract(MAIN,'static void Voice_RotateCapture(')
cc=shutil.which('gcc') or shutil.which('cc')
if not cc:raise SystemExit('HOST GCC required')
with TemporaryDirectory() as td:
    src=Path(td)/'rotate.c';src.write_text(before+'\n'+actual+'\n'+after)
    binary=Path(td)/'rotate'
    subprocess.run([cc,'-std=gnu11','-Wall','-Wextra','-Werror','-Wno-unused-function',
        '-Wno-unused-variable','-I',str(ROOT/'Core/Inc'),str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('HOST integration only: no motor hardware / CubeIDE build / Flash claim.')
