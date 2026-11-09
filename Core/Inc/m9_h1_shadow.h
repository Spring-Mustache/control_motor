#ifndef M9_H1_SHADOW_H
#define M9_H1_SHADOW_H

/* M9-H1: PURE-C diagnostic PD. All output is signed SHADOW UNITS,
 * NOT PWM/CCR, and there is deliberately no motor API in this module.
 * These exploratory gains are not validated on actual hardware.
 */
#include <stdint.h>

#define M9H1_WINDOW_MS        100U
#define M9H1_STALE_MS         250U
#define M9H1_MAX_STEP_RAW      90U
#define M9H1_ENTER_RAW          8U
#define M9H1_EXIT_RAW           4U
#define M9H1_DEADBAND_RAW       5U
#define M9H1_KP_Q8           256
#define M9H1_KD_Q8           128
#define M9H1_CAP_UNITS         40

typedef struct
{
  uint16_t goal_raw;
  uint16_t reference_raw;
  uint16_t last_raw;
  uint32_t reference_ms;
  uint32_t last_sample_ms;
  int16_t error_raw, velocity_raw_s, candidate_units;
  uint16_t peak_velocity, peak_candidate;
  uint16_t samples, noise_suppressed, invalid, stale;
  uint16_t jump_rejected, zero_dt, saturated, brake_frames, reversals;
  int8_t prior_command_sign;
  uint8_t have_reference, velocity_ready, position_engaged;
  uint8_t stale_latched;
} M9H1Axis_t;

typedef struct
{
  M9H1Axis_t left, right;
  uint32_t seq;
  uint8_t source, reason;
} M9H1Shadow_t;

int16_t M9H1_DeltaRaw(uint16_t from_raw, uint16_t to_raw);
void M9H1_Begin(M9H1Shadow_t *d, uint32_t seq, uint8_t source,
                uint16_t left_goal, uint16_t right_goal);
void M9H1_Feed(M9H1Shadow_t *d, uint8_t left, uint16_t raw,
               uint8_t valid, uint32_t sample_ms, uint32_t now_ms);
void M9H1_Tick(M9H1Shadow_t *d, uint32_t now_ms);
void M9H1_Finish(M9H1Shadow_t *d, uint8_t reason);

#endif /* M9_H1_SHADOW_H */
