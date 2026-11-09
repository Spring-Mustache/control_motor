#ifndef M9_H2B1_COMMISSIONING_H
#define M9_H2B1_COMMISSIONING_H
#include "m9_h2b1_hold.h"

/* Final software delivery: persistent two-axis, feedback-ended recovery.
 * PWM envelope and proportional terms come from M8 position motion:
 * LEFT175..300 + error/2, RIGHT50..100 + error/4. These are MOTION
 * settings, not measured holding-current/thermal ratings. No current
 * feedback exists. Physical acceptance stays explicit and remains0.
 * Stop at the goal; moving recovery is not split into short test pulses.
 * Stiction compensation rises only within these same M8 caps. A stalled
 * axis rests independently, retains its goal, and retries automatically.
 * STOP/NC/sensor/owner faults revoke; boot never invents a target.
 */
#define M9H2B1_POWER_COMMISSIONED 0U
#define M9H2B1_POWER_MODE        3U
#define M9H2B1_LEFT_MAX_PWM    300U
#define M9H2B1_RIGHT_MAX_PWM   100U
#define M9H2B1_LEFT_MIN_PWM    175U
#define M9H2B1_RIGHT_MIN_PWM    50U
#define M9H2B1_NO_PROGRESS_MS 1000U
#define M9H2B1_RECOVERY_MS    5000U
#define M9H2B1_REST_MS        3000U
#if M9H2B1_POWER_MODE != 3U
#error "Final delivery requires recovery profile3; acceptance is a separate record"
#endif
#if M9H2B1_LEFT_MAX_PWM > M9B1_RECOVERY_LEFT_CEILING || \
    M9H2B1_RIGHT_MAX_PWM > M9B1_RECOVERY_RIGHT_CEILING || \
    !M9H2B1_LEFT_MIN_PWM || !M9H2B1_RIGHT_MIN_PWM || \
    M9H2B1_LEFT_MIN_PWM > M9H2B1_LEFT_MAX_PWM || \
    M9H2B1_RIGHT_MIN_PWM > M9H2B1_RIGHT_MAX_PWM || \
    !M9H2B1_NO_PROGRESS_MS || M9H2B1_NO_PROGRESS_MS > 1000U || \
    M9H2B1_RECOVERY_MS > 5000U || \
    M9H2B1_RECOVERY_MS <= M9H2B1_NO_PROGRESS_MS || M9H2B1_REST_MS < 3000U
#error "Final recovery requires valid M8 motion envelope and response deadlines"
#endif
static const M9B1Config m9_h2b1_config = {
  {{M9H2B1_LEFT_MAX_PWM,M9H2B1_LEFT_MIN_PWM,128U,32U,2000U,200U,0U,0U},
   {M9H2B1_RIGHT_MAX_PWM,M9H2B1_RIGHT_MIN_PWM,64U,12U,1000U,200U,0U,0U}},
  8U,4U,5U,80U,40U,90U,80U,
  0U,M9H2B1_RECOVERY_MS,M9H2B1_NO_PROGRESS_MS,
  0U,0U,M9H2B1_REST_MS,0U,M9H2B1_POWER_MODE
};
#endif
