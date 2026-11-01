#ifndef STEER_S152_MANUAL_RELATION_H
#define STEER_S152_MANUAL_RELATION_H
#include <stdint.h>
/* S15.2 SHADOW ONLY. +/-45 degrees are CALIBRATED shaft coordinates,
 * NOT physical steering-wheel angles or Ackermann geometry.
 * qL=calibrated left, qR=negated calibrated right; both -1000..1000.
 */
#define M8S152_MAX_ROWS 96U
#define M8S152_PERIOD_MS 50U
#define M8S152_PREDICT_MS 120U /* historical S14 value, NOT tuned */
#define M8S152_MAX_AGE 100U
#define M8S152_MAX_SKEW 60U
#define M8S152_SHADOW_MAX_EXTRA 25U
#define M8S152_SHADOW_DEADBAND_PM 20

typedef struct { uint32_t stamp; int16_t q,speed; uint8_t valid; } M8S152Axis_t;
typedef struct { M8S152Axis_t l,r; int16_t initial; uint8_t initial_valid; } M8S152State_t;
typedef struct {
 int16_t qL,qR,error,predicted,initial,delta,velL,velR;
 uint8_t angle_valid,pred_valid;
} M8S152Plan_t;

static inline int16_t M8S152_Q(int16_t angle_cdeg,uint8_t left) {
 int32_t x=(left!=0U)?(int32_t)angle_cdeg:-(int32_t)angle_cdeg;
 if(x>4500L)x=4500L;
 if(x< -4500L)x= -4500L;
 return (int16_t)(x*1000L/4500L);
}
static inline int16_t M8S152_Clamp(int32_t x) {
 if(x>32767L)x=32767L;
 if(x< -32768L)x= -32768L;
 return (int16_t)x;
}
static inline uint8_t M8S152_Fresh(uint32_t now,uint32_t l,uint32_t r) {
 uint32_t a=(uint32_t)(l-r),b=(uint32_t)(r-l);
 return (uint8_t)(l!=0U&&r!=0U &&
     (uint32_t)(now-l)<=M8S152_MAX_AGE &&
     (uint32_t)(now-r)<=M8S152_MAX_AGE &&
     (a<=M8S152_MAX_SKEW||b<=M8S152_MAX_SKEW));
}
static inline void M8S152_Rate(M8S152Axis_t *s,uint32_t stamp,
                               int16_t q,uint32_t now) {
 int32_t d,v; uint32_t dt;
 if(s==0)return;
 if(stamp==0U||(uint32_t)(now-stamp)>M8S152_MAX_AGE) {
   s->valid=0U;return;
 }
 if(stamp==s->stamp)return; /* distinct AS5600 samples ONLY */
 if(s->stamp==0U) {
   s->stamp=stamp;s->q=q;s->valid=0U;return;
 }
 dt=(uint32_t)(stamp-s->stamp);
 d=(int32_t)q-(int32_t)s->q;
 s->stamp=stamp;s->q=q;
 if(dt<7U||dt>150U||d< -300L||d>300L) {
   s->valid=0U;return;
 }
 v=d*1000L/(int32_t)dt;
 if(v>2000L)v=2000L;
 if(v< -2000L)v= -2000L;
 if(s->valid!=0U)v=((int32_t)s->speed*3L+v)/4L;
 s->speed=(int16_t)v;s->valid=1U;
}
static inline M8S152Plan_t M8S152_Observe(M8S152State_t *s,
 int16_t angleL,int16_t angleR,uint32_t stampL,uint32_t stampR,
 uint32_t now,int8_t directionL,uint8_t geometry_ok,uint8_t both_moving) {
 M8S152Plan_t p={0};int32_t e,trend;
 if(s==0)return p;
 if(directionL==0||geometry_ok==0U) {
   s->l.valid=0U;s->r.valid=0U;return p;
 }
 p.qL=M8S152_Q(angleL,1U);p.qR=M8S152_Q(angleR,0U);
 if(M8S152_Fresh(now,stampL,stampR)==0U) {
   s->l.valid=0U;s->r.valid=0U;return p;
 }
 p.angle_valid=1U;
 M8S152_Rate(&s->l,stampL,p.qL,now);
 M8S152_Rate(&s->r,stampR,p.qR,now);
 e=(int32_t)p.qL-(int32_t)p.qR;
 if(directionL<0)e= -e;
 p.error=M8S152_Clamp(e);p.predicted=p.error;
 if(s->initial_valid==0U&&both_moving!=0U) {
   s->initial=p.error;s->initial_valid=1U;
 }
 p.initial=s->initial;p.delta=M8S152_Clamp(e-(int32_t)p.initial);
 p.velL=s->l.speed;p.velR=s->r.speed;
 if(both_moving==0U||s->l.valid==0U||s->r.valid==0U)return p;
 trend=(int32_t)s->l.speed-(int32_t)s->r.speed;
 if(directionL<0)trend= -trend;
 p.predicted=M8S152_Clamp(e+trend*(int32_t)M8S152_PREDICT_MS/1000L);
 p.pred_valid=1U;return p;
}
/* A hypothetical RIGHT-only PWM request: NEVER copy to PWM/CCR.
 * Current active envelope ONLY, not dormant proposed 300 CCR.
 * No requests after either axis enters <=180RAW terminal region.
 */
static inline uint16_t M8S152_ShadowRight(uint16_t appliedR,uint16_t capR,
 uint16_t remainingL,uint16_t remainingR,const M8S152Plan_t *p) {
 uint32_t extra,want;
 if(p==0||p->pred_valid==0U||appliedR==0U||capR<=appliedR||
    remainingL<=180U||remainingR<=180U||
    p->predicted<=M8S152_SHADOW_DEADBAND_PM)return appliedR;
 extra=(uint32_t)(p->predicted-M8S152_SHADOW_DEADBAND_PM)/2U;
 if(extra>M8S152_SHADOW_MAX_EXTRA)extra=M8S152_SHADOW_MAX_EXTRA;
 want=(uint32_t)appliedR+extra;
 if(want>capR)want=capR;
 return (uint16_t)want;
}
#endif
