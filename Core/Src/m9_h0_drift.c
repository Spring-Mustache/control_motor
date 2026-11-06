#include "m9_h0_drift.h"
#include <string.h>

uint16_t M9H0_AbsDelta(uint16_t from_raw, uint16_t to_raw)
{
  int32_t delta = (int32_t)to_raw - (int32_t)from_raw;
  if ((from_raw > 4095U) || (to_raw > 4095U)) return 4095U;
  if (delta > 2047) delta -= 4096;
  if (delta < -2048) delta += 4096;
  return (uint16_t)((delta < 0) ? -delta : delta);
}

void M9H0_Begin(M9H0Drift_t *d, uint32_t seq, uint32_t now_ms,
                uint8_t source, uint8_t mode, uint16_t goal_l,
                uint16_t goal_r, uint16_t raw_l, uint16_t raw_r)
{
  (void)memset(d, 0, sizeof(*d));
  d->seq = seq;
  d->start_ms = now_ms;
  d->source = source;
  d->mode = mode;
  d->left.goal_raw = goal_l;
  d->right.goal_raw = goal_r;
  d->left.start_raw = raw_l;
  d->right.start_raw = raw_r;
  d->left.end_raw = raw_l;
  d->right.end_raw = raw_r;
  d->left.peak_error_raw = M9H0_AbsDelta(goal_l, raw_l);
  d->right.peak_error_raw = M9H0_AbsDelta(goal_r, raw_r);
}

void M9H0_Feed(M9H0Drift_t *d, uint8_t left, uint16_t raw,
               uint8_t valid, uint32_t sample_ms)
{
  M9H0Axis_t *a = (left != 0U) ? &d->left : &d->right;
  uint16_t drift, error, step;
  uint32_t dt, velocity;
  if ((valid == 0U) || (raw > 4095U))
  {
    if (a->invalid_samples < UINT16_MAX) a->invalid_samples++;
    return;
  }
  drift = M9H0_AbsDelta(a->start_raw, raw);
  error = M9H0_AbsDelta(a->goal_raw, raw);
  if (drift > a->peak_drift_raw) a->peak_drift_raw = drift;
  if (error > a->peak_error_raw) a->peak_error_raw = error;
  if (a->has_sample != 0U)
  {
    step = M9H0_AbsDelta(a->end_raw, raw);
    if ((step > 90U) && (a->jump_samples < UINT16_MAX))
      a->jump_samples++;
    dt = (uint32_t)(sample_ms - a->last_sample_ms);
    if (dt == 0U)
    {
      if (a->zero_dt_samples < UINT16_MAX) a->zero_dt_samples++;
    }
    else
    {
      velocity = (uint32_t)step * 1000U / dt;
      if (velocity > UINT16_MAX) velocity = UINT16_MAX;
      if (velocity > a->peak_velocity_raw_s)
        a->peak_velocity_raw_s = (uint16_t)velocity;
    }
  }
  a->end_raw = raw;
  a->last_sample_ms = sample_ms;
  a->has_sample = 1U;
  if (a->samples < UINT16_MAX) a->samples++;
}

void M9H0_Finish(M9H0Drift_t *d, uint32_t now_ms, uint8_t reason)
{
  d->duration_ms = (uint32_t)(now_ms - d->start_ms);
  d->reason = reason;
}
