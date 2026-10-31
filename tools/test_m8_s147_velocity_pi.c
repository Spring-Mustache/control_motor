/* Exact production S14.7 helper; no STM32/HAL dependencies. */
#include <assert.h>
#include <stdio.h>
#include "steer_s147_velocity_pi.h"

int main(void)
{
  M8S147AxisState_t l={0},r={0},ref={0};
  M8S147AxisPlan_t p,q;
  uint16_t lt=0,rt=0;
  int i;
  M8S147_Targets(250,200,&lt,&rt);
  assert(lt==560 && rt==780);
  M8S147_Targets(100,300,&lt,&rt);
  assert(lt==680 && rt==680);
  M8S147_Targets(1000,1000,&lt,&rt);
  assert(lt==560 && rt==680);
  /* RIGHT gets a real boost with headroom, bounded to old envelope. */
  p=M8S147_AxisStep(&r,125,150,380,680,400,1,1000,125);
  assert(p.pwm>=125 && p.pwm<=137 && p.boost>0 && p.error_pms==300);
  q=M8S147_AxisStep(&l,330,375,490,560,400,1,1000,330);
  assert(q.pwm>=330 && q.pwm<=342 && q.boost>0);
  /* Repeated main-loop calls with SAME sensor sample do not integrate. */
  {
    int32_t integral=r.integral;
    for(i=0;i<30;i++)
      (void)M8S147_AxisStep(&r,125,150,380,680,400,0,1000,137);
    assert(r.integral==integral);
  }
  /* Each fresh sample may increase, but not beyond owner cap or +12. */
  for(i=1;i<50;i++)
  {
    p=M8S147_AxisStep(&r,125,150,350,800,400,1,
                      (uint32_t)(1000+i*50),150);
    assert(p.pwm>=125 && p.pwm<=150 && p.boost<=25);
    assert(r.integral<=6000);
  }
  /* Positive saturation must not wind up integral. */
  {
    int32_t integral=r.integral;
    for(i=50;i<80;i++)
      (void)M8S147_AxisStep(&r,125,150,200,800,400,1,
                            (uint32_t)(1000+i*50),150);
    assert(r.integral==integral);
  }
  /* Final 180 RAW is immutable for any measured speed, target or I. */
  p=M8S147_AxisStep(&r,72,100,0,800,180,1,6000,150);
  assert(p.pwm==72 && p.boost==0 && r.integral==0);
  /* Low owner/zero CCR cannot be raised from a STOP. */
  p=M8S147_AxisStep(&r,0,150,0,800,600,1,6050,0);
  assert(p.pwm==0 && p.boost==0);
  /* End of owner envelope: no extra power. */
  p=M8S147_AxisStep(&r,125,125,0,800,400,1,6100,125);
  assert(p.pwm==125 && p.boost==0);
  /* Fresh invalid dt must fail back to owner without boost. */
  p=M8S147_AxisStep(&ref,125,150,300,680,400,1,1000,125);
  assert(p.boost>0);
  p=M8S147_AxisStep(&ref,125,150,300,680,400,1,1400,150);
  assert(p.pwm==125 && ref.integral==0 && ref.last_boost==0);
  /* Overspeed never creates negative trim and clears stored I. */
  p=M8S147_AxisStep(&l,330,375,900,560,400,1,1050,350);
  assert(p.pwm==330 && p.boost==0 && l.integral==0);
  /* This tool never reads/writes motor GPIO and never changes direction. */
  puts("PASS M8-S14.7: two-axis PI targets, independent caps, +12 rise,");
  puts("      boost-only, fresh-sample I, antiwindup, SOFT cutoff, STOP zero");
  return 0;
}
