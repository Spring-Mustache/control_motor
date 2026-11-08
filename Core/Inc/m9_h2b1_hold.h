#ifndef M9_H2B1_HOLD_H
#define M9_H2B1_HOLD_H
#include <stdint.h>

/* Persistent, TWO-axis position HOLD. There is deliberately no session
 * expiry and no single-axis bench mode. PWM units below are real timer
 * counts; gains/limits must be commissioned, never copied from H1 scores.
 */
#define M9H2B1_INTEGRATED 1
typedef enum {
  M9B1_OFF=0, M9B1_WAIT_DATA=1, M9B1_UNCOMMISSIONED=2,
  M9B1_HOLDING=3, M9B1_REVOKED=4, M9B1_FAULT=5
} M9B1State;
typedef enum {
  M9B1_NONE=0, M9B1_NEW_OWNER=1, M9B1_STOP=2, M9B1_NC=3,
  M9B1_SENSOR=4, M9B1_RANGE=5, M9B1_DISABLED=6,
  M9B1_CALIBRATION_REQUIRED=7, M9B1_BAD_CONFIG=8,
  M9B1_SERVICE_GAP=9, M9B1_PORT_FAULT=10
} M9B1Reason;
typedef struct {
  uint16_t max_pwm, min_pwm;
  uint16_t kp_q8, kd_q8; /* PWM/RAW and PWM/(RAW/s), scaled by256. */
  uint16_t rise_per_second, reversal_zero_ms;
} M9B1AxisConfig;
typedef struct {
  M9B1AxisConfig axis[2]; /* index0 LEFT, index1 RIGHT */
  uint16_t enter_raw, exit_raw, deadband_raw, max_error_raw;
  uint16_t sample_stale_ms, velocity_window_ms, max_jump_raw;
  uint16_t service_gap_ms;
  uint8_t power_commissioned;
} M9B1Config;
typedef struct {
  uint32_t now_ms, sample_ms[2], sample_seq[2];
  uint16_t raw[2];
  uint8_t valid[2], safety_ok, owners_free, geometry_ok, nc_mask, enabled;
} M9B1Input;
typedef struct {
  uint32_t last_sample_ms, last_seq, velocity_ms, reversal_ms;
  uint32_t rise_remainder, good_samples;
  uint16_t raw, velocity_raw, peak_pwm, peak_error;
  int16_t error, pwm;
  int32_t velocity;
  int8_t last_sign, pending_sign;
  uint8_t initialized, velocity_ready, position_latched, reversing;
} M9B1Axis;
typedef struct {
  M9B1Axis axis[2];
  uint16_t goal[2];
  uint32_t seq, started_ms, last_step_ms, updates;
  uint32_t applied_ms, last_applied_ms;
  int16_t applied_pwm[2];
  uint8_t source, active, state, reason, lease, ever_applied;
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
  void (*pwm)(void *context, uint8_t axis, uint16_t value);
  void (*direction)(void *context, uint8_t axis, int8_t raw_sign);
  uint32_t (*now)(void *context);
} M9B1Port;
uint8_t M9B1_Apply(M9B1Hold *h, const M9B1Config *c, const M9B1Port *p);
void M9B1_Cut(M9B1Hold *h, const M9B1Port *p, uint8_t reason);
#endif
