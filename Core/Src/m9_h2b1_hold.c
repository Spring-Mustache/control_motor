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
     c->deadband_raw>=c->enter_raw || c->max_error_raw<c->enter_raw ||
     c->max_error_raw>=2048U || c->sample_stale_ms==0U ||
     c->velocity_window_ms<20U || c->velocity_window_ms>c->sample_stale_ms ||
     c->service_gap_ms==0U || c->max_jump_raw==0U || c->max_jump_raw>=2048U ||
     c->power_mode>M9B1_POWER_COMMISSIONED)return 0U;
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
  }
  return 1U;
}
void M9B1_Revoke(M9B1Hold *h,uint8_t reason)
{
  zero_demand(h);h->active=0U;h->reason=reason;
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
    if(a->peak_error>c->max_error_raw){M9B1_Revoke(h,M9B1_RANGE);return 0U;}
  }
  h->active=1U;h->state=M9B1_WAIT_DATA;return 1U;
}
static uint8_t sample(M9B1Axis *a,const M9B1Config *c,const M9B1Input *in,
                       unsigned i,uint16_t goal)
{
  if(in->sample_seq[i]!=a->last_seq){
    uint32_t dt=(uint32_t)(in->sample_ms[i]-a->velocity_ms);
    int16_t movement=delta(a->raw,in->raw[i]);
    if(magnitude(movement)>c->max_jump_raw)return 0U;
    a->raw=in->raw[i];a->last_seq=in->sample_seq[i];
    a->last_sample_ms=in->sample_ms[i];inc32(&a->good_samples);
    if(dt>=c->velocity_window_ms){
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
 * Both axes are cut on any tuning fault; the goal never auto-resumes.
 */
static uint8_t tuning_guard(M9B1Hold *h,const M9B1Config *c,
                              uint32_t now,uint32_t dt)
{
  unsigned i;
  if(c->power_mode!=M9B1_POWER_TUNING)return M9B1_NONE;
  for(i=0;i<2U;i++){
    M9B1Axis *a=&h->axis[i];
    uint16_t error=magnitude(a->error);
    if(h->applied_pwm[i]){
      if(a->effort_ms+dt>c->tuning_effort_ms){
        h->fault_axes|=(uint8_t)(1U<<i);return M9B1_EFFORT_BUDGET;
      }
      a->effort_ms+=dt;a->recovery_remainder=0U;
    }else{
      uint32_t recovery,denominator=(uint32_t)c->tuning_window_ms-c->tuning_effort_ms;
      a->recovery_remainder+=dt*(uint32_t)c->tuning_effort_ms;
      recovery=a->recovery_remainder/denominator;
      a->recovery_remainder%=denominator;
      if(recovery>=a->effort_ms){a->effort_ms=0U;a->recovery_remainder=0U;}
      else a->effort_ms-=recovery;
    }
    if(!h->applied_pwm[i] || error<=c->exit_raw){a->progress_tracking=0U;continue;}
    if(!a->progress_tracking){
      a->progress_tracking=1U;a->progress_error=error;a->progress_ms=now-dt;
    }else if((uint32_t)error+2U<=a->progress_error){
      a->progress_error=error;a->progress_ms=now;
    }
    if((uint32_t)(now-a->progress_ms)>=c->tuning_no_progress_ms){
      h->fault_axes|=(uint8_t)(1U<<i);return M9B1_NO_PROGRESS;
    }
  }
  return M9B1_NONE;
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
    if(magnitude(h->axis[i].error)>c->max_error_raw){
      M9B1_Revoke(h,M9B1_RANGE);return;
    }
  }
  inc32(&h->updates);
  if(c->power_mode==M9B1_POWER_LOCKED){zero_demand(h);h->state=M9B1_UNCOMMISSIONED;
    h->reason=M9B1_CALIBRATION_REQUIRED;return;}
  if(!h->axis[0].velocity_ready || !h->axis[1].velocity_ready){
    zero_demand(h);h->state=M9B1_WAIT_DATA;return;
  }
  reason=tuning_guard(h,c,in->now_ms,dt);
  if(reason){M9B1_Revoke(h,reason);return;}
  for(i=0;i<2U;i++)(void)demand(&h->axis[i],&c->axis[i],c,in->now_ms,dt);
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
  uint32_t irq;unsigned i;int16_t command[2];
  if(!port_valid(p)){M9B1_Revoke(h,M9B1_PORT_FAULT);return 0U;}
  if(!h->active || h->state!=M9B1_HOLDING || !M9B1_ConfigValid(c) ||
     c->power_mode==M9B1_POWER_LOCKED){
    if(h->lease)M9B1_Cut(h,p,h->reason);
    return 0U;
  }
  command[0]=h->axis[0].pwm;command[1]=h->axis[1].pwm;
  irq=p->lock(p->context);
  if(!p->allowed(p->context))goto blocked;
  for(i=0;i<2U;i++){
    if(magnitude(command[i])>c->axis[i].max_pwm)goto blocked;
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
  p->pwm(p->context,0U,magnitude(command[0]));
  if(!p->allowed(p->context))goto blocked;
  p->pwm(p->context,1U,magnitude(command[1]));
  h->lease=1U;M9B1_Applied(h,p->now(p->context),command[0],command[1]);
  p->unlock(p->context,irq);return 1U;
blocked:
  p->pwm(p->context,0U,0U);p->pwm(p->context,1U,0U);
  M9B1_Applied(h,p->now(p->context),0,0);h->lease=0U;
  M9B1_Revoke(h,M9B1_PORT_FAULT);p->unlock(p->context,irq);return 0U;
}
