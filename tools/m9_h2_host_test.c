#include "m9_h2_hold_guard.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static M9H2Input input(uint32_t now,uint16_t l,uint16_t r) {
  M9H2Input x;
  (void)memset(&x,0,sizeof(x));
  x.now_ms=now; x.raw_l=l; x.raw_r=r;
  x.sample_l_ms=now; x.sample_r_ms=now;
  x.sample_seq_l=now+1U;x.sample_seq_r=now+1U;
  x.valid_l=1U;x.valid_r=1U;x.safety_ok=1U;
  x.owners_idle=1U;x.wheels_stopped=1U;
  x.monitoring_allowed=1U;
  return x;
}
int main(void) {
  M9H2Guard g;
  M9H2Input x;
  /* This was the actual on-MCU failure at 8214610: Voice DONE
   * changed CONTROL_MODE_VOICE to normal IDLE after 13ms.
   * Never confuse that expected release with a new owner.
   */
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_IDLE,0U,0U,0U)==1U);
  assert(M9H2_ModeAllowed(2U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_IDLE,0U,0U,0U)==1U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_VOICE,0U,0U,0U)==1U);
  assert(M9H2_ModeAllowed(1U,M9H2_CONTROL_PS2,
                          M9H2_CONTROL_IDLE,0U,0U,0U)==1U);
  assert(M9H2_ModeAllowed(1U,M9H2_CONTROL_PS2,
                          M9H2_CONTROL_PS2,0U,1U,0U)==1U);
  assert(M9H2_ModeAllowed(1U,M9H2_CONTROL_PS2,
                          M9H2_CONTROL_IDLE,0U,1U,0U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_IDLE,1U,0U,0U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_IDLE,0U,1U,0U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_IDLE,0U,0U,1U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_PS2,0U,1U,0U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                          M9H2_CONTROL_STOP,0U,0U,0U)==0U);
  assert(M9H2_ModeAllowed(1U,M9H2_CONTROL_PS2,
                          M9H2_CONTROL_VOICE,0U,0U,0U)==0U);
  assert(M9H2_ModeAllowed(3U,M9H2_CONTROL_IDLE,
                          M9H2_CONTROL_IDLE,0U,0U,0U)==0U);
  unsigned i;
  M9H2_Reset(&g);
  assert(g.state==M9H2_OFF && g.duty_l==0U && g.duty_r==0U);
  M9H2_Begin(&g,1U,100U,3U,1000U,600U,0U,0U);
  assert(g.state==M9H2_MONITOR);
  for(i=0U;i<18U;i++){
    x=input(100U+i*20U,(uint16_t)(1000U+(i&1U)),600U);
    x.request_power=1U; /* Even hostile runtime request MUST be inert. */
    M9H2_Step(&g,&x);
    assert(g.duty_l==0U && g.duty_r==0U);
    assert(g.state!=M9H2_ACTIVE && g.state!=M9H2_ARMED);
  }
  assert(g.blocked_power_requests>0U);
  assert(g.state==M9H2_SHADOW);
  assert(g.score_l==0 && g.score_r==0);
  x=input(480U,1012U,600U);
  M9H2_Step(&g,&x);
  assert(g.active_window && g.duty_l==0 && g.duty_r==0);
  assert(g.score_l!=0 || g.peak_score_l>0);
  x=input(500U,1012U,600U);x.nc_mask=1U;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_NC);
  assert(g.duty_l==0U && g.duty_r==0U);
  x=input(520U,1000U,600U);M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.duty_l==0U);
  M9H2_Begin(&g,2U,1000U,2U,1886U,662U,0U,0U);
  x=input(1100U,1886U,662U);x.owners_idle=0;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_OFF && g.reason==M9H2_REASON_OWNER);
  M9H2_Begin(&g,3U,1000U,3U,1000U,600U,0U,0U);
  x=input(1200U,1000U,600U);x.safety_ok=0;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_SAFETY);
  M9H2_Begin(&g,4U,1000U,3U,1000U,600U,0U,0U);
  x=input(1100U,1000U,600U);x.sample_l_ms=700U;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_SENSOR);
  M9H2_Begin(&g,5U,1000U,3U,1000U,600U,0U,0U);
  x=input(1100U,1050U,600U);
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_RANGE);
  M9H2_Begin(&g,6U,1000U,3U,1000U,600U,0U,0U);
  x=input(4000U,1000U,600U);
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_COMPLETE && g.reason==M9H2_REASON_WINDOW_DONE);
  /* Valid 3-second Voice->Idle monitor remains read-only throughout. */
  M9H2_Begin(&g,11U,1000U,3U,1545U,351U,0U,0U);
  for(i=0U;i<32U;i++){
    uint8_t mode=(i==0U)?M9H2_CONTROL_VOICE:M9H2_CONTROL_IDLE;
    x=input(1000U+i*100U,1548U,355U);
    x.owners_idle=M9H2_ModeAllowed(3U,M9H2_CONTROL_VOICE,
                                  mode,0U,0U,0U);
    M9H2_Step(&g,&x);
    assert(g.duty_l==0U && g.duty_r==0U);
  }
  assert(g.state==M9H2_COMPLETE && g.reason==M9H2_REASON_WINDOW_DONE);
  assert(g.ended_ms-g.started_ms==3000U);
  M9H2_Begin(&g,7U,1000U,3U,1000U,600U,0U,0U);
  x=input(1100U,1000U,600U);x.wheels_stopped=0;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_OFF && g.reason==M9H2_REASON_OWNER);
  M9H2_Begin(&g,8U,1000U,3U,1000U,600U,0U,0U);
  x=input(1100U,1000U,600U);x.monitoring_allowed=0;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_SAFETY);
  M9H2_Begin(&g,9U,1000U,3U,1000U,600U,0U,0U);
  x=input(1100U,1000U,600U);x.valid_r=0;
  M9H2_Step(&g,&x);
  assert(g.state==M9H2_FAULT && g.reason==M9H2_REASON_SENSOR);
  M9H2_Begin(&g,10U,1000U,3U,1000U,600U,0U,0U);
  x=input(1000U,1000U,600U);M9H2_Step(&g,&x);
  x=input(1100U,1000U,600U);M9H2_Step(&g,&x);
  x=input(1200U,1001U,600U);M9H2_Step(&g,&x);
  x=input(1300U,1000U,600U);M9H2_Step(&g,&x);
  assert(g.pd.left.velocity_raw_s==0);
  assert(g.duty_l==0 && g.duty_r==0 && !g.output_ever_nonzero);
  puts("PASS M9-H2A pure-C monitor, near-goal shadow and hard power-off");
  return 0;
}
