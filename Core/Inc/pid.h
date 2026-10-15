#ifndef PID_H
#define PID_H

#include <stdint.h>

typedef struct
{
  float kp;
  float ki;
  float kd;

  float error;
  float integral;
  float derivative;
  float prev_error;
  float output;

  float integral_limit;
  uint8_t initialized;

} PID_Controller_t;

void PID_Init(volatile PID_Controller_t *pid,
              float kp,
              float ki,
              float kd,
              float integral_limit);

void PID_Reset(volatile PID_Controller_t *pid);

float PID_Update(volatile PID_Controller_t *pid,
                 float setpoint,
                 float measurement,
                 float feedforward,
                 float dt_s,
                 float output_min,
                 float output_max);

#endif /* PID_H */
