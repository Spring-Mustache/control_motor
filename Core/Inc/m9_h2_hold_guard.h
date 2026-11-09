#ifndef M9_H2_HOLD_GUARD_H
#define M9_H2_HOLD_GUARD_H
#include <stdint.h>
#include "m9_h1_shadow.h"

/* H2-A is SUPERVISOR ONLY. Actuation interlock is HARD CLOSED:
 * there is no motor command, timer, GPIO or HAL API in this module.
 * Do not replace with a nonzero duty until motor current/thermal,
 * driver PWM=0 behavior, DIR and NC/STOP bench gates are verified.
 */
#define M9H2_WINDOW_MS            3000U
#define M9H2_SAMPLE_STALE_MS       250U
#define M9H2_MAX_HOLD_ERROR_RAW     32U
#define M9H2_SHADOW_SCORE_CAP       20
#define M9H2_KP_Q8                128
#define M9H2_KD_Q8                 32
#define M9H2_POWER_PERMITTED        0U

/* Must remain identical to main.c ControlMode_t:
 * CONTROL_MODE_IDLE=0, VOICE=1, PS2=2, STOP=3.
 */
#define M9H2_CONTROL_IDLE  0U
#define M9H2_CONTROL_VOICE 1U
#define M9H2_CONTROL_PS2   2U
#define M9H2_CONTROL_STOP  3U

typedef enum {
  M9H2_OFF = 0,
  M9H2_MONITOR = 1,
  M9H2_SHADOW = 2,
  M9H2_ARMED = 3,
  M9H2_ACTIVE = 4,
  M9H2_FAULT = 5,
  M9H2_COMPLETE = 6
} M9H2State;

typedef enum {
  M9H2_REASON_NONE = 0,
  M9H2_REASON_WINDOW_DONE = 1,
  M9H2_REASON_SAFETY = 2,
  M9H2_REASON_OWNER = 3,
  M9H2_REASON_NC = 4,
  M9H2_REASON_SENSOR = 5,
  M9H2_REASON_TIMEOUT = 6,
  M9H2_REASON_RANGE = 7,
  M9H2_REASON_DISARM = 8
} M9H2Reason;

typedef struct {
  uint32_t now_ms;
  uint16_t raw_l, raw_r;
  uint32_t sample_l_ms, sample_r_ms;
  uint32_t sample_seq_l, sample_seq_r;
  uint8_t valid_l, valid_r;
  uint8_t nc_mask;
  uint8_t safety_ok;
  uint8_t owners_idle;
  uint8_t wheels_stopped;
  uint8_t monitoring_allowed;
  uint8_t request_power;
} M9H2Input;

typedef struct {
  M9H1Shadow_t pd;
  uint32_t seq, started_ms, ended_ms;
  uint32_t seen_l, seen_r;
  uint16_t goal_l, goal_r, updates, blocked_power_requests;
  uint16_t fresh_l, fresh_r, errors, peak_error_l, peak_error_r;
  uint16_t peak_score_l, peak_score_r, denied_nc, denied_owner;
  int16_t score_l, score_r;
  uint16_t duty_l, duty_r; /* MUST be 0 for all H2-A paths. */
  uint8_t source, state, reason;
  uint8_t active_window, output_ever_nonzero;
} M9H2Guard;

/* Allow only the NORMAL post-DONE Voice/PS2 -> IDLE handoff.
 * Any new request, PS2 takeover, STOP or unknown transition revokes it.
 * 'voice_busy' means execution/ready/request/owned/pending Voice work.
 * This helper never grants actuator permission.
 */
uint8_t M9H2_ModeAllowed(uint8_t source, uint8_t started_mode,
                        uint8_t current_mode, uint8_t voice_busy,
                        uint8_t ps2_manual, uint8_t ps2_cancel);
void M9H2_Reset(M9H2Guard *g);
void M9H2_Begin(M9H2Guard *g, uint32_t seq, uint32_t now_ms,
                uint8_t source, uint16_t goal_l, uint16_t goal_r,
                uint32_t initial_seq_l, uint32_t initial_seq_r);
void M9H2_Step(M9H2Guard *g, const M9H2Input *in);
void M9H2_Cancel(M9H2Guard *g, uint8_t reason);
#endif /* M9_H2_HOLD_GUARD_H */
