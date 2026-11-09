#include "m9_h0_drift.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
  M9H0Drift_t d;
  assert(M9H0_AbsDelta(4094, 2) == 4U);
  assert(M9H0_AbsDelta(2, 4094) == 4U);
  assert(M9H0_AbsDelta(0, 2048) == 2048U);
  assert(M9H0_AbsDelta(4096, 0) == 4095U);
  M9H0_Begin(&d, 12U, 1000U, 2U, 4U, 1886U, 662U, 1884U, 660U);
  assert(d.left.peak_error_raw == 2U && d.right.peak_error_raw == 2U);
  M9H0_Feed(&d, 1U, 1880U, 1U, 1100U);
  M9H0_Feed(&d, 0U, 660U, 1U, 1100U);
  M9H0_Feed(&d, 1U, 1880U, 1U, 1100U);
  assert(d.left.zero_dt_samples == 1U);
  M9H0_Feed(&d, 1U, 1866U, 1U, 1200U);
  assert(d.left.peak_drift_raw == 18U);
  assert(d.left.peak_error_raw == 20U);
  assert(d.left.peak_velocity_raw_s == 140U);
  M9H0_Feed(&d, 0U, 4096U, 1U, 1200U);
  M9H0_Feed(&d, 0U, 660U, 0U, 1200U);
  assert(d.right.invalid_samples == 2U && d.right.samples == 1U);
  M9H0_Feed(&d, 1U, 1930U, 1U, 1300U);
  assert(d.left.peak_velocity_raw_s == 640U);
  M9H0_Feed(&d, 1U, 4090U, 1U, 1400U);
  assert(d.left.jump_samples == 1U);
  M9H0_Finish(&d, 4350U, 2U);
  assert(d.duration_ms == 3350U && d.reason == 2U);
  assert(d.seq == 12U && d.mode == 4U && d.source == 2U);
  puts("PASS M9-H0 pure drift tracker (HOST only)");
  return 0;
}
