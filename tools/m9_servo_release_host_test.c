#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifdef M9B1_TEST_RECOVERY_BASELINE
#include "m9_h2b1_recovery_profile.h"
#else
#include "m9_h2b1_commissioning.h"
#endif
#include <assert.h>
/* Stress fixtures only: gains, friction and delay are NOT identified motors.
 * Signed actuator lag represents possible driver ramping. Release covers
 * both a held displacement and removal of an already-balanced load.
 * Measure transient settling/crossings, not just a final sample at45s. */
int main(void){
 double gains[]={.2,.5,1},inertias[]={.02,.06,.12},lags[]={0,.06,.12};
 unsigned cases=0,bad=0,max_settle=0,max_cross=0;
 for(int gi=0;gi<3;gi++)for(int ji=0;ji<3;ji++)for(int li=0;li<3;li++)for(int fr=0;fr<2;fr++)for(int sc=0;sc<2;sc++)for(int dir=-1;dir<=1;dir+=2){
  M9B1Hold h={0};M9B1Input in={0};in.raw[0]=1886;in.raw[1]=662;
  in.enabled=in.valid[0]=in.valid[1]=in.safety_ok=in.owners_free=in.geometry_ok=1;
  if(!M9B1_Begin(&h,&m9_h2b1_config,&in,1,2,1886,662))return 2;
  double x[]={1886,662},v[]={0,0},u[]={0,0},peak[]={0,0};int lastside[]={0,0},cross[]={0,0};unsigned lastout[]={0,0};
  unsigned release=sc?20000:2000,end=release+15000;
  for(unsigned t=10;t<=end;t+=10){
   if(!sc && t==1000){x[0]+=dir*60;x[1]-=dir*60;}
   unsigned side=(t/10)%2;
   in.now_ms=t;in.sample_ms[side]=t;++in.sample_seq[side];in.raw[side]=(unsigned)llround(x[side]);
   M9B1_Step(&h,&m9_h2b1_config,&in);M9B1_Applied(&h,t,h.axis[0].pwm,h.axis[1].pwm);
   if(!h.active){fprintf(stderr,"revoked reason%d\n",h.reason);return 3;}
   for(int i=0;i<2;i++){
    u[i]+=(h.applied_pwm[i]-u[i])*.01/(lags[li]+.01);
    double load=(sc && t>=1000 && t<release)?dir*(i?-35:120):0;
    double f=fr?(i?15:60):0,net=u[i]-load;
    net=fabs(net)>f?copysign(fabs(net)-f,net):0;
    v[i]+=(gains[gi]*net-v[i])*.01/(inertias[ji]+.01);
    if(!sc && t>=1000 && t<release)v[i]=0;
    x[i]+=v[i]*.01;
    double error=x[i]-(i?662:1886);
    if(t>=release){
     if(fabs(error)>3 || fabs(v[i])>8)lastout[i]=t;
     if(fabs(error)>peak[i])peak[i]=fabs(error);
     int s=(error>3)?1:(error< -3)?-1:0;
     if(s){if(lastside[i] && s!=lastside[i])cross[i]++;lastside[i]=s;}
    }
   }
  }
  ++cases;
  for(int i=0;i<2;i++){
   unsigned settling=lastout[i]?lastout[i]-release:0;
   if(settling>max_settle)max_settle=settling;
   if((unsigned)cross[i]>max_cross)max_cross=(unsigned)cross[i];
  }
  if((lastout[0] && lastout[0]-release>6000) ||
     (lastout[1] && lastout[1]-release>6000) || cross[0]>3 || cross[1]>3)++bad;
 }
#ifdef M9B1_TEST_RECOVERY_BASELINE
 assert(bad>0);
 printf("BASELINE profile3: %u/%u synthetic release cases violate settling/crossing bounds; max settle=%ums, crossings=%u\n",bad,cases,max_settle,max_cross);
#else
 printf("SERVO profile4: %u/%u synthetic release cases violate bounds; max settle=%ums, crossings=%u\n",bad,cases,max_settle,max_cross);
 assert(bad==0);
#endif
 return 0;
}
