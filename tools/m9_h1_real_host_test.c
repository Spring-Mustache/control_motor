#include "m9_h1_real_trace.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  M9RealTrace_t d;
  uint32_t t;
  M9REAL_Begin(&d,1U,3U,4U,1000U,500U,950U,450U,1000U);
  for(t=1000U;t<=1800U;t+=20U) {
    uint16_t l=(uint16_t)(950U+(t-1000U)/10U);
    uint16_t r=(uint16_t)(450U+(t-1000U)/10U);
    M9REAL_Observe(&d,t,l,r,1U,1U,1U,1U,t,t,175U,90U,0U,1U,1U);
  }
  M9REAL_Finish(&d,1810U,2U,1030U,530U);
  assert(d.active==0U && d.result==2U && d.duration_ms==810U);
  assert(d.stored>=9U && d.stored<=11U && d.overflows==0U);
  assert(d.frame[0].raw_l==950U && d.frame[0].ccr_l==175U);
  assert(d.frame[d.stored-1U].t_ms>=720U);
  assert(d.pd.left.peak_candidate>0U && d.pd.right.peak_velocity>0U);
  assert(d.peak_ccr_l==175U && d.peak_ccr_r==90U);
  puts("PASS M9-H1 real-data recorder C host data integrity (synthetic inputs only for test)");
  return 0;
}
