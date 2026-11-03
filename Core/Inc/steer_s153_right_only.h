#ifndef STEER_S153_RIGHT_ONLY_H
#define STEER_S153_RIGHT_ONLY_H
#include <stdint.h>
#include "steer_s152_manual_relation.h"

/* M8-S15.3 ACTIVE ONLY WHEN EXPLICITLY OPTED IN at a NEW manual stroke.
 * Experiment: no LEFT intervention, no RIGHT taper. Never exceeds the
 * current Level2/owner RIGHT envelope or acts near either 180RAW SOFT.
 * Calibrated angle proxy != measured wheel angle/Ackermann geometry.
 * Pure C: output proposal only; MCU atomic STOP/NC owner remains final.
 */
#define M8S153_RIGHT_MAX_EXTRA_CCR 12U
#define M8S153_RIGHT_DEADBAND_PM 20
#define M8S153_START_MOTION_RAW 12U
#define M8S153_TERMINAL_RAW 180U
/* D-pad LEFT baseline: RIGHT often ahead by 20..50pm mid-stroke,
 * but arrives 396..440ms late. This is an experimental preview,
 * restricted to BOTH axes 320..650 RAW from their own SOFT.
 */
#define M8S153_RMIN_MID_MIN_RAW 320U
#define M8S153_RMIN_MID_MAX_RAW 650U
#define M8S153_RMIN_MID_BIAS_PM 80L
#define M8S153_RMIN_MAX_LEAD_PM 80L

typedef struct {
 uint16_t right_pwm, boost_ccr;
 uint8_t eligible, requested, early;
} M8S153Plan_t;

static inline M8S153Plan_t M8S153_PlanRight(
 uint16_t owner_l,uint16_t owner_r,uint16_t cap_r,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t moved_l,uint16_t moved_r,
 uint8_t moving,uint8_t nc_clear,const M8S152Plan_t *obs)
{
 M8S153Plan_t p={owner_r,0U,0U,0U,0U};
 uint32_t want,room;
 if(obs==0||moving==0U||nc_clear==0U||
    owner_l==0U||owner_r==0U||cap_r<=owner_r||
    remain_l<=M8S153_TERMINAL_RAW||
    remain_r<=M8S153_TERMINAL_RAW||
    moved_l<M8S153_START_MOTION_RAW||
    moved_r<M8S153_START_MOTION_RAW||
    obs->angle_valid==0U||obs->pred_valid==0U)
   return p;
 p.eligible=1U;
 if(obs->predicted<=M8S153_RIGHT_DEADBAND_PM)return p;
 p.requested=1U;
 want=(uint32_t)(obs->predicted-M8S153_RIGHT_DEADBAND_PM)/2U;
 if(want>M8S153_RIGHT_MAX_EXTRA_CCR)want=M8S153_RIGHT_MAX_EXTRA_CCR;
 room=(uint32_t)cap_r-owner_r;
 if(want>room)want=room;
 p.boost_ccr=(uint16_t)want;
 p.right_pwm=(uint16_t)((uint32_t)owner_r+want);
 return p;
}
/* Data-derived early RIGHT-only preview. Outside R_MIN middle
 * window reuse EXACT original S15.3 proposal. No changed motor owner,
 * LEFT path, last-100-RAW PWM, independent SOFT, NC or STOP.
 */
static inline M8S153Plan_t M8S153_PlanRightMid(
 uint16_t owner_l,uint16_t owner_r,uint16_t cap_r,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t moved_l,uint16_t moved_r,
 uint8_t moving,uint8_t nc_clear,uint8_t right_to_min,
 const M8S152Plan_t *obs)
{
 M8S153Plan_t base=M8S153_PlanRight(owner_l,owner_r,cap_r,
      remain_l,remain_r,moved_l,moved_r,moving,nc_clear,obs);
 M8S153Plan_t candidate;
 M8S152Plan_t adjusted;
 if(right_to_min!=1U||obs==0||obs->pred_valid==0U||
    remain_l<=M8S153_RMIN_MID_MIN_RAW||
    remain_r<=M8S153_RMIN_MID_MIN_RAW||
    remain_l>M8S153_RMIN_MID_MAX_RAW||
    remain_r>M8S153_RMIN_MID_MAX_RAW||
    obs->predicted<=-M8S153_RMIN_MAX_LEAD_PM)
   return base;
 adjusted=*obs;
 adjusted.predicted=M8S152_Clamp((int32_t)obs->predicted+
                                     M8S153_RMIN_MID_BIAS_PM);
 candidate=M8S153_PlanRight(owner_l,owner_r,cap_r,
      remain_l,remain_r,moved_l,moved_r,moving,nc_clear,&adjusted);
 if(candidate.boost_ccr>base.boost_ccr){candidate.early=1U;return candidate;}
 return base;
}
#endif
