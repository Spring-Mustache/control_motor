#ifndef M9_H2B1_COMMISSIONING_H
#define M9_H2B1_COMMISSIONING_H
#include "m9_h2b1_hold.h"

/* UNKNOWN physical settings, NOT zero-current ratings. User-reported
 * motor-lead CENTER current LEFT0.8..1A / RIGHT~2A is running current,
 * not a continuous standstill rating. No measured current/temperature
 * feedback is connected to STM32. Never equate PWM cap to an amp limit.
 * Full persistent controller and actuator route are implemented; physical
 * power is closed until these motor/driver settings are commissioned.
 * A debugger/serial request cannot change these const settings.
 */
#define M9H2B1_POWER_COMMISSIONED 0U
#define M9H2B1_LEFT_MAX_PWM      0U
#define M9H2B1_RIGHT_MAX_PWM     0U
#define M9H2B1_LEFT_KP_Q8        0U
#define M9H2B1_RIGHT_KP_Q8       0U
#define M9H2B1_LEFT_KD_Q8        0U
#define M9H2B1_RIGHT_KD_Q8       0U
#define M9H2B1_LEFT_RISE        0U
#define M9H2B1_RIGHT_RISE       0U
#define M9H2B1_LEFT_REV_ZERO_MS 0U
#define M9H2B1_RIGHT_REV_ZERO_MS 0U
#if M9H2B1_POWER_COMMISSIONED && \
   (!M9H2B1_LEFT_MAX_PWM || !M9H2B1_RIGHT_MAX_PWM || \
    !M9H2B1_LEFT_KP_Q8 || !M9H2B1_RIGHT_KP_Q8 || \
    !M9H2B1_LEFT_KD_Q8 || !M9H2B1_RIGHT_KD_Q8 || \
    !M9H2B1_LEFT_RISE || !M9H2B1_RIGHT_RISE || \
    !M9H2B1_LEFT_REV_ZERO_MS || !M9H2B1_RIGHT_REV_ZERO_MS)
#error "H2-B1 requires commissioned two-axis PWM/gains/reversal settings"
#endif
static const M9B1Config m9_h2b1_config = {
  {{M9H2B1_LEFT_MAX_PWM,0U,M9H2B1_LEFT_KP_Q8,M9H2B1_LEFT_KD_Q8,
      M9H2B1_LEFT_RISE,M9H2B1_LEFT_REV_ZERO_MS},
   {M9H2B1_RIGHT_MAX_PWM,0U,M9H2B1_RIGHT_KP_Q8,M9H2B1_RIGHT_KD_Q8,
      M9H2B1_RIGHT_RISE,M9H2B1_RIGHT_REV_ZERO_MS}},
  8U,4U,5U,32U,250U,100U,90U,250U,M9H2B1_POWER_COMMISSIONED
};
#endif
