#ifndef STEER_S151_TRACE_H
#define STEER_S151_TRACE_H
#include <stdint.h>
/* Planned RIGHT Level2 ceiling 300 CCR is NEVER used for actuation. */
#define M8S151_RIGHT_PROPOSED_CAP 300U
#define M8S151_RIGHT_ACTIVE_CAP 200U
#define M8S151_MAX_ROWS 96U
#define M8S151_PERIOD_MS 50U
static inline int16_t M8S151_SignedDelta(uint16_t now,uint16_t before)
{ return (int16_t)(uint16_t)(now-before); }
static inline uint8_t M8S151_DeltaValid(uint32_t dt,uint8_t enabled)
{ return (uint8_t)(enabled!=0U && dt>=10U && dt<=250U); }
#endif
