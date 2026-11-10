#ifndef M9_H2B1_HOLD_H
#define M9_H2B1_HOLD_H
#include <stdint.h>

/* Persistent, TWO-axis position HOLD. There is deliberately no session
 * expiry and no single-axis bench mode. PWM units below are real timer
 * counts. TUNING enables an explicitly experimental guarded profile;
 * COMMISSIONED is a separate physical acceptance state, never inferred
 * from a PWM cap, a host test or H1 shadow scores.
 */
#define M9H2B1_INTEGRATED 1
typedef enum {
  M9B1_OFF=0, M9B1_WAIT_DATA=1, M9B1_UNCOMMISSIONED=2,
  M9B1_HOLDING=3, M9B1_REVOKED=4, M9B1_FAULT=5,
  M9B1_COOLDOWN=6 /* target retained, both outputs zero until budget recovers */
} M9B1State;
typedef enum {
  M9B1_NONE=0, M9B1_NEW_OWNER=1, M9B1_STOP=2, M9B1_NC=3,
  M9B1_SENSOR=4, M9B1_RANGE=5, M9B1_DISABLED=6,
  M9B1_CALIBRATION_REQUIRED=7, M9B1_BAD_CONFIG=8,
  M9B1_SERVICE_GAP=9, M9B1_PORT_FAULT=10,
  M9B1_NO_PROGRESS=11, M9B1_EFFORT_BUDGET=12, M9B1_ASSIST_BUDGET=13
} M9B1Reason;
typedef enum {
  M9B1_POWER_LOCKED=0, M9B1_POWER_TUNING=1, M9B1_POWER_COMMISSIONED=2,
  M9B1_POWER_RECOVERY=3, /* historical recovery with periodic rest */
  M9B1_POWER_SERVO=4 /* continuous load-rejecting position servo */
} M9B1PowerMode;
#define M9B1_RECOVERY_LEFT_CEILING 300U
#define M9B1_RECOVERY_RIGHT_CEILING 100U
#define M9B1_OUTPUT_LEASE_MS 50U
#define M9B1_RECOVERY_POLL_MS 10U /* one sensor/visit; nominal 20ms per axis */
#define M9B1_SERVO_LEFT_KI_Q8 2048U /* 8 CCR/(RAW*s) */
#define M9B1_SERVO_RIGHT_KI_Q8 1024U /* 4 CCR/(RAW*s) */
#define M9B1_TUNING_LEFT_CEILING 30U
#define M9B1_TUNING_RIGHT_CEILING 10U
#define M9B1_ASSIST_LEFT_CEILING 300U
#define M9B1_ASSIST_RIGHT_CEILING 85U
#define M9B1_ASSIST_LEFT_MS 200U
#define M9B1_ASSIST_RIGHT_MS 12U
#define M9B1_ASSIST_ROUND_MAX 4U
#define M9B1_ASSIST_PULSE 1U
#define M9B1_ASSIST_SETTLE 2U
typedef struct {
  uint16_t max_pwm, min_pwm;
  uint16_t kp_q8, kd_q8; /* PWM/RAW and PWM/(RAW/s), scaled by256. */
  uint16_t rise_per_second, reversal_zero_ms;
  uint16_t assist_pwm, assist_ms;
} M9B1AxisConfig;
typedef struct {
  M9B1AxisConfig axis[2]; /* index0 LEFT, index1 RIGHT */
  /* Position error starts correction; it is not a cancellation limit.
   * The main adapter validates measured/goal geometry independently. */
  uint16_t enter_raw, exit_raw, deadband_raw;
  uint16_t sample_stale_ms, velocity_window_ms, max_jump_raw;
  uint16_t service_gap_ms;
  uint16_t tuning_window_ms, tuning_effort_ms, tuning_no_progress_ms;
  uint16_t assist_still_ms, assist_off_ms, assist_cooldown_ms;
  uint8_t assist_max_pulses;
  uint8_t power_mode;
} M9B1Config;
typedef struct {
  uint32_t now_ms, sample_ms[2], sample_seq[2];
  uint16_t raw[2];
  uint8_t valid[2], safety_ok, owners_free, geometry_ok, nc_mask, enabled;
} M9B1Input;
typedef struct {
  uint32_t last_sample_ms, last_seq, velocity_ms, reversal_ms;
  uint32_t rise_remainder, good_samples;
  uint32_t effort_ms, recovery_remainder, progress_on_ms, applied_ms;
  uint32_t assist_since_ms, assist_still_since_ms, assist_sample_seq, assist_count;
  uint16_t raw, velocity_raw, peak_pwm, peak_error;
  uint16_t progress_error;
  int16_t error, pwm;
  int32_t velocity;
  int8_t last_sign, pending_sign;
  uint8_t initialized, velocity_ready, position_latched, reversing;
  uint8_t progress_tracking;
  int8_t assist_sign;
  uint8_t assist_phase, assist_started, assist_still_tracking, assist_round;
  uint32_t recovery_on_ms, recovery_rest_ms, recovery_stuck_ms;
  uint16_t recovery_best_error;
  uint16_t recovery_boost_ms;
  uint8_t recovery_tracking, recovery_rest, recovery_reason;
  int32_t integral_q8, integral_remainder;
  int32_t servo_velocity_q8;
  uint32_t servo_wrong_i_ms;
} M9B1Axis;
typedef struct {
  M9B1Axis axis[2];
  uint16_t goal[2];
  uint32_t seq, started_ms, last_step_ms, updates;
  uint32_t applied_ms, last_applied_ms, retry_count;
  uint32_t cooldown_since_ms;
  int16_t applied_pwm[2];
  uint8_t source, active, state, reason, lease, ever_applied, fault_axes;
} M9B1Hold;
void M9B1_Reset(M9B1Hold *h);
uint8_t M9B1_ConfigValid(const M9B1Config *c);
uint8_t M9B1_Begin(M9B1Hold *h, const M9B1Config *c,
                   const M9B1Input *in, uint32_t seq, uint8_t source,
                   uint16_t goal_l, uint16_t goal_r);
void M9B1_Revoke(M9B1Hold *h, uint8_t reason);
void M9B1_Step(M9B1Hold *h, const M9B1Config *c, const M9B1Input *in);
void M9B1_Applied(M9B1Hold *h, uint32_t now_ms, int16_t left, int16_t right);
/* HAL-free atomic actuator port, compiled against inert mocks in HOST tests. */
typedef struct {
  void *context;
  uint32_t (*lock)(void *context);
  void (*unlock)(void *context, uint32_t previous);
  uint8_t (*allowed)(void *context);
  uint16_t (*pwm)(void *context, uint8_t axis, uint16_t value); /* actual write */
  void (*direction)(void *context, uint8_t axis, int8_t raw_sign);
  uint32_t (*now)(void *context);
} M9B1Port;
uint8_t M9B1_Apply(M9B1Hold *h, const M9B1Config *c, const M9B1Port *p);
void M9B1_Cut(M9B1Hold *h, const M9B1Port *p, uint8_t reason);
#endif
