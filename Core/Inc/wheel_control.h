#ifndef WHEEL_CONTROL_H
#define WHEEL_CONTROL_H

#include "motor.h"
#include "pid.h"
#include <stdint.h>

typedef enum
{
  WHEEL_COMMAND_PROFILE_FIXED = 0,
  WHEEL_COMMAND_PROFILE_MANUAL = 1
} WheelCommandProfile_t;

typedef struct
{
  int16_t target_command;
  int16_t ramp_command;

  float raw_target_rpm;
  float command_target_rpm;
  float target_rpm;
  uint8_t command_profile;
  uint8_t soft_landing_active;
  uint8_t zero_soft_landing_active;
  float zero_ff_taper_scale;
  float target_accel_rpm_per_s;
  float target_decel_rpm_per_s;
  float target_zero_rpm_per_s;

  /* Per-wheel feedforward output. feedforward_pwm is legacy average. */
  float feedforward_pwm;
  float feedforward_pwm_lu;
  float feedforward_pwm_ld;
  float feedforward_pwm_ru;
  float feedforward_pwm_rd;

  float rpm_lu;
  float rpm_ld;
  float rpm_ru;
  float rpm_rd;

  int16_t pwm_lu;
  int16_t pwm_ld;
  int16_t pwm_ru;
  int16_t pwm_rd;

  uint8_t pid_enable;
  uint8_t pid_active;
  uint8_t reverse_wait_active;

  uint32_t pid_update_count;
  uint32_t reverse_count;

  float tune_kp;
  float tune_ki;
  float tune_kd;
  float tune_integral_limit;

  /* Legacy average + per-wheel feedforward parameters. */
  float ff_pwm_per_rpm;
  float ff_k_lu;
  float ff_k_ld;
  float ff_k_ru;
  float ff_k_rd;
  float ff_offset_lu;
  float ff_offset_ld;
  float ff_offset_ru;
  float ff_offset_rd;

} WheelControlDebug_t;

/*
 * Các biến public này chủ yếu phục vụ Live Expressions/tuning.
 * State nội bộ còn lại được giữ private trong wheel_control.c.
 */
extern volatile int16_t wheel_target_pwm;
extern volatile int16_t wheel_current_pwm;
extern volatile uint8_t wheel_reverse_wait_active;
extern volatile uint32_t wheel_reverse_count;

extern volatile uint8_t wheel_pid_enable;
extern volatile uint8_t wheel_pid_active;

/* Runtime state used by the autonomous/manual validation logger. */
extern volatile uint8_t wheel_pid_integral_hold_active;
extern volatile float wheel_ff_accel_k;
extern volatile float wheel_ff_accel_pwm;
extern volatile uint8_t wheel_sync_enable;
extern volatile uint8_t wheel_sync_active;
extern volatile float wheel_sync_k_pwm_per_rpm;
extern volatile float wheel_sync_max_pwm;
extern volatile float wheel_sync_spread_enter_rpm;
extern volatile float wheel_sync_spread_exit_rpm;
extern volatile float wheel_sync_spread_rpm;
extern volatile float wheel_sync_ref_rpm;
extern volatile float wheel_sync_error_lu_rpm;
extern volatile float wheel_sync_error_ld_rpm;
extern volatile float wheel_sync_error_ru_rpm;
extern volatile float wheel_sync_error_rd_rpm;
extern volatile float wheel_sync_pwm_lu;
extern volatile float wheel_sync_pwm_ld;
extern volatile float wheel_sync_pwm_ru;
extern volatile float wheel_sync_pwm_rd;

/* Auto feedforward calibration test.
 * Đặt wheel_ff_test_start = 1 khi xe đứng yên.
 * Profile mặc định: 30 -> 50 -> 70 -> 85 rpm, mỗi mức 4 s.
 * Logger được main.c tự start/stop.
 *
 * status: 0 idle, 1 running, 2 done, 3 rejected-not-stopped, 4 aborted.
 */
extern volatile uint8_t wheel_ff_test_start;
extern volatile uint8_t wheel_ff_test_active;
extern volatile uint8_t wheel_ff_test_done;
extern volatile uint8_t wheel_ff_test_status;
extern volatile uint8_t wheel_ff_test_stage;
extern volatile uint8_t wheel_ff_test_level;

extern volatile float wheel_ff_test_rpm_1;
extern volatile float wheel_ff_test_rpm_2;
extern volatile float wheel_ff_test_rpm_3;
extern volatile float wheel_ff_test_rpm_4;

extern volatile uint32_t wheel_ff_test_prep_ms;
extern volatile uint32_t wheel_ff_test_hold_ms;
extern volatile uint32_t wheel_ff_test_final_ms;

extern volatile float wheel_ff_test_current_command_rpm;
extern volatile uint32_t wheel_ff_test_stage_elapsed_ms;
extern volatile uint32_t wheel_ff_test_elapsed_ms;
extern volatile uint32_t wheel_ff_test_run_count;

extern volatile uint8_t wheel_ff_test_log_start_request;
extern volatile uint8_t wheel_ff_test_log_stop_request;

/* Command shaping trong miền RPM.
 * raw_target_rpm là target trực tiếp từ joystick/D-pad/Voice.
 * wheel_command_target_rpm là target sau hysteresis manual, trước slew.
 * wheel_pid_target_rpm là target đã làm mượt và đưa vào PID.
 *
 * MANUAL: PS2 analog LY.
 * FIXED:  D-pad Up/Down, Voice và autonomous fixed-speed.
 * Các biến manual/fixed là tuning độc lập; wheel_target_* là rate đang active.
 */
extern volatile float wheel_target_raw_rpm;
extern volatile float wheel_command_target_rpm;
extern volatile float wheel_pid_target_rpm;
extern volatile uint8_t wheel_manual_soft_landing_active;
extern volatile uint8_t wheel_manual_zero_soft_landing_active;
extern volatile float wheel_manual_zero_ff_taper_scale;
extern volatile WheelCommandProfile_t wheel_command_profile;

extern volatile float wheel_manual_accel_rpm_per_s;
extern volatile float wheel_manual_decel_rpm_per_s;
extern volatile float wheel_manual_zero_rpm_per_s;
extern volatile float wheel_manual_soft_landing_window_rpm;
extern volatile float wheel_manual_soft_landing_min_rate_rpm_per_s;
extern volatile float wheel_manual_zero_soft_landing_window_rpm;
extern volatile float wheel_manual_zero_soft_landing_min_rate_rpm_per_s;
extern volatile float wheel_manual_target_hysteresis_rpm;

extern volatile float wheel_fixed_accel_rpm_per_s;
extern volatile float wheel_fixed_decel_rpm_per_s;
extern volatile float wheel_fixed_zero_rpm_per_s;

extern volatile float wheel_target_accel_rpm_per_s;
extern volatile float wheel_target_decel_rpm_per_s;
extern volatile float wheel_target_zero_rpm_per_s;

/* Per-wheel feedforward tuning.
 * PWMff = sign(target) * (Kff * |target_rpm| + offset).
 * Chỉnh khi xe đang dừng. wheel_ff_pwm_per_rpm là legacy average/debug.
 */
extern volatile float wheel_ff_k_lu;
extern volatile float wheel_ff_k_ld;
extern volatile float wheel_ff_k_ru;
extern volatile float wheel_ff_k_rd;

extern volatile float wheel_ff_offset_lu;
extern volatile float wheel_ff_offset_ld;
extern volatile float wheel_ff_offset_ru;
extern volatile float wheel_ff_offset_rd;
extern volatile float wheel_ff_offset_rd_reverse;

extern volatile float wheel_ff_pwm_per_rpm;

extern volatile int16_t wheel_pwm_lu;
extern volatile int16_t wheel_pwm_ld;
extern volatile int16_t wheel_pwm_ru;
extern volatile int16_t wheel_pwm_rd;

extern volatile uint32_t wheel_pid_update_count;
extern volatile uint32_t wheel_pid_last_sample_tick;

/* Shared tuning controls for Live Expressions.
 * Edit kp/ki/kd/integral_limit, then set apply_tuning = 1.
 * The request auto-clears after all wheel PID objects are updated/reset.
 */
extern volatile float wheel_pid_tune_kp;
extern volatile float wheel_pid_tune_ki;
extern volatile float wheel_pid_tune_kd;
extern volatile float wheel_pid_tune_integral_limit;
extern volatile uint8_t wheel_pid_apply_tuning;
extern volatile uint32_t wheel_pid_tuning_apply_count;

/* Steady-state anti-hunt / HOLD.
 * Khi target đã hết ramp và error nhỏ, controller chuyển sang FF + I-hold
 * để PWM gần như phẳng như open-loop. Nếu error tăng đủ lớn, full PI tự thức lại.
 */
extern volatile uint8_t wheel_pid_hunt_enable;
extern volatile float wheel_pid_hunt_enter_error_rpm;
extern volatile float wheel_pid_hunt_exit_error_rpm;
extern volatile uint8_t wheel_pid_hunt_enter_samples;
extern volatile uint8_t wheel_pid_hunt_exit_samples;
extern volatile float wheel_pid_hunt_i_deadband_rpm;
extern volatile float wheel_pid_hunt_ki_scale;
extern volatile uint8_t wheel_pid_hunt_active_lu;
extern volatile uint8_t wheel_pid_hunt_active_ld;
extern volatile uint8_t wheel_pid_hunt_active_ru;
extern volatile uint8_t wheel_pid_hunt_active_rd;
extern volatile uint32_t wheel_pid_hunt_entry_count;

/* A1: bounded adaptive I in FIXED plateaus, outside steady-hold.
 * wheel_plateau_adapt_i_mask: LU=1, LD=2, RU=4, RD=8. */
extern volatile uint8_t wheel_plateau_adapt_i_enable;
extern volatile uint8_t wheel_plateau_adapt_i_wheels;
extern volatile float wheel_plateau_adapt_i_ki;
extern volatile float wheel_plateau_adapt_i_min_error_rpm;
extern volatile float wheel_plateau_adapt_i_max_step_pwm;
extern volatile uint8_t wheel_plateau_adapt_i_mask;
extern volatile uint32_t wheel_plateau_adapt_i_sample_count;

/* RU FIXED-plateau capture assist. */
extern volatile uint8_t wheel_ru_capture_assist_enable;
extern volatile float wheel_ru_capture_assist_k_pwm_per_rpm;
extern volatile float wheel_ru_capture_assist_max_pwm;
extern volatile uint32_t wheel_ru_capture_assist_timeout_ms;
extern volatile uint8_t wheel_ru_capture_assist_active;
extern volatile float wheel_ru_capture_assist_pwm;
extern volatile uint32_t wheel_ru_capture_assist_entry_count;
extern volatile uint32_t wheel_ru_capture_assist_timeout_count;

extern volatile uint8_t wheel_encoder_available_lu;
extern volatile uint8_t wheel_encoder_available_ld;
extern volatile uint8_t wheel_encoder_available_ru;
extern volatile uint8_t wheel_encoder_available_rd;

extern volatile PID_Controller_t wheel_pid_lu;
extern volatile PID_Controller_t wheel_pid_ld;
extern volatile PID_Controller_t wheel_pid_ru;
extern volatile PID_Controller_t wheel_pid_rd;

extern volatile WheelControlDebug_t wheel_ctrl_debug;

void WheelControl_Init(Motor_HandleTypeDef *left_up,
                       Motor_HandleTypeDef *right_up,
                       Motor_HandleTypeDef *left_down,
                       Motor_HandleTypeDef *right_down,
                       int16_t pwm_max);

void WheelControl_SetCommandProfile(WheelCommandProfile_t profile);
void WheelControl_SetTargetPWM(int16_t target_pwm);
void WheelControl_EmergencyStop(void);
void WheelControl_Update(void);

#endif /* WHEEL_CONTROL_H */
