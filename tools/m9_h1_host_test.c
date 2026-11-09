#include "m9_h1_shadow.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
  M9H1Shadow_t d;
  unsigned i;
  assert(M9H1_DeltaRaw(4094U, 2U) == 4);
  assert(M9H1_DeltaRaw(2U, 4094U) == -4);
  assert(M9H1_DeltaRaw(4096U, 0U) == 0);

  /* Stopped shaft with 1 RAW jitter never produces spurious D demand. */
  M9H1_Begin(&d, 1U, 2U, 1886U, 662U);
  M9H1_Feed(&d, 1U, 1886U, 1U, 0U, 0U);
  for (i = 1; i <= 25; ++i)
    M9H1_Feed(&d, 1U, (uint16_t)(1886U + (i & 1U)),
              1U, i*11U, i*11U);
  /* A one-count change over 100ms is explicitly filtered out. */
  M9H1_Feed(&d, 1U, 1887U, 1U, 375U, 375U);
  assert(d.left.velocity_raw_s == 0);
  assert(d.left.candidate_units == 0);
  assert(d.left.noise_suppressed >= 1U);

  /* Outward disturbance: when shaft moves away, candidate returns.
   * Abrupt load release: fast motion toward goal changes proposed
   * sign BEFORE reaching goal, damping instead of pushing through.
   */
  M9H1_Begin(&d, 2U, 3U, 1000U, 662U);
  M9H1_Feed(&d, 1U, 1000U, 1U, 1000U, 1000U);
  M9H1_Feed(&d, 1U, 970U, 1U, 1100U, 1100U);
  assert(d.left.error_raw == 30);
  assert(d.left.velocity_raw_s == -300);
  assert(d.left.candidate_units > 0);
  assert(d.left.peak_candidate <= M9H1_CAP_UNITS);
  M9H1_Feed(&d, 1U, 990U, 1U, 1200U, 1200U);
  assert(d.left.error_raw == 10);
  assert(d.left.velocity_raw_s == 200);
  assert(d.left.candidate_units < 0);
  assert(d.left.brake_frames >= 1U);
  assert(d.left.reversals >= 1U);
  M9H1_Feed(&d, 1U, 1005U, 1U, 1300U, 1300U);
  assert(d.left.error_raw == -5);
  assert(d.left.candidate_units < 0);
  M9H1_Finish(&d, 1U);
  assert(d.left.candidate_units == 0 && d.reason == 1U);

  /* Stale sensor and failed status cannot retain prior nonzero output. */
  M9H1_Begin(&d, 3U, 1U, 1000U, 662U);
  M9H1_Feed(&d, 1U, 1000U, 1U, 0U, 0U);
  M9H1_Feed(&d, 1U, 970U, 1U, 100U, 100U);
  assert(d.left.candidate_units > 0);
  M9H1_Tick(&d, 351U);
  assert(d.left.stale == 1U && d.left.candidate_units == 0);
  M9H1_Tick(&d, 900U);
  assert(d.left.stale == 1U);
  M9H1_Feed(&d, 1U, 980U, 0U, 900U, 900U);
  assert(d.left.invalid == 1U && d.left.candidate_units == 0);
  M9H1_Feed(&d, 1U, 980U, 1U, 0U, 900U);
  assert(d.left.invalid == 2U);
  M9H1_Feed(&d, 1U, 980U, 1U, 1000U, 1000U);
  M9H1_Feed(&d, 1U, 1002U, 1U, 1000U, 1000U);
  assert(d.left.zero_dt == 1U && d.left.candidate_units == 0);
  M9H1_Feed(&d, 1U, 1200U, 1U, 1100U, 1100U);
  assert(d.left.jump_rejected == 1U && d.left.candidate_units == 0);
  M9H1_Feed(&d, 0U, 662U, 1U, 1000U, 1000U);
  M9H1_Feed(&d, 0U, 661U, 1U, 1100U, 1100U);
  assert(d.right.velocity_raw_s == 0 && d.right.candidate_units == 0);
  puts("PASS M9-H1 exact pure-C PD shadow, load-release sign, noise, stale, wrap");
  return 0;
}
