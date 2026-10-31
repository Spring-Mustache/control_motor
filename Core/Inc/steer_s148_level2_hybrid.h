#ifndef STEER_S148_LEVEL2_HYBRID_H
#define STEER_S148_LEVEL2_HYBRID_H
#include <stdint.h>
#include "steer_s142_sync.h"

/* Pure M8-S14.8: per-axis MAX(PI, P+D) positive boost; never SUM.
 * No P+D trim is used, so a leading LEFT is never slowed for skew.
 * Caller enforces fresh AS5600, BOTH >180RAW, owner/NC/SOFT/STOP,
 * and after this helper the independent atomic late-IRQ gate.
 */
typedef struct {
  uint16_t left_pwm,right_pwm;
  uint8_t pd_win_l,pd_win_r,pi_win_l,pi_win_r;
} M8S148Plan_t;
static inline uint16_t M8S148_Select(uint16_t owner,uint16_t cap,
    uint16_t prev,uint16_t pd_boost,uint16_t pi_candidate,
    uint8_t *pd_win,uint8_t *pi_win)
{
  uint32_t pd=(uint32_t)owner+(uint32_t)pd_boost;
  uint32_t pi=pi_candidate;
  uint32_t demand=owner;
  uint32_t allowed=(uint32_t)prev+12U;
  if(owner==0U||cap<=owner)return owner;
  if(pd>cap)pd=cap;
  if(pi>cap)pi=cap;
  if(pd>demand)demand=pd;
  if(pi>demand)demand=pi;
  /* Preserve the already-approved owner PWM even when owner itself
   * rose before this function; extra boost cannot exceed +12 CCR. */
  if(allowed<owner)allowed=owner;
  if(demand>allowed)demand=allowed;
  if(demand>cap)demand=cap;
  *pd_win=(uint8_t)(pd>owner&&pd>=pi&&demand>owner);
  *pi_win=(uint8_t)(pi>owner&&pi>pd&&demand>owner);
  return (uint16_t)demand;
}
static inline M8S148Plan_t M8S148_Merge(
    uint16_t owner_l,uint16_t owner_r,
    uint16_t cap_l,uint16_t cap_r,
    uint16_t previous_l,uint16_t previous_r,
    const M8S142SyncPlan_t *pd,
    uint16_t pi_l,uint16_t pi_r)
{
  M8S148Plan_t out={owner_l,owner_r,0U,0U,0U,0U};
  if(pd==0||owner_l==0U||owner_r==0U)return out;
  out.left_pwm=M8S148_Select(owner_l,cap_l,previous_l,
       pd->boost_l,pi_l,&out.pd_win_l,&out.pi_win_l);
  out.right_pwm=M8S148_Select(owner_r,cap_r,previous_r,
       pd->boost_r,pi_r,&out.pd_win_r,&out.pi_win_r);
  return out;
}
#endif /* STEER_S148_LEVEL2_HYBRID_H */
