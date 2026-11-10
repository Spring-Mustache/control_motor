#include "m9_h2b1_hold.h"
#include <string.h>
#include <limits.h>

static int16_t delta(uint16_t from, uint16_t to)
{
  int32_t d=(int32_t)to-from;
  if(d>2047)d-=4096;
  if(d< -2048)d+=4096;
  return (int16_t)d;
}
static uint16_t magnitude(int32_t x)
{
  if(x<0)x=-x;
  return (uint16_t)(x>65535?65535:x);
}
static void inc32(uint32_t *n){if(*n!=UINT32_MAX)++*n;}
static void zero_demand(M9B1Hold *h)
{
  h->axis[0].pwm=0;h->axis[1].pwm=0;
}
void M9B1_Reset(M9B1Hold *h){(void)memset(h,0,sizeof(*h));}
uint8_t M9B1_ConfigValid(const M9B1Config *c)
{
  unsigned i;
  if(c==0 || c->enter_raw<=c->exit_raw || c->exit_raw>c->deadband_raw ||
     c->deadband_raw>=c->enter_raw || c->sample_stale_ms==0U ||
     c->velocity_window_ms<20U || c->velocity_window_ms>c->sample_stale_ms ||
     c->service_gap_ms==0U || c->max_jump_raw==0U || c->max_jump_raw>=2048U ||
     c->power_mode>M9B1_POWER_SERVO)return 0U;
  if(c->power_mode==M9B1_POWER_SERVO &&
     (c->axis[0].max_pwm>M9B1_RECOVERY_LEFT_CEILING ||
      c->axis[1].max_pwm>M9B1_RECOVERY_RIGHT_CEILING ||
      c->axis[0].min_pwm || c->axis[1].min_pwm ||
      c->axis[0].assist_pwm || c->axis[1].assist_pwm ||
      c->axis[0].assist_ms || c->axis[1].assist_ms ||
      c->tuning_window_ms || c->tuning_effort_ms || c->tuning_no_progress_ms ||
      c->assist_cooldown_ms || c->sample_stale_ms>80U || c->service_gap_ms>80U))return 0U;
  if(c->power_mode==M9B1_POWER_RECOVERY &&
     (c->axis[0].max_pwm>M9B1_RECOVERY_LEFT_CEILING ||
      c->axis[1].max_pwm>M9B1_RECOVERY_RIGHT_CEILING ||
      !c->axis[0].min_pwm || !c->axis[1].min_pwm ||
      c->axis[0].assist_pwm || c->axis[1].assist_pwm ||
      c->axis[0].assist_ms || c->axis[1].assist_ms ||
      !c->tuning_no_progress_ms || c->tuning_no_progress_ms>1000U ||
      c->tuning_effort_ms<=c->tuning_no_progress_ms || c->tuning_effort_ms>5000U ||
      c->assist_cooldown_ms<3000U || c->assist_cooldown_ms>60000U ||
      c->sample_stale_ms>80U || c->service_gap_ms>80U))return 0U;
  if(c->power_mode==M9B1_POWER_TUNING &&
     (!c->tuning_effort_ms || c->tuning_window_ms<=c->tuning_effort_ms ||
      !c->tuning_no_progress_ms || c->tuning_no_progress_ms>5000U ||
      c->axis[0].max_pwm>M9B1_TUNING_LEFT_CEILING ||
      c->axis[1].max_pwm>M9B1_TUNING_RIGHT_CEILING))return 0U;
  for(i=0;i<2U;i++){
    const M9B1AxisConfig *a=&c->axis[i];
    if(a->max_pwm>999U || a->min_pwm>a->max_pwm)return 0U;
    if(c->power_mode!=M9B1_POWER_LOCKED && (!a->max_pwm || !a->kp_q8 || !a->kd_q8 ||
        !a->rise_per_second || !a->reversal_zero_ms))return 0U;
    if(a->assist_pwm || a->assist_ms){
      if(a->assist_pwm<=a->max_pwm || !a->assist_ms ||
         a->assist_pwm>(i?M9B1_ASSIST_RIGHT_CEILING:M9B1_ASSIST_LEFT_CEILING) ||
         a->assist_ms>(i?M9B1_ASSIST_RIGHT_MS:M9B1_ASSIST_LEFT_MS) ||
         c->assist_still_ms<200U || c->assist_off_ms<200U ||
         c->assist_cooldown_ms<3000U || c->assist_cooldown_ms>60000U ||
         !c->assist_max_pulses || c->assist_max_pulses>M9B1_ASSIST_ROUND_MAX)
        return 0U;
    }
  }
  if((c->axis[0].assist_pwm!=0U)!=(c->axis[1].assist_pwm!=0U))return 0U;
  return 1U;
}
void M9B1_Revoke(M9B1Hold *h,uint8_t reason)
{
  unsigned i;
  zero_demand(h);h->active=0U;h->reason=reason;
  for(i=0;i<2U;i++){
    h->axis[i].assist_phase=0U;h->axis[i].assist_started=0U;
    h->axis[i].assist_still_tracking=0U;
    h->axis[i].integral_q8=0;h->axis[i].integral_remainder=0;
  }
  h->state=(reason==M9B1_NEW_OWNER || reason==M9B1_DISABLED ||
            reason==M9B1_STOP)?M9B1_REVOKED:M9B1_FAULT;
  /* lease is released only after the actuator port has zeroed its outputs. */
}
static uint8_t input_reason(const M9B1Config *c,const M9B1Input *in)
{
  unsigned i;
  if(!in->enabled)return M9B1_DISABLED;
  if(!in->safety_ok)return M9B1_STOP;
  if(in->nc_mask)return M9B1_NC;
  if(!in->owners_free)return M9B1_NEW_OWNER;
  if(!in->geometry_ok)return M9B1_RANGE;
  for(i=0;i<2U;i++)if(!in->valid[i] || in->raw[i]>4095U ||
      (uint32_t)(in->now_ms-in->sample_ms[i])>c->sample_stale_ms)
    return M9B1_SENSOR;
  return M9B1_NONE;
}
uint8_t M9B1_Begin(M9B1Hold *h,const M9B1Config *c,const M9B1Input *in,
                   uint32_t seq,uint8_t source,uint16_t goal_l,uint16_t goal_r)
{
  unsigned i;uint8_t reason;
  /* Caller must release the prior lease before replacing a goal. */
  if(h->lease)return 0U;
  M9B1_Reset(h);
  if(!M9B1_ConfigValid(c)){M9B1_Revoke(h,M9B1_BAD_CONFIG);return 0U;}
  reason=input_reason(c,in);
  if(reason){M9B1_Revoke(h,reason);return 0U;}
  if(!seq || source<1U || source>3U || goal_l>4095U || goal_r>4095U){
    M9B1_Revoke(h,M9B1_RANGE);return 0U;
  }
  h->goal[0]=goal_l;h->goal[1]=goal_r;
  h->seq=seq;h->source=source;h->started_ms=in->now_ms;
  h->last_step_ms=in->now_ms;h->last_applied_ms=in->now_ms;
  for(i=0;i<2U;i++){
    M9B1Axis *a=&h->axis[i];
    a->raw=in->raw[i];a->velocity_raw=in->raw[i];
    a->last_sample_ms=in->sample_ms[i];a->velocity_ms=in->sample_ms[i];
    a->last_seq=in->sample_seq[i];a->initialized=1U;
    a->error=delta(a->raw,h->goal[i]);a->peak_error=magnitude(a->error);
  }
  h->active=1U;h->state=M9B1_WAIT_DATA;return 1U;
}
static uint8_t sample(M9B1Axis *a,const M9B1Config *c,const M9B1Input *in,
                       unsigned i,uint16_t goal)
{
  if(in->sample_seq[i]!=a->last_seq){
    uint32_t dt=(uint32_t)(in->sample_ms[i]-a->velocity_ms);
    uint32_t sample_dt=(uint32_t)(in->sample_ms[i]-a->last_sample_ms);
    int16_t movement=delta(a->raw,in->raw[i]);
    if(magnitude(movement)>c->max_jump_raw)return 0U;
    a->raw=in->raw[i];a->last_seq=in->sample_seq[i];
    a->last_sample_ms=in->sample_ms[i];inc32(&a->good_samples);
    if(c->power_mode==M9B1_POWER_SERVO){
      /* Keep sub-RAW/sample motion in Q8. The historical estimator
       * discards1RAW/40ms as noise, hiding slow returns from D entirely.
       * A60ms low-pass uses actual sample time, once per fresh sample. */
      if(sample_dt){
        int64_t rate=(int64_t)movement*256000/(int32_t)sample_dt;
        a->servo_velocity_q8+=(int32_t)((rate-a->servo_velocity_q8)*sample_dt/
                                       (60U+sample_dt));
      }
      a->velocity=a->servo_velocity_q8/256;
      if(dt>=c->velocity_window_ms)a->velocity_ready=1U;
    }else if(dt>=c->velocity_window_ms){
      int16_t distance=delta(a->velocity_raw,a->raw);
      a->velocity=(magnitude(distance)<=1U)?0:(int32_t)distance*1000/(int32_t)dt;
      a->velocity_raw=a->raw;a->velocity_ms=in->sample_ms[i];
      a->velocity_ready=1U;
    }
  }
  if((uint32_t)(in->now_ms-a->last_sample_ms)>c->sample_stale_ms)return 0U;
  a->error=delta(a->raw,goal);
  if(magnitude(a->error)>a->peak_error)a->peak_error=magnitude(a->error);
  return 1U;
}
/* This is an output-time/response guard, NOT current or I2t protection.
 * Charge actual acknowledged output time, then recover only while zero.
 * No fixed-window reset: crossing a clock boundary cannot double a burst.
 * No-progress/budget exhaustion rests BOTH axes without discarding the
 * goal. Retry only after BOTH budgets fully recover. Safety faults still
 * revoke the goal and never auto-resume. This is NOT a current limit.
 */
static uint8_t tuning_guard(M9B1Hold *h,const M9B1Config *c,
                              uint32_t now,uint32_t dt)
{
  unsigned i;uint8_t reason=M9B1_NONE,axes=0U;
  (void)now;
  if(c->power_mode!=M9B1_POWER_TUNING)return M9B1_NONE;
  for(i=0;i<2U;i++){
    M9B1Axis *a=&h->axis[i];
    uint16_t error=magnitude(a->error);
    if(h->applied_pwm[i]){
      a->effort_ms+=dt;a->recovery_remainder=0U;
      if(a->effort_ms>=c->tuning_effort_ms){
        reason=M9B1_EFFORT_BUDGET;axes|=(uint8_t)(1U<<i);
      }
    }else{
      uint32_t recovery,denominator=(uint32_t)c->tuning_window_ms-c->tuning_effort_ms;
      a->recovery_remainder+=dt*(uint32_t)c->tuning_effort_ms;
      recovery=a->recovery_remainder/denominator;
      a->recovery_remainder%=denominator;
      if(recovery>=a->effort_ms){a->effort_ms=0U;a->recovery_remainder=0U;}
      else a->effort_ms-=recovery;
    }
    if(h->state==M9B1_COOLDOWN || error<=c->exit_raw){
      a->progress_tracking=0U;a->progress_on_ms=0U;continue;
    }
    if(a->progress_tracking && (uint32_t)error+2U<=a->progress_error){
      a->progress_error=error;a->progress_on_ms=0U;
    }
    /* Short zero-output gaps must not erase a stalled attempt's history.
     * Improvement while coasting DOES reset the powered-time counter. */
    if(!h->applied_pwm[i])continue;
    if(!a->progress_tracking){
      a->progress_tracking=1U;a->progress_error=error;a->progress_on_ms=0U;
    }
    a->progress_on_ms+=dt;
    if(a->progress_on_ms>=c->tuning_no_progress_ms){
      if(reason==M9B1_NONE)reason=M9B1_NO_PROGRESS;
      axes|=(uint8_t)(1U<<i);
    }
  }
  if(reason!=M9B1_NONE)h->fault_axes=axes;
  return reason;
}
static void assist_settle(M9B1Axis *a,uint32_t now)
{
  a->assist_phase=M9B1_ASSIST_SETTLE;a->assist_since_ms=now;
  a->assist_sample_seq=a->last_seq;a->assist_started=0U;
  a->assist_still_tracking=0U;a->pwm=0;a->rise_remainder=0U;
}
static uint8_t assist_demand(M9B1Axis *a,const M9B1AxisConfig *cfg,
                               const M9B1Config *c,uint32_t now)
{
  int16_t base=a->pwm;int8_t sign=(a->error>0)?1:-1;
  if(!cfg->assist_pwm)return M9B1_NONE;
  if(a->assist_phase==M9B1_ASSIST_PULSE){
    if(magnitude(a->error)<=c->exit_raw || sign!=a->assist_sign ||
       a->reversing || !base || ((base>0)?1:-1)!=a->assist_sign ||
       (a->assist_started && (uint32_t)(now-a->assist_since_ms)>=cfg->assist_ms)){
      assist_settle(a,now);return M9B1_NONE;
    }
    a->pwm=(int16_t)(a->assist_sign*(int32_t)cfg->assist_pwm);
  }else if(a->assist_phase==M9B1_ASSIST_SETTLE){
    a->pwm=0;a->rise_remainder=0U;
    if((uint32_t)(now-a->assist_since_ms)>=c->assist_off_ms &&
       a->last_seq!=a->assist_sample_seq)a->assist_phase=0U;
    return M9B1_NONE;
  }else{
    if(magnitude(a->error)<c->enter_raw || magnitude(a->velocity)>20U ||
       a->reversing || !base || ((base>0)?1:-1)!=sign){
      a->assist_still_tracking=0U;return M9B1_NONE;
    }
    if(a->assist_round>=c->assist_max_pulses)return M9B1_ASSIST_BUDGET;
    if(!a->assist_still_tracking){
      a->assist_still_tracking=1U;a->assist_still_since_ms=now;
      a->assist_sample_seq=a->last_seq;return M9B1_NONE;
    }
    if((uint32_t)(now-a->assist_still_since_ms)<c->assist_still_ms ||
       a->last_seq==a->assist_sample_seq)return M9B1_NONE;
    a->assist_phase=M9B1_ASSIST_PULSE;a->assist_started=0U;
    a->assist_sign=sign;a->assist_still_tracking=0U;
    a->pwm=(int16_t)(sign*(int32_t)cfg->assist_pwm);
  }
  if(magnitude(a->pwm)>a->peak_pwm)a->peak_pwm=magnitude(a->pwm);
  return M9B1_NONE;
}
static void cooldown(M9B1Hold *h,uint8_t reason,uint32_t now)
{
  unsigned i;zero_demand(h);h->state=M9B1_COOLDOWN;h->reason=reason;
  h->cooldown_since_ms=now;
  for(i=0;i<2U;i++){
    M9B1Axis *a=&h->axis[i];a->progress_tracking=0U;a->progress_on_ms=0U;
    a->reversing=0U;a->rise_remainder=0U;a->assist_phase=0U;
    a->assist_started=0U;a->assist_still_tracking=0U;
  }
}
static int16_t demand(M9B1Axis *a,const M9B1AxisConfig *cfg,
                       const M9B1Config *c,uint32_t now,uint32_t dt)
{
  int32_t effective=0;
  int64_t pd;
  uint32_t wanted,previous,step;int8_t sign;
  if(magnitude(a->error)>=c->enter_raw)a->position_latched=1U;
  if(magnitude(a->error)<=c->exit_raw)a->position_latched=0U;
  if(a->position_latched){
    if(a->error>(int32_t)c->deadband_raw)effective=a->error-c->deadband_raw;
    if(a->error<-(int32_t)c->deadband_raw)effective=a->error+c->deadband_raw;
  }
  /* Unlike B0's sign veto, D may oppose position error: this is the
   * damping needed during a return/release. Reversal is zero-blanked. */
  pd=((int64_t)cfg->kp_q8*effective-(int64_t)cfg->kd_q8*a->velocity)/256;
  if(pd==0){a->pwm=0;a->reversing=0U;a->rise_remainder=0U;return 0;}
  sign=(pd>0)?1:-1;
  wanted=(uint32_t)((pd>0)?pd:-pd);
  if(wanted>cfg->max_pwm)wanted=cfg->max_pwm;
  if(wanted<cfg->min_pwm)wanted=cfg->min_pwm;
  if(a->last_sign && sign!=a->last_sign){
    if(!a->reversing || a->pending_sign!=sign){
      a->reversing=1U;a->pending_sign=sign;a->reversal_ms=now;
    }
    a->pwm=0;a->rise_remainder=0U;
    if((uint32_t)(now-a->reversal_ms)<cfg->reversal_zero_ms)return 0;
    a->reversing=0U;
  }else a->reversing=0U;
  previous=magnitude(a->pwm);
  a->rise_remainder+=(uint32_t)cfg->rise_per_second*dt;
  step=a->rise_remainder/1000U;a->rise_remainder%=1000U;
  if(wanted>previous && wanted-previous>step)wanted=previous+step;
  a->last_sign=sign;a->pwm=(int16_t)((int32_t)sign*(int32_t)wanted);
  if(wanted>a->peak_pwm)a->peak_pwm=(uint16_t)wanted;
  return a->pwm;
}
/* Position recovery with bounded stiction compensation. Each axis owns
 * its own progress/rest clock: a stalled LEFT cannot silence RIGHT.
 * Counters use acknowledged powered time, never wall-clock session age.
 * Returning/coasting progress resets the no-progress counter. The goal
 * remains valid through a rest; actual safety faults revoke in Step.
 */
static void recovery_axis(M9B1Hold *h,const M9B1Config *c,unsigned i,
                          uint32_t now,uint32_t dt)
{
  M9B1Axis *a=&h->axis[i];const M9B1AxisConfig *cfg=&c->axis[i];
  uint16_t error=magnitude(a->error);int8_t sign=(a->error>0)?1:-1;
  int32_t toward=(int32_t)sign*a->velocity,drive;
  uint32_t wanted,previous,step,boost;
  if(error>=c->enter_raw)a->position_latched=1U;
  if(error<=c->exit_raw){
    a->position_latched=0U;a->recovery_tracking=0U;
    a->recovery_on_ms=0U;a->recovery_stuck_ms=0U;
    a->recovery_boost_ms=0U;
  }
  if(a->recovery_rest){
    a->pwm=0;a->rise_remainder=0U;
    if((uint32_t)(now-a->recovery_rest_ms)<c->assist_cooldown_ms)return;
    a->recovery_rest=0U;a->recovery_reason=0U;
    a->recovery_tracking=0U;inc32(&h->retry_count);
  }
  if(!a->position_latched){a->pwm=0;a->rise_remainder=0U;return;}
  if(!a->recovery_tracking){
    a->recovery_tracking=1U;a->recovery_best_error=error;
    a->recovery_stuck_ms=0U;a->recovery_on_ms=0U;
  }
  if((uint32_t)error+2U<=a->recovery_best_error){
    a->recovery_best_error=error;a->recovery_stuck_ms=0U;
  }
  if(h->applied_pwm[i]){a->recovery_on_ms+=dt;a->recovery_stuck_ms+=dt;}
  if(a->recovery_stuck_ms>=c->tuning_no_progress_ms ||
     a->recovery_on_ms>=c->tuning_effort_ms){
    a->recovery_reason=(a->recovery_stuck_ms>=c->tuning_no_progress_ms)?
        M9B1_NO_PROGRESS:M9B1_EFFORT_BUDGET;
    a->recovery_rest=1U;a->recovery_rest_ms=now;
    a->pwm=0;a->rise_remainder=0U;return;
  }
  /* Coast ahead of arrival using one velocity window of lookahead.
   * D reduces forward torque only; do not fight a returning wheel with
   * a fresh reverse kick. Overshoot still corrects after zero blanking. */
  if(toward>0 && (uint32_t)toward*c->velocity_window_ms>=
      (uint32_t)(error-c->exit_raw)*1000U){
    a->pwm=0;a->rise_remainder=0U;return;
  }
  /* Keep the learned breakaway contribution throughout this recovery.
   * Dropping it after each 2RAW of progress recreates stop/start stiction.
   * Reset at the goal; never exceed the original M8 motion ceiling. */
  if(h->applied_pwm[i] && a->recovery_stuck_ms>200U){
    boost=(uint32_t)a->recovery_boost_ms+dt;
    a->recovery_boost_ms=(uint16_t)(boost>300U?300U:boost);
  }
  boost=a->recovery_boost_ms;
  drive=(int32_t)cfg->min_pwm+(int32_t)cfg->kp_q8*error/256+
        (int32_t)(boost*(cfg->max_pwm-cfg->min_pwm)/300U);
  if(toward>0)drive-=(int32_t)cfg->kd_q8*toward/256;
  if(drive<=0){a->pwm=0;a->rise_remainder=0U;return;}
  wanted=(uint32_t)drive;
  if(wanted<cfg->min_pwm)wanted=cfg->min_pwm;
  if(wanted>cfg->max_pwm)wanted=cfg->max_pwm;
  if(a->last_sign && sign!=a->last_sign){
    if(!a->reversing || a->pending_sign!=sign){
      a->reversing=1U;a->pending_sign=sign;a->reversal_ms=now;
    }
    a->pwm=0;a->rise_remainder=0U;
    if((uint32_t)(now-a->reversal_ms)<cfg->reversal_zero_ms)return;
  }
  a->reversing=0U;previous=magnitude(a->pwm);
  a->rise_remainder+=(uint32_t)cfg->rise_per_second*dt;
  step=a->rise_remainder/1000U;a->rise_remainder%=1000U;
  if(wanted>previous && wanted-previous>step)wanted=previous+step;
  a->last_sign=sign;a->pwm=(int16_t)(sign*(int32_t)wanted);
  if(wanted>a->peak_pwm)a->peak_pwm=(uint16_t)wanted;
}
/* PI position stiffness plus velocity damping. The integral is the
 * retained load-balancing command, NOT a travel timer: e==0 freezes I
 * instead of discarding torque. Saturation and reversal blanking cannot
 * wind it farther into an unavailable output. No motion is not a fault.
 */
static void servo_axis(M9B1Axis *a,const M9B1AxisConfig *cfg,
                       const M9B1Config *c,unsigned i,uint32_t now,uint32_t dt)
{
  int32_t e=0,limit=(int32_t)cfg->max_pwm*256;
  int32_t ki=i?M9B1_SERVO_RIGHT_KI_Q8:M9B1_SERVO_LEFT_KI_Q8;
  int64_t pd,increment,trial,output;int8_t sign;
  uint32_t wanted,previous,step;
  if(a->error>(int32_t)c->deadband_raw)e=a->error-c->deadband_raw;
  else if(a->error<-(int32_t)c->deadband_raw)e=a->error+c->deadband_raw;
  /* Discard only excess learned torque while motion is carrying the
   * axis across/past its goal. A stationary loaded axis keeps its I.
   * Feedback motion is essential: zero error alone never clears I. */
  uint8_t approaching=(uint8_t)((int32_t)a->error*a->velocity>0 &&
                                 magnitude(a->velocity)>=5U);
  if((int64_t)e*a->integral_q8<0){
    if(a->servo_wrong_i_ms<60U)a->servo_wrong_i_ms+=dt;
  }else a->servo_wrong_i_ms=0U;
  uint8_t unloading=(uint8_t)((int64_t)a->integral_q8*a->velocity>0 &&
      magnitude(a->velocity)>=12U && magnitude(a->error)>c->deadband_raw &&
      ((int32_t)a->error*a->velocity<=0 ||
       (uint32_t)magnitude(a->error)*1000U<=
       (uint32_t)magnitude(a->velocity)*160U));
  /* A released load can stop in stiction just past the goal. Once the
   * opposite error persists across multiple samples, unwind old torque
   * even at low velocity; otherwise I can take seconds to change sign. */
  if(a->servo_wrong_i_ms>=60U && magnitude(a->error)>2U && !approaching)unloading=1U;
  if(unloading){
    int32_t reduction=(int32_t)((int64_t)a->integral_q8*dt/120);
    a->integral_q8-=reduction;a->integral_remainder=0;
    if(magnitude(a->integral_q8)<256U)a->integral_q8=0;
  }
  pd=(int64_t)cfg->kp_q8*e-(int64_t)cfg->kd_q8*a->velocity;
  increment=(int64_t)ki*e*dt+a->integral_remainder;
  trial=(int64_t)a->integral_q8+increment/1000;
  if(trial>limit)trial=limit;
  if(trial< -limit)trial= -limit;
  output=pd+trial;
  if(!a->reversing && !approaching &&
     (!unloading || (int64_t)e*a->integral_q8<=0) &&
     !((output>limit && e>0) || (output< -limit && e<0))){
    a->integral_q8=(int32_t)trial;
    a->integral_remainder=(int32_t)(increment%1000);
  }else a->integral_remainder=0;
  output=pd+a->integral_q8;
  if(output>limit)output=limit;
  if(output< -limit)output= -limit;
  output/=256;
  if(output==0){a->pwm=0;a->rise_remainder=0;a->reversing=0;return;}
  sign=output>0?1:-1;wanted=(uint32_t)(output>0?output:-output);
  if(a->last_sign && sign!=a->last_sign){
    if(!a->reversing || a->pending_sign!=sign){
      a->reversing=1;a->pending_sign=sign;a->reversal_ms=now;
    }
    a->pwm=0;a->rise_remainder=0;
    if((uint32_t)(now-a->reversal_ms)<cfg->reversal_zero_ms)return;
  }
  a->reversing=0;previous=magnitude(a->pwm);
  a->rise_remainder+=(uint32_t)cfg->rise_per_second*dt;
  step=a->rise_remainder/1000U;a->rise_remainder%=1000U;
  if(wanted>previous && wanted-previous>step)wanted=previous+step;
  a->last_sign=sign;a->pwm=(int16_t)(sign*(int32_t)wanted);
  if(wanted>a->peak_pwm)a->peak_pwm=(uint16_t)wanted;
}
void M9B1_Step(M9B1Hold *h,const M9B1Config *c,const M9B1Input *in)
{
  unsigned i;uint8_t reason;uint32_t dt;
  if(!h->active){zero_demand(h);return;}
  if(!M9B1_ConfigValid(c)){M9B1_Revoke(h,M9B1_BAD_CONFIG);return;}
  reason=input_reason(c,in);
  if(reason){M9B1_Revoke(h,reason);return;}
  dt=(uint32_t)(in->now_ms-h->last_step_ms);h->last_step_ms=in->now_ms;
  if(dt>c->service_gap_ms){M9B1_Revoke(h,M9B1_SERVICE_GAP);return;}
  for(i=0;i<2U;i++){
    if(!sample(&h->axis[i],c,in,i,h->goal[i])){
      M9B1_Revoke(h,M9B1_SENSOR);return;
    }
  }
  inc32(&h->updates);
  if(c->power_mode==M9B1_POWER_LOCKED){zero_demand(h);h->state=M9B1_UNCOMMISSIONED;
    h->reason=M9B1_CALIBRATION_REQUIRED;return;}
  if(!h->axis[0].velocity_ready || !h->axis[1].velocity_ready){
    zero_demand(h);h->state=M9B1_WAIT_DATA;return;
  }
  if(c->power_mode==M9B1_POWER_SERVO){
    for(i=0U;i<2U;i++)servo_axis(&h->axis[i],&c->axis[i],c,i,in->now_ms,dt);
    h->fault_axes=0;h->reason=M9B1_NONE;h->state=M9B1_HOLDING;return;
  }
  if(c->power_mode==M9B1_POWER_RECOVERY){
    h->fault_axes=0U;h->reason=M9B1_NONE;
    for(i=0U;i<2U;i++){
      recovery_axis(h,c,i,in->now_ms,dt);
      if(h->axis[i].recovery_rest){
        h->fault_axes|=(uint8_t)(1U<<i);
        h->reason=h->axis[i].recovery_reason;
      }
    }
    h->state=(h->fault_axes && !h->axis[0].pwm && !h->axis[1].pwm)?
        M9B1_COOLDOWN:M9B1_HOLDING;
    return;
  }
  reason=tuning_guard(h,c,in->now_ms,dt);
  if(reason){
    cooldown(h,reason,in->now_ms);return;
  }
  if(h->state==M9B1_COOLDOWN){
    zero_demand(h);
    if(h->axis[0].effort_ms || h->axis[1].effort_ms)return;
    if(c->axis[0].assist_pwm &&
       (uint32_t)(in->now_ms-h->cooldown_since_ms)<c->assist_cooldown_ms)return;
    h->axis[0].assist_round=0U;h->axis[1].assist_round=0U;
    inc32(&h->retry_count);
  }
  for(i=0;i<2U;i++){
    (void)demand(&h->axis[i],&c->axis[i],c,in->now_ms,dt);
    reason=assist_demand(&h->axis[i],&c->axis[i],c,in->now_ms);
    if(reason){h->fault_axes=(uint8_t)(1U<<i);cooldown(h,reason,in->now_ms);return;}
  }
  h->state=M9B1_HOLDING;h->reason=M9B1_NONE;
}
void M9B1_Applied(M9B1Hold *h,uint32_t now,int16_t left,int16_t right)
{
  unsigned i;uint32_t dt=(uint32_t)(now-h->last_applied_ms);
  for(i=0;i<2U;i++)if(h->applied_pwm[i]){
    if(UINT32_MAX-h->axis[i].applied_ms<dt)h->axis[i].applied_ms=UINT32_MAX;
    else h->axis[i].applied_ms+=dt;
  }
  if(h->applied_pwm[0] || h->applied_pwm[1]){
    if(UINT32_MAX-h->applied_ms<dt)h->applied_ms=UINT32_MAX;
    else h->applied_ms+=dt;
  }
  h->last_applied_ms=now;h->applied_pwm[0]=left;h->applied_pwm[1]=right;
  for(i=0;i<2U;i++)if(h->axis[i].assist_phase==M9B1_ASSIST_PULSE){
    M9B1Axis *a=&h->axis[i];int16_t output=i?right:left;
    if(output && !a->assist_started){
      a->assist_started=1U;a->assist_since_ms=now;++a->assist_round;
      inc32(&a->assist_count);
    }else if(!output)assist_settle(a,now);
  }
  if(left || right)h->ever_applied=1U;
}
static uint8_t port_valid(const M9B1Port *p)
{
  return (uint8_t)(p && p->lock && p->unlock && p->allowed && p->pwm &&
                   p->direction && p->now);
}
void M9B1_Cut(M9B1Hold *h,const M9B1Port *p,uint8_t reason)
{
  if(h->lease && port_valid(p)){
    uint32_t irq=p->lock(p->context);
    p->pwm(p->context,0U,0U);p->pwm(p->context,1U,0U);
    M9B1_Applied(h,p->now(p->context),0,0);
    p->unlock(p->context,irq);
  }
  h->lease=0U;M9B1_Revoke(h,reason);
}
uint8_t M9B1_Apply(M9B1Hold *h,const M9B1Config *c,const M9B1Port *p)
{
  uint32_t irq;unsigned i;int16_t command[2],actual[2];
  if(!port_valid(p)){M9B1_Revoke(h,M9B1_PORT_FAULT);return 0U;}
  if(!h->active || (h->state!=M9B1_HOLDING && h->state!=M9B1_COOLDOWN) ||
     !M9B1_ConfigValid(c) ||
     c->power_mode==M9B1_POWER_LOCKED){
    if(h->lease)M9B1_Cut(h,p,h->reason);
    return 0U;
  }
  command[0]=(h->state==M9B1_COOLDOWN)?0:h->axis[0].pwm;
  command[1]=(h->state==M9B1_COOLDOWN)?0:h->axis[1].pwm;
  irq=p->lock(p->context);
  if(!p->allowed(p->context))goto blocked;
  for(i=0;i<2U;i++){
    if(magnitude(command[i])>c->axis[i].max_pwm &&
       (h->axis[i].assist_phase!=M9B1_ASSIST_PULSE ||
        magnitude(command[i])!=c->axis[i].assist_pwm ||
        ((command[i]>0)?1:-1)!=h->axis[i].assist_sign))goto blocked;
    /* Zero before changing DIR, also on the first lease acquisition. */
    if((command[i]>0 && h->applied_pwm[i]<=0) ||
       (command[i]<0 && h->applied_pwm[i]>=0)){
      p->pwm(p->context,(uint8_t)i,0U);
      p->direction(p->context,(uint8_t)i,(command[i]>0)?1:-1);
    }
  }
  /* GPIO/pending-EXTI/epoch/owner/sensor recheck after DIR and before CCR. */
  if(!p->allowed(p->context))goto blocked;
  /* Claim before the first write so a guard can distinguish our new
   * CCR from an unexpected pre-existing owner on its second recheck. */
  h->lease=1U;
  actual[0]=(int16_t)p->pwm(p->context,0U,magnitude(command[0]));
  if(actual[0] && actual[0]!=magnitude(command[0]))goto blocked;
  if(command[0]<0)actual[0]=(int16_t)-actual[0];
  if(!p->allowed(p->context))goto blocked;
  actual[1]=(int16_t)p->pwm(p->context,1U,magnitude(command[1]));
  if(actual[1] && actual[1]!=magnitude(command[1]))goto blocked;
  if(command[1]<0)actual[1]=(int16_t)-actual[1];
  h->lease=1U;M9B1_Applied(h,p->now(p->context),actual[0],actual[1]);
  p->unlock(p->context,irq);return 1U;
blocked:
  p->pwm(p->context,0U,0U);p->pwm(p->context,1U,0U);
  M9B1_Applied(h,p->now(p->context),0,0);h->lease=0U;
  M9B1_Revoke(h,M9B1_PORT_FAULT);p->unlock(p->context,irq);return 0U;
}
