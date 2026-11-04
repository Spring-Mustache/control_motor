#ifndef STEER_S153_FAR_TRIM_H
#define STEER_S153_FAR_TRIM_H
#include <stdint.h>
#include "steer_s152_manual_relation.h"

/* M8-S15.3 optional far-zone RIGHT phase trim.
 * Candidate SHADOW by default. This pure function never writes motor IO.
 * It is NOT a restoration of unrestricted S14.2 RIGHT trim (up to 48CCR).
 * Every distance is RAW remaining to independent SOFT, not NC contacts.
 */
#define M8S153_FAR_RELEASE_RAW 420U
#define M8S153_FAR_FULL_RAW 548U
#define M8S153_FAR_MAX_TRIM_CCR 16U
#define M8S153_FAR_LEAD_DEADBAND_PM 120L

typedef struct { uint16_t right_pwm,trim_ccr; uint8_t eligible; } M8S153FarPlan_t;

static inline M8S153FarPlan_t M8S153_FarTrim(
 uint16_t owner_l,uint16_t owner_r,uint16_t floor_r,
 uint16_t remain_l,uint16_t remain_r,
 uint8_t both_moving,uint8_t nc_clear,const M8S152Plan_t *obs)
{
 M8S153FarPlan_t p={0};
 uint16_t min_rem,room,release_cap;
 int32_t actual_lead,pred_lead,stable_lead,want;
 p.right_pwm=owner_r;
 /* Rejected on invalid/stale AS5600 samples, NC, STOP, zero owner
  * or if either axis is in the protected terminal buffer >420RAW.
  */
 if(obs==0||both_moving!=1U||nc_clear!=1U||
    owner_l==0U||owner_r==0U||owner_r<=floor_r||
    obs->angle_valid!=1U||obs->pred_valid!=1U||
    remain_l<=M8S153_FAR_RELEASE_RAW||
    remain_r<=M8S153_FAR_RELEASE_RAW)return p;
 p.eligible=1U;
 /* Starting-position-relative AND forecast lead must agree; both
  * negative indicate RIGHT ahead. Reject short velocity spikes.
  */
 actual_lead=-(int32_t)obs->delta;
 pred_lead=(int32_t)obs->initial-(int32_t)obs->predicted;
 if(actual_lead<=M8S153_FAR_LEAD_DEADBAND_PM||
    pred_lead<=M8S153_FAR_LEAD_DEADBAND_PM)return p;
 stable_lead=(actual_lead<pred_lead)?actual_lead:pred_lead;
 want=(stable_lead-M8S153_FAR_LEAD_DEADBAND_PM)/12L;
 if(want<=0L)return p;
 if(want>(int32_t)M8S153_FAR_MAX_TRIM_CCR)
   want=(int32_t)M8S153_FAR_MAX_TRIM_CCR;
 /* Fade continuously to 0 near 420RAW so terminal 180/100RAW
  * retains the original independent RIGHT owner, and never reduce
  * below the RIGHT owner's original 50CCR floor.
  */
 min_rem=(remain_l<remain_r)?remain_l:remain_r;
 release_cap=(uint16_t)((min_rem-M8S153_FAR_RELEASE_RAW)/8U);
 if(release_cap>M8S153_FAR_MAX_TRIM_CCR)
   release_cap=M8S153_FAR_MAX_TRIM_CCR;
 room=(uint16_t)(owner_r-floor_r);
 if((uint32_t)want>room)want=room;
 if((uint32_t)want>release_cap)want=release_cap;
 p.trim_ccr=(uint16_t)want;
 p.right_pwm=(uint16_t)(owner_r-p.trim_ccr);
 return p;
}
#endif
