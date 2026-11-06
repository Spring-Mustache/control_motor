#include "m9_h1_shadow.h"
#include <string.h>

/* Signed shortest path in AS5600 12-bit space; INVALID if outside range. */
int16_t M9H1_DeltaRaw(uint16_t from_raw, uint16_t to_raw)
{
  int32_t d;
  if ((from_raw > 4095U) || (to_raw > 4095U)) return 0;
  d = (int32_t)to_raw - (int32_t)from_raw;
  if (d > 2047) d -= 4096;
  if (d < -2048) d += 4096;
  return (int16_t)d;
}

static uint16_t M9H1_Abs(int32_t x)
{
  return (uint16_t)((x < 0) ? -x : x);
}

static void M9H1_Inc(uint16_t *counter)
{
  if (*counter != UINT16_MAX) ++(*counter);
}

static void M9H1_ClearOutput(M9H1Axis_t *a)
{
  a->velocity_ready = 0U;
  a->velocity_raw_s = 0;
  a->candidate_units = 0;
  a->prior_command_sign = 0;
  a->position_engaged = 0U;
}

void M9H1_Begin(M9H1Shadow_t *d, uint32_t seq, uint8_t source,
                uint16_t left_goal, uint16_t right_goal)
{
  (void)memset(d, 0, sizeof(*d));
  d->seq = seq;
  d->source = source;
  d->left.goal_raw = left_goal;
  d->right.goal_raw = right_goal;
}

static void M9H1_Candidate(M9H1Axis_t *a)
{
  int32_t e, e_eff, u_q8, u;
  int8_t sign;
  uint16_t abse;
  if (a->velocity_ready == 0U)
  {
    a->candidate_units = 0;
    return;
  }
  e = a->error_raw;
  abse = M9H1_Abs(e);
  if (abse >= M9H1_ENTER_RAW) a->position_engaged = 1U;
  else if (abse <= M9H1_EXIT_RAW) a->position_engaged = 0U;

  e_eff = 0;
  if (a->position_engaged != 0U)
  {
    if (e > M9H1_DEADBAND_RAW) e_eff = e - M9H1_DEADBAND_RAW;
    else if (e < -(int32_t)M9H1_DEADBAND_RAW)
      e_eff = e + M9H1_DEADBAND_RAW;
  }
  /* Q8 fixed-point PD: Kp*position error - Kd*velocity.
   * Damping remains active inside positional hysteresis if motion is real.
   * Signed shadow units are NOT electrical duty, torque or current.
   */
  u_q8 = M9H1_KP_Q8 * e_eff -
         M9H1_KD_Q8 * (int32_t)a->velocity_raw_s;
  u = u_q8 / 256;
  if (u > M9H1_CAP_UNITS)
  {
    u = M9H1_CAP_UNITS;
    M9H1_Inc(&a->saturated);
  }
  else if (u < -M9H1_CAP_UNITS)
  {
    u = -M9H1_CAP_UNITS;
    M9H1_Inc(&a->saturated);
  }
  a->candidate_units = (int16_t)u;
  if (M9H1_Abs(u) > a->peak_candidate)
    a->peak_candidate = M9H1_Abs(u);
  sign = (u > 0) ? 1 : ((u < 0) ? -1 : 0);
  if ((sign != 0) && (a->prior_command_sign != 0) &&
      (sign != a->prior_command_sign))
    M9H1_Inc(&a->reversals);
  if (sign != 0) a->prior_command_sign = sign;
  if (((e > 0) && (u < 0)) || ((e < 0) && (u > 0)))
    M9H1_Inc(&a->brake_frames);
}

void M9H1_Feed(M9H1Shadow_t *d, uint8_t left, uint16_t raw,
               uint8_t valid, uint32_t sample_ms, uint32_t now_ms)
{
  M9H1Axis_t *a = (left != 0U) ? &d->left : &d->right;
  int16_t delta;
  uint32_t elapsed, abs_delta, velocity;

  if ((valid == 0U) || (raw > 4095U) ||
      ((uint32_t)(now_ms - sample_ms) > M9H1_STALE_MS))
  {
    M9H1_Inc(&a->invalid);
    M9H1_ClearOutput(a);
    a->have_reference = 0U;
    return;
  }

  if (a->have_reference == 0U)
  {
    a->reference_raw = raw;
    a->last_raw = raw;
    a->reference_ms = sample_ms;
    a->last_sample_ms = sample_ms;
    a->have_reference = 1U;
    a->stale_latched = 0U;
    a->error_raw = M9H1_DeltaRaw(raw, a->goal_raw);
    M9H1_Inc(&a->samples);
    return;
  }
  elapsed = (uint32_t)(sample_ms - a->last_sample_ms);
  if (elapsed == 0U)
  {
    M9H1_Inc(&a->zero_dt);
    a->candidate_units = 0;
    return;
  }
  if (elapsed > M9H1_STALE_MS)
  {
    M9H1_Inc(&a->stale);
    M9H1_ClearOutput(a);
    a->have_reference = 0U;
    return;
  }
  delta = M9H1_DeltaRaw(a->last_raw, raw);
  if (M9H1_Abs(delta) > M9H1_MAX_STEP_RAW)
  {
    M9H1_Inc(&a->jump_rejected);
    M9H1_ClearOutput(a);
    a->have_reference = 0U;
    return;
  }
  a->last_raw = raw;
  a->last_sample_ms = sample_ms;
  a->error_raw = M9H1_DeltaRaw(raw, a->goal_raw);
  a->stale_latched = 0U;
  M9H1_Inc(&a->samples);

  elapsed = (uint32_t)(sample_ms - a->reference_ms);
  if (elapsed >= M9H1_WINDOW_MS)
  {
    delta = M9H1_DeltaRaw(a->reference_raw, raw);
    abs_delta = M9H1_Abs(delta);
    /* One AS5600 count over >=100ms is treated as quantization noise.
     * This deliberately sacrifices very-low-speed detection in H1.
     */
    if (abs_delta <= 1U)
    {
      if (abs_delta == 1U) M9H1_Inc(&a->noise_suppressed);
      a->velocity_raw_s = 0;
    }
    else
    {
      velocity = abs_delta * 1000U / elapsed;
      if (velocity > 32767U) velocity = 32767U;
      a->velocity_raw_s = (int16_t)((delta < 0) ?
          -(int32_t)velocity : (int32_t)velocity);
      if (M9H1_Abs(a->velocity_raw_s) > a->peak_velocity)
        a->peak_velocity = M9H1_Abs(a->velocity_raw_s);
    }
    a->reference_raw = raw;
    a->reference_ms = sample_ms;
    a->velocity_ready = 1U;
  }
  M9H1_Candidate(a);
}

void M9H1_Tick(M9H1Shadow_t *d, uint32_t now_ms)
{
  M9H1Axis_t *axes[2] = {&d->left, &d->right};
  uint8_t i;
  for (i = 0U; i < 2U; ++i)
  {
    M9H1Axis_t *a = axes[i];
    if ((a->have_reference != 0U) &&
        ((uint32_t)(now_ms - a->last_sample_ms) > M9H1_STALE_MS))
    {
      if (a->stale_latched == 0U) M9H1_Inc(&a->stale);
      a->stale_latched = 1U;
      a->have_reference = 0U;
      M9H1_ClearOutput(a);
    }
  }
}

void M9H1_Finish(M9H1Shadow_t *d, uint8_t reason)
{
  d->reason = reason;
  d->left.candidate_units = 0;
  d->right.candidate_units = 0;
}
