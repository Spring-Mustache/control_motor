#include "m9_h1_real_trace.h"
#include <string.h>

static void Count16(uint16_t *n) { if (*n != UINT16_MAX) ++*n; }
static uint16_t Min16(uint32_t n) {
  return (n > UINT16_MAX) ? UINT16_MAX : (uint16_t)n;
}

void M9REAL_Begin(M9RealTrace_t *d, uint32_t seq, uint8_t source,
                  uint8_t mode, uint16_t goal_l, uint16_t goal_r,
                  uint16_t raw_l, uint16_t raw_r, uint32_t now_ms)
{
  (void)memset(d, 0, sizeof(*d));
  d->seq = seq; d->source = source; d->mode = mode;
  d->start_ms = now_ms;
  d->goal_l = goal_l; d->goal_r = goal_r;
  d->start_l = raw_l; d->start_r = raw_r;
  d->end_l = raw_l; d->end_r = raw_r;
  d->active = 1U;
  M9H1_Begin(&d->pd, seq, source, goal_l, goal_r);
}

void M9REAL_Observe(M9RealTrace_t *d, uint32_t now_ms,
                    uint16_t raw_l, uint16_t raw_r,
                    uint8_t fresh_l, uint8_t fresh_r,
                    uint8_t valid_l, uint8_t valid_r,
                    uint32_t sample_l_ms, uint32_t sample_r_ms,
                    uint16_t ccr_l, uint16_t ccr_r,
                    uint8_t nc_mask, uint8_t state_l, uint8_t state_r)
{
  uint32_t age_l = now_ms - sample_l_ms;
  uint32_t age_r = now_ms - sample_r_ms;
  M9RealFrame_t *p;
  if (d->active == 0U) return;
  if (ccr_l > d->peak_ccr_l) d->peak_ccr_l = ccr_l;
  if (ccr_r > d->peak_ccr_r) d->peak_ccr_r = ccr_r;
  if ((fresh_l != 0U) || (fresh_r != 0U))
  {
    if ((fresh_l != 0U) && (valid_l != 0U) &&
        (age_l <= M9REAL_MAX_SAMPLE_AGE_MS) && raw_l <= 4095U)
      M9H1_Feed(&d->pd, 1U, raw_l, 1U, sample_l_ms, now_ms);
    else if (fresh_l != 0U) {
      Count16(&d->bad);
      M9H1_Feed(&d->pd, 1U, raw_l, 0U, sample_l_ms, now_ms);
    }
    if ((fresh_r != 0U) && (valid_r != 0U) &&
        (age_r <= M9REAL_MAX_SAMPLE_AGE_MS) && raw_r <= 4095U)
      M9H1_Feed(&d->pd, 0U, raw_r, 1U, sample_r_ms, now_ms);
    else if (fresh_r != 0U) {
      Count16(&d->bad);
      M9H1_Feed(&d->pd, 0U, raw_r, 0U, sample_r_ms, now_ms);
    }
  }
  M9H1_Tick(&d->pd, now_ms);
  if ((d->stored > 0U) &&
      ((now_ms - d->start_ms) - d->frame[d->stored - 1U].t_ms <
       M9REAL_FRAME_PERIOD_MS))
  {
    return;
  }
  if ((fresh_l == 0U) && (fresh_r == 0U)) {
    Count16(&d->skipped); return;
  }
  if ((valid_l == 0U) || (valid_r == 0U) ||
      age_l > M9REAL_MAX_SAMPLE_AGE_MS ||
      age_r > M9REAL_MAX_SAMPLE_AGE_MS ||
      raw_l > 4095U || raw_r > 4095U) {
    Count16(&d->stale); return;
  }
  if (d->stored >= M9REAL_MAX_FRAMES) {
    Count16(&d->overflows); return;
  }
  p = &d->frame[d->stored++];
  p->t_ms = now_ms - d->start_ms;
  p->raw_l = raw_l; p->raw_r = raw_r;
  p->err_l = M9H1_DeltaRaw(raw_l, d->goal_l);
  p->err_r = M9H1_DeltaRaw(raw_r, d->goal_r);
  p->vel_l = d->pd.left.velocity_raw_s;
  p->vel_r = d->pd.right.velocity_raw_s;
  p->cmd_l = d->pd.left.candidate_units;
  p->cmd_r = d->pd.right.candidate_units;
  p->ccr_l = ccr_l; p->ccr_r = ccr_r;
  p->age_l = Min16(age_l);
  p->age_r = Min16(age_r);
  p->nc_mask = nc_mask; p->state_l = state_l; p->state_r = state_r;
}
void M9REAL_Finish(M9RealTrace_t *d, uint32_t now_ms, uint8_t result,
                   uint16_t raw_l, uint16_t raw_r)
{
  if (d->active == 0U) return;
  d->duration_ms = now_ms - d->start_ms;
  d->result = result;
  d->end_l = raw_l; d->end_r = raw_r;
  M9H1_Finish(&d->pd, result);
  d->active = 0U;
}
