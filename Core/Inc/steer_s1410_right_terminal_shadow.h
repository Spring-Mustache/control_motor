#ifndef STEER_S1410_RIGHT_TERMINAL_SHADOW_H
#define STEER_S1410_RIGHT_TERMINAL_SHADOW_H
#include <stdint.h>

/* M8-S14.10 PURE SHADOW-ONLY: a speed-adaptive terminal taper hypothesis.
 * The caller MUST NEVER assign candidate_pwm to TIM/owner/CCR.
 * Based on observed ON-only RIGHT lag after 180 RAW; this profile is
 * illustrative, NOT approved for motor output. An observed 50-ms
 * travel proxy is NOT passive coasting distance or NC clearance.
 */
#define M8S1410_TERMINAL_RAW 180U
#define M8S1410_APPROACH_OFF_RAW 40U
#define M8S1410_MAX_SHADOW_EXTRA_CCR 20U
#define M8S1410_SPEED_AGE_MAX_MS 100U
#define M8S1410_FORECAST_MS 50U

typedef struct {
  uint16_t target_raw_s;
  uint16_t measured_raw_s;
  uint16_t owner_pwm;
  uint16_t candidate_pwm;
  uint16_t forecast_50ms_raw;
  uint16_t estimated_to_soft_ms;
  uint8_t valid;
  uint8_t would_raise;
  uint8_t distance_proxy_warning;
} M8S1410Shadow_t;

static inline M8S1410Shadow_t M8S1410_Plan(
    uint16_t remaining_raw,uint16_t filtered_speed_raw_s,
    uint16_t owner_pwm,uint16_t existing_owner_cap,
    uint8_t sensor_speed_valid,uint32_t sensor_age_ms,
    uint8_t moving_r_min)
{
  M8S1410Shadow_t out={0};
  uint32_t target,extra,travel;
  out.owner_pwm=owner_pwm;
  out.candidate_pwm=owner_pwm; /* NO actuator can see this shadow */
  if(moving_r_min==0U||owner_pwm==0U||remaining_raw==0U||
     remaining_raw>M8S1410_TERMINAL_RAW||
     sensor_speed_valid==0U||sensor_age_ms>M8S1410_SPEED_AGE_MAX_MS||
     filtered_speed_raw_s>10000U||existing_owner_cap<owner_pwm)
    return out;
  out.valid=1U;
  out.measured_raw_s=filtered_speed_raw_s;
  travel=((uint32_t)filtered_speed_raw_s*M8S1410_FORECAST_MS+999U)/1000U;
  if(travel>65535U)travel=65535U;
  out.forecast_50ms_raw=(uint16_t)travel;
  out.distance_proxy_warning=(uint8_t)(travel>=remaining_raw);
  out.estimated_to_soft_ms=(filtered_speed_raw_s>0U)?
    (uint16_t)((((uint32_t)remaining_raw*1000U)/filtered_speed_raw_s>65535U)?
      65535U:((uint32_t)remaining_raw*1000U)/filtered_speed_raw_s):65535U;
  /* Example per-distance speed target (RAW/s), deliberately mild
   * and prohibited from real output until coast/current are measured.
   * 180RAW->510, 100RAW->350, 40RAW->230.
   */
  target=150U+2U*(uint32_t)remaining_raw;
  out.target_raw_s=(uint16_t)target;
  /* Never hypothesize MORE power inside 40RAW; owner taper/STOP only. */
  if(remaining_raw<=M8S1410_APPROACH_OFF_RAW ||
     filtered_speed_raw_s>=target ||
     owner_pwm>=existing_owner_cap)return out;
  extra=(target-filtered_speed_raw_s)/20U;
  if(extra>M8S1410_MAX_SHADOW_EXTRA_CCR)
    extra=M8S1410_MAX_SHADOW_EXTRA_CCR;
  if(extra>(uint32_t)(existing_owner_cap-owner_pwm))
    extra=(uint32_t)(existing_owner_cap-owner_pwm);
  out.candidate_pwm=(uint16_t)(owner_pwm+extra);
  out.would_raise=(uint8_t)(extra>0U);
  return out;
}
#endif /* STEER_S1410_RIGHT_TERMINAL_SHADOW_H */
