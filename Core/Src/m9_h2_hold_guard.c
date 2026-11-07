#include "m9_h2_hold_guard.h"
#include <string.h>

static void inc16(uint16_t *x)
{
  if (*x != UINT16_MAX) ++(*x);
}
static uint16_t absi16(int32_t x)
{
  return (uint16_t)((x < 0) ? -x : x);
}
static int16_t shadow_score(int16_t error, int16_t velocity)
{
  int32_t eff = 0, score;
  /* Inside the near-goal hold domain only. Hysteresis/quantization
   * suppression belongs to the filtered M9-H1 axis observer.
   */
  if (error > (int16_t)M9H1_DEADBAND_RAW)
    eff = (int32_t)error - (int32_t)M9H1_DEADBAND_RAW;
  else if (error < -(int16_t)M9H1_DEADBAND_RAW)
    eff = (int32_t)error + (int32_t)M9H1_DEADBAND_RAW;
  score = ((int32_t)M9H2_KP_Q8 * eff -
           (int32_t)M9H2_KD_Q8 * velocity) / 256;
  if (score > M9H2_SHADOW_SCORE_CAP) score = M9H2_SHADOW_SCORE_CAP;
  if (score < -M9H2_SHADOW_SCORE_CAP) score = -M9H2_SHADOW_SCORE_CAP;
  return (int16_t)score;
}
static void zero_output(M9H2Guard *g)
{
  g->duty_l = 0U;
  g->duty_r = 0U;
}
void M9H2_Reset(M9H2Guard *g)
{
  (void)memset(g, 0, sizeof(*g));
}
void M9H2_Begin(M9H2Guard *g, uint32_t seq, uint32_t now_ms,
                uint8_t source, uint16_t goal_l, uint16_t goal_r,
                uint32_t initial_seq_l, uint32_t initial_seq_r)
{
  M9H2_Reset(g);
  g->seq = seq;
  g->source = source;
  g->goal_l = goal_l;
  g->goal_r = goal_r;
  g->started_ms = now_ms;
  g->seen_l = initial_seq_l;
  g->seen_r = initial_seq_r;
  if ((goal_l > 4095U) || (goal_r > 4095U) ||
      (source < 1U) || (source > 3U))
  {
    g->state = M9H2_FAULT;
    g->reason = M9H2_REASON_RANGE;
    return;
  }
  M9H1_Begin(&g->pd, seq, source, goal_l, goal_r);
  g->state = M9H2_MONITOR;
  g->active_window = 1U;
}
void M9H2_Cancel(M9H2Guard *g, uint8_t reason)
{
  zero_output(g);
  g->pd.left.candidate_units = 0;
  g->pd.right.candidate_units = 0;
  g->score_l = 0;
  g->score_r = 0;
  g->reason = reason;
  g->state = ((reason == M9H2_REASON_OWNER) ||
              (reason == M9H2_REASON_DISARM)) ?
              M9H2_OFF : M9H2_FAULT;
  g->active_window = 0U;
}
void M9H2_Step(M9H2Guard *g, const M9H2Input *in)
{
  uint8_t fresh_l, fresh_r;
  if (g->active_window == 0U) { zero_output(g); return; }
  zero_output(g); /* HARD output interlock on EVERY iteration. */
  g->ended_ms = in->now_ms;
  if ((in->safety_ok != 1U) || (in->monitoring_allowed != 1U))
  {
    M9H2_Cancel(g, M9H2_REASON_SAFETY);
    return;
  }
  if ((in->owners_idle != 1U) || (in->wheels_stopped != 1U))
  {
    inc16(&g->denied_owner);
    M9H2_Cancel(g, M9H2_REASON_OWNER);
    return;
  }
  if (in->nc_mask != 0U)
  {
    inc16(&g->denied_nc);
    M9H2_Cancel(g, M9H2_REASON_NC);
    return;
  }
  if ((in->valid_l != 1U) || (in->valid_r != 1U) ||
      (in->raw_l > 4095U) || (in->raw_r > 4095U) ||
      ((uint32_t)(in->now_ms - in->sample_l_ms) > M9H2_SAMPLE_STALE_MS) ||
      ((uint32_t)(in->now_ms - in->sample_r_ms) > M9H2_SAMPLE_STALE_MS))
  {
    inc16(&g->errors);
    M9H2_Cancel(g, M9H2_REASON_SENSOR);
    return;
  }
  if ((uint32_t)(in->now_ms - g->started_ms) >= M9H2_WINDOW_MS)
  {
    g->state = M9H2_COMPLETE;
    g->reason = M9H2_REASON_WINDOW_DONE;
    g->active_window = 0U;
    return;
  }
  fresh_l = (in->sample_seq_l != g->seen_l) ? 1U : 0U;
  fresh_r = (in->sample_seq_r != g->seen_r) ? 1U : 0U;
  g->seen_l = in->sample_seq_l;
  g->seen_r = in->sample_seq_r;
  if (fresh_l != 0U)
  {
    M9H1_Feed(&g->pd, 1U, in->raw_l, 1U,
              in->sample_l_ms, in->now_ms);
    inc16(&g->fresh_l);
  }
  if (fresh_r != 0U)
  {
    M9H1_Feed(&g->pd, 0U, in->raw_r, 1U,
              in->sample_r_ms, in->now_ms);
    inc16(&g->fresh_r);
  }
  M9H1_Tick(&g->pd, in->now_ms);
  if ((g->pd.left.stale != 0U) || (g->pd.right.stale != 0U) ||
      (g->pd.left.jump_rejected != 0U) ||
      (g->pd.right.jump_rejected != 0U) ||
      (g->pd.left.invalid != 0U) || (g->pd.right.invalid != 0U))
  {
    inc16(&g->errors);
    M9H2_Cancel(g, M9H2_REASON_SENSOR);
    return;
  }
  if (absi16(g->pd.left.error_raw) > M9H2_MAX_HOLD_ERROR_RAW ||
      absi16(g->pd.right.error_raw) > M9H2_MAX_HOLD_ERROR_RAW)
  {
    inc16(&g->errors);
    M9H2_Cancel(g, M9H2_REASON_RANGE);
    return;
  }
  g->state = M9H2_MONITOR;
  if ((g->pd.left.velocity_ready != 0U) &&
      (g->pd.right.velocity_ready != 0U))
  {
    g->score_l = shadow_score(g->pd.left.error_raw,
                             g->pd.left.velocity_raw_s);
    g->score_r = shadow_score(g->pd.right.error_raw,
                             g->pd.right.velocity_raw_s);
    if (absi16(g->pd.left.error_raw) <= M9H1_EXIT_RAW &&
        g->pd.left.velocity_raw_s == 0)
      g->score_l = 0;
    if (absi16(g->pd.right.error_raw) <= M9H1_EXIT_RAW &&
        g->pd.right.velocity_raw_s == 0)
      g->score_r = 0;
    if (absi16(g->pd.left.error_raw) > g->peak_error_l)
      g->peak_error_l = absi16(g->pd.left.error_raw);
    if (absi16(g->pd.right.error_raw) > g->peak_error_r)
      g->peak_error_r = absi16(g->pd.right.error_raw);
    if (absi16(g->score_l) > g->peak_score_l)
      g->peak_score_l = absi16(g->score_l);
    if (absi16(g->score_r) > g->peak_score_r)
      g->peak_score_r = absi16(g->score_r);
    g->state = M9H2_SHADOW;
  }
  if (in->request_power != 0U)
  {
    inc16(&g->blocked_power_requests);
    /* No ACTIVE path in this build: driver currents, stall heating and
     * endstop power arbitration have not been bench certified. */
    if (M9H2_POWER_PERMITTED != 0U)
      M9H2_Cancel(g, M9H2_REASON_DISARM);
  }
  inc16(&g->updates);
  zero_output(g);
}
