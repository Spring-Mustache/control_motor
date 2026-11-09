/* SYNTHETIC HOST fixtures. Values here are NOT motor calibration. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "m9_h2b1_hold.h"
#include "m9_h2b1_commissioning.h"
typedef struct {
  uint32_t now,irq,locks,unlocks,writes,nonzero,allowed_calls,fail_call;
  uint16_t pwm[2];int8_t dir[2];
} Mock;
static uint32_t lock(void *v){Mock *m=v;uint32_t x=m->irq;m->irq=1U;++m->locks;return x;}
static void unlock(void *v,uint32_t x){Mock *m=v;m->irq=x;++m->unlocks;}
static uint8_t allowed(void *v){Mock *m=v;assert(m->irq==1U);++m->allowed_calls;
  return (uint8_t)(!m->fail_call || m->allowed_calls!=m->fail_call);}
static void pwm(void *v,uint8_t a,uint16_t n){Mock *m=v;assert(m->irq==1U);
  m->pwm[a]=n;++m->writes;if(n)++m->nonzero;}
static void direction(void *v,uint8_t a,int8_t sign){Mock *m=v;
  assert(m->irq==1U && m->pwm[a]==0U);m->dir[a]=sign;}
static uint32_t now(void *v){return ((Mock *)v)->now;}
static M9B1Config config(void)
{
  M9B1Config c={{{20,0,512,64,1000,50},{15,0,512,64,1000,50}},
    8,4,5,32,250,100,90,250,0,0,0,M9B1_POWER_COMMISSIONED};return c;
}
static M9B1Input input(uint32_t t,uint16_t l,uint16_t r)
{
  M9B1Input in={0}; /* overwritten below, no hardware */
  (void)memset(&in,0,sizeof(in));
  in.now_ms=t;in.sample_ms[0]=t;in.sample_ms[1]=t;
  in.sample_seq[0]=t+1U;in.sample_seq[1]=t+1U;
  in.raw[0]=l;in.raw[1]=r;in.valid[0]=in.valid[1]=1U;
  in.enabled=in.safety_ok=in.owners_free=in.geometry_ok=1U;return in;
}
static void tick(M9B1Hold *h,const M9B1Config *c,Mock *m,const M9B1Port *p,
                   uint32_t t,uint16_t l,uint16_t r)
{
  M9B1Input in=input(t,l,r);m->now=t;M9B1_Step(h,c,&in);(void)M9B1_Apply(h,c,p);
}
static void start(M9B1Hold *h,M9B1Config *c,Mock *m,M9B1Port *p,
                    uint32_t t,uint16_t l,uint16_t r)
{
  M9B1Input in=input(t,l,r);M9B1_Reset(h);(void)memset(m,0,sizeof(*m));
  m->now=t;*p=(M9B1Port){m,lock,unlock,allowed,pwm,direction,now};
  assert(M9B1_Begin(h,c,&in,1U,2U,1000U,1000U));
}
static void continuous_and_independent(void)
{
  M9B1Config c=config();M9B1Hold h;Mock m;M9B1Port p;uint32_t t;
  start(&h,&c,&m,&p,0U,985U,1015U);
  for(t=10;t<=120000;t+=10)tick(&h,&c,&m,&p,t,985U,1015U);
  assert(h.active && h.state==M9B1_HOLDING && h.lease);
  assert(h.axis[0].pwm>0 && h.axis[1].pwm<0);
  assert(m.pwm[0]<=20 && m.pwm[1]<=15 && h.applied_ms>100000U);
  assert(m.dir[0]==1 && m.dir[1]==-1 && m.locks==m.unlocks && !m.irq);
  tick(&h,&c,&m,&p,120010U,1000U,1015U);
  tick(&h,&c,&m,&p,120110U,1000U,1015U);
  assert(h.axis[0].pwm==0 && h.axis[1].pwm<0 && h.active);
  /* A collector/reporting window does not exist in the control API. */
  M9B1_Cut(&h,&p,M9B1_STOP);assert(!h.active && !h.lease && !m.pwm[0] && !m.pwm[1]);
  tick(&h,&c,&m,&p,120120U,985U,1015U);assert(!h.active && !m.pwm[0]);
}
static void release_damping(void)
{
  M9B1Config c=config();M9B1Hold h;Mock m;M9B1Port p;
  start(&h,&c,&m,&p,0U,970U,1000U);
  tick(&h,&c,&m,&p,100U,970U,1000U);assert(h.axis[0].pwm>0);
  tick(&h,&c,&m,&p,200U,990U,1000U);
  assert(h.axis[0].error>0 && h.axis[0].velocity>0 && h.axis[0].pwm==0);
  tick(&h,&c,&m,&p,240U,990U,1000U);assert(h.axis[0].pwm==0);
  tick(&h,&c,&m,&p,250U,990U,1000U);
  assert(h.axis[0].error>0 && h.axis[0].pwm<0); /* brake before crossing */
  tick(&h,&c,&m,&p,350U,1000U,1000U);
  tick(&h,&c,&m,&p,450U,1000U,1000U);
  assert(h.active && h.axis[0].pwm==0 && h.axis[1].pwm==0);
}
static void revocations(void)
{
  M9B1Config c=config();M9B1Hold h;Mock m;M9B1Port p;M9B1Input in;
  unsigned k;
  for(k=0;k<10U;k++){
    start(&h,&c,&m,&p,0U,985U,1015U);tick(&h,&c,&m,&p,100U,985U,1015U);
    in=input(110U,985U,1015U);
    switch(k){
      case 0:in.safety_ok=0;break;case 1:in.owners_free=0;break;
      case 2:in.nc_mask=1;break;case 3:in.valid[1]=0;break;
      case 4:in.sample_ms[0]=in.now_ms-251U;break;
      case 5:in.geometry_ok=0;break;case 6:in.enabled=0;break;
      case 7:in.raw[0]=1100U;break;case 8:in.raw[0]=960U;break;
      default:in=input(400U,985U,1015U);break;
    }
    m.now=in.now_ms;M9B1_Step(&h,&c,&in);(void)M9B1_Apply(&h,&c,&p);
    assert(!h.active && !h.lease && m.pwm[0]==0 && m.pwm[1]==0);
    in=input(in.now_ms+10U,985U,1015U);M9B1_Step(&h,&c,&in);
    assert(!h.active); /* no automatic re-arm of a canceled target */
  }
}
static void commissioned_interlock(void)
{
  M9B1Hold h;Mock m;M9B1Port p;M9B1Config c=m9_h2b1_config;
  uint32_t t;M9B1Input in;
  c.power_mode=M9B1_POWER_LOCKED;
  start(&h,&c,&m,&p,0U,985U,1015U);
  for(t=10;t<60000;t+=10)tick(&h,&c,&m,&p,t,985U,1015U);
  assert(h.active && h.state==M9B1_UNCOMMISSIONED && !h.lease && !m.writes);
  assert(!h.ever_applied && h.reason==M9B1_CALIBRATION_REQUIRED);
  M9B1_Reset(&h);c.power_mode=M9B1_POWER_COMMISSIONED;
  c.axis[0].max_pwm=0U;in=input(0U,1000U,1000U);
  assert(!M9B1_Begin(&h,&c,&in,1,2,1000,1000) && h.reason==M9B1_BAD_CONFIG);
}
static void atomic_rechecks(void)
{
  M9B1Config c=config();M9B1Hold h;Mock m;M9B1Port p;
  unsigned fail;
  for(fail=1;fail<=3;fail++){
    start(&h,&c,&m,&p,0,985,1015);m.fail_call=fail;
    tick(&h,&c,&m,&p,100,985,1015);
    assert(!h.active && !h.lease && !m.pwm[0] && !m.pwm[1]);
    assert(m.locks==m.unlocks && m.irq==0 && h.reason==M9B1_PORT_FAULT);
  }
  start(&h,&c,&m,&p,0,985,1015);m.irq=1;
  tick(&h,&c,&m,&p,100,985,1015);assert(m.irq==1 && h.lease);
  {M9B1Input in=input(110U,1000U,1000U);uint32_t seq=h.seq;
   assert(!M9B1_Begin(&h,&c,&in,2U,3U,1000,1000));assert(h.seq==seq && h.active);}
}
static void wrap_and_jitter(void)
{
  M9B1Config c=config();M9B1Hold h;Mock m;M9B1Port p;
  uint32_t start_ms=UINT32_MAX-200U,t;
  start(&h,&c,&m,&p,start_ms,1000,1000);
  for(t=10;t<=10000;t+=10)tick(&h,&c,&m,&p,start_ms+t,
      (uint16_t)(1000+(t/10)%2),(uint16_t)(1000+(t/10)%2));
  assert(h.active && !h.axis[0].pwm && !h.axis[1].pwm && !h.ever_applied);
  assert(h.axis[0].velocity==0 && h.axis[1].velocity==0);
}
static void production_tuning_profile(void)
{
  M9B1Config c=m9_h2b1_config;M9B1Hold h;Mock m;M9B1Port p;
  uint32_t t,applied;
  assert(c.power_mode==M9B1_POWER_TUNING && !M9H2B1_POWER_COMMISSIONED);
  assert(c.axis[0].max_pwm==30U && c.axis[1].max_pwm==10U);
  assert(M9B1_ConfigValid(&c));
  start(&h,&c,&m,&p,0,1000,1000);
  for(t=10;t<=120000;t+=10)tick(&h,&c,&m,&p,t,1000,1000);
  assert(h.active && !h.ever_applied && !h.axis[0].effort_ms);
  /* The production profile really applies both axes, with independent caps. */
  for(;t<=120500;t+=10)tick(&h,&c,&m,&p,t,988,1012);
  assert(h.active && m.pwm[0] && m.pwm[1]);
  assert(m.dir[0]==1 && m.dir[1]==-1);
  assert(m.pwm[0]<=30U && m.pwm[1]<=10U && h.ever_applied);
  applied=h.applied_ms;assert(applied>0 && h.axis[0].applied_ms>0 && h.axis[1].applied_ms>0);
  for(;t<=122000 && h.active;t+=10)tick(&h,&c,&m,&p,t,988,1012);
  assert(!h.active && h.reason==M9B1_NO_PROGRESS && h.fault_axes);
  assert(!h.lease && !m.pwm[0] && !m.pwm[1]);
  tick(&h,&c,&m,&p,t+10,1000,1000);assert(!h.active); /* new goal required */
  /* Tuning cannot remove guards or silently raise the electrical test caps. */
  c=m9_h2b1_config;c.axis[1].max_pwm=11U;assert(!M9B1_ConfigValid(&c));
  c=m9_h2b1_config;c.tuning_no_progress_ms=0;assert(!M9B1_ConfigValid(&c));
  c=m9_h2b1_config;c.tuning_window_ms=c.tuning_effort_ms;assert(!M9B1_ConfigValid(&c));
}
static void effort_recovery_and_wrap(void)
{
  M9B1Config c=m9_h2b1_config;M9B1Hold h;Mock m;M9B1Port p;
  uint32_t offset=UINT32_MAX-300U,t,before;
  /* HOST-only long progress timeout isolates the effort-budget fault. */
  c.tuning_no_progress_ms=5000;
  start(&h,&c,&m,&p,offset,988,1012);
  for(t=10;t<=1000;t+=10)tick(&h,&c,&m,&p,offset+t,988,1012);
  before=h.axis[0].effort_ms;assert(before>500 && before<1500);
  /* Returning to goal drops output; zero time recovers gradually, not by reset. */
  for(;t<=2000;t+=10)tick(&h,&c,&m,&p,offset+t,1000,1000);
  assert(h.active && !m.pwm[0] && h.axis[0].effort_ms<before);
  assert(h.axis[0].effort_ms>0);
  for(;t<=5000 && h.active;t+=10)tick(&h,&c,&m,&p,offset+t,988,1012);
  assert(!h.active && h.reason==M9B1_EFFORT_BUDGET && h.fault_axes);
  assert(!m.pwm[0] && !m.pwm[1] && !h.lease);
}
int main(void)
{
  continuous_and_independent();release_damping();revocations();
  commissioned_interlock();atomic_rechecks();wrap_and_jitter();
  production_tuning_profile();effort_recovery_and_wrap();
  puts("PASS H2-B1 actual C: persistent dual-axis 120s, damping/reversal, fault revocation, atomic output, power interlock, wrap/jitter");
  return 0;
}
