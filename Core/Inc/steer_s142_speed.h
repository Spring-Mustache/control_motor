#ifndef STEER_S142_SPEED_H
#define STEER_S142_SPEED_H
#include <stdint.h>
/* Pure manual-only experimental ceiling controller; no motor I/O. */
static inline uint16_t M8S142_Cap(uint8_t left,uint8_t level)
{
  if(level==1U)return (left!=0U)?375U:150U;
  if(level==2U)return (left!=0U)?450U:200U;
  return (left!=0U)?300U:100U;
}
static inline uint16_t M8S142_Envelope(uint8_t left,uint8_t level,uint16_t remaining)
{
  uint16_t original=(left!=0U)?300U:100U;
  uint16_t high=M8S142_Cap(left,level);
  if(remaining<=180U||high<=original)return original;
  if(remaining>=320U)return high;
  return (uint16_t)(original+((uint32_t)(high-original)*(remaining-180U))/140U);
}
/* Reserve ceiling headroom for angular-error correction.
 * Both baseline demands exceed the old 300/100 at full D-pad.
 */
static inline uint16_t M8S142_Base(uint8_t left,uint8_t level)
{
  if(level==1U)return (left!=0U)?330U:125U;
  if(level==2U)return (left!=0U)?405U:175U;
  return (left!=0U)?300U:100U;
}
static inline uint16_t M8S142_RX(uint16_t minimum,uint16_t maximum,uint16_t amount)
{
  if(maximum<minimum)maximum=minimum;
  if(amount>117U)amount=117U;
  return (uint16_t)(minimum+((uint32_t)(maximum-minimum)*amount)/117U);
}
static inline uint16_t M8S142_Rise(uint16_t target,uint16_t previous,uint16_t old_cap)
{
  if(previous<old_cap)previous=old_cap;
  if(target>(uint32_t)previous+12U)target=(uint16_t)(previous+12U);
  return target;
}
#endif
