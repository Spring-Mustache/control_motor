#include "m9_h2b_hold_policy.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static M9H2Guard guard(void)
{
  M9H2Guard g; (void)memset(&g,0,sizeof(g));
  g.seq=7U; g.source=3U; g.state=M9H2_MONITOR;
  g.active_window=1U; g.started_ms=1000U;
  g.pd.left.error_raw=0;g.pd.right.error_raw=0;
  return g;
}
int main(void)
{
  M9H2BPolicy p; M9H2Guard g=guard();
  uint32_t i;
  M9H2B_Reset(&p);
  assert(p.state==M9H2B_OFF && p.motor_ccr_l==0U);
  M9H2B_Begin(&p,&g);
  assert(p.state==M9H2B_MONITOR && p.active==1U);
  for(i=0;i<10U;i++){
    g.state=M9H2_SHADOW;
    g.pd.left.error_raw=14;
    g.pd.right.error_raw=-14;
    g.score_l=12;g.score_r=-12;
    M9H2B_Step(&p,&g,1010U+i*20U,1U);
    assert(p.motor_ccr_l==0U && p.motor_ccr_r==0U);
    assert(p.candidate_l>=0 && p.candidate_l<=12);
    assert(p.candidate_r<=0 && p.candidate_r>=-12);
    assert(p.state==M9H2B_CANDIDATE);
  }
  assert(p.peak_candidate_l==12U && p.peak_candidate_r==12U);
  assert(p.blocked_power==10U && !p.active_power_ever);
  g.pd.left.error_raw=14;g.score_l=-20;
  M9H2B_Step(&p,&g,1240U,1U);
  assert(p.damping_veto_l==1U);
  assert(p.candidate_l<=12 && p.motor_ccr_l==0);
  g.pd.left.error_raw=3;g.score_l=20;
  g.pd.right.error_raw=-3;g.score_r=-20;
  M9H2B_Step(&p,&g,1260U,1U);
  assert(p.candidate_l==0 && p.candidate_r==0);
  g.state=M9H2_FAULT;g.reason=M9H2_REASON_NC;g.active_window=0U;
  M9H2B_Step(&p,&g,1280U,1U);
  assert(p.state==M9H2B_REVOKED && p.reason==M9H2B_GUARD_REVOKED);
  assert(p.motor_ccr_l==0U && p.motor_ccr_r==0U);
  g=guard();M9H2B_Begin(&p,&g);
  g.state=M9H2_COMPLETE;g.reason=M9H2_REASON_WINDOW_DONE;
  g.active_window=0U;
  M9H2B_Step(&p,&g,4000U,1U);
  assert(p.state==M9H2B_COMPLETE && p.reason==M9H2B_WINDOW_DONE);
  assert(p.motor_ccr_l==0U && p.motor_ccr_r==0U);
  g=guard();M9H2B_Begin(&p,&g);
  g.seq=8U;M9H2B_Step(&p,&g,1100U,1U);
  assert(p.state==M9H2B_REVOKED && p.reason==M9H2B_MISMATCH);
  g=guard();M9H2B_Begin(&p,&g);
  g.state=M9H2_SHADOW;g.score_l=15;g.pd.left.error_raw=18;
  M9H2B_Step(&p,&g,4001U,1U);
  assert(p.state==M9H2B_REVOKED && p.reason==M9H2B_EXPIRED);
  g=guard();g.active_window=0U;M9H2B_Begin(&p,&g);
  assert(p.state==M9H2B_REVOKED && p.reason==M9H2B_MISMATCH);
  assert(p.motor_ccr_l==0 && p.motor_ccr_r==0);
  puts("PASS M9-H2B candidate policy with hard physical output lock");
  return 0;
}
