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
 /* Regression replay of RMIN195/SOFT245 at 19:33. ePred is
  * intentionally very negative: velocity and remaining ETA decide.
  */
 {
  M8S152Plan_t mid={0};mid.angle_valid=1U;mid.pred_valid=1U;
  mid.predicted=-322;mid.velL=1463;mid.velR=1726;
  p=M8S153_PlanRightMid(450,175,200,582,444,192,317,1,1,1,&mid);
  assert(p.eta_valid==1U&&p.eta_lag_ms==94);
  assert(p.right_pwm==177&&p.boost_ccr==2&&p.early==1U);
  mid.predicted=-138;mid.velL=1573;mid.velR=1689;
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.eta_valid==1U&&p.eta_lag_ms==271);
  assert(p.right_pwm==187&&p.boost_ccr==12&&p.early==1U);
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,0,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  mid.predicted=400; /* even lagging RIGHT has no boost near endpoint */
  p=M8S153_PlanRightMid(450,175,200,320,423,452,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,490,320,282,439,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,180,180,590,580,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,651,423,121,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,490,651,282,108,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,0,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,0,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(0,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,0,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==0&&p.early==0U);
  p=M8S153_PlanRightMid(450,201,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==201&&p.early==0U);
  p=M8S153_PlanRightMid(450,198,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==200&&p.boost_ccr==2);
  mid.pred_valid=0U;
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  mid.pred_valid=1U;mid.angle_valid=0U;
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  mid.angle_valid=1U;mid.velL=-10;mid.velR=1689;
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  mid.velL=1573;mid.velR=249;
  p=M8S153_PlanRightMid(450,175,200,490,423,282,336,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  mid.velR=1689;
  p=M8S153_PlanRightMid(450,175,200,490,423,30,40,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  p=M8S153_PlanRightMid(450,175,200,490,423,750,740,1,1,1,&mid);
  assert(p.right_pwm==175&&p.early==0U);
  for(unsigned owner=0;owner<=220;owner+=4)
   for(unsigned cap=100;cap<=200;cap+=10)
    for(int pred=-400;pred<=400;pred+=40){
     mid.predicted=(int16_t)pred;
     M8S153Plan_t x=M8S153_PlanRightMid(450,(uint16_t)owner,
         (uint16_t)cap,490,423,282,336,1,1,1,&mid);
     assert(x.right_pwm>=owner);
     assert(x.right_pwm-owner<=M8S153_RIGHT_MAX_EXTRA_CCR);
     if(cap>owner)assert(x.right_pwm<=cap);
     assert(x.early==(x.boost_ccr>0U));
    }
 }
 puts("M8-S15.3 RIGHT-only pure HOST PASS");return 0;
}
