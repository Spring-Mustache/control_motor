#ifndef VOICE_S154_ROUTE_H
#define VOICE_S154_ROUTE_H
#include <stdint.h>

/*
 * VC-02 integration I1 (pre-S15.4 synchronization).
 * PURE C route arithmetic only: no CCR writes, no motor power grants.
 * "degrees" here are voice steering SETPOINT units, linearly mapped
 * between calibrated CENTER and conservative manual SOFT endpoints.
 * NOT a certified physical wheel angle / Ackermann conversion.
 * Physical actuation is separately opt-in and safety-gated in main.c.
 */
typedef struct {
  uint8_t valid;
  uint16_t left_raw;
  uint16_t right_raw;
} VoiceS154Goals_t;

static inline uint8_t VoiceS154_DurationValid(uint8_t seconds)
{
  return ((seconds>=5U)&&(seconds<=120U)&&((seconds%5U)==0U))?1U:0U;
}
static inline int8_t VoiceS154_PivotDirection(uint8_t event_code)
{
  /* Operator convention: A4=LEFT=L2=-1; A7=RIGHT=L1=+1 */
  if(event_code==0xA4U)return -1;
  if(event_code==0xA7U)return 1;
  return 0;
}
static inline uint8_t VoiceS154_PivotNcReady(
  uint8_t physical_nc,uint8_t left_min_bit,uint8_t right_min_bit,
  uint8_t left_max_bit,uint8_t right_max_bit)
{
  uint8_t all=(uint8_t)(left_min_bit|right_min_bit|
                         left_max_bit|right_max_bit);
  uint8_t expect=(uint8_t)(left_min_bit|right_min_bit);
  return ((physical_nc&all)==expect)?1U:0U;
}
/* Direction -1 means LEFT, +1 RIGHT, angle 5..45 in increments of5.
 * 0deg is a separate CENTER command handled in the engine. */
static inline VoiceS154Goals_t VoiceS154_AngleGoals(
 int8_t turn,uint8_t angle,
 uint16_t left_center,uint16_t right_center,
 uint16_t left_soft_max,uint16_t right_soft_min,
 uint16_t left_soft_min,uint16_t right_soft_max)
{
 VoiceS154Goals_t g={0};
 int32_t end_l,end_r,gl,gr;
 if((turn!=-1)&&(turn!=1))return g;
 if((angle<5U)||(angle>45U)||((angle%5U)!=0U))return g;
 if(!(left_soft_max<left_center && left_center<left_soft_min &&
      right_soft_min<right_center && right_center<right_soft_max))
   return g;
 end_l=(turn<0)?left_soft_max:left_soft_min;
 end_r=(turn<0)?right_soft_min:right_soft_max;
 gl=(int32_t)left_center+
    ((end_l-(int32_t)left_center)*(int32_t)angle)/45L;
 gr=(int32_t)right_center+
    ((end_r-(int32_t)right_center)*(int32_t)angle)/45L;
 if(gl<0||gl>4095||gr<0||gr>4095)return g;
 g.valid=1U;
 g.left_raw=(uint16_t)gl;
 g.right_raw=(uint16_t)gr;
 return g;
}
#endif
