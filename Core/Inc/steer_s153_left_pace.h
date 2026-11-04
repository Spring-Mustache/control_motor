#ifndef STEER_S153_LEFT_PACE_H
#define STEER_S153_LEFT_PACE_H
#include <stdint.h>
#include "steer_s152_manual_relation.h"

/* M8-S15.3 candidate: estimate time-to-go to independent SOFT, then
 * reduce LEFT only if RIGHT is forecast to arrive later.
 *
 * Four f2137556 strokes suggest markedly different last-180RAW tails.
 * These are PILOT parameters, not calibrated physical constants.
 * No RIGHT PWM write; ZERO change within last 320RAW on EITHER axis.
 */
#define M8S153_PACE_TAIL_FROM_RAW         180U
#define M8S153_PACE_RELEASE_RAW           320U
#define M8S153_PACE_FULL_RAW              464U
#define M8S153_PACE_MAX_REDUCTION_CCR      72U
#define M8S153_PACE_LAG_DEADBAND_MS        60L
#define M8S153_PACE_MS_PER_CCR              5L
#define M8S153_PACE_MIN_SPAN_RAW         700U
#define M8S153_PACE_MAX_SPAN_RAW        1000U
#define M8S153_PACE_MIN_Q_SPEED         1100L
#define M8S153_PACE_MIN_RAW_SPEED        150U
#define M8S153_PACE_MAX_LAG_MS          1500L
/* Tail duration from four 2026-11-04 f2137556 curves at 180RAW.
 * RIGHT to MIN: LEFT 468/468ms; RIGHT 852/864ms.
 * RIGHT to MAX: LEFT 672/612ms; RIGHT 744/816ms.
 */
#define M8S153_PACE_RMIN_TAIL_L_MS        468U
#define M8S153_PACE_RMIN_TAIL_R_MS        858U
#define M8S153_PACE_RMAX_TAIL_L_MS        642U
#define M8S153_PACE_RMAX_TAIL_R_MS        780U

typedef struct {
 uint16_t left_pwm,reduction_ccr;
 int16_t forecast_right_late_ms;
 uint8_t eligible,eta_valid;
} M8S153PacePlan_t;

static inline M8S153PacePlan_t M8S153_LeftPacePlan(
 uint16_t owner_l,uint16_t owner_r,uint16_t floor_l,
 uint16_t remain_l,uint16_t remain_r,
 uint16_t span_l,uint16_t span_r,
 int8_t direction_l,uint8_t both_moving,uint8_t nc_clear,
 const M8S152Plan_t *obs)
{
 M8S153PacePlan_t p={0};
 int32_t vel_l,vel_r,lag,want;
 uint32_t raw_speed_l,raw_speed_r,eta_l,eta_r;
 uint16_t closest,fade_cap,room;
 p.left_pwm=owner_l;
 if(obs==0||direction_l==0||both_moving!=1U||nc_clear!=1U||
    owner_l<=floor_l||owner_r==0U||
    obs->angle_valid!=1U||obs->pred_valid!=1U||
    remain_l<=M8S153_PACE_RELEASE_RAW||
    remain_r<=M8S153_PACE_RELEASE_RAW||
    span_l<M8S153_PACE_MIN_SPAN_RAW||
    span_l>M8S153_PACE_MAX_SPAN_RAW||
    span_r<M8S153_PACE_MIN_SPAN_RAW||
    span_r>M8S153_PACE_MAX_SPAN_RAW)return p;
 /* qL and qR are direction-aligned shaft coordinate rates, so both
  * must be moving in the requested command direction. Reject jitter,
  * reversals and zero/weak-rate samples rather than guessing an ETA.
  */
 vel_l=(int32_t)obs->velL;
 vel_r=(int32_t)obs->velR;
 if(direction_l<0){vel_l=-vel_l;vel_r=-vel_r;}
 if(vel_l<M8S153_PACE_MIN_Q_SPEED||
    vel_r<M8S153_PACE_MIN_Q_SPEED)return p;
 raw_speed_l=(uint32_t)vel_l*(uint32_t)span_l/2000U;
 raw_speed_r=(uint32_t)vel_r*(uint32_t)span_r/2000U;
 if(raw_speed_l<M8S153_PACE_MIN_RAW_SPEED||
    raw_speed_r<M8S153_PACE_MIN_RAW_SPEED)return p;
 /* Magnitudes are bounded: 2000q/s x 1000RAW; division below safe. */
 eta_l=((uint32_t)(remain_l-M8S153_PACE_TAIL_FROM_RAW)*1000U)
           /raw_speed_l +
       ((direction_l>0)?M8S153_PACE_RMIN_TAIL_L_MS:
                         M8S153_PACE_RMAX_TAIL_L_MS);
 eta_r=((uint32_t)(remain_r-M8S153_PACE_TAIL_FROM_RAW)*1000U)
           /raw_speed_r +
       ((direction_l>0)?M8S153_PACE_RMIN_TAIL_R_MS:
                         M8S153_PACE_RMAX_TAIL_R_MS);
 lag=(int32_t)eta_r-(int32_t)eta_l;
 if(lag< -M8S153_PACE_MAX_LAG_MS||
    lag>M8S153_PACE_MAX_LAG_MS)return p;
 p.eligible=1U;p.eta_valid=1U;
 p.forecast_right_late_ms=(int16_t)lag;
 if(lag<=M8S153_PACE_LAG_DEADBAND_MS)return p;
 want=(lag-M8S153_PACE_LAG_DEADBAND_MS)/M8S153_PACE_MS_PER_CCR;
 if(want<=0L)return p;
 if(want>(int32_t)M8S153_PACE_MAX_REDUCTION_CCR)
   want=(int32_t)M8S153_PACE_MAX_REDUCTION_CCR;
 closest=(remain_l<remain_r)?remain_l:remain_r;
 /* At 464RAW and above full reduction is allowable; it fades to
  * ZERO by 320RAW for both axes, well before owner final 180/100RAW.
  */
 fade_cap=(uint16_t)((closest-M8S153_PACE_RELEASE_RAW)/2U);
 if(fade_cap>M8S153_PACE_MAX_REDUCTION_CCR)
   fade_cap=M8S153_PACE_MAX_REDUCTION_CCR;
 room=(uint16_t)(owner_l-floor_l);
 if((uint32_t)want>room)want=room;
 if((uint32_t)want>fade_cap)want=fade_cap;
 p.reduction_ccr=(uint16_t)want;
 p.left_pwm=(uint16_t)(owner_l-p.reduction_ccr);
 return p;
}
#endif /* STEER_S153_LEFT_PACE_H */
