#include "as5600.h"

#define AS5600_I2C_TIMEOUT_MS  2U

void AS5600_Init(AS5600_HandleTypeDef *sensor, I2C_HandleTypeDef *hi2c)
{
  if (sensor != NULL)
  {
    sensor->hi2c = hi2c;
  }
}

HAL_StatusTypeDef AS5600_ReadRawAndStatus(AS5600_HandleTypeDef *sensor,
                                         uint16_t *raw_angle,
                                         uint8_t *status)
{
  uint8_t data[3];
  HAL_StatusTypeDef result;

  if ((sensor == NULL) || (sensor->hi2c == NULL) ||
      (raw_angle == NULL) || (status == NULL))
  {
    return HAL_ERROR;
  }

  /*
   * STATUS (0x0B), RAW ANGLE high (0x0C) và low (0x0D)
   * nằm liên tiếp nên đọc trong một giao dịch I2C.
   */
  result = HAL_I2C_Mem_Read(sensor->hi2c,
                            AS5600_I2C_ADDRESS_HAL,
                            AS5600_REG_STATUS,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            sizeof(data),
                            AS5600_I2C_TIMEOUT_MS);

  if (result != HAL_OK)
  {
    return result;
  }

  *status = data[0];
  *raw_angle = (uint16_t)((((uint16_t)data[1] & 0x0FU) << 8U) |
                         (uint16_t)data[2]);

  return HAL_OK;
}

float AS5600_RawToDegrees(uint16_t raw_angle)
{
  return ((float)(raw_angle & 0x0FFFU) * 360.0f) /
         (float)AS5600_COUNTS_PER_REV;
}

int16_t AS5600_ShortestDeltaCounts(uint16_t from_raw, uint16_t to_raw)
{
  int32_t delta = (int32_t)(to_raw & 0x0FFFU) -
                  (int32_t)(from_raw & 0x0FFFU);

  if (delta > (AS5600_HALF_COUNTS - 1))
  {
    delta -= (int32_t)AS5600_COUNTS_PER_REV;
  }
  else if (delta < -AS5600_HALF_COUNTS)
  {
    delta += (int32_t)AS5600_COUNTS_PER_REV;
  }

  return (int16_t)delta;
}

float AS5600_DeltaCountsToDegrees(int16_t delta_counts)
{
  return ((float)delta_counts * 360.0f) /
         (float)AS5600_COUNTS_PER_REV;
}
