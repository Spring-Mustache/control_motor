#ifndef STEER_S15_COMMON_TIME_H
#define STEER_S15_COMMON_TIME_H
#include <stdint.h>

/* M8-S15: a single clock for two INDEPENDENT calibrated angle goals.
 * Pure planner: no HAL, pins, driver or safety bypass. Units: cdeg, ms,
 * cdeg/s, and CCR. It cannot promise physical simultaneous arrival.
 * Production caller must enforce each owner SOFT/NC/STOP and atomic CCR.
 */
#define M8S15_MAX_CDEG 4500
#define M8S15_MIN_TIME_MS 1200U
#define M8S15_MAX_TIME_MS 4200U
#define M8S15_AUTO_MAX_CDEG_S 6500U
#define M8S15_RISE_CCR 12U
typedef struct {
  int16_t start_l,goal_l,start_r,goal_r;
  uint32_t start_tick;
  uint16_t total_ms;
  uint8_t ready;
} M8S15Session_t;
typedef struct {
  int16_t ref_cdeg;
  int16_t error_cdeg;
  uint16_t ref_speed_cdeg_s;
  uint16_t output_ccr;
  uint8_t valid;
  uint8_t saturated;
} M8S15AxisPlan_t;
typedef struct {
  M8S15AxisPlan_t left,right;
  uint16_t elapsed_ms;
  uint16_t phase_permille;
  uint8_t valid;
} M8S15PairPlan_t;
static inline uint32_t M8S15_Abs32(int32_t x)
{ return (uint32_t)(x<0?-x:x); }
static inline uint8_t M8S15_Bounds(int16_t x)
{ return (uint8_t)(x>=-M8S15_MAX_CDEG && x<=M8S15_MAX_CDEG); }
static inline uint16_t M8S15_RequiredTime(int16_t s_l,int16_t g_l,
                                           int16_t s_r,int16_t g_r,
                                           uint16_t requested_ms)
{
  uint32_t dl=M8S15_Abs32((int32_t)g_l-s_l);
  uint32_t dr=M8S15_Abs32((int32_t)g_r-s_r);
  uint32_t d=(dl>dr)?dl:dr;
  uint32_t min_time=(uint32_t)((3ULL*d*1000U+
                             2U*M8S15_AUTO_MAX_CDEG_S-1U)/
                             (2U*M8S15_AUTO_MAX_CDEG_S))+150U;
  uint32_t t=(requested_ms!=0U)?requested_ms:min_time;
  if(t<M8S15_MIN_TIME_MS)t=M8S15_MIN_TIME_MS;
  if(t>M8S15_MAX_TIME_MS)t=M8S15_MAX_TIME_MS;
  /* Request shorter than conservative travel-time minimum is rejected
   * instead of silently commanding faster-than-modeled velocity.
   */
  if(t<min_time)return 0U;
  return (uint16_t)t;
}
static inline uint8_t M8S15_Init(M8S15Session_t *s,
    int16_t start_l,int16_t goal_l,int16_t start_r,int16_t goal_r,
    uint16_t requested_ms,uint32_t now)
{
  uint16_t t;
  *s=(M8S15Session_t){0};
  if(!M8S15_Bounds(start_l)||!M8S15_Bounds(goal_l)||
     !M8S15_Bounds(start_r)||!M8S15_Bounds(goal_r))return 0U;
  if(start_l==goal_l||start_r==goal_r)return 0U;
  t=M8S15_RequiredTime(start_l,goal_l,start_r,goal_r,requested_ms);
  if(t==0U)return 0U;
  s->start_l=start_l;s->goal_l=goal_l;
  s->start_r=start_r;s->goal_r=goal_r;
  s->start_tick=now;s->total_ms=t;s->ready=1U;
  return 1U;
}
static inline M8S15AxisPlan_t M8S15_Axis(
  int16_t start,int16_t goal,int16_t measured,
  uint16_t measured_speed_cdeg_s,
  uint16_t owner,uint16_t cap,uint16_t floor,
  uint16_t prev,uint16_t distance_raw,uint16_t phase,
  uint16_t duration_ms,uint8_t feedback_fresh)
{
  M8S15AxisPlan_t r={0};
  int32_t delta=(int32_t)goal-start;
  uint32_t pos_fraction,vel_fraction,vel,dist;
  int32_t ref,err,normalized_err,desired,correction;
  uint32_t max_rise;
  if(!feedback_fresh||!M8S15_Bounds(measured)||
     !M8S15_Bounds(start)||!M8S15_Bounds(goal)||
     delta==0||owner==0U||cap<owner||duration_ms==0U)return r;
  pos_fraction=(uint32_t)(((uint64_t)phase*phase*
                           (3000U-2U*(uint32_t)phase)+500000U)/1000000U);
  ref=(int32_t)start+(int32_t)(((int64_t)delta*
                                   (int64_t)pos_fraction)/1000);
  if(ref>4500)ref=4500;
  if(ref< -4500)ref=-4500;
  vel_fraction=(uint32_t)(6U*(uint32_t)phase*(1000U-phase));
  vel=(uint32_t)(((uint64_t)M8S15_Abs32(delta)*vel_fraction)/
                  ((uint64_t)duration_ms*1000ULL));
  if(vel>65535U)vel=65535U;
  err=ref-measured;
  normalized_err=(delta>0)?err:-err;
  /* One controller, not PI/P+D/S149 stacking. Individual feedforward
   * has distinct L/R owner; normalized angular errors provide correction.
   * Output may REDUCE the leading motor to honor shared arrival time.
   */
  correction=normalized_err/18+
      ((int32_t)vel-(int32_t)measured_speed_cdeg_s)/180;
  desired=(int32_t)owner+correction;
  if(floor>owner)floor=owner;
  if(desired<(int32_t)floor)desired=floor;
  if(desired>(int32_t)cap)desired=cap;
  /* Never BOOST the tested baseline in final 180 RAW. Slowdown
   * is allowed; independent per-axis SOFT and NC still authoritative.
   */
  if(distance_raw<=180U&&desired>(int32_t)owner)desired=owner;
  max_rise=(uint32_t)prev+M8S15_RISE_CCR;
  if(desired>(int32_t)max_rise)desired=(int32_t)max_rise;
  if(desired<0)desired=0;
  dist=(uint32_t)desired;
  r.ref_cdeg=(int16_t)ref;
  r.error_cdeg=(int16_t)err;
  r.ref_speed_cdeg_s=(uint16_t)vel;
  r.output_ccr=(uint16_t)dist;
  r.valid=1U;
  r.saturated=(uint8_t)((desired==(int32_t)floor)||
                        (desired==(int32_t)cap));
  return r;
}
static inline M8S15PairPlan_t M8S15_Step(
  const M8S15Session_t *s,uint32_t now,
  int16_t angle_l,int16_t angle_r,
  uint16_t speed_l,uint16_t speed_r,
  uint16_t owner_l,uint16_t owner_r,
  uint16_t cap_l,uint16_t cap_r,
  uint16_t floor_l,uint16_t floor_r,
  uint16_t prev_l,uint16_t prev_r,
  uint16_t dist_raw_l,uint16_t dist_raw_r,
  uint8_t fresh_l,uint8_t fresh_r)
{
  M8S15PairPlan_t p={0};
  uint32_t elapsed,phase;
  if(!s->ready||!fresh_l||!fresh_r)return p;
  elapsed=(uint32_t)(now-s->start_tick);
  if(elapsed>s->total_ms)elapsed=s->total_ms;
  phase=(uint32_t)(((uint64_t)elapsed*1000U)/s->total_ms);
  if(phase>1000U)phase=1000U;
  p.left=M8S15_Axis(s->start_l,s->goal_l,angle_l,speed_l,owner_l,
                   cap_l,floor_l,prev_l,dist_raw_l,(uint16_t)phase,
                   s->total_ms,fresh_l);
  p.right=M8S15_Axis(s->start_r,s->goal_r,angle_r,speed_r,owner_r,
                   cap_r,floor_r,prev_r,dist_raw_r,(uint16_t)phase,
                   s->total_ms,fresh_r);
  if(!p.left.valid||!p.right.valid)return p;
  p.elapsed_ms=(uint16_t)elapsed;
  p.phase_permille=(uint16_t)phase;
  p.valid=1U;
  return p;
}
#endif
