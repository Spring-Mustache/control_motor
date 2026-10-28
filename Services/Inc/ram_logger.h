/* Logger from Adaptive Plateau-I v1 baseline 49c4699.
 * Keep ABI, symbol names and CSV schema stable during architecture refactor.
 */
#ifndef RAM_LOGGER_H
#define RAM_LOGGER_H

#include <stdint.h>
#include "motor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WHEEL_LOG_PERIOD_MS                  20U
#define WHEEL_LOG_MAX_SAMPLES                6000U
#define WHEEL_LOG_CSV_SCALE                   1000.0f
#define WHEEL_LOG_CSV_LINE_MAX                512U
#define WHEEL_LOG_BUTTON_DEBOUNCE_MS           250U
#define WHEEL_LOG_USER_EDGE_DEBOUNCE_MS         40U
#define WHEEL_LOG_USER_LONG_PRESS_MS          3000U

/* Original 72-byte sample; 6000 entries (432000 bytes) in RAM. */
typedef struct
{
  uint32_t time_ms;

  /* PS2 / command path. */
  uint8_t ps2_ly;
  uint8_t control_mode;
  uint8_t integral_hold;
  uint8_t sync_active;
  uint8_t hunt_lu;
  uint8_t hunt_ld;
  uint8_t hunt_ru;
  uint8_t hunt_rd;
  uint8_t capture_ru_active;
  uint8_t plateau_adapt_i_mask; /* previously 1 byte of struct padding */

  int16_t target_pwm;
  uint8_t command_profile;
  uint8_t soft_landing_active;

  /* Target chain: raw -> manual hysteresis/accepted -> slew -> PID. */
  float raw_target_rpm;
  float command_target_rpm;
  float pid_target_rpm;

  float lu_rpm;
  float ld_rpm;
  float ru_rpm;
  float rd_rpm;

  float sync_spread_rpm;
  float ff_accel_pwm;
  float capture_ru_pwm;

  int16_t lu_pwm;
  int16_t ld_pwm;
  int16_t ru_pwm;
  int16_t rd_pwm;
  uint16_t zero_soft_landing_active;
  uint16_t zero_ff_taper_x1000;

} WheelLogSample_t;

/* Keep existing Live Expressions and external test scripts working. */
extern volatile WheelLogSample_t wheel_log[WHEEL_LOG_MAX_SAMPLES];
extern volatile uint8_t wheel_log_start;
extern volatile uint8_t wheel_log_stop;
extern volatile uint8_t wheel_log_active;
extern volatile uint8_t wheel_log_done;
extern volatile uint32_t wheel_log_count;
extern volatile uint32_t wheel_log_start_tick;
extern volatile uint32_t wheel_log_last_tick;
extern volatile uint8_t wheel_log_dump_request;
extern volatile uint8_t wheel_log_dump_active;
extern volatile uint8_t wheel_log_dump_done;
extern volatile uint8_t wheel_log_dump_status;
extern volatile uint32_t wheel_log_dump_lines;
extern volatile uint8_t wheel_log_button_press_request;
extern volatile uint32_t wheel_log_button_press_count;
extern volatile uint32_t wheel_log_button_last_tick;
extern volatile uint32_t wheel_log_user_clear_count;
extern volatile uint8_t wheel_log_user_clear_blocked;
extern volatile uint8_t wheel_log_m8_s00_ready;
extern volatile uint8_t wheel_log_m8_s00_button_dump_request;
extern volatile uint8_t wheel_log_force_pid_on_start;
extern volatile float wheel_log_kp_lu_snapshot;
extern volatile float wheel_log_ki_lu_snapshot;
extern volatile float wheel_log_kd_lu_snapshot;
extern volatile float wheel_log_kp_ld_snapshot;
extern volatile float wheel_log_ki_ld_snapshot;
extern volatile float wheel_log_kd_ld_snapshot;
extern volatile float wheel_log_kp_ru_snapshot;
extern volatile float wheel_log_ki_ru_snapshot;
extern volatile float wheel_log_kd_ru_snapshot;
extern volatile float wheel_log_kp_rd_snapshot;
extern volatile float wheel_log_ki_rd_snapshot;
extern volatile float wheel_log_kd_rd_snapshot;
extern volatile float wheel_log_ff_snapshot;
extern volatile float wheel_log_ff_k_lu_snapshot;
extern volatile float wheel_log_ff_k_ld_snapshot;
extern volatile float wheel_log_ff_k_ru_snapshot;
extern volatile float wheel_log_ff_k_rd_snapshot;
extern volatile float wheel_log_ff_offset_lu_snapshot;
extern volatile float wheel_log_ff_offset_ld_snapshot;
extern volatile float wheel_log_ff_offset_ru_snapshot;
extern volatile float wheel_log_ff_offset_rd_snapshot;
extern volatile float wheel_log_ff_offset_rd_reverse_snapshot;
extern volatile float wheel_log_ka_snapshot;
extern volatile float wheel_log_sync_k_snapshot;
extern volatile float wheel_log_sync_max_snapshot;
extern volatile float wheel_log_sync_enter_snapshot;
extern volatile float wheel_log_sync_exit_snapshot;
extern volatile float wheel_log_slew_accel_snapshot;
extern volatile float wheel_log_slew_decel_snapshot;
extern volatile float wheel_log_slew_zero_snapshot;
extern volatile uint8_t wheel_log_profile_snapshot;
extern volatile float wheel_log_manual_accel_snapshot;
extern volatile float wheel_log_manual_decel_snapshot;
extern volatile float wheel_log_manual_zero_snapshot;
extern volatile float wheel_log_manual_soft_window_snapshot;
extern volatile float wheel_log_manual_soft_min_rate_snapshot;
extern volatile float wheel_log_manual_zero_soft_window_snapshot;
extern volatile float wheel_log_manual_zero_soft_min_rate_snapshot;
extern volatile float wheel_log_manual_hysteresis_snapshot;
extern volatile float wheel_log_fixed_accel_snapshot;
extern volatile float wheel_log_fixed_decel_snapshot;
extern volatile float wheel_log_fixed_zero_snapshot;

/* Init only binds peripheral and wheel references; does NOT clear RAM log. */
void RamLogger_Init(UART_HandleTypeDef *uart3,
                    Motor_HandleTypeDef *motor_lu,
                    Motor_HandleTypeDef *motor_ld,
                    Motor_HandleTypeDef *motor_ru,
                    Motor_HandleTypeDef *motor_rd);

extern volatile uint32_t wheel_log_boot_reset_flags;
extern volatile uint8_t wheel_log_recovered;
extern volatile uint8_t wheel_log_recovery_status;
void RamLogger_BootRecover(uint32_t reset_flags);
void RamLogger_OnUserButtonInterrupt(uint32_t tick_ms);
void RamLogger_ButtonUpdate(void);
void RamLogger_Update(uint8_t ps2_ly, uint8_t mode_value);
void RamLogger_DumpCsvUpdate(void);

#ifdef __cplusplus
}
#endif
#endif /* RAM_LOGGER_H */
