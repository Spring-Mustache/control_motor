#ifndef STEER_S143_FASTSTART_H
#define STEER_S143_FASTSTART_H
#include <stdint.h>
/* Pure cached AS5600 neutral stillness; NO peripheral writes/reads. */
#define M8S143_QUIET_MS 100U
#define M8S143_MAX_AGE_MS 65U
#define M8S143_MAX_GAP_MS 80U
typedef struct {
 uint16_t anchor_raw,last_raw;
 uint32_t quiet_since_ms,last_sample_ms;
 uint8_t distinct_samples,valid;
} M8S143_Quiet;
static inline void M8S143_Reset(M8S143_Quiet *q)
{ *q=(M8S143_Quiet){0}; }
static inline uint16_t M8S143_AbsDelta(uint16_t a,uint16_t b)
{
 int32_t d=(int32_t)a-(int32_t)b;
 if(d>2048)d-=4096;
 if(d< -2048)d+=4096;
 return (uint16_t)((d<0)?-d:d);
}
static inline void M8S143_Observe(M8S143_Quiet *q,
 uint32_t now,uint32_t sample_ms,uint16_t raw,uint8_t allowed)
{
 if(!allowed||sample_ms==0U||
    (uint32_t)(now-sample_ms)>M8S143_MAX_AGE_MS)
 {M8S143_Reset(q);return;}
 if(!q->valid||
    (uint32_t)(sample_ms-q->last_sample_ms)>M8S143_MAX_GAP_MS)
 {
   q->valid=1U;q->anchor_raw=raw;q->last_raw=raw;
   q->last_sample_ms=sample_ms;q->quiet_since_ms=sample_ms;
   q->distinct_samples=1U;return;
 }
 if(sample_ms==q->last_sample_ms)return;
 if(M8S143_AbsDelta(raw,q->last_raw)>1U||
    M8S143_AbsDelta(raw,q->anchor_raw)>2U)
 {q->anchor_raw=raw;q->quiet_since_ms=sample_ms;q->distinct_samples=1U;}
 else if(q->distinct_samples<255U)q->distinct_samples++;
 q->last_raw=raw;q->last_sample_ms=sample_ms;
}
static inline uint8_t M8S143_Ready(const M8S143_Quiet *q,
 uint32_t now,uint32_t sample_ms,uint16_t raw)
{
 return (uint8_t)(q->valid&&q->distinct_samples>=3U&&
  (uint32_t)(q->last_sample_ms-q->quiet_since_ms)>=M8S143_QUIET_MS&&
  (uint32_t)(now-q->last_sample_ms)<=M8S143_MAX_AGE_MS&&
  (uint32_t)(now-sample_ms)<=M8S143_MAX_AGE_MS&&
  M8S143_AbsDelta(raw,q->last_raw)<=2U&&
  M8S143_AbsDelta(raw,q->anchor_raw)<=2U);
}
#endif
