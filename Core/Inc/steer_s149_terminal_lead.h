#ifndef STEER_S149_TERMINAL_LEAD_H
#define STEER_S149_TERMINAL_LEAD_H
#include <stdint.h>

/* M8-S14.9 source pilot: predict remaining time to the unchanged
 * 180-RAW coordinator cutoff using each axis' own filtered AS5600
 * angular velocity, plus the measured RIGHT-minus-LEFT terminal
 * lag from M8-S14.8 four-stroke ON-only report (337 ms).
 * This is a bounded hypothesis, NOT a validated motor dynamics model.
 * Extra duty is permitted only by the original independent owner cap
 * and final +12 CCR rise / NC / STOP / SOFT gating in the caller.
 */
#define M8S149_TERMINAL_SKEW_MS 337L
#define M8S149_PREDICT_DEADBAND_MS 50L
#define M8S149_MS_PER_EXTRA_CCR 12L
#define M8S149_MAX_EXTRA_CCR 25U

typedef struct {
  uint16_t request_ccr;
  int16_t predicted_skew_ms;
  uint16_t left_to_cutoff_ms;
  uint16_t right_to_cutoff_ms;
} M8S149Prediction_t;

static inline uint32_t M8S149_RawSpeed(uint16_t speed_cdeg_s,
                                       uint16_t span_raw,
                                       int32_t span_cdeg)
{
  uint32_t v;
  if(span_cdeg<=0||span_raw==0U)return 0U;
  v=((uint32_t)speed_cdeg_s*(uint32_t)span_raw)/(uint32_t)span_cdeg;
  if(v<250U)v=250U;     /* prevent noisy near-zero rate amplification */
  if(v>2000U)v=2000U;   /* only a conservative forecast, not a command */
  return v;
}

static inline M8S149Prediction_t M8S149_Predict(
    uint16_t dist_l,uint16_t dist_r,
    uint16_t span_raw_l,uint16_t span_raw_r,
    int32_t span_cdeg_l,int32_t span_cdeg_r,
    uint16_t speed_cdeg_l,uint16_t speed_cdeg_r)
{
  M8S149Prediction_t p={0U,0,0U,0U};
  uint32_t speed_l,speed_r;
  int32_t prediction,request;
  if(dist_l<=180U||dist_r<=180U||
     speed_cdeg_l<50U||speed_cdeg_r<50U)return p;
  speed_l=M8S149_RawSpeed(speed_cdeg_l,span_raw_l,span_cdeg_l);
  speed_r=M8S149_RawSpeed(speed_cdeg_r,span_raw_r,span_cdeg_r);
  if(speed_l==0U||speed_r==0U)return p;
  p.left_to_cutoff_ms=(uint16_t)(((uint32_t)(dist_l-180U)*1000U)/speed_l);
  p.right_to_cutoff_ms=(uint16_t)(((uint32_t)(dist_r-180U)*1000U)/speed_r);
  prediction=(int32_t)p.right_to_cutoff_ms-
             (int32_t)p.left_to_cutoff_ms+M8S149_TERMINAL_SKEW_MS;
  if(prediction>1000L)prediction=1000L;
  if(prediction< -1000L)prediction= -1000L;
  p.predicted_skew_ms=(int16_t)prediction;
  request=(prediction-M8S149_PREDICT_DEADBAND_MS)/
          M8S149_MS_PER_EXTRA_CCR;
  if(request<0L)request=0L;
  if(request>(int32_t)M8S149_MAX_EXTRA_CCR)
    request=M8S149_MAX_EXTRA_CCR;
  p.request_ccr=(uint16_t)request;
  return p;
}

/* Post-hybrid additional candidate: choose the MAX of the already
 * selected PI/P+D boost and the bounded terminal-lead request.
 * NEVER add the candidate boosts or slow LEFT. Full owner cap and
 * 12-CCR rise limits are re-applied. PWM=0 stays 0.
 */
static inline uint16_t M8S149_ChooseRight(
    uint16_t owner,uint16_t hybrid,uint16_t cap,
    uint16_t previous_final,uint16_t request_ccr)
{
  uint32_t want=(uint32_t)owner+(uint32_t)request_ccr;
  uint32_t allowed=(uint32_t)previous_final+12U;
  uint32_t result=hybrid;
  if(owner==0U||hybrid==0U||cap<=owner)return hybrid;
  if(want>result)result=want;
  if(allowed<owner)allowed=owner;
  if(result>allowed)result=allowed;
  if(result>cap)result=cap;
  return (uint16_t)result;
}
#endif
