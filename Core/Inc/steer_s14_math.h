#ifndef STEER_S14_MATH_H
#define STEER_S14_MATH_H
#include <stdint.h>

/* M8-S14: pure cross-coupled steering planner, no motor/IRQ access.
 * Inputs use SIGNED-ALONG-COMMAND calibrated centidegrees and cdeg/s.
 * A predictive phase error combines angular travel and relative speed.
 * The ONLY requested action is a bounded PWM reduction of the leader.
 * No speed-up, direction write or active brake is generated here.
 */
typedef struct
{
  int16_t predicted_error_cdeg;
  uint16_t trim_left_pwm;
  uint16_t trim_right_pwm;
} M8S14Plan_t;

static inline uint16_t M8S14_Slew(uint16_t current,
                                   uint16_t target, uint16_t step)
{
  if (step < 1U) step = 1U;
  if (step > 12U) step = 12U;
  if (target > current)
    return ((uint32_t)target - current > step) ?
        (uint16_t)(current + step) : target;
  return ((uint32_t)current - target > step) ?
      (uint16_t)(current - step) : target;
}

static inline M8S14Plan_t M8S14_Plan(int16_t error_cdeg,
                                      uint16_t speed_l_cdeg_s,
                                      uint16_t speed_r_cdeg_s,
                                      uint16_t distance_min_raw)
{
  M8S14Plan_t plan = {0};
  int32_t predicted = (int32_t)error_cdeg +
      ((int32_t)speed_l_cdeg_s - (int32_t)speed_r_cdeg_s) * 120 / 1000;
  uint32_t excess, reduction;
  if (predicted > 10000) predicted = 10000;
  if (predicted < -10000) predicted = -10000;
  plan.predicted_error_cdeg = (int16_t)predicted;
  /* Existing owner controls the last 100 RAW; fade S14 from 200 RAW.
   * The PWM floor, soft target and NC are enforced by main.c.
   */
  if (distance_min_raw <= 100U) return plan;
  if (predicted > 50)
  {
    excess = (uint32_t)(predicted - 50);
    reduction = excess * 25U / 100U; /* 25 CCR per degree of lead */
    if (reduction > 96U) reduction = 96U;
    if (distance_min_raw < 200U)
      reduction = reduction * (distance_min_raw - 100U) / 100U;
    plan.trim_left_pwm = (uint16_t)reduction;
  }
  else if (predicted < -50)
  {
    excess = (uint32_t)(-predicted - 50);
    reduction = excess * 25U / 100U;
    if (reduction > 48U) reduction = 48U;
    if (distance_min_raw < 200U)
      reduction = reduction * (distance_min_raw - 100U) / 100U;
    plan.trim_right_pwm = (uint16_t)reduction;
  }
  return plan;
}
#endif /* STEER_S14_MATH_H */
