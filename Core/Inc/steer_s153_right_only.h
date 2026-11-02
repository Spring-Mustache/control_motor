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

typedef struct {
 uint16_t right_pwm, boost_ccr;
 uint8_t eligible, requested;
} M8S153Plan_t;

static inline M8S153Plan_t M8S153_PlanRight(
 uint16_t owner_l,uint16_t owner_r,uint16_t cap_r,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t moved_l,uint16_t moved_r,
 uint8_t moving,uint8_t nc_clear,const M8S152Plan_t *obs)
{
 M8S153Plan_t p={owner_r,0U,0U,0U};
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
#endif
