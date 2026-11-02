#include "steer_s153_right_only.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 M8S152Plan_t o={0}; M8S153Plan_t p;
 o.angle_valid=1U;o.pred_valid=1U;o.predicted=80;
 p=M8S153_PlanRight(405,175,200,500,500,100,100,1,1,&o);
 assert(p.eligible&&p.requested&&p.right_pwm==187&&p.boost_ccr==12);
 assert(M8S153_PlanRight(405,195,200,500,500,100,100,1,1,&o).right_pwm==200);
 assert(M8S153_PlanRight(405,175,200,180,500,100,100,1,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,175,200,500,180,100,100,1,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,175,200,500,500,11,100,1,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,175,200,500,500,100,11,1,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,0,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,1,0,&o).right_pwm==175);
 assert(M8S153_PlanRight(0,175,200,500,500,100,100,1,1,&o).right_pwm==175);
 assert(M8S153_PlanRight(405,0,200,500,500,100,100,1,1,&o).right_pwm==0);
 assert(M8S153_PlanRight(405,200,200,500,500,100,100,1,1,&o).right_pwm==200);
 assert(M8S153_PlanRight(405,201,200,500,500,100,100,1,1,&o).right_pwm==201);
 o.predicted=20;
 p=M8S153_PlanRight(405,175,200,500,500,100,100,1,1,&o);
 assert(p.eligible&&!p.requested&&p.right_pwm==175);
 o.predicted=-200;
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,1,1,&o).right_pwm==175);
 o.predicted=80;o.pred_valid=0;
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,1,1,&o).right_pwm==175);
 o.pred_valid=1;o.angle_valid=0;
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,1,1,&o).right_pwm==175);
 o.angle_valid=1;
 assert(M8S153_PlanRight(405,175,200,500,500,100,100,1,1,0).right_pwm==175);
 for(unsigned a=1;a<201;a+=3) {
   for(unsigned cap=75;cap<=200;cap+=5) {
     M8S153Plan_t x=M8S153_PlanRight(450,(uint16_t)a,(uint16_t)cap,190,320,100,100,1,1,&o);
     assert(x.right_pwm>=a);
     assert(x.right_pwm-a<=12);
     if(cap>a)assert(x.right_pwm<=cap);
   }
 }
 puts("M8-S15.3 RIGHT-only pure HOST PASS");return 0;
}
