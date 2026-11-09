/* Historical profile for regression ONLY; never included by firmware. */
#ifndef M9_H2B1_LEGACY_TUNING_H
#define M9_H2B1_LEGACY_TUNING_H
#include "m9_h2b1_hold.h"

/* USER authorized powered two-axis tuning using existing STOP/NC,
 * without a prerequisite motor-supply disconnect test (2026-11-09).
 * Initial engineering test values, NOT motor or thermal calibration.
 * Continuous PD remains CCR30/10. Separate short stiction-assist pulses
 * use LEFT CENTER travel cap300CCR in a feedback-shortened <=200ms
 * recovery window. RIGHT keeps fine values85CCR/12ms, with settle,
 * finite attempts and SysTick cutoff. These are experimental choices,
 * NOT verified motor/current/thermal limits or a continuous PWM floor.
 * No external ammeter/temperature feedback reaches MCU.
 * Boot has no goal/output. Only a fresh, eligible BOTH-DONE handoff
 * starts persistent HOLD. No debugger/serial write can raise these caps.
 * No-progress/effort events retain the target, rest both outputs, and
 * retry after the output-time budgets recover. STOP/NC/sensor/range and
 * new-owner revocations remain latched; there is no current feedback.
 */
#define M9H2B1_POWER_COMMISSIONED 0U
#define M9H2B1_POWER_MODE        1U /* 0 locked, 1 tuning, 2 commissioned */
#define M9H2B1_LEFT_MAX_PWM      30U
#define M9H2B1_RIGHT_MAX_PWM     10U
#define M9H2B1_LEFT_ASSIST_PWM  300U
#define M9H2B1_RIGHT_ASSIST_PWM  85U
#define M9H2B1_LEFT_ASSIST_MS   200U
#define M9H2B1_RIGHT_ASSIST_MS   12U
#define M9H2B1_ASSIST_STILL_MS  200U
#define M9H2B1_ASSIST_OFF_MS    200U
#define M9H2B1_ASSIST_REST_MS  3000U
#define M9H2B1_ASSIST_PULSES      4U
#define M9H2B1_LEFT_KP_Q8       512U
#define M9H2B1_RIGHT_KP_Q8      192U
#define M9H2B1_LEFT_KD_Q8        32U
#define M9H2B1_RIGHT_KD_Q8       12U
#define M9H2B1_LEFT_RISE        60U
#define M9H2B1_RIGHT_RISE       20U
#define M9H2B1_LEFT_REV_ZERO_MS 200U
#define M9H2B1_RIGHT_REV_ZERO_MS 200U
#define M9H2B1_TUNING_WINDOW_MS 10000U
#define M9H2B1_TUNING_EFFORT_MS 1500U
#define M9H2B1_NO_PROGRESS_MS   1000U
#if M9H2B1_POWER_MODE > 2U || \
    (M9H2B1_POWER_MODE == 2U && !M9H2B1_POWER_COMMISSIONED)
#error "H2-B1 commissioned mode requires physical acceptance"
#endif
#if M9H2B1_POWER_MODE && \
   (!M9H2B1_LEFT_MAX_PWM || !M9H2B1_RIGHT_MAX_PWM || \
    !M9H2B1_LEFT_KP_Q8 || !M9H2B1_RIGHT_KP_Q8 || \
    !M9H2B1_LEFT_KD_Q8 || !M9H2B1_RIGHT_KD_Q8 || \
    !M9H2B1_LEFT_RISE || !M9H2B1_RIGHT_RISE || \
    !M9H2B1_LEFT_REV_ZERO_MS || !M9H2B1_RIGHT_REV_ZERO_MS)
#error "H2-B1 requires valid two-axis PWM/gains/reversal settings"
#endif
#if M9H2B1_POWER_MODE == 1U && \
   (M9H2B1_LEFT_MAX_PWM > M9B1_TUNING_LEFT_CEILING || \
    M9H2B1_RIGHT_MAX_PWM > M9B1_TUNING_RIGHT_CEILING || \
    !M9H2B1_TUNING_EFFORT_MS || \
    M9H2B1_TUNING_WINDOW_MS <= M9H2B1_TUNING_EFFORT_MS || \
    !M9H2B1_NO_PROGRESS_MS || M9H2B1_NO_PROGRESS_MS > 5000U)
#error "H2-B1 tuning requires bounded PWM, effort and progress guards"
#endif
#if M9H2B1_LEFT_ASSIST_PWM > M9B1_ASSIST_LEFT_CEILING || \
    M9H2B1_RIGHT_ASSIST_PWM > M9B1_ASSIST_RIGHT_CEILING || \
    M9H2B1_LEFT_ASSIST_MS > M9B1_ASSIST_LEFT_MS || \
    M9H2B1_RIGHT_ASSIST_MS > M9B1_ASSIST_RIGHT_MS || \
    M9H2B1_ASSIST_STILL_MS < 200U || M9H2B1_ASSIST_OFF_MS < 200U || \
    M9H2B1_ASSIST_REST_MS < 3000U || !M9H2B1_ASSIST_PULSES || \
    M9H2B1_ASSIST_PULSES > M9B1_ASSIST_ROUND_MAX
#error "H2-B1 assist requires bounded PWM, pulse duration, settle and rest"
#endif
static const M9B1Config m9_h2b1_config = {
  {{M9H2B1_LEFT_MAX_PWM,0U,M9H2B1_LEFT_KP_Q8,M9H2B1_LEFT_KD_Q8,
      M9H2B1_LEFT_RISE,M9H2B1_LEFT_REV_ZERO_MS,
      M9H2B1_LEFT_ASSIST_PWM,M9H2B1_LEFT_ASSIST_MS},
   {M9H2B1_RIGHT_MAX_PWM,0U,M9H2B1_RIGHT_KP_Q8,M9H2B1_RIGHT_KD_Q8,
      M9H2B1_RIGHT_RISE,M9H2B1_RIGHT_REV_ZERO_MS,
      M9H2B1_RIGHT_ASSIST_PWM,M9H2B1_RIGHT_ASSIST_MS}},
  8U,4U,5U,250U,100U,90U,250U,
  M9H2B1_TUNING_WINDOW_MS,M9H2B1_TUNING_EFFORT_MS,
  M9H2B1_NO_PROGRESS_MS,M9H2B1_ASSIST_STILL_MS,M9H2B1_ASSIST_OFF_MS,
  M9H2B1_ASSIST_REST_MS,M9H2B1_ASSIST_PULSES,M9H2B1_POWER_MODE
};
#endif
