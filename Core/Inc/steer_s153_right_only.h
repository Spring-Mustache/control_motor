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
/* 19:33 RMIN195/SOFT245: last180RAW elapsed LEFT495/473ms,
 * RIGHT880/891ms for D-pad LEFT/RX LEFT (only 2 traces).
 * Means LEFT484ms RIGHT886ms are a PILOT, not a physical law.
 * S15.2 velocity is q-permille/s, NOT RAW/s. Convert via
 * validated near-full-length stroke (moved+remain) / 2000.
 * Anticipate only when BOTH shafts remain >320 and <=650 RAW.
 */
#define M8S153_RMIN_MID_MIN_RAW 320U
#define M8S153_RMIN_MID_MAX_RAW 650U
#define M8S153_ETA_TAIL_REFERENCE_RAW 180U
#define M8S153_ETA_TAIL_LEFT_MS 484U
#define M8S153_ETA_TAIL_RIGHT_MS 886U
#define M8S153_ETA_MIN_SPAN_RAW 700U
#define M8S153_ETA_MAX_SPAN_RAW 1000U
#define M8S153_ETA_MIN_Q_SPEED 250
#define M8S153_ETA_DEADBAND_MS 60L
#define M8S153_ETA_MS_PER_CCR 16L

typedef struct {
 uint16_t right_pwm, boost_ccr;
 uint8_t eligible, requested, early, eta_valid;
 int16_t eta_lag_ms;
} M8S153Plan_t;

static inline M8S153Plan_t M8S153_PlanRight(
 uint16_t owner_l,uint16_t owner_r,uint16_t cap_r,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t moved_l,uint16_t moved_r,
 uint8_t moving,uint8_t nc_clear,const M8S152Plan_t *obs)
{
 M8S153Plan_t p={0};p.right_pwm=owner_r;
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
/* ETA-only RIGHT-to-R_MIN controller. Old reactive M8S153_PlanRight()
 * kept as a pure-C reference, NOT called on the active branch.
 * NO S15.3 PWM increase when either shaft <=320RAW to its own SOFT.
 * Original independent STOP/NC/owner/atomic CCR gates remain downstream.
 */
static inline M8S153Plan_t M8S153_PlanRightMid(
 uint16_t owner_l,uint16_t owner_r,uint16_t cap_r,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t moved_l,uint16_t moved_r,
 uint8_t moving,uint8_t nc_clear,uint8_t right_to_min,
 const M8S152Plan_t *obs)
{
 M8S153Plan_t p={0};
 uint32_t span_l,span_r,raw_speed_l,raw_speed_r,eta_l,eta_r,want,room;
 int32_t lag;
 p.right_pwm=owner_r;
 if(right_to_min!=1U||moving==0U||nc_clear==0U||obs==0||
    owner_l==0U||owner_r==0U||cap_r<=owner_r||
    remain_l<=M8S153_RMIN_MID_MIN_RAW||
    remain_r<=M8S153_RMIN_MID_MIN_RAW||
    remain_l>M8S153_RMIN_MID_MAX_RAW||
    remain_r>M8S153_RMIN_MID_MAX_RAW||
    moved_l<M8S153_START_MOTION_RAW||
    moved_r<M8S153_START_MOTION_RAW||
    obs->angle_valid==0U||obs->pred_valid==0U||
    obs->velL<M8S153_ETA_MIN_Q_SPEED||
    obs->velR<M8S153_ETA_MIN_Q_SPEED)
    return p;
 span_l=(uint32_t)moved_l+(uint32_t)remain_l;
 span_r=(uint32_t)moved_r+(uint32_t)remain_r;
 if(span_l<M8S153_ETA_MIN_SPAN_RAW||
    span_l>M8S153_ETA_MAX_SPAN_RAW||
    span_r<M8S153_ETA_MIN_SPAN_RAW||
    span_r>M8S153_ETA_MAX_SPAN_RAW)
    return p;
 /* S15.2 velQ = q per mille per SECOND; full stroke ~2000 q.
  * All integer math bounded by spans<=1000, remain<=650, vel<=2000.
  */
 raw_speed_l=((uint32_t)obs->velL*span_l)/2000U;
 raw_speed_r=((uint32_t)obs->velR*span_r)/2000U;
 if(raw_speed_l==0U||raw_speed_r==0U)return p;
 eta_l=(((uint32_t)remain_l-M8S153_ETA_TAIL_REFERENCE_RAW)*1000U)
       /raw_speed_l+M8S153_ETA_TAIL_LEFT_MS;
 eta_r=(((uint32_t)remain_r-M8S153_ETA_TAIL_REFERENCE_RAW)*1000U)
       /raw_speed_r+M8S153_ETA_TAIL_RIGHT_MS;
 lag=(int32_t)eta_r-(int32_t)eta_l;
 p.eligible=1U;p.eta_valid=1U;p.eta_lag_ms=M8S152_Clamp(lag);
 if(lag<=M8S153_ETA_DEADBAND_MS)return p;
 want=(uint32_t)(lag-M8S153_ETA_DEADBAND_MS)
     /(uint32_t)M8S153_ETA_MS_PER_CCR;
 if(want>M8S153_RIGHT_MAX_EXTRA_CCR)want=M8S153_RIGHT_MAX_EXTRA_CCR;
 room=(uint32_t)cap_r-owner_r;
 if(want>room)want=room;
 if(want==0U)return p;
 p.boost_ccr=(uint16_t)want;
 p.right_pwm=(uint16_t)((uint32_t)owner_r+want);
 p.requested=1U;p.early=1U;
 return p;
}
#endif
