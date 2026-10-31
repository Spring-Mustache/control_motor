#ifndef STEER_S147_VELOCITY_PI_H
#define STEER_S147_VELOCITY_PI_H
#include <stdint.h>

/* M8-S14.7. Pure, motor-I/O-free, BOOST-ONLY velocity PI.
 * Unit: permille of current calibrated steering stroke per second.
 * Caller must validate both AS5600 samples and independently enforce
 * NC/STOP/SOFT/state/direction before AND after this helper.
 * Level1/Level2 D-pad LEFT -> R_MIN opt-in trial.
 */
#define M8S147_MIN_DIST_RAW 180U
#define M8S147_MAX_EXTRA_CCR 40U
#define M8S147_MAX_RISE_CCR 12U
#define M8S147_NORMAL_TARGET_PMS 560U
#define M8S147_RMIN_RIGHT_TARGET_PMS 680U
typedef struct {
  int32_t integral;
  uint32_t last_sample_ms;
  uint16_t last_boost;
} M8S147AxisState_t;
typedef struct {
  uint16_t pwm,boost;
  int16_t error_pms;
  uint8_t saturated;
} M8S147AxisPlan_t;

static inline uint16_t M8S147_Min(uint16_t a,uint16_t b)
{ return (a<b)?a:b; }

/* Shared progress-coordinated trajectory, never slowing an axis.
 * RIGHT anticipates the longer terminal taper observed in the A/B logs.
 * The leader does not lose PWM: only lagger's positive velocity target
 * is increased. Both targets are deliberately tunable only in source,
 * not arbitrary debug power knobs on an unvalidated chassis.
 */
static inline void M8S147_Targets(uint16_t progress_l,uint16_t progress_r,
                                 uint16_t *target_l,uint16_t *target_r)
{
  int32_t gap=(int32_t)progress_l-(int32_t)progress_r;
  if(progress_l>1000U)progress_l=1000U;
  if(progress_r>1000U)progress_r=1000U;
  gap=(int32_t)progress_l-(int32_t)progress_r;
  *target_l=M8S147_NORMAL_TARGET_PMS;
  *target_r=M8S147_RMIN_RIGHT_TARGET_PMS;
  if(gap>0)
    *target_r=(uint16_t)(*target_r+M8S147_Min((uint16_t)gap*2U,120U));
  else if(gap<0)
    *target_l=(uint16_t)(*target_l+M8S147_Min((uint16_t)(-gap)*2U,120U));
}

/* Input owner_pwm was already limited by the original per-axis envelope.
 * PI never produces less than owner demand, never exceeds independent cap,
 * and never changes PWM inside 180 RAW of the independently computed SOFT.
 * Update I only for fresh distinct sensor samples with real timestamps.
 * A saturated positive error freezes I (anti-windup).
 */
static inline M8S147AxisPlan_t M8S147_AxisStep(
    M8S147AxisState_t *st,uint16_t owner_pwm,uint16_t cap,
    uint16_t measured_pms,uint16_t target_pms,uint16_t distance_raw,
    uint8_t fresh,uint32_t sample_ms,uint16_t prev_final)
{
  M8S147AxisPlan_t out={owner_pwm,0U,0,0U};
  int32_t err,request,room,step_ms;
  uint16_t extra;
  if(st==0)return out;
  if(owner_pwm==0U||distance_raw<=M8S147_MIN_DIST_RAW||cap<=owner_pwm)
  {
    st->integral=0;
    st->last_boost=0U;
    st->last_sample_ms=0U;
    return out;
  }
  err=(int32_t)target_pms-(int32_t)measured_pms;
  if(err>32767)err=32767;
  if(err< -32768)err= -32768;
  out.error_pms=(int16_t)err;
  room=(int32_t)cap-(int32_t)owner_pwm;
  if(fresh!=0U&&sample_ms!=0U&&sample_ms!=st->last_sample_ms)
  {
    step_ms=st->last_sample_ms==0U?20L:
        (int32_t)(uint32_t)(sample_ms-st->last_sample_ms);
    st->last_sample_ms=sample_ms;
    if(step_ms<7||step_ms>150)
    {
      st->integral=0;
      st->last_boost=0U;
      return out; /* reject backward/old/jittered sample */
    }
    if(err<=0)st->integral=0;
    request=err*10L/100L+st->integral/400L;
    if(request<0)request=0;
    if(request>(int32_t)M8S147_MAX_EXTRA_CCR)
      request=M8S147_MAX_EXTRA_CCR;
    if(err>0 && request<room && request<(int32_t)M8S147_MAX_EXTRA_CCR)
    {
      st->integral+=err*step_ms/40L;
      if(st->integral>6000)st->integral=6000;
      if(st->integral<0)st->integral=0;
    }
    request=err*10L/100L+st->integral/400L;
    if(request<0)request=0;
    if(request>(int32_t)M8S147_MAX_EXTRA_CCR)request=M8S147_MAX_EXTRA_CCR;
    st->last_boost=(uint16_t)request;
  }
  extra=M8S147_Min(st->last_boost,M8S147_MAX_EXTRA_CCR);
  if(extra>(uint16_t)room)extra=(uint16_t)room;
  out.saturated=(uint8_t)((err>0)&&((uint32_t)st->last_boost>(uint32_t)room));
  /* Preserve original owner's 12-CCR rise bound, even if the
   * independent baseline happens to rise faster this control pass.
   */
  {
    uint32_t allowed=(uint32_t)prev_final+M8S147_MAX_RISE_CCR;
    uint32_t requested=(uint32_t)owner_pwm+extra;
    if(allowed<(uint32_t)owner_pwm)allowed=owner_pwm;
    if(requested>allowed)requested=allowed;
    if(requested>cap)requested=cap;
    out.pwm=(uint16_t)requested;
  }
  out.boost=(uint16_t)(out.pwm-owner_pwm);
  return out;
}
#endif
