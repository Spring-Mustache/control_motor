#ifndef STEER_S144_RMIN_CATCHUP_H
#define STEER_S144_RMIN_CATCHUP_H
#include <stdint.h>
/*
 * Anticipatory tracking bias for RIGHT-to-R_MIN only. The existing
 * S14.2 synchronized P+D planner decides the actual PWM and holds
 * its per-axis cap/stop/soft/IRQ/freshness guards unchanged.
 *
 * RIGHT was observed to lag LEFT ~0.5s specifically in the LAST
 * 100 RAW to R_MIN. We ask for up to 2.0 calibrated degrees of
 * early RIGHT lead while BOTH axes still have >320 RAW clearance.
 * Bias fades fully between 380 and 320 RAW. It never touches the
 * independent final soft approach or creates PWM directly.
 *
 * Trial is opt-in, Level 1, D-pad LEFT only and valid trajectory only.
 */
static inline uint16_t M8S144_RminBias(
    uint8_t enabled, uint8_t level, uint8_t dpad_left,
    uint8_t right_to_min, uint16_t remaining_l, uint16_t remaining_r,
    int16_t measured_gap_cdeg)
{
  uint16_t least=(remaining_l<remaining_r)?remaining_l:remaining_r;
  if (enabled!=1U || level!=1U || dpad_left!=1U ||
      right_to_min!=1U || least<=320U ||
      measured_gap_cdeg < -250)
    return 0U; /* A motor already >2.5deg ahead needs no extra lead. */
  if (least>=380U)return 200U;
  return (uint16_t)((uint32_t)200U*(least-320U)/60U);
}
#endif
