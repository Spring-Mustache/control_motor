#include "steer_s152_manual_relation.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 M8S152State_t s={0};M8S152Plan_t p;
 assert(M8S152_Q(2250,1U)==500);
 assert(M8S152_Q(-2250,0U)==500);
 assert(M8S152_Q(9000,1U)==1000);
 assert(M8S152_Q(-9000,1U)==-1000);
 assert(M8S152_Fresh(1050U,1000U,1020U));
 assert(!M8S152_Fresh(1250U,1000U,1020U));
 assert(!M8S152_Fresh(1070U,1000U,1070U));
 p=M8S152_Observe(&s,0,0,1000U,1000U,1030U,1,1,1);
 assert(p.angle_valid&&!p.pred_valid&&p.error==0);
 p=M8S152_Observe(&s,900,-450,1100U,1100U,1120U,1,1,1);
 assert(p.angle_valid&&p.pred_valid&&p.qL==200&&p.qR==100);
 assert(p.error==100&&p.predicted>p.error&&p.delta==100);
 assert(M8S152_ShadowRight(175,200,400,400,&p)>175);
 assert(M8S152_ShadowRight(195,200,400,400,&p)==200);
 assert(M8S152_ShadowRight(175,200,180,400,&p)==175);
 assert(M8S152_ShadowRight(175,200,400,180,&p)==175);
 assert(M8S152_ShadowRight(0,200,400,400,&p)==0);
 p=M8S152_Observe(&s,900,-450,1100,1100,1140,1,1,1);
 assert(p.pred_valid); /* repeated tick cannot integrate a new sample */
 p=M8S152_Observe(&s,900,-450,1100,1100,1210,1,1,1);
 assert(!p.angle_valid&&!p.pred_valid);
 assert(M8S152_ShadowRight(175,200,400,400,&p)==175);
 s=(M8S152State_t){0};
 p=M8S152_Observe(&s,-900,450,1000,1000,1020,-1,1,1);
 assert(p.error==100);
 p=M8S152_Observe(&s,-1800,450,1090,1090,1110,-1,1,1);
 assert(p.error==300&&p.predicted>p.error);
 p=M8S152_Observe(&s,0,0,1200,1200,1210,-1,0,1);
 assert(!p.angle_valid&&!p.pred_valid);
 p=M8S152_Observe(&s,-1800,450,1200,1200,1210,-1,1,0);
 assert(p.angle_valid&&!p.pred_valid);
 assert(M8S152_ShadowRight(175,200,400,400,&p)==175);
 s=(M8S152State_t){0};
 p=M8S152_Observe(&s,0,0,0xfffffff0U,0xfffffff0U,2U,1,1,1);
 assert(p.angle_valid);
 p=M8S152_Observe(&s,450,-450,36U,36U,50U,1,1,1);
 assert(p.pred_valid);
 p=M8S152_Observe(&s,4000,-4000,70U,70U,85U,1,1,1);
 assert(!p.pred_valid);
 puts("M8-S15.2 passive manual relation HOST PASS");return 0;
}
