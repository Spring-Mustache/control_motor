#ifndef M9_H1_REAL_TRACE_H
#define M9_H1_REAL_TRACE_H
#include <stdint.h>
#include "m9_h1_shadow.h"

/* Read-only flight recorder for REAL M8 steering motion. No HAL, I2C,
 * motor or UART APIs. Single pending trace; no recording when full.
 */
#define M9REAL_MAX_FRAMES 80U
#define M9REAL_FRAME_PERIOD_MS 80U
#define M9REAL_MAX_SAMPLE_AGE_MS 80U

typedef struct {
  uint32_t t_ms;
  uint16_t raw_l, raw_r, ccr_l, ccr_r;
  int16_t err_l, err_r, vel_l, vel_r, cmd_l, cmd_r;
  uint16_t age_l, age_r;
  uint8_t nc_mask, state_l, state_r;
} M9RealFrame_t;

typedef struct {
  M9H1Shadow_t pd;
  M9RealFrame_t frame[M9REAL_MAX_FRAMES];
  uint32_t seq, start_ms, duration_ms;
  uint16_t goal_l, goal_r, start_l, start_r, end_l, end_r;
  uint16_t peak_ccr_l, peak_ccr_r;
  uint16_t stored, overflows, bad, skipped, stale;
  uint8_t source, mode, result, active;
} M9RealTrace_t;

void M9REAL_Begin(M9RealTrace_t *d, uint32_t seq, uint8_t source,
                  uint8_t mode, uint16_t goal_l, uint16_t goal_r,
                  uint16_t raw_l, uint16_t raw_r, uint32_t now_ms);
void M9REAL_Observe(M9RealTrace_t *d, uint32_t now_ms,
                    uint16_t raw_l, uint16_t raw_r,
                    uint8_t fresh_l, uint8_t fresh_r,
                    uint8_t valid_l, uint8_t valid_r,
                    uint32_t sample_l_ms, uint32_t sample_r_ms,
                    uint16_t ccr_l, uint16_t ccr_r,
                    uint8_t nc_mask, uint8_t state_l, uint8_t state_r);
void M9REAL_Finish(M9RealTrace_t *d, uint32_t now_ms, uint8_t result,
                   uint16_t raw_l, uint16_t raw_r);
#endif
