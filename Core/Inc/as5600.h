#ifndef AS5600_H
#define AS5600_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"

#define AS5600_I2C_ADDRESS_7BIT      0x36U
#define AS5600_I2C_ADDRESS_HAL       (AS5600_I2C_ADDRESS_7BIT << 1U)

#define AS5600_REG_STATUS            0x0BU
#define AS5600_REG_RAW_ANGLE_H       0x0CU

#define AS5600_STATUS_MD             (1U << 5)
#define AS5600_STATUS_ML             (1U << 4)
#define AS5600_STATUS_MH             (1U << 3)

#define AS5600_COUNTS_PER_REV        4096U
#define AS5600_HALF_COUNTS           2048

typedef struct
{
  I2C_HandleTypeDef *hi2c;
} AS5600_HandleTypeDef;

void AS5600_Init(AS5600_HandleTypeDef *sensor, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef AS5600_ReadRawAndStatus(AS5600_HandleTypeDef *sensor,
                                         uint16_t *raw_angle,
                                         uint8_t *status);
float AS5600_RawToDegrees(uint16_t raw_angle);
int16_t AS5600_ShortestDeltaCounts(uint16_t from_raw, uint16_t to_raw);
float AS5600_DeltaCountsToDegrees(int16_t delta_counts);

#ifdef __cplusplus
}
#endif

#endif /* AS5600_H */
