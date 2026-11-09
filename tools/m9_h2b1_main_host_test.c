/* HOST ONLY: production main.inc and extracted main.c ownership hooks.
 * Synthetic PWM/gains here are NOT a proposal for the real motors. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "m9_h2b1_hold.h"
#define GPIO_PIN_5 (1U<<5)
#define GPIO_PIN_8 (1U<<8)
#define GPIO_PIN_10 (1U<<10)
#define GPIO_PIN_11 (1U<<11)
#define TIM_CHANNEL_2 2U
#define TIM_CHANNEL_1 1U
#define STEER_SAFETY_INCIDENT_LOCKOUT 0U
#define CONTROL_MODE_STOP 3U
#define STEER_AXIS_DONE 4U
#define STEER_LIMIT_L_MIN_MASK 1U
#define STEER_LIMIT_L_MAX_MASK 2U
#define STEER_LIMIT_R_MIN_MASK 4U
#define STEER_LIMIT_R_MAX_MASK 8U
#define STEER_POS_LEFT_CENTER_RAW 1886U
#define STEER_POS_RIGHT_CENTER_RAW 662U
typedef unsigned GPIO_PinState;
#define GPIO_PIN_RESET 0U
#define GPIO_PIN_SET 1U
static int htim12,htim14,port_left,port_right;
#define MOTOR_DIR_TURN_left_GPIO_Port (&port_left)
#define MOTOR_DIR_TURN_right_GPIO_Port (&port_right)
#define MOTOR_DIR_TURN_left_Pin 14U
#define MOTOR_DIR_TURN_right_Pin 10U
static struct {uint32_t PR;} exti;
#define EXTI (&exti)
static uint32_t clock_ms,primask,dir_calls;
static uint16_t hardware_pwm[2];static GPIO_PinState hardware_dir[2];
static uint32_t HAL_GetTick(void){return clock_ms;}
static uint32_t __get_PRIMASK(void){return primask;}
static void __disable_irq(void){primask=1U;}
static void __enable_irq(void){primask=0U;}
static uint16_t get_compare(int *t,unsigned channel){(void)channel;return hardware_pwm[t==&htim12?0:1];}
static void set_compare(int *t,unsigned channel,uint16_t value){(void)channel;hardware_pwm[t==&htim12?0:1]=value;}
#define __HAL_TIM_GET_COMPARE(t,c) get_compare(t,c)
#define __HAL_TIM_SET_COMPARE(t,c,v) set_compare(t,c,v)
static void HAL_GPIO_WritePin(int *port,unsigned pin,GPIO_PinState level)
{
  unsigned axis=port==&port_left?0U:1U;(void)pin;
  assert(primask==1U && hardware_pwm[axis]==0U);
  hardware_dir[axis]=level;++dir_calls;
}
static struct {uint8_t Connected,Mode,R1,R2,Select,L1,L2;} PS2_Pad;
static uint8_t m9_h2b1_enable=1U,m9_h2b1_pending;
static M9B1Hold m9_h2b1_hold;
static volatile uint32_t m9_h2b1_nc_epoch;
static uint32_t m9_h2b1_seen_nc_epoch,m9_h2b1_sequence;
static uint32_t m9_h2b1_report_ms,m9_h2b1_dropped;
#if defined(M9B1_USE_TUNING_PROFILE)
#include "m9_h2b1_commissioning.h"
#else
static const M9B1Config m9_h2b1_config={
 {{20,0,512,64,1000,50},{15,0,512,64,1000,50}},8,4,5,32,250,100,90,250,
 0,0,0,M9B1_POWER_COMMISSIONED}; /* synthetic continuous fixture */
#endif
static uint16_t steer_pos_left_raw=1876,steer_pos_right_raw=652;
static uint32_t steer_pos_left_last_sample_ms,steer_pos_right_last_sample_ms;
static uint32_t steer_pos_left_new_sample_count,steer_pos_right_new_sample_count;
static uint8_t debug_as5600_left_comm_ok=1,debug_as5600_right_comm_ok=1;
static uint8_t debug_as5600_left_status=1,debug_as5600_right_status=1;
static uint8_t steer_safety_motion_permit=1,steer_safety_trip_latched;
static uint8_t control_stop_latched,control_mode,steer_safety_neutral_required;
static uint8_t ps2_cancel_hold,ps2_spin_rf_latched,debug_as5600_poll_enable;
static uint8_t steer_pos_active,steer_manual_active,steer_cal_active;
static uint8_t steer_test_running,steer_test_arm,steer_test_request;
static uint8_t voice_pos_request,voice_pos_owned,voice_rotate_stage,spin_active;
static uint8_t steer_pos_left_state,steer_pos_right_state;
static uint8_t steer_pos_left_result,steer_pos_right_result,steer_pos_command;
static uint8_t steer_pos_limit_trip_mask;
static uint8_t steer_pos_previous_buttons,raw_nc;
static uint16_t voice_pos_goal_l,voice_pos_goal_r;
static uint16_t steer_pos_left_pwm,steer_pos_right_pwm;
static uint16_t steer_manual_left_pwm,steer_manual_right_pwm;
static uint16_t steer_cal_left_pwm,steer_cal_right_pwm;
static int8_t manual_direction;
static uint8_t steer_voice_s154_active,steer_pos_escape_mask,steer_pos_result,voice_pos_last_result;
static uint32_t steer_pos_finish_count,steer_nc_local_stop_count;
static uint8_t steer_nc_local_stop_last_mask,steer_safety_expected_nc_mask;
static uint32_t steer_safety_expected_nc_edges;
static uint8_t steer_safety_pwm_initialized=1U;
static uint8_t AS5600_DebugReadingValid(uint8_t x,uint8_t status){(void)x;return status;}
static uint8_t Steering_TestRawLimitMask(void){return raw_nc;}
static uint8_t WheelControl_SpinIsActive(void){return spin_active;}
static int8_t Steering_ManualRequestDirection(void){return manual_direction;}
static uint8_t Steering_SafetyExpectedNcMask(void){return 0U;}
static uint8_t Steering_SafetyHardEnvelopeValid(uint8_t left,uint16_t raw)
{return left?(uint8_t)(raw>=1374U&&raw<=2297U):(uint8_t)(raw>=195U&&raw<=1029U);}
static void M9REAL_FinishForPosition(uint8_t r){(void)r;}
static void Steering_R1RightFineCapture(uint8_t r){(void)r;}
static void Steering_VoiceRightFineCapture(uint8_t r){(void)r;}
static void Steering_AngleWaypointReset(void){}
static void M8S00_CaptureStop(uint8_t a,uint8_t b,uint8_t c,uint8_t d)
{(void)a;(void)b;(void)c;(void)d;}
static void M8S00_ArrivalFinish(uint8_t r){(void)r;}
static void M9H0_StartAfterDone(uint8_t r)
{(void)r;assert(!hardware_pwm[0]&&!hardware_pwm[1]);}
static void M9H2_StartAfterDone(uint8_t r)
{(void)r;assert(!hardware_pwm[0]&&!hardware_pwm[1]);}
#include "m9_h2b1_main.inc"

/* PRODUCTION_FUNCTIONS */

static void sample_at(uint32_t ms)
{
 clock_ms=ms;steer_pos_left_last_sample_ms=ms;steer_pos_right_last_sample_ms=ms;
 ++steer_pos_left_new_sample_count;++steer_pos_right_new_sample_count;
}
static void begin_center(uint8_t source)
{
 steer_pos_active=1U;steer_pos_command=1U;
 steer_pos_left_state=steer_pos_right_state=STEER_AXIS_DONE;
 steer_pos_left_result=steer_pos_right_result=2U;
 voice_pos_owned=(source==2U);hardware_pwm[0]=7U;hardware_pwm[1]=8U;
 Steering_PositionFinish(2U);
 assert(!hardware_pwm[0]&&!hardware_pwm[1]&&m9_h2b1_hold.active);
 assert(!steer_pos_active && !voice_pos_owned && m9_h2b1_hold.source==source);
}
static void pass(uint32_t ms)
{
 sample_at(ms);M9B1_PreOwnerService();
 if(!M9B1_KeepOutput())Turn_Motors_Stop(); /* actual IDLE retention predicate */
 Steering_PositionLeftOff();Steering_PositionRightOff();
 Steering_ManualLeftOff();Steering_ManualRightOff();
 Steering_CalLeftOff();Steering_CalRightOff();M9B1_Update();
}
int main(void)
{
 uint32_t ms;
 PS2_Pad.Connected=1U;PS2_Pad.Mode=0x73U;sample_at(0U);
 M9B1_Update();
 assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
#if defined(M9B1_USE_TUNING_PROFILE)
 steer_pos_left_raw=1883;steer_pos_right_raw=659;
 begin_center(2U);
 for(ms=10;ms<=60000;ms+=10)pass(ms);
 assert(m9_h2b1_hold.active && !m9_h2b1_hold.ever_applied);
 steer_pos_left_raw=1874;steer_pos_right_raw=650;
 for(;ms<=60500;ms+=10)pass(ms);
 assert(hardware_pwm[0] && hardware_pwm[0]<=30U);
 assert(hardware_pwm[1] && hardware_pwm[1]<=10U);
 assert(hardware_dir[0]==GPIO_PIN_RESET && hardware_dir[1]==GPIO_PIN_SET);
 for(;ms<=62000 && m9_h2b1_hold.active;ms+=10)pass(ms);
 assert(!m9_h2b1_hold.active && !hardware_pwm[0] && !hardware_pwm[1]);
 assert(m9_h2b1_hold.reason==M9B1_NO_PROGRESS && m9_h2b1_hold.fault_axes);
 puts("PASS production tuning profile through actual HAL/main hooks: persistent quiet60s, two-axis power, bounded caps, no-progress cut");
 return 0;
#endif
 begin_center(2U);assert(M9B1_KeepOutput());
 for(ms=10;ms<=60000;ms+=10)pass(ms);
 assert(m9_h2b1_hold.active&&m9_h2b1_hold.lease);
 assert(hardware_pwm[0]&&hardware_pwm[1]&&dir_calls);
 assert(hardware_dir[0]==GPIO_PIN_RESET&&hardware_dir[1]==GPIO_PIN_SET);
 /* Each side's normal idle cleanup must not chop a live HOLD. */
 {uint16_t l=hardware_pwm[0],r=hardware_pwm[1];
  Steering_PositionLeftOff();Steering_PositionRightOff();
  Steering_ManualLeftOff();Steering_ManualRightOff();
  Steering_CalLeftOff();Steering_CalRightOff();
  assert(hardware_pwm[0]==l&&hardware_pwm[1]==r);}
 manual_direction=1;M9B1_PreOwnerService();
 assert(!m9_h2b1_hold.active&&!hardware_pwm[0]&&!hardware_pwm[1]);
 manual_direction=0;pass(60010U);assert(!m9_h2b1_hold.active);
 /* An eligible NEW completed command reacquires; a held R1 is not new. */
 sample_at(60100U);PS2_Pad.R1=1U;steer_pos_previous_buttons=1U;
 begin_center(1U);pass(60200U);assert(m9_h2b1_hold.active);
 PS2_Pad.R1=0;steer_pos_previous_buttons=0;pass(60300U);
 PS2_Pad.R1=1U;M9B1_PreOwnerService();assert(!m9_h2b1_hold.active);
 PS2_Pad.R1=0;steer_pos_previous_buttons=0;
 sample_at(60400U);begin_center(2U);pass(60500U);
 /* Real NC ISR body cuts corresponding channel; retained epoch prevents
  * a bounced-clear input from being re-powered by the next service. */
 assert(hardware_pwm[0]);Steering_SafetyOnLimitInterrupt(STEER_LIMIT_L_MIN_MASK);
 raw_nc=0;assert(hardware_pwm[0]==0U);pass(60510U);
 assert(!m9_h2b1_hold.active&&!hardware_pwm[0]&&!hardware_pwm[1]);
 sample_at(60600U);begin_center(2U);pass(60700U);
 EXTI->PR=GPIO_PIN_10;M9B1_Update();
 assert(!m9_h2b1_hold.active&&!hardware_pwm[0]&&!hardware_pwm[1]);EXTI->PR=0;
 sample_at(60800U);begin_center(2U);pass(60900U);
 Turn_Motors_Stop();assert(!m9_h2b1_hold.active&&!hardware_pwm[0]&&!hardware_pwm[1]);
 pass(61000U);assert(!m9_h2b1_hold.active);
 /* R2/Voice45 NC completions never acquire HOLD. */
 steer_pos_active=1;steer_pos_command=2U;steer_pos_left_result=3U;steer_pos_right_result=3U;
 Steering_PositionFinish(2U);assert(!m9_h2b1_hold.active);
 steer_pos_active=1U;steer_pos_command=1U;steer_pos_limit_trip_mask=1U;
 steer_pos_left_result=steer_pos_right_result=2U;
 Steering_PositionFinish(2U);assert(!m9_h2b1_hold.active);
 steer_pos_limit_trip_mask=0U;
 /* Safety/read transport failure revokes rather than retrying a target. */
 sample_at(61100U);begin_center(2U);pass(61200U);
 debug_as5600_right_comm_ok=0U;M9B1_PreOwnerService();
 assert(!m9_h2b1_hold.active&&!hardware_pwm[0]&&!hardware_pwm[1]);
 puts("PASS H2-B1 production main hooks: DONE handoff, idle retention60s, raw DIR mapping, fresh PS2 owner, NC ISR/late edge, explicit STOP, exclusions");
 return 0;
}
