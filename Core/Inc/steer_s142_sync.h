#ifndef STEER_S142_SYNC_H
#define STEER_S142_SYNC_H
#include <stdint.h>

/* Pure controller. Unit: calibrated, direction-aligned 0.01 degree
 * travelled and degree centi-units/second. PWM units are TIM CCR.
 * Output is confined to independent owner-provided limits.
 */
typedef struct
{
  uint16_t left_pwm, right_pwm;
  uint16_t boost_l, boost_r, trim_l, trim_r;
  int16_t predict_cdeg;
  uint8_t sat_l, sat_r;
} M8S142SyncPlan_t;

static inline M8S142SyncPlan_t M8S142_SyncPlan(
  uint16_t duty_l, uint16_t duty_r, uint16_t cap_l, uint16_t cap_r,
  uint16_t floor_l, uint16_t floor_r, int16_t travel_gap_cdeg,
  uint16_t speed_l_cdeg_s, uint16_t speed_r_cdeg_s,
  uint16_t min_remaining_raw)
{
  M8S142SyncPlan_t p = {0};
  int32_t prediction = (int32_t)travel_gap_cdeg +
    ((int32_t)speed_l_cdeg_s - (int32_t)speed_r_cdeg_s)*120/1000;
  uint32_t need, room, trim;
  if (prediction>9000) prediction=9000;
  if (prediction< -9000) prediction= -9000;
  p.predict_cdeg=(int16_t)prediction;
  if(duty_l>cap_l)duty_l=cap_l;
  if(duty_r>cap_r)duty_r=cap_r;
  p.left_pwm=duty_l;
  p.right_pwm=duty_r;
  /* The independent owner controls near-SOFT stopping and the zero
   * command. Never increase or slow either axis within 180 RAW.
   */
  if(duty_l==0U || duty_r==0U || min_remaining_raw<=180U)return p;
  if(prediction>40)
  {
    /* LEFT leads -> increase RIGHT first; taper LEFT only after
     * RIGHT PWM has exhausted permitted headroom AND still lags.
     */
    need=(uint32_t)(prediction-40)*30U/100U;
    if(need>75U)need=75U;
    room=(cap_r>duty_r)?(uint32_t)(cap_r-duty_r):0U;
    p.boost_r=(uint16_t)((need<room)?need:room);
    p.right_pwm=(uint16_t)(duty_r+p.boost_r);
    p.sat_r=(need>room && need!=0U)?1U:0U;
    if(p.sat_r && prediction>150 &&
       (uint32_t)speed_r_cdeg_s<=(uint32_t)speed_l_cdeg_s+100U)
    {
      trim=(uint32_t)(prediction-150)*12U/100U;
      if(trim>48U)trim=48U;
      if(duty_l>floor_l)
      {
        room=(uint32_t)(duty_l-floor_l);
        if(trim>room)trim=room;
        p.trim_l=(uint16_t)trim;
        p.left_pwm=(uint16_t)(duty_l-p.trim_l);
      }
    }
  }
  else if(prediction< -40)
  {
    /* RIGHT leads -> increase LEFT first; taper RIGHT only after
     * LEFT PWM is capped and cannot catch up.
     */
    need=(uint32_t)(-prediction-40)*30U/100U;
    if(need>75U)need=75U;
    room=(cap_l>duty_l)?(uint32_t)(cap_l-duty_l):0U;
    p.boost_l=(uint16_t)((need<room)?need:room);
    p.left_pwm=(uint16_t)(duty_l+p.boost_l);
    p.sat_l=(need>room && need!=0U)?1U:0U;
    if(p.sat_l && prediction< -150 &&
       (uint32_t)speed_l_cdeg_s<=(uint32_t)speed_r_cdeg_s+100U)
    {
      trim=(uint32_t)(-prediction-150)*12U/100U;
      if(trim>48U)trim=48U;
      if(duty_r>floor_r)
      {
        room=(uint32_t)(duty_r-floor_r);
        if(trim>room)trim=room;
        p.trim_r=(uint16_t)trim;
        p.right_pwm=(uint16_t)(duty_r-p.trim_r);
      }
    }
  }
  return p;
}
#endif /* STEER_S142_SYNC_H */
