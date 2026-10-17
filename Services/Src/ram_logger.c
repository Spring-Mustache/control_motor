/* RAM wheel logger / CSV VCP dump.
 * Extracted from Core/Src/main.c at baseline 49c4699.
 * All sample fields, flags, header/data CSV formatting and button flow
 * intentionally retain their original behavior.
 */
#include "ram_logger.h"
#include "main.h"
#include "wheel_control.h"
#include <stdio.h>
#include <string.h>

/* Bound once after motor setup; not owners of these resources. */
static UART_HandleTypeDef *s_uart3;
static Motor_HandleTypeDef *s_motor_lu;
static Motor_HandleTypeDef *s_motor_ld;
static Motor_HandleTypeDef *s_motor_ru;
static Motor_HandleTypeDef *s_motor_rd;

void RamLogger_Init(UART_HandleTypeDef *uart3,
                    Motor_HandleTypeDef *motor_lu,
                    Motor_HandleTypeDef *motor_ld,
                    Motor_HandleTypeDef *motor_ru,
                    Motor_HandleTypeDef *motor_rd)
{
  s_uart3 = uart3;
  s_motor_lu = motor_lu;
  s_motor_ld = motor_ld;
  s_motor_ru = motor_ru;
  s_motor_rd = motor_rd;
}

/* EXTI-safe: post event only; button state machine runs in main loop. */
void RamLogger_OnUserButtonInterrupt(uint32_t tick_ms)
{
  if ((wheel_log_button_press_count == 0U) ||
      ((uint32_t)(tick_ms - wheel_log_button_last_tick) >=
       WHEEL_LOG_BUTTON_DEBOUNCE_MS))
  {
    wheel_log_button_last_tick = tick_ms;
    wheel_log_button_press_count++;
    wheel_log_button_press_request = 1U;
  }
}

/* ============================================================
 * WHEEL DATA LOGGER - AUTONOMOUS RAM BUFFER
 *
 * USER button workflow (không cần debugger trong lúc xe chạy):
 *   - Nhấn USER ngắn (thả trước 3 giây): START -> STOP -> DUMP.
 *   - Giữ USER đủ 3 giây khi xe đứng yên và log đã hoàn tất:
 *       xóa log, LD3 OFF, cho phép bài đo mới.
 *   - RESET: khởi tạo lại điều khiển, giữ log hợp lệ đã hoàn tất.
 *
 * Khi wheel_log_done=1, mọi yêu cầu START (kể cả debugger) bị từ chối.
 * Sau khi đã lưu CSV, dùng giữ USER 3 giây để mở khóa bài ghi mới.
 * Dữ liệu trong .noinit chỉ giữ được khi SRAM còn nguồn và không bị
 * debugger/programmer ghi đè; RESET khi còn nguồn không cố tình xóa log.
 *
 * Buffer 6000 mẫu x 20 ms ~= 120 s.
 * ============================================================
 */

/*
 * Buffer để volatile để compiler không loại bỏ dữ liệu chỉ vì
 * chương trình chính không đọc lại buffer này.
 */

/* Retained across warm reset only; invalid after loss of SRAM power.
 * 6000 x 72 bytes and metadata are placed in NOLOAD linker section.
 */
#define WC_LOG_MAGIC 0x4D365233UL
#define WC_LOG_VERSION 1UL
typedef struct {
  uint32_t magic, version, count, samples_crc, config_crc;
  float config[41];
  uint8_t profile, pad[3];
} WC_LogRetained_t;

__attribute__((section(".noinit.wheel_log"), aligned(8), used))
volatile WheelLogSample_t wheel_log[WHEEL_LOG_MAX_SAMPLES];
__attribute__((section(".noinit.wheel_log"), aligned(8), used))
static volatile WC_LogRetained_t wc_log_retained;

/* Visible in Live Expressions: 1=recovered; 2=power reset; 3=invalid. */
volatile uint32_t wheel_log_boot_reset_flags = 0U;
volatile uint8_t wheel_log_recovered = 0U;
volatile uint8_t wheel_log_recovery_status = 0U;

static uint32_t WC_LogCrcStep(uint32_t crc, uint8_t byte)
{
  uint32_t n;
  crc ^= byte;
  for (n = 0U; n < 8U; ++n)
    crc = (crc & 1U) ? ((crc >> 1U) ^ 0xEDB88320UL) : (crc >> 1U);
  return crc;
}

static uint32_t WC_LogCrcBuffer(const volatile uint8_t *data, uint32_t size)
{
  uint32_t crc = 0xFFFFFFFFUL, n;
  for (n = 0U; n < size; ++n) crc = WC_LogCrcStep(crc, data[n]);
  return ~crc;
}

static uint32_t WC_LogSamplesCrc(uint32_t count)
{
  return WC_LogCrcBuffer((const volatile uint8_t *)wheel_log,
                         count * (uint32_t)sizeof(WheelLogSample_t));
}

static uint32_t WC_LogConfigCrc(void)
{
  return WC_LogCrcBuffer((const volatile uint8_t *)&wc_log_retained.config[0],
                         (uint32_t)sizeof(wc_log_retained.config) + 4U);
}

static void WC_LogInvalidate(void) { wc_log_retained.magic = 0U; __DSB(); }

static void WC_LogCommit(void)
{
  uint32_t i = 0U;
  wc_log_retained.magic = 0U;
  wc_log_retained.version = WC_LOG_VERSION;
  wc_log_retained.count = wheel_log_count;
  wc_log_retained.config[i++] = wheel_log_kp_lu_snapshot;
  wc_log_retained.config[i++] = wheel_log_ki_lu_snapshot;
  wc_log_retained.config[i++] = wheel_log_kd_lu_snapshot;
  wc_log_retained.config[i++] = wheel_log_kp_ld_snapshot;
  wc_log_retained.config[i++] = wheel_log_ki_ld_snapshot;
  wc_log_retained.config[i++] = wheel_log_kd_ld_snapshot;
  wc_log_retained.config[i++] = wheel_log_kp_ru_snapshot;
  wc_log_retained.config[i++] = wheel_log_ki_ru_snapshot;
  wc_log_retained.config[i++] = wheel_log_kd_ru_snapshot;
  wc_log_retained.config[i++] = wheel_log_kp_rd_snapshot;
  wc_log_retained.config[i++] = wheel_log_ki_rd_snapshot;
  wc_log_retained.config[i++] = wheel_log_kd_rd_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_k_lu_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_k_ld_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_k_ru_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_k_rd_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_offset_lu_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_offset_ld_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_offset_ru_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_offset_rd_snapshot;
  wc_log_retained.config[i++] = wheel_log_ff_offset_rd_reverse_snapshot;
  wc_log_retained.config[i++] = wheel_log_ka_snapshot;
  wc_log_retained.config[i++] = wheel_log_sync_k_snapshot;
  wc_log_retained.config[i++] = wheel_log_sync_max_snapshot;
  wc_log_retained.config[i++] = wheel_log_sync_enter_snapshot;
  wc_log_retained.config[i++] = wheel_log_sync_exit_snapshot;
  wc_log_retained.config[i++] = wheel_log_slew_accel_snapshot;
  wc_log_retained.config[i++] = wheel_log_slew_decel_snapshot;
  wc_log_retained.config[i++] = wheel_log_slew_zero_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_accel_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_decel_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_zero_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_soft_window_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_soft_min_rate_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_zero_soft_window_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_zero_soft_min_rate_snapshot;
  wc_log_retained.config[i++] = wheel_log_manual_hysteresis_snapshot;
  wc_log_retained.config[i++] = wheel_log_fixed_accel_snapshot;
  wc_log_retained.config[i++] = wheel_log_fixed_decel_snapshot;
  wc_log_retained.config[i++] = wheel_log_fixed_zero_snapshot;
  wc_log_retained.profile = wheel_log_profile_snapshot;
  wc_log_retained.pad[0] = wc_log_retained.pad[1] = wc_log_retained.pad[2] = 0U;
  wc_log_retained.samples_crc = WC_LogSamplesCrc(wheel_log_count);
  wc_log_retained.config_crc = WC_LogConfigCrc();
  __DSB();
  wc_log_retained.magic = WC_LOG_MAGIC; /* commit marker must be last */
  __DSB();
}

void RamLogger_BootRecover(uint32_t reset_flags)
{
  uint32_t i = 0U;
  wheel_log_boot_reset_flags = reset_flags;
  wheel_log_recovered = 0U;
  wheel_log_recovery_status = 0U;
  if ((reset_flags & (RCC_CSR_PORRSTF | RCC_CSR_BORRSTF)) != 0U) {
    WC_LogInvalidate();
    wheel_log_recovery_status = 2U;
    return;
  }
  if ((wc_log_retained.magic != WC_LOG_MAGIC) ||
      (wc_log_retained.version != WC_LOG_VERSION) ||
      (wc_log_retained.count == 0U) ||
      (wc_log_retained.count > WHEEL_LOG_MAX_SAMPLES)) {
    wheel_log_recovery_status = 3U;
    return;
  }
  if ((wc_log_retained.samples_crc != WC_LogSamplesCrc(wc_log_retained.count)) ||
      (wc_log_retained.config_crc != WC_LogConfigCrc())) {
    WC_LogInvalidate();
    wheel_log_recovery_status = 3U;
    return;
  }
  wheel_log_count = wc_log_retained.count;
  wheel_log_kp_lu_snapshot = wc_log_retained.config[i++];
  wheel_log_ki_lu_snapshot = wc_log_retained.config[i++];
  wheel_log_kd_lu_snapshot = wc_log_retained.config[i++];
  wheel_log_kp_ld_snapshot = wc_log_retained.config[i++];
  wheel_log_ki_ld_snapshot = wc_log_retained.config[i++];
  wheel_log_kd_ld_snapshot = wc_log_retained.config[i++];
  wheel_log_kp_ru_snapshot = wc_log_retained.config[i++];
  wheel_log_ki_ru_snapshot = wc_log_retained.config[i++];
  wheel_log_kd_ru_snapshot = wc_log_retained.config[i++];
  wheel_log_kp_rd_snapshot = wc_log_retained.config[i++];
  wheel_log_ki_rd_snapshot = wc_log_retained.config[i++];
  wheel_log_kd_rd_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_k_lu_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_k_ld_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_k_ru_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_k_rd_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_offset_lu_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_offset_ld_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_offset_ru_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_offset_rd_snapshot = wc_log_retained.config[i++];
  wheel_log_ff_offset_rd_reverse_snapshot = wc_log_retained.config[i++];
  wheel_log_ka_snapshot = wc_log_retained.config[i++];
  wheel_log_sync_k_snapshot = wc_log_retained.config[i++];
  wheel_log_sync_max_snapshot = wc_log_retained.config[i++];
  wheel_log_sync_enter_snapshot = wc_log_retained.config[i++];
  wheel_log_sync_exit_snapshot = wc_log_retained.config[i++];
  wheel_log_slew_accel_snapshot = wc_log_retained.config[i++];
  wheel_log_slew_decel_snapshot = wc_log_retained.config[i++];
  wheel_log_slew_zero_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_accel_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_decel_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_zero_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_soft_window_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_soft_min_rate_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_zero_soft_window_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_zero_soft_min_rate_snapshot = wc_log_retained.config[i++];
  wheel_log_manual_hysteresis_snapshot = wc_log_retained.config[i++];
  wheel_log_fixed_accel_snapshot = wc_log_retained.config[i++];
  wheel_log_fixed_decel_snapshot = wc_log_retained.config[i++];
  wheel_log_fixed_zero_snapshot = wc_log_retained.config[i++];
  wheel_log_profile_snapshot = wc_log_retained.profile;
  wheel_log_active = 0U;
  wheel_log_done = 1U;
  wheel_log_recovered = 1U;
  wheel_log_recovery_status = 1U;
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
}


/* Request từ debugger. Logger tự reset request sau khi nhận. */
volatile uint8_t wheel_log_start = 0U;
volatile uint8_t wheel_log_stop = 0U;

/* Trạng thái logger. */
volatile uint8_t wheel_log_active = 0U;
volatile uint8_t wheel_log_done = 0U;

/* Số mẫu đã ghi hợp lệ trong wheel_log[]. */
volatile uint32_t wheel_log_count = 0U;


/* Debug thời gian logger. */
volatile uint32_t wheel_log_start_tick = 0U;
volatile uint32_t wheel_log_last_tick = 0U;

/* CSV dump through USART3 / ST-LINK Virtual COM Port.
 * Trigger only after vehicle is stopped and logger has finished.
 */
volatile uint8_t wheel_log_dump_request = 0U;
volatile uint8_t wheel_log_dump_active = 0U;
volatile uint8_t wheel_log_dump_done = 0U;
volatile uint8_t wheel_log_dump_status = 0U;
volatile uint32_t wheel_log_dump_lines = 0U;

/* USER button only posts an event from EXTI; all work is done in main loop. */
volatile uint8_t wheel_log_button_press_request = 0U;
volatile uint32_t wheel_log_button_press_count = 0U;
volatile uint32_t wheel_log_button_last_tick = 0U;
volatile uint32_t wheel_log_user_clear_count = 0U;
volatile uint8_t wheel_log_user_clear_blocked = 0U;

/* 1 = start logger bằng USER button sẽ bật closed-loop PID. */
volatile uint8_t wheel_log_force_pid_on_start = 1U;

/* Gain/feedforward snapshot taken exactly when a new log starts. */
volatile float wheel_log_kp_lu_snapshot = 0.0f;
volatile float wheel_log_ki_lu_snapshot = 0.0f;
volatile float wheel_log_kd_lu_snapshot = 0.0f;
volatile float wheel_log_kp_ld_snapshot = 0.0f;
volatile float wheel_log_ki_ld_snapshot = 0.0f;
volatile float wheel_log_kd_ld_snapshot = 0.0f;
volatile float wheel_log_kp_ru_snapshot = 0.0f;
volatile float wheel_log_ki_ru_snapshot = 0.0f;
volatile float wheel_log_kd_ru_snapshot = 0.0f;
volatile float wheel_log_kp_rd_snapshot = 0.0f;
volatile float wheel_log_ki_rd_snapshot = 0.0f;
volatile float wheel_log_kd_rd_snapshot = 0.0f;
volatile float wheel_log_ff_snapshot = 0.0f;
/* Per-wheel static FF snapshot for cruise/steady-state trim analysis. */
volatile float wheel_log_ff_k_lu_snapshot = 0.0f;
volatile float wheel_log_ff_k_ld_snapshot = 0.0f;
volatile float wheel_log_ff_k_ru_snapshot = 0.0f;
volatile float wheel_log_ff_k_rd_snapshot = 0.0f;
volatile float wheel_log_ff_offset_lu_snapshot = 0.0f;
volatile float wheel_log_ff_offset_ld_snapshot = 0.0f;
volatile float wheel_log_ff_offset_ru_snapshot = 0.0f;
volatile float wheel_log_ff_offset_rd_snapshot = 0.0f;
volatile float wheel_log_ff_offset_rd_reverse_snapshot = 0.0f;

volatile float wheel_log_ka_snapshot = 0.0f;
volatile float wheel_log_sync_k_snapshot = 0.0f;
volatile float wheel_log_sync_max_snapshot = 0.0f;
volatile float wheel_log_sync_enter_snapshot = 0.0f;
volatile float wheel_log_sync_exit_snapshot = 0.0f;
volatile float wheel_log_slew_accel_snapshot = 0.0f;
volatile float wheel_log_slew_decel_snapshot = 0.0f;
volatile float wheel_log_slew_zero_snapshot = 0.0f;
volatile uint8_t wheel_log_profile_snapshot = 0U;

volatile float wheel_log_manual_accel_snapshot = 0.0f;
volatile float wheel_log_manual_decel_snapshot = 0.0f;
volatile float wheel_log_manual_zero_snapshot = 0.0f;
volatile float wheel_log_manual_soft_window_snapshot = 0.0f;
volatile float wheel_log_manual_soft_min_rate_snapshot = 0.0f;
volatile float wheel_log_manual_zero_soft_window_snapshot = 0.0f;
volatile float wheel_log_manual_zero_soft_min_rate_snapshot = 0.0f;
volatile float wheel_log_manual_hysteresis_snapshot = 0.0f;
volatile float wheel_log_fixed_accel_snapshot = 0.0f;
volatile float wheel_log_fixed_decel_snapshot = 0.0f;
volatile float wheel_log_fixed_zero_snapshot = 0.0f;



/*
 * ============================================================
 * WHEEL DATA LOGGER UPDATE
 *
 * Gọi liên tục trong main loop.
 *
 * - Không blocking.
 * - Không dùng HAL_Delay riêng.
 * - Ghi 1 mẫu mỗi WHEEL_LOG_PERIOD_MS.
 * - Tự dừng khi buffer đầy.
 * ============================================================
 */
static int32_t Wheel_LogFloatToMilli(float value)
{
  float scaled = value * WHEEL_LOG_CSV_SCALE;

  if (scaled >= 0.0f)
  {
    return (int32_t)(scaled + 0.5f);
  }

  return (int32_t)(scaled - 0.5f);
}


static uint8_t Wheel_LogUartWrite(const char *text)
{
  uint16_t length;

  if (text == NULL)
  {
    return 0U;
  }

  length = (uint16_t)strlen(text);

  if (HAL_UART_Transmit(s_uart3,
                        (uint8_t *)text,
                        length,
                        1000U) != HAL_OK)
  {
    return 0U;
  }

  return 1U;
}


/*
 * Dump completed RAM log as integer-scaled CSV over USART3.
 * Float printf is intentionally avoided, so CubeIDE does not need
 * the -u _printf_float linker option. RPM/gains are sent x1000.
 *
 * Status:
 *   0 = idle / no request yet
 *   1 = dump complete
 *   2 = rejected: log is empty/not finished
 *   3 = rejected: vehicle command/PWM is not zero
 *   4 = UART transmit error
 */
void RamLogger_DumpCsvUpdate(void)
{
  char line[WHEEL_LOG_CSV_LINE_MAX];
  uint32_t i;
  int length;

  if (wheel_log_dump_request == 0U)
  {
    return;
  }

  wheel_log_dump_request = 0U;
  wheel_log_dump_done = 0U;
  wheel_log_dump_lines = 0U;

  if ((wheel_log_active != 0U) ||
      (wheel_log_done == 0U) ||
      (wheel_log_count == 0U))
  {
    wheel_log_dump_status = 2U;
    return;
  }

  /* Never block the control loop for a serial dump while the vehicle moves. */
  if ((wheel_ctrl_debug.target_command != 0) ||
      (wheel_ctrl_debug.ramp_command != 0) ||
      (wheel_pwm_lu != 0) ||
      (wheel_pwm_ld != 0) ||
      (wheel_pwm_ru != 0) ||
      (wheel_pwm_rd != 0))
  {
    wheel_log_dump_status = 3U;
    return;
  }

  wheel_log_dump_active = 1U;
  wheel_log_dump_status = 0U;
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);

  if (Wheel_LogUartWrite("#WHEEL_ANALOG_CSV_BEGIN\r\n") == 0U)
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#format,command_shaper_v5_zero_ff_taper_steady_hold_ru_capture_plateau_i_v1\r\n"
                    "#samples,%lu\r\n"
                    "#period_ms,%lu\r\n",
                    (unsigned long)wheel_log_count,
                    (unsigned long)WHEEL_LOG_PERIOD_MS);
  if ((length <= 0) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#LU_gains_x1000,%ld,%ld,%ld\r\n"
                    "#LD_gains_x1000,%ld,%ld,%ld\r\n"
                    "#RU_gains_x1000,%ld,%ld,%ld\r\n"
                    "#RD_gains_x1000,%ld,%ld,%ld\r\n",
                    (long)Wheel_LogFloatToMilli(wheel_log_kp_lu_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ki_lu_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kd_lu_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kp_ld_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ki_ld_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kd_ld_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kp_ru_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ki_ru_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kd_ru_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kp_rd_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ki_rd_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_kd_rd_snapshot));
  if ((length <= 0) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#FF_K_x1000,%ld,%ld,%ld,%ld\r\n"
                    "#FF_OFFSET_x1000,%ld,%ld,%ld,%ld\r\n"
                    "#FF_OFFSET_RD_REVERSE_x1000,%ld\r\n",
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_k_lu_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_k_ld_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_k_ru_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_k_rd_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_offset_lu_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_offset_ld_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_offset_ru_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_offset_rd_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_ff_offset_rd_reverse_snapshot));
  if ((length <= 0) ||
      ((size_t)length >= sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#Ka_x1000,%ld\r\n"
                    "#sync_x1000,%ld,%ld,%ld,%ld\r\n"
                    "#profile_at_start,%u\r\n"
                    "#active_slew_at_start_x1000,%ld,%ld,%ld\r\n"
                    "#manual_slew_x1000,%ld,%ld,%ld\r\n"
                    "#manual_softlanding_x1000,%ld,%ld,%ld\r\n"
                    "#manual_zero_softlanding_x1000,%ld,%ld\r\n"
                    "#manual_zero_ff_taper,offset_only,uses_zero_soft_window\r\n"
                    "#fixed_slew_x1000,%ld,%ld,%ld\r\n",
                    (long)Wheel_LogFloatToMilli(wheel_log_ka_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_sync_k_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_sync_max_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_sync_enter_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_sync_exit_snapshot),
                    (unsigned int)wheel_log_profile_snapshot,
                    (long)Wheel_LogFloatToMilli(wheel_log_slew_accel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_slew_decel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_slew_zero_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_accel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_decel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_zero_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_soft_window_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_soft_min_rate_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_hysteresis_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_zero_soft_window_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_manual_zero_soft_min_rate_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_fixed_accel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_fixed_decel_snapshot),
                    (long)Wheel_LogFloatToMilli(wheel_log_fixed_zero_snapshot));
  if ((length <= 0) ||
      ((size_t)length >= sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#encoder_available_lu,%u\r\n"
                    "#encoder_cpr_lu_x1000,%ld\r\n",
                    (unsigned int)wheel_encoder_available_lu,
                    (long)Wheel_LogFloatToMilli(s_motor_lu->counts_per_revolution));
  if ((length <= 0) ||
      ((uint32_t)length >= (uint32_t)sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#steady_hold,%u,%ld,%ld,%u,%u,%ld,%ld\r\n",
                    (unsigned int)wheel_pid_hunt_enable,
                    (long)Wheel_LogFloatToMilli(wheel_pid_hunt_enter_error_rpm),
                    (long)Wheel_LogFloatToMilli(wheel_pid_hunt_exit_error_rpm),
                    (unsigned int)wheel_pid_hunt_enter_samples,
                    (unsigned int)wheel_pid_hunt_exit_samples,
                    (long)Wheel_LogFloatToMilli(wheel_pid_hunt_i_deadband_rpm),
                    (long)Wheel_LogFloatToMilli(wheel_pid_hunt_ki_scale));
  if ((length <= 0) ||
      ((uint32_t)length >= (uint32_t)sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#ru_capture_assist,%u,%ld,%ld,%lu,%lu,%lu\r\n",
                    (unsigned int)wheel_ru_capture_assist_enable,
                    (long)Wheel_LogFloatToMilli(wheel_ru_capture_assist_k_pwm_per_rpm),
                    (long)Wheel_LogFloatToMilli(wheel_ru_capture_assist_max_pwm),
                    (unsigned long)wheel_ru_capture_assist_timeout_ms,
                    (unsigned long)wheel_ru_capture_assist_entry_count,
                    (unsigned long)wheel_ru_capture_assist_timeout_count);
  if ((length <= 0) ||
      ((uint32_t)length >= (uint32_t)sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  length = snprintf(line, sizeof(line),
                    "#plateau_adapt_i,%u,%u,%ld,%ld,%ld,%lu\r\n",
                    (unsigned int)wheel_plateau_adapt_i_enable,
                    (unsigned int)wheel_plateau_adapt_i_wheels,
                    (long)Wheel_LogFloatToMilli(wheel_plateau_adapt_i_ki),
                    (long)Wheel_LogFloatToMilli(wheel_plateau_adapt_i_min_error_rpm),
                    (long)Wheel_LogFloatToMilli(wheel_plateau_adapt_i_max_step_pwm),
                    (unsigned long)wheel_plateau_adapt_i_sample_count);
  if ((length <= 0) ||
      ((uint32_t)length >= (uint32_t)sizeof(line)) ||
      (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                         (uint16_t)length, 1000U) != HAL_OK))
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  if (Wheel_LogUartWrite(
      "time_ms,ps2_ly,control_mode,command_profile,target_pwm,"
      "raw_target_mrpm,command_target_mrpm,pid_target_mrpm,"
      "lu_mrpm,ld_mrpm,ru_mrpm,rd_mrpm,"
      "lu_pwm,ld_pwm,ru_pwm,rd_pwm,"
      "integral_hold,soft_landing_active,zero_soft_landing_active,zero_ff_taper_x1000,sync_active,hunt_lu,hunt_ld,hunt_ru,hunt_rd,capture_ru_active,plateau_adapt_i_mask,sync_spread_mrpm,ff_accel_pwm_x1000,capture_ru_pwm_x1000,"
      "lu_err_mrpm,ld_err_mrpm,ru_err_mrpm,rd_err_mrpm\r\n") == 0U)
  {
    wheel_log_dump_status = 4U;
    wheel_log_dump_active = 0U;
    HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
    return;
  }

  for (i = 0U; i < wheel_log_count; i++)
  {
    int32_t raw_target =
        Wheel_LogFloatToMilli(wheel_log[i].raw_target_rpm);
    int32_t command_target =
        Wheel_LogFloatToMilli(wheel_log[i].command_target_rpm);
    int32_t pid_target =
        Wheel_LogFloatToMilli(wheel_log[i].pid_target_rpm);
    int32_t lu = Wheel_LogFloatToMilli(wheel_log[i].lu_rpm);
    int32_t ld = Wheel_LogFloatToMilli(wheel_log[i].ld_rpm);
    int32_t ru = Wheel_LogFloatToMilli(wheel_log[i].ru_rpm);
    int32_t rd = Wheel_LogFloatToMilli(wheel_log[i].rd_rpm);
    int32_t sync_spread =
        Wheel_LogFloatToMilli(wheel_log[i].sync_spread_rpm);
    int32_t ff_accel =
        Wheel_LogFloatToMilli(wheel_log[i].ff_accel_pwm);
    int32_t capture_ru_pwm =
        Wheel_LogFloatToMilli(wheel_log[i].capture_ru_pwm);

    length = snprintf(
        line, sizeof(line),
        "%lu,%u,%u,%u,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
        (unsigned long)wheel_log[i].time_ms,
        (unsigned int)wheel_log[i].ps2_ly,
        (unsigned int)wheel_log[i].control_mode,
        (unsigned int)wheel_log[i].command_profile,
        (int)wheel_log[i].target_pwm,
        (long)raw_target,
        (long)command_target,
        (long)pid_target,
        (long)lu,
        (long)ld,
        (long)ru,
        (long)rd,
        (int)wheel_log[i].lu_pwm,
        (int)wheel_log[i].ld_pwm,
        (int)wheel_log[i].ru_pwm,
        (int)wheel_log[i].rd_pwm,
        (unsigned int)wheel_log[i].integral_hold,
        (unsigned int)wheel_log[i].soft_landing_active,
        (unsigned int)wheel_log[i].zero_soft_landing_active,
        (unsigned int)wheel_log[i].zero_ff_taper_x1000,
        (unsigned int)wheel_log[i].sync_active,
        (unsigned int)wheel_log[i].hunt_lu,
        (unsigned int)wheel_log[i].hunt_ld,
        (unsigned int)wheel_log[i].hunt_ru,
        (unsigned int)wheel_log[i].hunt_rd,
        (unsigned int)wheel_log[i].capture_ru_active,
        (unsigned int)wheel_log[i].plateau_adapt_i_mask,
        (long)sync_spread,
        (long)ff_accel,
        (long)capture_ru_pwm,
        (long)(pid_target - lu),
        (long)(pid_target - ld),
        (long)(pid_target - ru),
        (long)(pid_target - rd));

    if ((length <= 0) ||
        ((uint32_t)length >= (uint32_t)sizeof(line)) ||
        (HAL_UART_Transmit(s_uart3, (uint8_t *)line,
                           (uint16_t)length, 1000U) != HAL_OK))
    {
      wheel_log_dump_status = 4U;
      wheel_log_dump_active = 0U;
      HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
      return;
    }

    wheel_log_dump_lines = i + 1U;
  }

  (void)Wheel_LogUartWrite("#WHEEL_ANALOG_CSV_END\r\n");

  wheel_log_dump_status = 1U;
  wheel_log_dump_done = 1U;
  wheel_log_dump_active = 0U;
  HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
}



/*
 * USER short press is dispatched on debounced RELEASE, not on EXTI rising,
 * so holding for 3 s never accidentally queues a CSV dump or starts a log.
 * PC13 is HIGH when the NUCLEO USER button is pressed.
 */
static uint8_t WC_LogVehicleStopped(void)
{
  if ((wheel_ctrl_debug.target_command != 0) ||
      (wheel_ctrl_debug.ramp_command != 0) ||
      (wheel_pwm_lu != 0) || (wheel_pwm_ld != 0) ||
      (wheel_pwm_ru != 0) || (wheel_pwm_rd != 0))
  {
    return 0U;
  }

  /* Guard residual motion, even with zero requested PWM. */
  if (((s_motor_lu != NULL) &&
       ((s_motor_lu->current_rpm > 2.0f) || (s_motor_lu->current_rpm < -2.0f))) ||
      ((s_motor_ld != NULL) &&
       ((s_motor_ld->current_rpm > 2.0f) || (s_motor_ld->current_rpm < -2.0f))) ||
      ((s_motor_ru != NULL) &&
       ((s_motor_ru->current_rpm > 2.0f) || (s_motor_ru->current_rpm < -2.0f))) ||
      ((s_motor_rd != NULL) &&
       ((s_motor_rd->current_rpm > 2.0f) || (s_motor_rd->current_rpm < -2.0f))))
  {
    return 0U;
  }
  return 1U;
}

static void WC_LogHandleShortUserPress(void)
{
  if (wheel_ff_test_active != 0U)
  {
    return;
  }
  if (wheel_log_active != 0U)
  {
    wheel_log_stop = 1U;
    return;
  }
  if (wheel_log_done != 0U)
  {
    wheel_log_dump_request = 1U;
    return;
  }

  if ((wheel_log_dump_active != 0U) || (WC_LogVehicleStopped() == 0U))
  {
    return; /* Do not start logger from an unsafe state. */
  }
  if (wheel_log_force_pid_on_start != 0U)
  {
    wheel_pid_enable = 1U;
  }
  wheel_log_start = 1U;
}

/* Only USER >=3s can discard a completed log. No motor or UART output
 * is interrupted; a rejected long hold is consumed, never a short press.
 */
static void WC_LogHandleLongUserPress(void)
{
  if ((wheel_log_done == 0U) || (wheel_log_active != 0U) ||
      (wheel_log_dump_active != 0U) ||
      (wheel_log_dump_request != 0U) ||
      (wheel_ff_test_active != 0U) ||
      (WC_LogVehicleStopped() == 0U))
  {
    wheel_log_user_clear_blocked = 1U;
    return;
  }

  WC_LogInvalidate();
  wheel_log_start = 0U;
  wheel_log_stop = 0U;
  wheel_log_active = 0U;
  wheel_log_done = 0U;
  wheel_log_count = 0U;
  wheel_log_start_tick = 0U;
  wheel_log_last_tick = 0U;
  wheel_log_dump_request = 0U;
  wheel_log_dump_done = 0U;
  wheel_log_dump_status = 0U;
  wheel_log_dump_lines = 0U;
  wheel_log_recovered = 0U;
  wheel_log_recovery_status = 0U;
  wheel_log_user_clear_blocked = 0U;
  wheel_log_user_clear_count++;
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
}

void RamLogger_ButtonUpdate(void)
{
  static uint8_t last_raw = 0U;
  static uint8_t stable = 0U;
  static uint8_t long_processed = 0U;
  static uint32_t raw_changed_at = 0U;
  static uint32_t pressed_at = 0U;
  uint32_t now = HAL_GetTick();
  uint8_t raw = (HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin) ==
                 GPIO_PIN_SET) ? 1U : 0U;

  /* Existing EXTI callback remains for press count/time debug; polling
   * determines actual button duration and safely consumes its request.
   */
  wheel_log_button_press_request = 0U;

  if (raw != last_raw)
  {
    last_raw = raw;
    raw_changed_at = now;
  }

  if ((raw != stable) &&
      ((uint32_t)(now - raw_changed_at) >=
       WHEEL_LOG_USER_EDGE_DEBOUNCE_MS))
  {
    stable = raw;
    if (stable != 0U)
    {
      pressed_at = now;
      long_processed = 0U;
    }
    else if (long_processed == 0U)
    {
      /* If scheduler was delayed across the 3s boundary, preserve the
       * duration decision rather than treating a long hold as a tap.
       */
      if ((uint32_t)(now - pressed_at) >= WHEEL_LOG_USER_LONG_PRESS_MS)
      {
        WC_LogHandleLongUserPress();
      }
      else
      {
        WC_LogHandleShortUserPress();
      }
    }
  }

  if ((stable != 0U) && (raw != 0U) && (long_processed == 0U) &&
      ((uint32_t)(now - pressed_at) >= WHEEL_LOG_USER_LONG_PRESS_MS))
  {
    long_processed = 1U;
    WC_LogHandleLongUserPress();
  }
}

void RamLogger_Update(uint8_t ps2_ly, uint8_t mode_value)
{
  uint32_t now = HAL_GetTick();
  uint32_t index;

  /*
   * START request có ưu tiên trước.
   * Dù qua USER hay debugger, START không được ghi đè log hoàn tất.
   * Chỉ giữ USER 3 giây khi xe dừng mới xóa log trước bài tiếp theo.
   */
  if (wheel_log_start != 0U)
  {
    wheel_log_start = 0U;
    if ((wheel_log_done != 0U) || (wheel_log_active != 0U) ||
        (wheel_log_dump_active != 0U) || (wheel_log_dump_request != 0U))
    {
      return; /* never overwrite completed/ongoing data by START */
    }
    wheel_log_stop = 0U;

    WC_LogInvalidate();
    wheel_log_count = 0U;
    wheel_log_done = 0U;
    wheel_log_active = 1U;

    wheel_log_dump_done = 0U;
    wheel_log_dump_status = 0U;
    wheel_log_dump_lines = 0U;

    wheel_log_kp_lu_snapshot = wheel_pid_lu.kp;
    wheel_log_ki_lu_snapshot = wheel_pid_lu.ki;
    wheel_log_kd_lu_snapshot = wheel_pid_lu.kd;
    wheel_log_kp_ld_snapshot = wheel_pid_ld.kp;
    wheel_log_ki_ld_snapshot = wheel_pid_ld.ki;
    wheel_log_kd_ld_snapshot = wheel_pid_ld.kd;
    wheel_log_kp_ru_snapshot = wheel_pid_ru.kp;
    wheel_log_ki_ru_snapshot = wheel_pid_ru.ki;
    wheel_log_kd_ru_snapshot = wheel_pid_ru.kd;
    wheel_log_kp_rd_snapshot = wheel_pid_rd.kp;
    wheel_log_ki_rd_snapshot = wheel_pid_rd.ki;
    wheel_log_kd_rd_snapshot = wheel_pid_rd.kd;
    wheel_log_ff_snapshot = wheel_ff_pwm_per_rpm;
    wheel_log_ff_k_lu_snapshot = wheel_ff_k_lu;
    wheel_log_ff_k_ld_snapshot = wheel_ff_k_ld;
    wheel_log_ff_k_ru_snapshot = wheel_ff_k_ru;
    wheel_log_ff_k_rd_snapshot = wheel_ff_k_rd;
    wheel_log_ff_offset_lu_snapshot = wheel_ff_offset_lu;
    wheel_log_ff_offset_ld_snapshot = wheel_ff_offset_ld;
    wheel_log_ff_offset_ru_snapshot = wheel_ff_offset_ru;
    wheel_log_ff_offset_rd_snapshot = wheel_ff_offset_rd;
    wheel_log_ff_offset_rd_reverse_snapshot = wheel_ff_offset_rd_reverse;

    wheel_log_ka_snapshot = wheel_ff_accel_k;
    wheel_log_sync_k_snapshot = wheel_sync_k_pwm_per_rpm;
    wheel_log_sync_max_snapshot = wheel_sync_max_pwm;
    wheel_log_sync_enter_snapshot = wheel_sync_spread_enter_rpm;
    wheel_log_sync_exit_snapshot = wheel_sync_spread_exit_rpm;
    wheel_log_slew_accel_snapshot = wheel_target_accel_rpm_per_s;
    wheel_log_slew_decel_snapshot = wheel_target_decel_rpm_per_s;
    wheel_log_slew_zero_snapshot = wheel_target_zero_rpm_per_s;
    wheel_log_profile_snapshot = (uint8_t)wheel_command_profile;

    wheel_log_manual_accel_snapshot = wheel_manual_accel_rpm_per_s;
    wheel_log_manual_decel_snapshot = wheel_manual_decel_rpm_per_s;
    wheel_log_manual_zero_snapshot = wheel_manual_zero_rpm_per_s;
    wheel_log_manual_soft_window_snapshot = wheel_manual_soft_landing_window_rpm;
    wheel_log_manual_soft_min_rate_snapshot =
        wheel_manual_soft_landing_min_rate_rpm_per_s;
    wheel_log_manual_zero_soft_window_snapshot =
        wheel_manual_zero_soft_landing_window_rpm;
    wheel_log_manual_zero_soft_min_rate_snapshot =
        wheel_manual_zero_soft_landing_min_rate_rpm_per_s;
    wheel_log_manual_hysteresis_snapshot = wheel_manual_target_hysteresis_rpm;
    wheel_log_fixed_accel_snapshot = wheel_fixed_accel_rpm_per_s;
    wheel_log_fixed_decel_snapshot = wheel_fixed_decel_rpm_per_s;
    wheel_log_fixed_zero_snapshot = wheel_fixed_zero_rpm_per_s;

    wheel_log_start_tick = now;

    /*
     * Cho phép ghi mẫu đầu tiên ngay ở lần gọi kế tiếp,
     * không cần chờ thêm 20 ms.
     */
    wheel_log_last_tick = now - WHEEL_LOG_PERIOD_MS;

    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
  }

  /*
   * STOP request: dừng ghi nhưng giữ nguyên toàn bộ dữ liệu RAM.
   */
  if (wheel_log_stop != 0U)
  {
    wheel_log_stop = 0U;
    wheel_log_active = 0U;
    wheel_log_done = 1U;
    WC_LogCommit();
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return;
  }

  if (wheel_log_active == 0U)
  {
    return;
  }

  if ((uint32_t)(now - wheel_log_last_tick) < WHEEL_LOG_PERIOD_MS)
  {
    return;
  }

  wheel_log_last_tick = now;

  index = wheel_log_count;

  if (index >= WHEEL_LOG_MAX_SAMPLES)
  {
    wheel_log_active = 0U;
    wheel_log_done = 1U;
    WC_LogCommit();
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return;
  }

  wheel_log[index].time_ms =
      (uint32_t)(now - wheel_log_start_tick);

  /* Command path: analog -> raw -> hysteresis/accepted -> slew -> PID. */
  wheel_log[index].ps2_ly = ps2_ly;
  wheel_log[index].control_mode = mode_value;
  wheel_log[index].integral_hold = wheel_pid_integral_hold_active;
  wheel_log[index].sync_active = wheel_sync_active;
  wheel_log[index].hunt_lu = wheel_pid_hunt_active_lu;
  wheel_log[index].hunt_ld = wheel_pid_hunt_active_ld;
  wheel_log[index].hunt_ru = wheel_pid_hunt_active_ru;
  wheel_log[index].hunt_rd = wheel_pid_hunt_active_rd;
  wheel_log[index].capture_ru_active = wheel_ru_capture_assist_active;
  wheel_log[index].plateau_adapt_i_mask = wheel_plateau_adapt_i_mask;
  wheel_log[index].target_pwm = wheel_target_pwm;
  wheel_log[index].command_profile = (uint8_t)wheel_command_profile;
  wheel_log[index].soft_landing_active = wheel_manual_soft_landing_active;
  wheel_log[index].zero_soft_landing_active =
      (uint16_t)wheel_manual_zero_soft_landing_active;
  {
    int32_t taper_x1000 =
        Wheel_LogFloatToMilli(wheel_manual_zero_ff_taper_scale);
    if (taper_x1000 < 0)
    {
      taper_x1000 = 0;
    }
    else if (taper_x1000 > 1000)
    {
      taper_x1000 = 1000;
    }
    wheel_log[index].zero_ff_taper_x1000 = (uint16_t)taper_x1000;
  }

  wheel_log[index].raw_target_rpm = wheel_target_raw_rpm;
  wheel_log[index].command_target_rpm = wheel_command_target_rpm;
  wheel_log[index].pid_target_rpm = wheel_pid_target_rpm;

  wheel_log[index].lu_rpm = s_motor_lu->current_rpm;
  wheel_log[index].ld_rpm = s_motor_ld->current_rpm;
  wheel_log[index].ru_rpm = s_motor_ru->current_rpm;
  wheel_log[index].rd_rpm = s_motor_rd->current_rpm;

  wheel_log[index].sync_spread_rpm = wheel_sync_spread_rpm;
  wheel_log[index].ff_accel_pwm = wheel_ff_accel_pwm;
  wheel_log[index].capture_ru_pwm = wheel_ru_capture_assist_pwm;

  /* PWM thực đang cấp cho từng bánh. */
  wheel_log[index].lu_pwm = wheel_pwm_lu;
  wheel_log[index].ld_pwm = wheel_pwm_ld;
  wheel_log[index].ru_pwm = wheel_pwm_ru;
  wheel_log[index].rd_pwm = wheel_pwm_rd;

  wheel_log_count = index + 1U;

  /*
   * Đủ buffer thì khóa log lại, không ghi tràn RAM.
   */
  if (wheel_log_count >= WHEEL_LOG_MAX_SAMPLES)
  {
    wheel_log_active = 0U;
    wheel_log_done = 1U;
    WC_LogCommit();
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
  }
}


