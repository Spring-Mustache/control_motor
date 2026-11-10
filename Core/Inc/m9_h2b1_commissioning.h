#ifndef M9_H2B1_COMMISSIONING_H
#define M9_H2B1_COMMISSIONING_H
#include "m9_h2b1_hold.h"
/* Continuous position servo. Integral output balances a sustained load
 * even at zero measured error. No periodic rest or no-motion cutoff:
 * zero velocity under load can be successful HOLD. No current sensor
 * exists; PWM caps below are inherited motion caps, not thermal ratings.
 * STOP/NC/invalid feedback/new owner revoke AND clear load compensation.
 */
#define M9H2B1_POWER_COMMISSIONED 0U
#define M9H2B1_POWER_MODE        4U
#define M9H2B1_LEFT_MAX_PWM    300U
#define M9H2B1_RIGHT_MAX_PWM   100U
#if M9H2B1_POWER_MODE != 4U
#error "Current delivery requires continuous servo profile4"
#endif
#if !M9H2B1_LEFT_MAX_PWM || !M9H2B1_RIGHT_MAX_PWM || \
    M9H2B1_LEFT_MAX_PWM > M9B1_RECOVERY_LEFT_CEILING || \
    M9H2B1_RIGHT_MAX_PWM > M9B1_RECOVERY_RIGHT_CEILING
#error "Servo requires valid PWM caps within the inherited M8 envelope"
#endif
static const M9B1Config m9_h2b1_config = {
  {{M9H2B1_LEFT_MAX_PWM,0U,2048U,512U,2000U,200U,0U,0U},
   {M9H2B1_RIGHT_MAX_PWM,0U,1024U,256U,1000U,200U,0U,0U}},
  2U,0U,1U,80U,40U,90U,80U,
  0U,0U,0U,0U,0U,0U,0U,M9H2B1_POWER_MODE
};
#endif
