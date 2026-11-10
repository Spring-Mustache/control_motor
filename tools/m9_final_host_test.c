/* Synthetic closed-loop plants: exercise software, NOT motor calibration. */
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "m9_h2b1_recovery_profile.h"
static M9B1Input input(uint32_t t, double l, double r)
{
 M9B1Input in={0};in.now_ms=t;in.sample_ms[0]=in.sample_ms[1]=t;
 in.sample_seq[0]=in.sample_seq[1]=t/10U+1U;
 in.raw[0]=(uint16_t)l;in.raw[1]=(uint16_t)r;
 in.valid[0]=in.valid[1]=in.enabled=in.safety_ok=in.owners_free=in.geometry_ok=1;
 return in;
}
static void step(M9B1Hold *h,uint32_t t,double l,double r)
{
 M9B1Input in=input(t,l,r);M9B1_Step(h,&m9_h2b1_config,&in);
 M9B1_Applied(h,t,h->axis[0].pwm,h->axis[1].pwm);
}
static void plant(unsigned friction,unsigned speed,unsigned inertia,int sign,unsigned displacement)
{
 static const double lf[]={150,220,280},rf[]={30,60,85};
 static const double gain[]={0.5,1.5,3},tau[]={0.02,0.06,0.12};
 M9B1Hold h;M9B1_Reset(&h);
 double x[2]={1886+sign*(double)displacement,662-sign*(double)displacement},v[2]={0,0};
 M9B1Input in=input(0,x[0],x[1]);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,1,2,1886,662));
 unsigned quiet=0;
 for(uint32_t t=10;t<=60000;t+=10){
  step(&h,t,x[0],x[1]);assert(h.active);
  for(unsigned i=0;i<2;i++){
   double u=h.applied_pwm[i],f=i?rf[friction]:lf[friction];
   double target=(fabs(u)>f)?copysign((fabs(u)-f)*gain[speed],u):0;
   v[i]+=(target-v[i])*0.01/(tau[inertia]+0.01);x[i]+=v[i]*0.01;
   assert(fabs(u)<=(i?100:300));
  }
  if(t>55000 && fabs(x[0]-1886)<8 && fabs(x[1]-662)<8)quiet++;
 }
 if(quiet<450){fprintf(stderr,"plant failure friction=%u speed=%u inertia=%u sign=%d displacement=%u err=%.2f,%.2f quiet=%u\n",friction,speed,inertia,sign,displacement,x[0]-1886,x[1]-662,quiet);assert(0);}
}
int main(void)
{
 assert(M9B1_ConfigValid(&m9_h2b1_config));
 for(unsigned f=0;f<3;f++)for(unsigned s=0;s<3;s++)for(unsigned j=0;j<3;j++)
  for(int dir=-1;dir<=1;dir+=2)for(unsigned e=8;e<=128;e*=4)plant(f,s,j,dir,e);
 M9B1Hold h;M9B1_Reset(&h);M9B1Input in=input(0,1886,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,1,2,1886,662));
 for(uint32_t t=10;t<=60000;t+=10)step(&h,t,1886+(t%3)-1,662);
 assert(h.active && !h.ever_applied); /* persistent, zero quiet heat */
 for(uint32_t t=60010;t<=61100;t+=10)step(&h,t,1850,662);
 assert(h.active && h.axis[0].recovery_rest && !h.applied_pwm[0]);
 for(uint32_t t=61110;t<=61400;t+=10)step(&h,t,1850,640);
 assert(h.axis[0].recovery_rest && h.applied_pwm[1]>0); /* independent */
 for(uint32_t t=61410;t<=64600;t+=10)step(&h,t,1850,662);
 assert(h.active && h.retry_count && h.applied_pwm[0]>0);
 M9B1_Revoke(&h,M9B1_STOP);step(&h,64610,1850,640);
 assert(!h.active && !h.applied_pwm[0] && !h.applied_pwm[1]);
 /* Bad final configs fail closed; wrap and reversal use unsigned clocks. */
 M9B1Config bad=m9_h2b1_config;bad.axis[0].max_pwm=301;assert(!M9B1_ConfigValid(&bad));
 bad=m9_h2b1_config;bad.tuning_no_progress_ms=0;assert(!M9B1_ConfigValid(&bad));
 M9B1_Reset(&h);in=input(UINT32_MAX-70,1850,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,2,2,1886,662));
 for(unsigned t=10;t<300;t+=10)step(&h,UINT32_MAX-70+t,1850,662);
 assert(h.active && h.applied_pwm[0]>0);
 uint32_t start=UINT32_MAX-70+300U;
 for(unsigned t=0;t<200;t+=10){step(&h,start+t,1920,662);assert(!h.applied_pwm[0]);}
 for(unsigned t=200;t<500;t+=10)step(&h,start+t,1920,662);
 assert(h.applied_pwm[0]<0);
 puts("PASS final recovery:162 closed-loop friction/inertia/direction models, quiet60s, stall/retry, independent axes, caps, STOP, reversal, tick wrap; NOT hardware calibration");
}
