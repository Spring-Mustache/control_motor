#ifndef STEER_S153_STALL_DIAG_H
#define STEER_S153_STALL_DIAG_H
#include <stdint.h>
/* Passive only: sensor sample timestamp can lead loop tick by a few ms. */
#define M8S153_DIAG_FUTURE_JITTER_MS 8U
#define M8S153_STOP5_NEUTRAL_REQUIRED 1U
#define M8S153_STOP5_DIRECT_REVERSAL 2U
static inline uint16_t M8S153_DiagGap(uint16_t raw,uint16_t goal)
{
 return raw>=goal?(uint16_t)(raw-goal):(uint16_t)(goal-raw);
}
static inline uint8_t M8S153_DiagFuture(uint32_t now,uint32_t stamp)
{
 return (uint8_t)(stamp!=0U&&now<stamp&&
         (stamp-now)<=M8S153_DIAG_FUTURE_JITTER_MS);
}
static inline uint32_t M8S153_DiagAgeMs(uint32_t now,uint32_t stamp)
{
 if(stamp==0U||M8S153_DiagFuture(now,stamp)!=0U)return 0U;
 return now-stamp;
}
#endif
