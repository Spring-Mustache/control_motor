#ifndef M9_H0_DRIFT_H
#define M9_H0_DRIFT_H

/* M9-H0: pure, read-only AS5600 drift accounting. No HAL, GPIO, PWM or I2C. */
#include <stdint.h>

#define M9H0_WINDOW_MS 3000U

typedef struct
{
  uint16_t goal_raw;
  uint16_t start_raw;
  uint16_t end_raw;
  uint16_t peak_drift_raw;
  uint16_t peak_error_raw;
  uint16_t peak_velocity_raw_s;
  uint16_t samples;
  uint16_t invalid_samples;
  uint16_t jump_samples;
  uint16_t zero_dt_samples;
  uint32_t last_sample_ms;
  uint8_t has_sample;
} M9H0Axis_t;

typedef struct
{
  M9H0Axis_t left;
  M9H0Axis_t right;
  uint32_t seq;
  uint32_t start_ms;
  uint32_t duration_ms;
  uint32_t dropped;
  uint8_t source; /* 1 PS2 R1, 2 VC02 B0, 3 VC02 5..40 */
  uint8_t mode;   /* 1 CENTER, 4 encoder-position voice command */
  uint8_t reason; /* 1 window elapsed, 2 new steering owner, 3 stop/fault */
  uint8_t wheels_moved;
} M9H0Drift_t;

uint16_t M9H0_AbsDelta(uint16_t from_raw, uint16_t to_raw);
void M9H0_Begin(M9H0Drift_t *d, uint32_t seq, uint32_t now_ms,
                uint8_t source, uint8_t mode, uint16_t goal_l,
                uint16_t goal_r, uint16_t raw_l, uint16_t raw_r);
void M9H0_Feed(M9H0Drift_t *d, uint8_t left, uint16_t raw,
               uint8_t valid, uint32_t sample_ms);
void M9H0_Finish(M9H0Drift_t *d, uint32_t now_ms, uint8_t reason);

#endif /* M9_H0_DRIFT_H */
