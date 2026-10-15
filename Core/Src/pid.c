#include "pid.h"
#include <stddef.h>

static float PID_Clamp(float value, float min_value, float max_value)
{
  if (value > max_value)
  {
    return max_value;
  }

  if (value < min_value)
  {
    return min_value;
  }

  return value;
}

void PID_Init(volatile PID_Controller_t *pid,
              float kp,
              float ki,
              float kd,
              float integral_limit)
{
  if (pid == NULL)
  {
    return;
  }

  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->integral_limit = (integral_limit >= 0.0f) ? integral_limit
                                                 : -integral_limit;

  PID_Reset(pid);
}

void PID_Reset(volatile PID_Controller_t *pid)
{
  if (pid == NULL)
  {
    return;
  }

  /* Không reset kp/ki/kd để cho phép tune hoặc Fuzzy-PID đổi gain online. */
  pid->error = 0.0f;
  pid->integral = 0.0f;
  pid->derivative = 0.0f;
  pid->prev_error = 0.0f;
  pid->output = 0.0f;
  pid->initialized = 0U;
}

float PID_Update(volatile PID_Controller_t *pid,
                 float setpoint,
                 float measurement,
                 float feedforward,
                 float dt_s,
                 float output_min,
                 float output_max)
{
  float error;
  float p_term;
  float d_term;
  float candidate_integral;
  float unsaturated;
  float saturated;

  if ((pid == NULL) ||
      (dt_s <= 0.0f) ||
      (output_min > output_max))
  {
    return 0.0f;
  }

  error = setpoint - measurement;
  pid->error = error;

  if (pid->initialized == 0U)
  {
    pid->prev_error = error;
    pid->derivative = 0.0f;
    pid->initialized = 1U;
  }
  else
  {
    pid->derivative = (error - pid->prev_error) / dt_s;
  }

  p_term = pid->kp * error;
  d_term = pid->kd * pid->derivative;

  candidate_integral =
      pid->integral + (pid->ki * error * dt_s);

  candidate_integral =
      PID_Clamp(candidate_integral,
                -pid->integral_limit,
                 pid->integral_limit);

  unsaturated =
      feedforward + p_term + candidate_integral + d_term;

  saturated = PID_Clamp(unsaturated, output_min, output_max);

  /*
   * Conditional-integration anti-windup:
   * - chưa bão hòa: nhận integral mới;
   * - bão hòa trên: chỉ tích phân nếu error kéo output xuống;
   * - bão hòa dưới: chỉ tích phân nếu error kéo output lên.
   */
  if (((unsaturated <= output_max) && (unsaturated >= output_min)) ||
      ((unsaturated > output_max) && (error < 0.0f)) ||
      ((unsaturated < output_min) && (error > 0.0f)))
  {
    pid->integral = candidate_integral;
  }

  unsaturated = feedforward + p_term + pid->integral + d_term;
  saturated = PID_Clamp(unsaturated, output_min, output_max);

  pid->output = saturated;
  pid->prev_error = error;

  return saturated;
}
