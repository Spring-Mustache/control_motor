#ifndef M9_H2B_HOLD_POLICY_H
#define M9_H2B_HOLD_POLICY_H

#include <stdint.h>
#include "m9_h2_hold_guard.h"

/* H2-B0: power-stage architecture and policy based on REAL M9 H1/H2 data.
 *
 * NOT an authorized motor current limit. No steering motor is powered
 * by this module; CCR remains zero until the actual 24V motor + SHB12
 * stall-current, thermal, PWM=0/brake and cutoff characteristics have
 * been measured and recorded. Never equate "score" to PWM duty.
 *
 * Strict compile-time lock, intentionally not runtime configurable.
 */
#define M9H2B_POWER_STAGE_ENABLED       0U
#define M9H2B_VERIFIED_MOTOR_PWM_LIMIT  0U
#define M9H2B_VERIFIED_ON_TIME_MS       0U
#define M9H2B_SCORE_CAP                20
#define M9H2B_SCORE_RISE_STEP            3
#define M9H2B_MAX_POLICY_MS          3000U
#define M9H2B_SCORE_ENTER_RAW           8U
#define M9H2B_SCORE_EXIT_RAW            4U

#if M9H2B_POWER_STAGE_ENABLED != 0U
#error "H2-B power must not be enabled before measured motor current/thermal/STOP/NC proof and redesigned actuator lease"
#endif
#if M9H2B_VERIFIED_MOTOR_PWM_LIMIT != 0U || M9H2B_VERIFIED_ON_TIME_MS != 0U
#error "H2-B calibration values are intentionally unknown until verified on rig"
#endif

typedef enum {
  M9H2B_OFF = 0,
  M9H2B_MONITOR = 1,
  M9H2B_CANDIDATE = 2,
  M9H2B_COMPLETE = 3,
  M9H2B_REVOKED = 4
} M9H2BState;

typedef enum {
  M9H2B_NONE = 0,
  M9H2B_WINDOW_DONE = 1,
  M9H2B_GUARD_REVOKED = 2,
  M9H2B_UNCALIBRATED = 3,
  M9H2B_MISMATCH = 4,
  M9H2B_EXPIRED = 5
} M9H2BReason;

typedef struct {
  uint32_t seq, start_ms, end_ms;
  uint16_t frames, eligible_l, eligible_r;
  uint16_t damping_veto_l, damping_veto_r;
  uint16_t blocked_power, bad_source;
  uint16_t peak_candidate_l, peak_candidate_r;
  int16_t candidate_l, candidate_r;
  uint16_t motor_ccr_l, motor_ccr_r;  /* ALWAYS ZERO */
  uint8_t active, state, reason, active_power_ever;
  uint8_t axis_l, axis_r;
} M9H2BPolicy;

void M9H2B_Reset(M9H2BPolicy *p);
void M9H2B_Begin(M9H2BPolicy *p, const M9H2Guard *g);
void M9H2B_Step(M9H2BPolicy *p, const M9H2Guard *g,
                 uint32_t now_ms, uint8_t request_power);
#endif
