/* Synthetic plant and actuator-state regression; no motor calibration. */
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "m9_h2b1_commissioning.h"
static M9B1Input input(uint32_t t,double l,double r){
 M9B1Input in={0};in.now_ms=t;in.sample_ms[0]=in.sample_ms[1]=t;
 in.sample_seq[0]=in.sample_seq[1]=t/10+1;
 in.raw[0]=(uint16_t)l;in.raw[1]=(uint16_t)r;
 in.enabled=in.valid[0]=in.valid[1]=in.safety_ok=in.owners_free=in.geometry_ok=1;return in;
}
static void step(M9B1Hold *h,uint32_t t,double l,double r){
 M9B1Input in=input(t,l,r);M9B1_Step(h,&m9_h2b1_config,&in);
 M9B1_Applied(h,t,h->axis[0].pwm,h->axis[1].pwm);
}
static void plant(double gain,double inertia,int dir,unsigned friction){
 M9B1Hold h;M9B1_Reset(&h);M9B1Input in=input(0,1886,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,1,2,1886,662));
 double x[2]={1886,662},v[2]={0,0};unsigned good_load=0,good_release=0;
 M9B1Input sampled=in;
 for(uint32_t t=10;t<=90000;t+=10){
  /* Actual nominal scheduler: one sensor every10ms, each axis20ms. */
  unsigned side=(t/10U)%2U;
  sampled.now_ms=t;sampled.raw[side]=(uint16_t)llround(x[side]);
  sampled.sample_ms[side]=t;++sampled.sample_seq[side];
  M9B1_Step(&h,&m9_h2b1_config,&sampled);
  M9B1_Applied(&h,t,h.axis[0].pwm,h.axis[1].pwm);
  assert(h.active && h.state!=M9B1_COOLDOWN);
  for(unsigned i=0;i<2;i++){
   double load=(t>=1000 && t<45000)?dir*(i?-35:120):0;
   double f=friction?(i?15:60):0;
   double net=(double)h.applied_pwm[i]-load;
   net=fabs(net)>f?copysign(fabs(net)-f,net):0;
   v[i]+=(gain*net-v[i])*0.01/(inertia+0.01);x[i]+=v[i]*0.01;
   assert(fabs((double)h.applied_pwm[i])<=(i?100:300));
  }
  if(t>40000 && t<45000 && fabs(x[0]-1886)<=3 && fabs(x[1]-662)<=3)good_load++;
  if(t>85000 && fabs(x[0]-1886)<=3 && fabs(x[1]-662)<=3)good_release++;
 }
 if(good_load<450 || good_release<450){fprintf(stderr,"FAIL gain=%g inertia=%g dir=%d load=%u release=%u err=%.3f/%.3f\n",gain,inertia,dir,good_load,good_release,x[0]-1886,x[1]-662);assert(0);}
}
int main(void){
 assert(M9B1_ConfigValid(&m9_h2b1_config));
 const double gains[]={0.2,0.5,1.0},inertias[]={0.02,0.06,0.12};
 for(unsigned g=0;g<3;g++)for(unsigned j=0;j<3;j++)for(int dir=-1;dir<=1;dir+=2)for(unsigned f=0;f<2;f++)plant(gains[g],inertias[j],dir,f);
 M9B1Hold h;M9B1_Reset(&h);M9B1Input in=input(0,1882,666);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,1,2,1886,662));
 for(unsigned t=10;t<=1000;t+=10)step(&h,t,1882,666);
 for(unsigned t=1010;t<=2000;t+=10)step(&h,t,1886,662);
 int16_t l=h.applied_pwm[0],r=h.applied_pwm[1];assert(l>0 && r<0);
 for(unsigned t=2010;t<=602000;t+=10){step(&h,t,1886,662);assert(h.applied_pwm[0]==l && h.applied_pwm[1]==r && h.active);}
 for(unsigned t=602010;t<=662000;t+=10){step(&h,t,1850,698);assert(h.applied_pwm[0]>0 && h.applied_pwm[1]<0 && h.state==M9B1_HOLDING);}
 assert(h.axis[0].integral_q8<=300*256 && h.axis[1].integral_q8>=-100*256);
 /* Saturation cannot build unbounded I; opposite error must unwind. */
 for(unsigned t=662010;t<=682000;t+=10)step(&h,t,1920,640);
 assert(h.applied_pwm[0]<0 && h.applied_pwm[1]>0);
 M9B1_Revoke(&h,M9B1_STOP);assert(!h.axis[0].integral_q8 && !h.axis[1].integral_q8);
 /* A zero command during blanking must not freeze I forever off-target. */
 M9B1_Reset(&h);in=input(0,1884,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,2,2,1886,662));
 h.axis[0].integral_q8=-4096;h.axis[0].reversing=1;
 for(unsigned t=10;t<=2000;t+=10)step(&h,t,1884,662);
 assert(h.axis[0].integral_q8> -4096 && h.applied_pwm[0]>0);
 /* Quantization jitter at the target does not erase a balanced load. */
 M9B1_Reset(&h);in=input(0,1886,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,3,2,1886,662));
 h.axis[0].integral_q8=120*256;h.axis[1].integral_q8=-35*256;
 for(unsigned t=20;t<=10000;t+=20){
  int jitter=(int)((t/20)%3)-1;step(&h,t,1886+jitter,662-jitter);
  assert(h.axis[0].integral_q8==120*256 && h.axis[1].integral_q8==-35*256);
 }
 /* Old load torque opposed to a persistent off-goal error unwinds
  * within1s, while the earlier exact-goal test requires it retained. */
 M9B1_Reset(&h);in=input(0,1890,658);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,4,2,1886,662));
 h.axis[0].integral_q8=120*256;h.axis[1].integral_q8=-35*256;
 for(unsigned t=10;t<=1000;t+=10)step(&h,t,1890,658);
 assert(h.applied_pwm[0]<0 && h.applied_pwm[1]>0);
 /* Slow measured return must reach the damping estimator, even if
  * individual fresh samples differ by only1RAW. */
 M9B1_Reset(&h);in=input(0,1850,662);
 assert(M9B1_Begin(&h,&m9_h2b1_config,&in,5,2,1886,662));
 unsigned detected=0;
 for(unsigned t=20;t<=1000;t+=20){step(&h,t,1850+t/100,662);if(h.axis[0].velocity>=5)detected++;}
 assert(detected>=25);
 puts("PASS jitter preserves load, opposite-error unwind, sub-RAW/sample damping");
 puts("PASS continuous SERVO:36 friction/sustained-load/removal models, exact-goal powered hold600s, forced error60s NO rest, saturation/unwind, STOP reset; synthetic only");
}
