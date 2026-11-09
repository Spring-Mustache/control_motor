#include "m9_h2b_hold_policy.h"
#include <string.h>

static void inc16(uint16_t *x)
{
  if (*x != UINT16_MAX) ++*x;
}
static uint16_t mag16(int32_t x)
{
  if (x < 0) x = -x;
  return (x > UINT16_MAX) ? UINT16_MAX : (uint16_t)x;
}
static int16_t slew_score(int16_t previous, int16_t requested)
{
  int32_t delta = (int32_t)requested - previous;
  if (delta > M9H2B_SCORE_RISE_STEP)
    delta = M9H2B_SCORE_RISE_STEP;
  if (delta < -(int32_t)M9H2B_SCORE_RISE_STEP)
    delta = -(int32_t)M9H2B_SCORE_RISE_STEP;
  return (int16_t)(previous + delta);
}
static int16_t candidate_axis(uint8_t *latched, int16_t err,
                              int16_t h2_score, int16_t previous,
                              uint16_t *veto)
{
  uint16_t magnitude = mag16(err);
  int16_t target = 0;
  if (*latched == 0U && magnitude >= M9H2B_SCORE_ENTER_RAW)
    *latched = 1U;
  if (*latched != 0U && magnitude <= M9H2B_SCORE_EXIT_RAW)
    *latched = 0U;
  if (*latched == 0U)
    return 0;

  /* H1 physical trace demonstrated a sign reversal far from target.
   * H2-B refuses a "push away from target" demand on either side:
   * zero torque request is safer than copying H1's signed PWM.
   * This is a candidate policy, not evidence of actual damping force.
   */
  if (((err > 0) && (h2_score > 0)) ||
      ((err < 0) && (h2_score < 0)))
  {
    target = h2_score;
    if (target > M9H2B_SCORE_CAP) target = M9H2B_SCORE_CAP;
    if (target < -M9H2B_SCORE_CAP) target = -M9H2B_SCORE_CAP;
  }
  else if (h2_score != 0)
  {
    inc16(veto);
    return 0; /* Immediate no-force candidate if damping reverses sign. */
  }
  else
  {
    return 0; /* Never persist a candidate against a zero score. */
  }
  /* Immediate zero when a safety gate fails is handled by Step.
   * This bounded value has NO motor PWM units or current calibration.
   */
  return slew_score(previous,target);
}
static void zero_motor(M9H2BPolicy *p)
{
  p->motor_ccr_l = 0U;
  p->motor_ccr_r = 0U;
}
void M9H2B_Reset(M9H2BPolicy *p)
{
  (void)memset(p,0,sizeof(*p));
}
void M9H2B_Begin(M9H2BPolicy *p, const M9H2Guard *g)
{
  M9H2B_Reset(p);
  if ((g->active_window != 1U) || (g->state != M9H2_MONITOR) ||
      (g->seq == 0U) || (g->source < 1U) || (g->source > 3U))
  {
    p->state = M9H2B_REVOKED;
    p->reason = M9H2B_MISMATCH;
    return;
  }
  p->seq = g->seq;
  p->start_ms = g->started_ms;
  p->state = M9H2B_MONITOR;
  p->active = 1U;
}
void M9H2B_Step(M9H2BPolicy *p, const M9H2Guard *g,
                uint32_t now_ms, uint8_t request_power)
{
  /* Both conditions are intentionally impossible in this revision:
   * no calibrated PWM limit AND no actual hardware-writing API.
   * Never attach this module directly to TIM CCR or motor DIR.
   */
  zero_motor(p);
  if (p->active == 0U) return;
  p->end_ms = now_ms;
  if (request_power != 0U) inc16(&p->blocked_power);
  if ((p->seq != g->seq) || (g->started_ms != p->start_ms))
  {
    inc16(&p->bad_source);
    p->state = M9H2B_REVOKED;
    p->reason = M9H2B_MISMATCH;
    p->active = 0U;
  }
  else if (g->active_window == 0U)
  {
    p->reason = (g->reason == M9H2_REASON_WINDOW_DONE) ?
        M9H2B_WINDOW_DONE : M9H2B_GUARD_REVOKED;
    p->state = (p->reason == M9H2B_WINDOW_DONE) ?
        M9H2B_COMPLETE : M9H2B_REVOKED;
    p->active = 0U;
  }
  else if ((uint32_t)(now_ms - p->start_ms) >= M9H2B_MAX_POLICY_MS)
  {
    p->state = M9H2B_REVOKED;
    p->reason = M9H2B_EXPIRED;
    p->active = 0U;
  }
  else if (g->state != M9H2_SHADOW)
  {
    p->state = M9H2B_MONITOR;
    p->candidate_l = 0;
    p->candidate_r = 0;
    p->axis_l = 0U;
    p->axis_r = 0U;
  }
  else
  {
    /* H2-A owns STOP/NC/owner/age/error>32 checks and velocity health.
     * A shadow candidate is accepted only inside this gated state.
     */
    p->candidate_l = candidate_axis(&p->axis_l,
        g->pd.left.error_raw,g->score_l,p->candidate_l,
        &p->damping_veto_l);
    p->candidate_r = candidate_axis(&p->axis_r,
        g->pd.right.error_raw,g->score_r,p->candidate_r,
        &p->damping_veto_r);
    if (p->candidate_l != 0) inc16(&p->eligible_l);
    if (p->candidate_r != 0) inc16(&p->eligible_r);
    if (mag16(p->candidate_l) > p->peak_candidate_l)
      p->peak_candidate_l = mag16(p->candidate_l);
    if (mag16(p->candidate_r) > p->peak_candidate_r)
      p->peak_candidate_r = mag16(p->candidate_r);
    p->state = M9H2B_CANDIDATE;
  }
  if (p->active == 0U)
  {
    p->candidate_l = 0;
    p->candidate_r = 0;
    p->axis_l = 0U;
    p->axis_r = 0U;
  }
  inc16(&p->frames);
  zero_motor(p); /* ASSERTED on every early-exit path. */
}
