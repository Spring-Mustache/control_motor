#include <assert.h>
#include <stdio.h>
#include "steer_s15_common_time.h"
int main(void)
{
 M8S15Session_t s={0};M8S15PairPlan_t p;
 assert(M8S15_Init(&s,0,3500,0,3000,2000U,100U));
 assert(s.goal_l==3500&&s.goal_r==3000&&s.total_ms==2000U);
 p=M8S15_Step(&s,1100,1750,1500,2600,2200,
      250,140,450,200,175,50,200,130,400,400,1,1);
 assert(p.valid&&p.phase_permille==500);
 assert(p.left.ref_cdeg==1750&&p.right.ref_cdeg==1500);
 assert(p.left.ref_speed_cdeg_s>p.right.ref_speed_cdeg_s);
 assert(p.left.output_ccr<=450&&p.right.output_ccr<=200);
 /* Independent targets, opposite signs, off-center starts */
 assert(M8S15_Init(&s,1600,-2000,-900,2500,0U,77U));
 p=M8S15_Step(&s,77+s.total_ms/2,0,0,600,600,190,90,
     300,100,100,45,190,90,200,200,1,1);
 assert(p.valid&&p.left.ref_cdeg<1600&&p.right.ref_cdeg> -900);
 /* Final 180 raw never receives a positive coordinator boost */
 p=M8S15_Step(&s,77+s.total_ms/2,0,0,100,100,190,90,
     300,100,100,45,190,90,150,150,1,1);
 assert(p.valid&&p.left.output_ccr<=190&&p.right.output_ccr<=90);
 /* Missing sensor/owner never energizes an axis */
 p=M8S15_Step(&s,77,0,0,0,0,190,90,300,100,100,45,
      190,90,250,250,0,1);
 assert(!p.valid);
 p=M8S15_Step(&s,77,0,0,0,0,190,0,300,100,100,45,
      190,90,250,250,1,1);
 assert(!p.valid);
 /* Tiny impossible requested T is rejected; 0 chooses shared T */
 assert(!M8S15_Init(&s,-4500,4500,-4500,4500,1200U,0));
 assert(M8S15_Init(&s,-4500,4500,-4500,4500,0,0));
 assert(s.total_ms>=2200);
 assert(!M8S15_Init(&s,0,0,0,1000,2000U,0));
 assert(!M8S15_Init(&s,5000,0,0,1000,2000U,0));
 /* owner 0 => never revived, terminal rise cannot exceed old PWM */
 {M8S15AxisPlan_t a=M8S15_Axis(0,1000,0,0,
      0,100,50,0,50,250,2000,1);assert(!a.valid);}
 puts("PASS M8-S15 common-clock dual independent targets and owner guards");
 return 0;
}
