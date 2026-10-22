/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ps2.h"
#include "motor.h"
#include "wheel_control.h"
#include "ram_logger.h"
#include "vc02.h"
#include "as5600.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
  VOICE_MOVE_NONE = 0,
  VOICE_MOVE_FORWARD,
  VOICE_MOVE_BACKWARD,
  VOICE_MOVE_ROTATE

} VoiceMovement_t;


typedef enum
{
  VOICE_TURN_NONE = 0,
  VOICE_TURN_LEFT,
  VOICE_TURN_RIGHT

} VoiceTurn_t;


/*
 * Trạng thái đang gom các thành phần trước TIME.
 * Các thành phần có thể đến theo bất kỳ thứ tự nào.
 */
typedef struct
{
  VoiceMovement_t movement;
  VoiceTurn_t turn;

  uint8_t angle_deg;

  uint8_t movement_valid;
  uint8_t turn_valid;
  uint8_t angle_valid;

  uint8_t movement_conflict;
  uint8_t steering_conflict;

} VoicePendingCommand_t;


/* Command đã được TIME chốt và kiểm tra hợp lệ. */
typedef struct
{
  VoiceMovement_t movement;
  VoiceTurn_t turn;

  uint8_t angle_deg;
  uint8_t speed_level;
  uint8_t duration_sec;

} VoiceVehicleCommand_t;


typedef enum
{
  VOICE_BUILD_RESULT_NONE = 0,
  VOICE_BUILD_RESULT_OK,
  VOICE_BUILD_ERROR_NO_MOVEMENT,
  VOICE_BUILD_ERROR_MOVEMENT_CONFLICT,
  VOICE_BUILD_ERROR_STEERING_CONFLICT,
  VOICE_BUILD_ERROR_STEERING_INCOMPLETE,
  VOICE_BUILD_ERROR_ROTATE_WITH_STEERING

} VoiceBuildResult_t;


/*
 * M3 - nguồn điều khiển hiện đang có quyền tác động lên xe.
 * STOP có ưu tiên cao nhất, sau đó PS2, sau đó Voice.
 */
typedef enum
{
  CONTROL_MODE_IDLE = 0,
  CONTROL_MODE_VOICE,
  CONTROL_MODE_PS2,
  CONTROL_MODE_STOP

} ControlMode_t;


/*
 * M7-S01: input-only limit switch diagnostics (no steering PWM output).
 * Wiring agreed for each switch: NC -> GPIO, COM -> STM32 GND.
 * Active HIGH = NC opened by actuation or by broken/disconnected wiring.
 */
typedef struct
{
  uint8_t raw;                    /* unfiltered GPIO IDR, 0=NC closed */
  uint8_t active;                 /* debounced HIGH, 1=limit open */
  uint32_t activate_count;       /* debounced transitions 0 -> 1 */
  uint32_t release_count;        /* debounced transitions 1 -> 0 */
  uint32_t exti_rise_count;      /* raw rising EXTI events (can bounce) */
  uint8_t initialized;
  uint8_t candidate;
  uint32_t candidate_since_ms;
} SteeringLimitDebug_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STEERING_LIMIT_DEBOUNCE_MS        40U
/* M7-S02: independent, bounded diagnostic duty/time per motor.
 * LEFT: 20%, 5000ms maximum (prior 30% observed too fast).
 * RIGHT: 10%, 5000ms maximum (previously observed R_MIN limit stop).
 * In all cases: stop sooner on any NC limit; PWM=0 != power off.
 */
#define STEER_TEST_LEFT_PWM_DUTY         200U
#define STEER_TEST_RIGHT_PWM_DUTY        100U
#define STEER_TEST_LEFT_MAX_RUN_MS       5000U
#define STEER_TEST_RIGHT_MAX_RUN_MS      5000U
#define STEER_TEST_ESCAPE_MAX_MS         1000U
#define STEER_LIMIT_R_MAX_MASK           0x01U
#define STEER_LIMIT_R_MIN_MASK           0x02U
#define STEER_LIMIT_L_MIN_MASK           0x04U
#define STEER_LIMIT_L_MAX_MASK           0x08U
#define MOTOR_PWM_MAX                     999
#define MOTOR_DPAD_PWM                    400

/*
 * M4' - PWM open-loop cho 5 mức tốc độ Voice.
 * Chưa dùng encoder/PID. Có thể hiệu chỉnh lại sau khi test thực tế.
 */
#define VOICE_PWM_VERY_SLOW               250
#define VOICE_PWM_SLOW                    400
#define VOICE_PWM_MEDIUM                  550
#define VOICE_PWM_FAST                    750
#define VOICE_PWM_FULL                    999


#define PS2_LY_FORWARD_LIMIT              125
#define PS2_LY_REVERSE_LIMIT              130

/*
 * Encoder bánh xe.
 *
 * LU mới dùng encoder 16 P/R. TIM8 chạy Encoder TI12 nên quadrature
 * đếm cả 4 cạnh; với cùng motor/hộp số như ba bánh đã hiệu chuẩn,
 * tổng count trên một vòng trục ra vẫn dùng 3257 count/rev.
 *
 * Tách LU thành hằng riêng để sau này có thể hiệu chỉnh độc lập nếu
 * phép đo cơ khí thực tế cho thấy CPR khác ba bánh còn lại.
 */
#define WHEEL_ENCODER_PULSES_PER_MOTOR_REV_LU   16.0f
#define WHEEL_ENCODER_COUNTS_PER_REV_LU       3257.0f
#define WHEEL_ENCODER_COUNTS_PER_REV_OTHER    3257.0f

/*
 * Đổi riêng từ 0U thành 1U nếu sau khi kiểm tra thực tế một motor
 * quay ngược chiều chuyển động mong muốn.
 */
#define MOTOR_LEFT_UP_DIR_INVERTED          1U
#define MOTOR_RIGHT_UP_DIR_INVERTED         1U
#define MOTOR_LEFT_DOWN_DIR_INVERTED        0U
#define MOTOR_RIGHT_DOWN_DIR_INVERTED       1U

/*
 * Chuẩn hóa dấu encoder theo chiều chuyển động của xe:
 * - Xe chạy TIẾN  -> RPM / total dương.
 * - Xe chạy LÙI   -> RPM / total âm.
 *
 * M6-S02 (kiểm tra trên xe, sau khi đổi cấu hình MOTOR_*_DIR_INVERTED):
 * - Lệnh -40 RPM làm xe LÙI đúng chiều vật lý.
 * - Cả bốn giá trị RPM encoder lại DƯƠNG khi target là -40 RPM.
 * - Vì vậy chỉ đảo dấu encoder của cả bốn bánh, GIỮ NGUYÊN chiều motor.
 * Cần xác nhận dấu RPM bằng Live Expressions sau khi flash lại.
 */
#define ENCODER_LEFT_UP_DIR_INVERTED         1U
#define ENCODER_RIGHT_UP_DIR_INVERTED        0U
#define ENCODER_LEFT_DOWN_DIR_INVERTED       1U
#define ENCODER_RIGHT_DOWN_DIR_INVERTED      0U




/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2007c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2007c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2007c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2007c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

LPTIM_HandleTypeDef hlptim1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim9;
TIM_HandleTypeDef htim10;
TIM_HandleTypeDef htim11;
TIM_HandleTypeDef htim12;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
/* ============================================================
 * MOTOR
 * ============================================================
 */
Motor_HandleTypeDef Motor_LeftUp;
Motor_HandleTypeDef Motor_RightUp;
Motor_HandleTypeDef Motor_LeftDown;
Motor_HandleTypeDef Motor_RightDown;

/* ============================================================
 * ENCODER RAW COUNTER DEBUG
 *
 * Xem 4 biến này trong Live Expressions để kiểm tra encoder.
 * Đây là bản debug thêm trên nền M4' hiện tại có ramp +
 * PS2 cancel-to-stop.
 * ============================================================
 */
volatile uint32_t debug_enc_lu_cnt = 0U;   /* Left Up    -> TIM8 */
volatile uint32_t debug_enc_ru_cnt = 0U;   /* Right Up   -> TIM1 */
volatile uint32_t debug_enc_ld_cnt = 0U;   /* Left Down  -> TIM2 */
volatile uint32_t debug_enc_rd_cnt = 0U;   /* Right Down -> TIM3 */

/*
 * Bộ đếm mở rộng 32-bit có dấu.
 * *_cnt là RAW timer và vẫn wrap ở 65535.
 * *_total cộng dồn delta nên có thể vượt 65535 và giữ được chiều +/-.
 */
volatile int32_t debug_enc_lu_total = 0;
volatile int32_t debug_enc_ru_total = 0;
volatile int32_t debug_enc_ld_total = 0;
volatile int32_t debug_enc_rd_total = 0;

static uint16_t debug_enc_lu_prev = 0U;
static uint16_t debug_enc_ru_prev = 0U;
static uint16_t debug_enc_ld_prev = 0U;
static uint16_t debug_enc_rd_prev = 0U;


/* ============================================================
 * STEERING ENCODER DEBUG
 *
 * LEFT steering:
 *   PD12 = TIM4_CH1
 *   PD13 = TIM4_CH2
 *
 * RIGHT steering:
 *   PG12 = LPTIM1_IN1
 *   PE1  = LPTIM1_IN2
 *
 * cnt: bộ đếm quadrature của peripheral
 * a/b: mức logic thô tại hai chân encoder, dùng để đối chiếu khi cần
 * ============================================================
 */
volatile uint32_t debug_turn_left_cnt = 0U;
volatile uint32_t debug_turn_right_cnt = 0U;

volatile uint8_t debug_turn_left_a = 0U;
volatile uint8_t debug_turn_left_b = 0U;
volatile uint8_t debug_turn_right_a = 0U;
volatile uint8_t debug_turn_right_b = 0U;

/* Trạng thái start encoder: 1 = HAL_OK, 0 = start lỗi/chưa chạy. */
volatile uint8_t debug_turn_left_start_ok = 0U;
volatile uint8_t debug_turn_right_start_ok = 0U;


/* ============================================================
 * AS5600 STEERING ANGLE DEBUG / MANUAL CALIBRATION
 *
 * Wiring hiện tại:
 *   RIGHT AS5600 -> I2C1 PB8/PB9
 *   LEFT  AS5600 -> I2C2 PB10/PB11
 *
 * Chỉ đọc cảm biến; KHÔNG điều khiển motor đánh lái.
 * RAW angle là 12-bit 0..4095.
 *
 * debug_as5600_cal_command dùng trong Live Expressions:
 *   0 = idle
 *   1 = capture LEFT  physical MIN
 *   2 = capture LEFT  CENTER
 *   3 = capture LEFT  physical MAX
 *   4 = capture RIGHT physical MIN
 *   5 = capture RIGHT CENTER
 *   6 = capture RIGHT physical MAX
 *   7 = reset toàn bộ giá trị capture
 *
 * MIN/MAX ở đây là hai đầu cơ khí, KHÔNG có nghĩa raw_min < raw_max.
 * Delta quanh CENTER được tính theo vòng 4096 count để xử lý trường hợp
 * góc đi qua biên 4095 -> 0.
 * ============================================================
 */
#define AS5600_STARTUP_SETTLE_MS 20U
#define AS5600_DEBUG_NOT_ATTEMPTED 0xFFU

AS5600_HandleTypeDef As5600_Left;
AS5600_HandleTypeDef As5600_Right;

/*
 * Tạm tắt polling AS5600 trong vòng lặp chính để không làm chậm wheel PID.
 * 0 = không đọc I2C AS5600; 1 = bật lại telemetry/calibration khi xe đang dừng.
 */
volatile uint8_t debug_as5600_poll_enable = 0U;

/* Each AS5600 bus is probed only when both bus lines were released HIGH at startup. */
static uint8_t as5600_left_probe_enabled = 0U;
static uint8_t as5600_right_probe_enabled = 0U;

volatile uint16_t debug_as5600_left_raw = 0U;
volatile uint16_t debug_as5600_right_raw = 0U;
volatile float debug_as5600_left_deg = 0.0f;
volatile float debug_as5600_right_deg = 0.0f;

volatile uint8_t debug_as5600_left_status = 0U;
volatile uint8_t debug_as5600_right_status = 0U;
volatile uint8_t debug_as5600_left_comm_ok = 0U;
volatile uint8_t debug_as5600_right_comm_ok = 0U;
volatile uint8_t debug_as5600_left_magnet_detected = 0U;
volatile uint8_t debug_as5600_right_magnet_detected = 0U;
volatile uint8_t debug_as5600_left_magnet_too_weak = 0U;
volatile uint8_t debug_as5600_right_magnet_too_weak = 0U;
volatile uint8_t debug_as5600_left_magnet_too_strong = 0U;
volatile uint8_t debug_as5600_right_magnet_too_strong = 0U;
volatile uint32_t debug_as5600_left_read_ok_count = 0U;
volatile uint32_t debug_as5600_right_read_ok_count = 0U;
volatile uint32_t debug_as5600_left_read_fail_count = 0U;
volatile uint32_t debug_as5600_right_read_fail_count = 0U;

/*
 * I2C ACK / HAL diagnostics for AS5600.
 * HAL_StatusTypeDef values: HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3.
 * HAL_I2C_GetError() is a bitmask; common NACK/address failure is HAL_I2C_ERROR_AF.
 */
volatile uint8_t debug_as5600_left_device_ready = 0U;
volatile uint8_t debug_as5600_right_device_ready = 0U;
volatile uint8_t debug_as5600_left_ready_status = (uint8_t)HAL_ERROR;
volatile uint8_t debug_as5600_right_ready_status = (uint8_t)HAL_ERROR;
volatile uint32_t debug_as5600_left_ready_error = HAL_I2C_ERROR_NONE;
volatile uint32_t debug_as5600_right_ready_error = HAL_I2C_ERROR_NONE;
volatile uint8_t debug_as5600_left_read_status = (uint8_t)HAL_ERROR;
volatile uint8_t debug_as5600_right_read_status = (uint8_t)HAL_ERROR;
volatile uint32_t debug_as5600_left_read_error = HAL_I2C_ERROR_NONE;
volatile uint32_t debug_as5600_right_read_error = HAL_I2C_ERROR_NONE;
volatile uint32_t debug_as5600_left_i2c_state = 0U;
volatile uint32_t debug_as5600_right_i2c_state = 0U;

/* I2C startup bus-recovery diagnostics for both steering AS5600 buses. */
volatile uint8_t debug_as5600_left_recovery_attempted = 0U;
volatile uint8_t debug_as5600_left_recovery_pulses = 0U;
volatile uint8_t debug_as5600_left_recovery_scl_high = 0U;
volatile uint8_t debug_as5600_left_recovery_sda_high = 0U;
volatile uint8_t debug_as5600_left_recovery_bus_ok = 0U;
volatile uint8_t debug_as5600_left_probe_blocked = 1U;
volatile uint32_t debug_as5600_left_startup_settle_ms = AS5600_STARTUP_SETTLE_MS;

volatile uint8_t debug_as5600_right_recovery_attempted = 0U;
volatile uint8_t debug_as5600_right_recovery_pulses = 0U;
volatile uint8_t debug_as5600_right_recovery_scl_high = 0U;
volatile uint8_t debug_as5600_right_recovery_sda_high = 0U;
volatile uint8_t debug_as5600_right_recovery_bus_ok = 0U;
volatile uint8_t debug_as5600_right_probe_blocked = 1U;
volatile uint32_t debug_as5600_right_startup_settle_ms = AS5600_STARTUP_SETTLE_MS;

volatile uint8_t debug_as5600_cal_command = 0U;
volatile uint8_t debug_as5600_cal_last_result = 0U; /* 1=OK, 2=LEFT invalid, 3=RIGHT invalid */

volatile uint16_t debug_as5600_left_min_raw = 0U;
volatile uint16_t debug_as5600_left_center_raw = 0U;
volatile uint16_t debug_as5600_left_max_raw = 0U;
volatile uint16_t debug_as5600_right_min_raw = 0U;
volatile uint16_t debug_as5600_right_center_raw = 0U;
volatile uint16_t debug_as5600_right_max_raw = 0U;

volatile uint8_t debug_as5600_left_min_valid = 0U;
volatile uint8_t debug_as5600_left_center_valid = 0U;
volatile uint8_t debug_as5600_left_max_valid = 0U;
volatile uint8_t debug_as5600_right_min_valid = 0U;
volatile uint8_t debug_as5600_right_center_valid = 0U;
volatile uint8_t debug_as5600_right_max_valid = 0U;

volatile int16_t debug_as5600_left_min_from_center_counts = 0;
volatile int16_t debug_as5600_left_max_from_center_counts = 0;
volatile int16_t debug_as5600_right_min_from_center_counts = 0;
volatile int16_t debug_as5600_right_max_from_center_counts = 0;
volatile uint16_t debug_as5600_left_span_counts = 0U;
volatile uint16_t debug_as5600_right_span_counts = 0U;

volatile float debug_as5600_left_min_from_center_deg = 0.0f;
volatile float debug_as5600_left_max_from_center_deg = 0.0f;
volatile float debug_as5600_right_min_from_center_deg = 0.0f;
volatile float debug_as5600_right_max_from_center_deg = 0.0f;
volatile float debug_as5600_left_span_deg = 0.0f;
volatile float debug_as5600_right_span_deg = 0.0f;


/* ============================================================
 * VC-02 - M1 DEBUG
 *
 * Các biến này vẫn giữ lại ở M2 để kiểm tra UART/parser.
 * ============================================================
 */
static VC02_Event_t vc02_event;

volatile uint32_t vc02_event_count = 0U;
volatile VC02_EventType_t vc02_last_event_type = VC02_EVENT_NONE;
volatile uint8_t vc02_last_value = 0U;
volatile uint8_t vc02_last_raw0 = 0U;
volatile uint8_t vc02_last_raw1 = 0U;
volatile uint32_t vc02_dropped_events = 0U;


/* ============================================================
 * VC-02 - M2 COMMAND BUILDER
 *
 * Quy tắc đã chốt:
 * - Movement / Turn / Angle có thể nói theo bất kỳ thứ tự nào.
 * - TIME phải là lệnh chốt cuối chuỗi.
 * - Speed là mode độc lập, mặc định Medium và được giữ lại.
 * - Clear command xóa pending nhưng không xóa speed mode.
 * - Zero degrees xóa riêng phần steering pending về thẳng 0 độ.
 * - Lệnh mâu thuẫn không tự ghi đè; cần Clear command
 *   (riêng Zero degrees được phép xóa phần steering).
 * - M4' hiện cho Voice chạy tiến/lùi open-loop qua tầng ramp.
 * ============================================================
 */
static VoicePendingCommand_t voice_pending_command;

volatile VoiceVehicleCommand_t voice_completed_command;

volatile uint8_t voice_speed_mode = VC02_SPEED_MEDIUM;
volatile uint8_t voice_command_complete = 0U;

volatile uint32_t voice_completed_count = 0U;
volatile uint32_t voice_invalid_count = 0U;
volatile uint32_t voice_clear_count = 0U;
volatile uint32_t voice_stop_count = 0U;

volatile VoiceBuildResult_t voice_last_build_result = VOICE_BUILD_RESULT_NONE;


/* ============================================================
 * M3 - CONTROL ARBITRATION
 *
 * Priority:
 *   STOP > PS2 manual input > Voice > Idle
 *
 * M3 đã giải quyết quyền điều khiển.
 * M4' dùng quyền VOICE để chạy 4 motor bánh open-loop tiến/lùi.
 * ============================================================
 */
volatile ControlMode_t control_mode = CONTROL_MODE_IDLE;

/* =1 khi PS2 hiện có thao tác thủ công được công nhận. */
volatile uint8_t ps2_manual_active = 0U;
/* After radio loss during spin, require a neutral PS2 packet before rearming. */
volatile uint8_t ps2_spin_rf_latched = 0U;

/*
 * =1 sau khi một thao tác PS2 đã được dùng để HỦY Voice.
 *
 * Trong trạng thái này thao tác PS2 đầu tiên KHÔNG được điều khiển motor.
 * Xe chỉ được yêu cầu giảm tốc về 0. Người dùng phải nhả tay/nút về
 * neutral trước khi PS2 được phép điều khiển ở thao tác kế tiếp.
 */
volatile uint8_t ps2_cancel_hold = 0U;

/* Debug: số lần thao tác PS2 đã hủy Voice và yêu cầu dừng xe. */
volatile uint32_t ps2_cancel_stop_count = 0U;

/* STOP được giữ cho tới khi tay PS2 trở về neutral. */
volatile uint8_t control_stop_latched = 0U;

/* Command vừa được TIME chốt và đang chờ arbitration nhận. */
volatile uint8_t voice_command_ready = 0U;

/* Command voice đã giành quyền điều khiển. */
volatile uint8_t voice_execution_active = 0U;

/* Snapshot command voice đang active. */
volatile VoiceVehicleCommand_t voice_active_command;

/* Debug: số lần PS2 đã hủy voice pending/ready/active. */
volatile uint32_t voice_cancel_by_ps2_count = 0U;

/* Debug: số lần arbitration nhận STOP. */
volatile uint32_t control_stop_count = 0U;


/* ============================================================
 * M4' - VOICE OPEN-LOOP DRIVE DEBUG
 * ============================================================
 */
volatile uint32_t voice_execution_start_tick = 0U;
volatile uint32_t voice_execution_duration_ms = 0U;
volatile uint32_t voice_execution_elapsed_ms = 0U;
volatile int16_t voice_execution_pwm = 0;
volatile uint32_t voice_execution_finish_count = 0U;
volatile uint32_t voice_unsupported_command_count = 0U;
volatile uint32_t voice_ignored_during_execution_count = 0U;

/* M7-S01: visible directly in CubeIDE Live Expressions.
 * Bit0 R_MAX (PF5); bit1 R_MIN (PF8); bit2 L_MIN (PD10); bit3 L_MAX (PD11).
 * No motor command or motor safety policy is implemented at this stage.
 */
volatile SteeringLimitDebug_t debug_limit_r_max = {0};
volatile SteeringLimitDebug_t debug_limit_r_min = {0};
volatile SteeringLimitDebug_t debug_limit_l_min = {0};
volatile SteeringLimitDebug_t debug_limit_l_max = {0};
volatile uint8_t debug_limit_raw_mask = 0U;
volatile uint8_t debug_limit_active_mask = 0U;
volatile uint8_t debug_limit_same_side_both_active = 0U;

/* M7-S02: 1 LEFT DIR0 -> L_MIN 20%; 2 LEFT DIR1 -> L_MAX 20%;
 * 3 RIGHT DIR0 -> R_MIN 10%; 4 RIGHT DIR1 -> R_MAX 10%.
 * Each command is <=5s UNLESS starting from its opposite limit:
 * 1 from L_MAX, 2 from L_MIN, 4 from R_MIN => 1s escape verification.
 * Escape success results 7 (PWM off); only a SEPARATE armed command
 * can travel toward the opposite switch after all masks return zero.
 * Results: 2=time limit, 3=refused, 4=limit trip, 5=abort,
 * 6=invalid, 7=escape released, 8=escape timeout.
 */
volatile uint8_t steer_test_arm = 0U;
volatile uint8_t steer_test_request = 0U;
volatile uint8_t steer_test_abort = 0U;
volatile uint8_t steer_test_running = 0U;
volatile uint8_t steer_test_result = 0U;
/* Latched diagnosis: 0 none, 1 second request, 2 explicit abort,
 * 3 safety condition changed. For cause=3 read reason_mask below.
 * Both values persist after the run, until the next accepted command.
 */
volatile uint8_t steer_test_abort_cause = 0U;
volatile uint32_t steer_test_abort_reason_mask = 0U;
volatile uint8_t steer_test_last_command = 0U;
volatile uint8_t steer_test_trip_mask = 0U;
/* Set only when a short escape proves the active switch has released.
 * No full RIGHT DIR1 travel unless R_MIN was released by RIGHT DIR1.
 */
volatile uint8_t steer_test_left_dir1_verified = 0U;
volatile uint8_t steer_test_right_dir1_verified = 0U;
volatile uint8_t steer_test_escape_mask = 0U; /* 2=R_MIN, 4=L_MIN, 8=L_MAX */
volatile uint8_t steer_test_exit_released_mask = 0U;
volatile uint8_t steer_test_left_escape_active = 0U; /* legacy view */
volatile uint16_t steer_test_pwm_applied = 0U;
volatile uint32_t steer_test_start_tick = 0U;
volatile uint32_t steer_test_elapsed_ms = 0U;
volatile uint32_t steer_test_run_count = 0U;
volatile uint32_t steer_test_rejected_count = 0U;
volatile uint32_t steer_test_limit_stop_count = 0U;
volatile uint16_t steer_test_encoder_start = 0U;
volatile int16_t steer_test_encoder_delta = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM9_Init(void);
static void MX_TIM2_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM10_Init(void);
static void MX_TIM11_Init(void);
static void MX_TIM12_Init(void);
static void MX_TIM14_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*
 * Recover RIGHT AS5600 I2C1 once at startup.
 * PB8/PB9 are driven only as open-drain: GPIO_PIN_SET releases the line
 * to the external pull-up; GPIO_PIN_RESET actively pulls it low.
 * This mirrors the proven LEFT/I2C2 recovery so the same known-good AS5600
 * can be moved to the RIGHT steering mechanism for manual calibration.
 */
static void AS5600_I2C1_BusRecover(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  const uint16_t scl_pin = AS5600_R_SCL_Pin;
  const uint16_t sda_pin = AS5600_R_SDA_Pin;
  const uint16_t bus_pins = (uint16_t)(AS5600_R_SCL_Pin | AS5600_R_SDA_Pin);
  uint8_t pulses = 0U;

  debug_as5600_right_recovery_attempted = 1U;

  (void)HAL_I2C_DeInit(&hi2c1);
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(AS5600_R_SCL_GPIO_Port, bus_pins, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = bus_pins;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(AS5600_R_SCL_GPIO_Port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(AS5600_R_SCL_GPIO_Port, bus_pins, GPIO_PIN_SET);
  HAL_Delay(1U);

  while ((HAL_GPIO_ReadPin(AS5600_R_SDA_GPIO_Port, sda_pin) == GPIO_PIN_RESET) &&
         (pulses < 9U))
  {
    HAL_GPIO_WritePin(AS5600_R_SCL_GPIO_Port, scl_pin, GPIO_PIN_RESET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(AS5600_R_SCL_GPIO_Port, scl_pin, GPIO_PIN_SET);
    HAL_Delay(1U);
    pulses++;
  }

  HAL_GPIO_WritePin(AS5600_R_SDA_GPIO_Port, sda_pin, GPIO_PIN_RESET);
  HAL_Delay(1U);
  HAL_GPIO_WritePin(AS5600_R_SCL_GPIO_Port, scl_pin, GPIO_PIN_SET);
  HAL_Delay(1U);
  HAL_GPIO_WritePin(AS5600_R_SDA_GPIO_Port, sda_pin, GPIO_PIN_SET);
  HAL_Delay(1U);

  debug_as5600_right_recovery_pulses = pulses;
  debug_as5600_right_recovery_scl_high =
      (HAL_GPIO_ReadPin(AS5600_R_SCL_GPIO_Port, scl_pin) == GPIO_PIN_SET) ? 1U : 0U;
  debug_as5600_right_recovery_sda_high =
      (HAL_GPIO_ReadPin(AS5600_R_SDA_GPIO_Port, sda_pin) == GPIO_PIN_SET) ? 1U : 0U;

  debug_as5600_right_recovery_bus_ok =
      ((debug_as5600_right_recovery_scl_high != 0U) &&
       (debug_as5600_right_recovery_sda_high != 0U)) ? 1U : 0U;

  as5600_right_probe_enabled = debug_as5600_right_recovery_bus_ok;
  debug_as5600_right_probe_blocked =
      (as5600_right_probe_enabled != 0U) ? 0U : 1U;

  HAL_GPIO_DeInit(AS5600_R_SCL_GPIO_Port, bus_pins);

  /* Recreate I2C1 using the 100-kHz timing configured in MX_I2C1_Init(). */
  MX_I2C1_Init();
}

/*
 * Recover LEFT AS5600 I2C2 once at startup.
 * PB10/PB11 are driven only as open-drain: GPIO_PIN_SET releases the line
 * to the external pull-up; GPIO_PIN_RESET actively pulls it low.
 * This clears a slave left mid-byte and also resets the I2C2 peripheral state.
 */
static void AS5600_I2C2_BusRecover(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  const uint16_t scl_pin = AS5600_L_SCL_Pin;
  const uint16_t sda_pin = AS5600_L_SDA_Pin;
  const uint16_t bus_pins = (uint16_t)(AS5600_L_SCL_Pin | AS5600_L_SDA_Pin);
  uint8_t pulses = 0U;

  debug_as5600_left_recovery_attempted = 1U;

  /* Fully reset I2C2 before taking temporary GPIO ownership of the bus. */
  (void)HAL_I2C_DeInit(&hi2c2);
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Preload ODR high so open-drain pins are released immediately. */
  HAL_GPIO_WritePin(AS5600_L_SCL_GPIO_Port, bus_pins, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = bus_pins;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(AS5600_L_SCL_GPIO_Port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(AS5600_L_SCL_GPIO_Port, bus_pins, GPIO_PIN_SET);
  HAL_Delay(1U);

  /* If SDA is held low, provide up to 9 SCL clocks so a slave can finish a byte. */
  while ((HAL_GPIO_ReadPin(AS5600_L_SDA_GPIO_Port, sda_pin) == GPIO_PIN_RESET) &&
         (pulses < 9U))
  {
    HAL_GPIO_WritePin(AS5600_L_SCL_GPIO_Port, scl_pin, GPIO_PIN_RESET);
    HAL_Delay(1U);
    HAL_GPIO_WritePin(AS5600_L_SCL_GPIO_Port, scl_pin, GPIO_PIN_SET);
    HAL_Delay(1U);
    pulses++;
  }

  /* Generate a STOP condition: SDA low -> SCL released high -> SDA released high. */
  HAL_GPIO_WritePin(AS5600_L_SDA_GPIO_Port, sda_pin, GPIO_PIN_RESET);
  HAL_Delay(1U);
  HAL_GPIO_WritePin(AS5600_L_SCL_GPIO_Port, scl_pin, GPIO_PIN_SET);
  HAL_Delay(1U);
  HAL_GPIO_WritePin(AS5600_L_SDA_GPIO_Port, sda_pin, GPIO_PIN_SET);
  HAL_Delay(1U);

  debug_as5600_left_recovery_pulses = pulses;
  debug_as5600_left_recovery_scl_high =
      (HAL_GPIO_ReadPin(AS5600_L_SCL_GPIO_Port, scl_pin) == GPIO_PIN_SET) ? 1U : 0U;
  debug_as5600_left_recovery_sda_high =
      (HAL_GPIO_ReadPin(AS5600_L_SDA_GPIO_Port, sda_pin) == GPIO_PIN_SET) ? 1U : 0U;

  debug_as5600_left_recovery_bus_ok =
      ((debug_as5600_left_recovery_scl_high != 0U) &&
       (debug_as5600_left_recovery_sda_high != 0U)) ? 1U : 0U;

  /*
   * If either line is still LOW here, do not continuously hammer I2C2 later.
   * The blocked flag deliberately survives until reset so Live Expressions
   * captures the actual startup condition instead of thousands of follow-on errors.
   */
  as5600_left_probe_enabled = debug_as5600_left_recovery_bus_ok;
  debug_as5600_left_probe_blocked =
      (as5600_left_probe_enabled != 0U) ? 0U : 1U;

  HAL_GPIO_DeInit(AS5600_L_SCL_GPIO_Port, bus_pins);

  /* Recreate I2C2 using the 100-kHz timing configured in MX_I2C2_Init(). */
  MX_I2C2_Init();
}

static uint8_t AS5600_DebugReadingValid(uint8_t comm_ok, uint8_t status)
{
  return ((comm_ok != 0U) && ((status & AS5600_STATUS_MD) != 0U) &&
          ((status & (AS5600_STATUS_ML | AS5600_STATUS_MH)) == 0U)) ? 1U : 0U;
}


static void AS5600_DebugRecomputeCalibration(void)
{
  if ((debug_as5600_left_center_valid != 0U) &&
      (debug_as5600_left_min_valid != 0U))
  {
    debug_as5600_left_min_from_center_counts =
        AS5600_ShortestDeltaCounts(debug_as5600_left_center_raw,
                                   debug_as5600_left_min_raw);
    debug_as5600_left_min_from_center_deg =
        AS5600_DeltaCountsToDegrees(debug_as5600_left_min_from_center_counts);
  }

  if ((debug_as5600_left_center_valid != 0U) &&
      (debug_as5600_left_max_valid != 0U))
  {
    debug_as5600_left_max_from_center_counts =
        AS5600_ShortestDeltaCounts(debug_as5600_left_center_raw,
                                   debug_as5600_left_max_raw);
    debug_as5600_left_max_from_center_deg =
        AS5600_DeltaCountsToDegrees(debug_as5600_left_max_from_center_counts);
  }

  if ((debug_as5600_left_min_valid != 0U) &&
      (debug_as5600_left_max_valid != 0U))
  {
    int16_t span = AS5600_ShortestDeltaCounts(debug_as5600_left_min_raw,
                                              debug_as5600_left_max_raw);
    if (span < 0)
    {
      span = (int16_t)(-span);
    }
    debug_as5600_left_span_counts = (uint16_t)span;
    debug_as5600_left_span_deg = AS5600_DeltaCountsToDegrees(span);
  }

  if ((debug_as5600_right_center_valid != 0U) &&
      (debug_as5600_right_min_valid != 0U))
  {
    debug_as5600_right_min_from_center_counts =
        AS5600_ShortestDeltaCounts(debug_as5600_right_center_raw,
                                   debug_as5600_right_min_raw);
    debug_as5600_right_min_from_center_deg =
        AS5600_DeltaCountsToDegrees(debug_as5600_right_min_from_center_counts);
  }

  if ((debug_as5600_right_center_valid != 0U) &&
      (debug_as5600_right_max_valid != 0U))
  {
    debug_as5600_right_max_from_center_counts =
        AS5600_ShortestDeltaCounts(debug_as5600_right_center_raw,
                                   debug_as5600_right_max_raw);
    debug_as5600_right_max_from_center_deg =
        AS5600_DeltaCountsToDegrees(debug_as5600_right_max_from_center_counts);
  }

  if ((debug_as5600_right_min_valid != 0U) &&
      (debug_as5600_right_max_valid != 0U))
  {
    int16_t span = AS5600_ShortestDeltaCounts(debug_as5600_right_min_raw,
                                              debug_as5600_right_max_raw);
    if (span < 0)
    {
      span = (int16_t)(-span);
    }
    debug_as5600_right_span_counts = (uint16_t)span;
    debug_as5600_right_span_deg = AS5600_DeltaCountsToDegrees(span);
  }
}


static void AS5600_DebugResetCalibration(void)
{
  debug_as5600_left_min_raw = 0U;
  debug_as5600_left_center_raw = 0U;
  debug_as5600_left_max_raw = 0U;
  debug_as5600_right_min_raw = 0U;
  debug_as5600_right_center_raw = 0U;
  debug_as5600_right_max_raw = 0U;

  debug_as5600_left_min_valid = 0U;
  debug_as5600_left_center_valid = 0U;
  debug_as5600_left_max_valid = 0U;
  debug_as5600_right_min_valid = 0U;
  debug_as5600_right_center_valid = 0U;
  debug_as5600_right_max_valid = 0U;

  debug_as5600_left_min_from_center_counts = 0;
  debug_as5600_left_max_from_center_counts = 0;
  debug_as5600_right_min_from_center_counts = 0;
  debug_as5600_right_max_from_center_counts = 0;
  debug_as5600_left_span_counts = 0U;
  debug_as5600_right_span_counts = 0U;

  debug_as5600_left_min_from_center_deg = 0.0f;
  debug_as5600_left_max_from_center_deg = 0.0f;
  debug_as5600_right_min_from_center_deg = 0.0f;
  debug_as5600_right_max_from_center_deg = 0.0f;
  debug_as5600_left_span_deg = 0.0f;
  debug_as5600_right_span_deg = 0.0f;
}


static void AS5600_DebugProcessCaptureCommand(void)
{
  uint8_t command = debug_as5600_cal_command;

  if (command == 0U)
  {
    return;
  }

  debug_as5600_cal_command = 0U;
  debug_as5600_cal_last_result = 0U;

  if (command == 7U)
  {
    AS5600_DebugResetCalibration();
    debug_as5600_cal_last_result = 1U;
    return;
  }

  if ((command >= 1U) && (command <= 3U))
  {
    if (AS5600_DebugReadingValid(debug_as5600_left_comm_ok,
                                 debug_as5600_left_status) == 0U)
    {
      debug_as5600_cal_last_result = 2U;
      return;
    }

    if (command == 1U)
    {
      debug_as5600_left_min_raw = debug_as5600_left_raw;
      debug_as5600_left_min_valid = 1U;
    }
    else if (command == 2U)
    {
      debug_as5600_left_center_raw = debug_as5600_left_raw;
      debug_as5600_left_center_valid = 1U;
    }
    else
    {
      debug_as5600_left_max_raw = debug_as5600_left_raw;
      debug_as5600_left_max_valid = 1U;
    }

    AS5600_DebugRecomputeCalibration();
    debug_as5600_cal_last_result = 1U;
    return;
  }

  if ((command >= 4U) && (command <= 6U))
  {
    if (AS5600_DebugReadingValid(debug_as5600_right_comm_ok,
                                 debug_as5600_right_status) == 0U)
    {
      debug_as5600_cal_last_result = 3U;
      return;
    }

    if (command == 4U)
    {
      debug_as5600_right_min_raw = debug_as5600_right_raw;
      debug_as5600_right_min_valid = 1U;
    }
    else if (command == 5U)
    {
      debug_as5600_right_center_raw = debug_as5600_right_raw;
      debug_as5600_right_center_valid = 1U;
    }
    else
    {
      debug_as5600_right_max_raw = debug_as5600_right_raw;
      debug_as5600_right_max_valid = 1U;
    }

    AS5600_DebugRecomputeCalibration();
    debug_as5600_cal_last_result = 1U;
  }
}


static void AS5600_DebugUpdate(void)
{
  uint16_t raw;
  uint8_t status;
  HAL_StatusTypeDef ready_result;
  HAL_StatusTypeDef read_result;

  /*
   * Probe ACK ở đúng địa chỉ AS5600 trước khi đọc register.
   * Việc này chỉ phục vụ debug phần cứng/I2C trong giai đoạn calibration.
   */
  if (as5600_left_probe_enabled != 0U)
  {
    debug_as5600_left_probe_blocked = 0U;

    ready_result = HAL_I2C_IsDeviceReady(As5600_Left.hi2c,
                                         AS5600_I2C_ADDRESS_HAL,
                                         1U,
                                         2U);
    debug_as5600_left_ready_status = (uint8_t)ready_result;
    debug_as5600_left_device_ready = (ready_result == HAL_OK) ? 1U : 0U;
    debug_as5600_left_ready_error = HAL_I2C_GetError(As5600_Left.hi2c);

    read_result = AS5600_ReadRawAndStatus(&As5600_Left, &raw, &status);
    debug_as5600_left_read_status = (uint8_t)read_result;
    debug_as5600_left_read_error = HAL_I2C_GetError(As5600_Left.hi2c);
    debug_as5600_left_i2c_state = (uint32_t)HAL_I2C_GetState(As5600_Left.hi2c);

    if (read_result == HAL_OK)
    {
      debug_as5600_left_raw = raw;
      debug_as5600_left_deg = AS5600_RawToDegrees(raw);
      debug_as5600_left_status = status;
      debug_as5600_left_comm_ok = 1U;
      debug_as5600_left_magnet_detected =
          ((status & AS5600_STATUS_MD) != 0U) ? 1U : 0U;
      debug_as5600_left_magnet_too_weak =
          ((status & AS5600_STATUS_ML) != 0U) ? 1U : 0U;
      debug_as5600_left_magnet_too_strong =
          ((status & AS5600_STATUS_MH) != 0U) ? 1U : 0U;
      debug_as5600_left_read_ok_count++;
    }
    else
    {
      debug_as5600_left_comm_ok = 0U;
      debug_as5600_left_read_fail_count++;
    }
  }
  else
  {
    /* Startup bus check failed: preserve that condition without repeated traffic. */
    debug_as5600_left_probe_blocked = 1U;
    debug_as5600_left_device_ready = 0U;
    debug_as5600_left_comm_ok = 0U;
    debug_as5600_left_ready_status = AS5600_DEBUG_NOT_ATTEMPTED;
    debug_as5600_left_ready_error = HAL_I2C_ERROR_NONE;
    debug_as5600_left_read_status = AS5600_DEBUG_NOT_ATTEMPTED;
    debug_as5600_left_read_error = HAL_I2C_ERROR_NONE;
    debug_as5600_left_i2c_state = (uint32_t)HAL_I2C_GetState(As5600_Left.hi2c);
  }

  if (as5600_right_probe_enabled != 0U)
  {
    debug_as5600_right_probe_blocked = 0U;

    ready_result = HAL_I2C_IsDeviceReady(As5600_Right.hi2c,
                                         AS5600_I2C_ADDRESS_HAL,
                                         1U,
                                         2U);
    debug_as5600_right_ready_status = (uint8_t)ready_result;
    debug_as5600_right_device_ready = (ready_result == HAL_OK) ? 1U : 0U;
    debug_as5600_right_ready_error = HAL_I2C_GetError(As5600_Right.hi2c);

    read_result = AS5600_ReadRawAndStatus(&As5600_Right, &raw, &status);
    debug_as5600_right_read_status = (uint8_t)read_result;
    debug_as5600_right_read_error = HAL_I2C_GetError(As5600_Right.hi2c);
    debug_as5600_right_i2c_state = (uint32_t)HAL_I2C_GetState(As5600_Right.hi2c);

    if (read_result == HAL_OK)
    {
      debug_as5600_right_raw = raw;
      debug_as5600_right_deg = AS5600_RawToDegrees(raw);
      debug_as5600_right_status = status;
      debug_as5600_right_comm_ok = 1U;
      debug_as5600_right_magnet_detected =
          ((status & AS5600_STATUS_MD) != 0U) ? 1U : 0U;
      debug_as5600_right_magnet_too_weak =
          ((status & AS5600_STATUS_ML) != 0U) ? 1U : 0U;
      debug_as5600_right_magnet_too_strong =
          ((status & AS5600_STATUS_MH) != 0U) ? 1U : 0U;
      debug_as5600_right_read_ok_count++;
    }
    else
    {
      debug_as5600_right_comm_ok = 0U;
      debug_as5600_right_read_fail_count++;
    }
  }
  else
  {
    debug_as5600_right_probe_blocked = 1U;
    debug_as5600_right_device_ready = 0U;
    debug_as5600_right_comm_ok = 0U;
    debug_as5600_right_ready_status = AS5600_DEBUG_NOT_ATTEMPTED;
    debug_as5600_right_ready_error = HAL_I2C_ERROR_NONE;
    debug_as5600_right_read_status = AS5600_DEBUG_NOT_ATTEMPTED;
    debug_as5600_right_read_error = HAL_I2C_ERROR_NONE;
    debug_as5600_right_i2c_state = (uint32_t)HAL_I2C_GetState(As5600_Right.hi2c);
  }

  AS5600_DebugProcessCaptureCommand();
}


/*
 * M7-S01 -- input-only limit switch monitor. No I2C, motor PWM, blocking
 * delays or UART traffic. Polling debounces actual state; EXTI counts edges.
 * A stuck/missing NC wire reads HIGH, so it is treated as active, not safe.
 */
static void SteeringLimits_DebugSampleOne(volatile SteeringLimitDebug_t *state,
                                          GPIO_TypeDef *port,
                                          uint16_t pin,
                                          uint32_t now)
{
  uint8_t raw = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1U : 0U;
  state->raw = raw;

  if (state->initialized == 0U)
  {
    state->candidate = raw;
    state->active = raw;
    state->candidate_since_ms = now;
    state->initialized = 1U;
    return;
  }

  if (raw != state->candidate)
  {
    state->candidate = raw;
    state->candidate_since_ms = now;
  }
  else if ((raw != state->active) &&
           ((uint32_t)(now - state->candidate_since_ms) >=
            STEERING_LIMIT_DEBOUNCE_MS))
  {
    state->active = raw;
    if (raw != 0U)
    {
      state->activate_count++;
    }
    else
    {
      state->release_count++;
    }
  }
}

static void SteeringLimits_DebugUpdate(void)
{
  uint8_t raw_mask;
  uint8_t active_mask;
  uint32_t now = HAL_GetTick();

  SteeringLimits_DebugSampleOne(&debug_limit_r_max,
                                LIMIT_R_MAX_GPIO_Port, LIMIT_R_MAX_Pin, now);
  SteeringLimits_DebugSampleOne(&debug_limit_r_min,
                                LIMIT_R_MIN_GPIO_Port, LIMIT_R_MIN_Pin, now);
  SteeringLimits_DebugSampleOne(&debug_limit_l_min,
                                LIMIT_L_MIN_GPIO_Port, LIMIT_L_MIN_Pin, now);
  SteeringLimits_DebugSampleOne(&debug_limit_l_max,
                                LIMIT_L_MAX_GPIO_Port, LIMIT_L_MAX_Pin, now);

  raw_mask = (uint8_t)((debug_limit_r_max.raw << 0) |
                       (debug_limit_r_min.raw << 1) |
                       (debug_limit_l_min.raw << 2) |
                       (debug_limit_l_max.raw << 3));
  active_mask = (uint8_t)((debug_limit_r_max.active << 0) |
                          (debug_limit_r_min.active << 1) |
                          (debug_limit_l_min.active << 2) |
                          (debug_limit_l_max.active << 3));
  debug_limit_raw_mask = raw_mask;
  debug_limit_active_mask = active_mask;
  debug_limit_same_side_both_active =
      (uint8_t)((((active_mask & 0x03U) == 0x03U) ? 0x01U : 0U) |
                (((active_mask & 0x0CU) == 0x0CU) ? 0x02U : 0U));
}



/*
 * M7-S02: mutually exclusive steering diagnostics, manual one-shot.
 * Direction map confirmed: LEFT DIR0->L_MIN, LEFT DIR1->L_MAX,
 * RIGHT DIR0->R_MIN. RIGHT DIR1->R_MAX remains to be verified.
 * Escape from active L_MAX (LEFT DIR0), L_MIN (LEFT DIR1),
 * or R_MIN (RIGHT DIR1) is <=1s; stop as soon as that switch releases.
 * Any newly rising limit cuts PWM; PWM=0 is not a hard supply cutoff.
 */
static void Steering_TestOutputsOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
  steer_test_pwm_applied = 0U;
}

static uint8_t Steering_TestRawLimitMask(void)
{
  return (uint8_t)(
      (((HAL_GPIO_ReadPin(LIMIT_R_MAX_GPIO_Port, LIMIT_R_MAX_Pin) ==
          GPIO_PIN_SET) ? 1U : 0U) << 0) |
      (((HAL_GPIO_ReadPin(LIMIT_R_MIN_GPIO_Port, LIMIT_R_MIN_Pin) ==
          GPIO_PIN_SET) ? 1U : 0U) << 1) |
      (((HAL_GPIO_ReadPin(LIMIT_L_MIN_GPIO_Port, LIMIT_L_MIN_Pin) ==
          GPIO_PIN_SET) ? 1U : 0U) << 2) |
      (((HAL_GPIO_ReadPin(LIMIT_L_MAX_GPIO_Port, LIMIT_L_MAX_Pin) ==
          GPIO_PIN_SET) ? 1U : 0U) << 3));
}

static uint16_t Steering_TestEncoderCount(uint8_t command)
{
  if ((command == 1U) || (command == 2U))
  {
    return (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
  }
  if ((command == 3U) || (command == 4U))
  {
    return (uint16_t)hlptim1.Instance->CNT;
  }
  return 0U;
}

static void Steering_TestFinish(uint8_t result)
{
  /* PWM=0 first, including in the EXTI handler; not power-disconnect. */
  Steering_TestOutputsOff();
  if (steer_test_running != 0U)
  {
    steer_test_elapsed_ms = (uint32_t)(HAL_GetTick() - steer_test_start_tick);
    steer_test_encoder_delta = (int16_t)(
        (uint16_t)(Steering_TestEncoderCount(steer_test_last_command) -
                   steer_test_encoder_start));
  }
  steer_test_running = 0U;
  steer_test_escape_mask = 0U;
  steer_test_left_escape_active = 0U;
  steer_test_result = result;
}

/*
 * A bitmask is collected once in the same cycle that causes a refusal
 * or abort. This avoids inspecting live flags AFTER they have returned
 * to normal (which previously obscured reason=5).
 *
 * bit 0: control_mode != IDLE      bit 1: STOP latch
 * bit 2: PS2 manual input          bit 3: PS2 cancel-hold
 * bit 4: voice execution           bit 5: voice command ready
 * bit 6: nonzero wheel cmd PWM     bit 7: nonzero actual wheel PWM
 * bit 8: nonzero wheel target RPM  bit 9: FF calibration active
 * bit10: wheel logger recording    bit11: wheel CSV dump/request
 * bit12: AS5600 polling enabled    bit13: at least one wheel moving
 */
static uint32_t Steering_TestUnsafeMask(void)
{
  uint32_t mask = 0U;

  if (control_mode != CONTROL_MODE_IDLE) mask |= (1UL << 0);
  if (control_stop_latched != 0U) mask |= (1UL << 1);
  if (ps2_manual_active != 0U) mask |= (1UL << 2);
  if (ps2_cancel_hold != 0U) mask |= (1UL << 3);
  if (voice_execution_active != 0U) mask |= (1UL << 4);
  if (voice_command_ready != 0U) mask |= (1UL << 5);
  if ((wheel_target_pwm != 0) || (wheel_current_pwm != 0))
    mask |= (1UL << 6);
  if ((wheel_pwm_lu != 0) || (wheel_pwm_ld != 0) ||
      (wheel_pwm_ru != 0) || (wheel_pwm_rd != 0))
    mask |= (1UL << 7);
  if ((wheel_pid_target_rpm > 0.5f) ||
      (wheel_pid_target_rpm < -0.5f))
    mask |= (1UL << 8);
  if (wheel_ff_test_active != 0U) mask |= (1UL << 9);
  if (wheel_log_active != 0U) mask |= (1UL << 10);
  if ((wheel_log_dump_request != 0U) ||
      (wheel_log_dump_active != 0U))
    mask |= (1UL << 11);
  if (debug_as5600_poll_enable != 0U) mask |= (1UL << 12);

  if ((Motor_LeftUp.current_rpm > 2.0f) ||
      (Motor_LeftUp.current_rpm < -2.0f) ||
      (Motor_LeftDown.current_rpm > 2.0f) ||
      (Motor_LeftDown.current_rpm < -2.0f) ||
      (Motor_RightUp.current_rpm > 2.0f) ||
      (Motor_RightUp.current_rpm < -2.0f) ||
      (Motor_RightDown.current_rpm > 2.0f) ||
      (Motor_RightDown.current_rpm < -2.0f))
  {
    mask |= (1UL << 13);
  }

  return mask;
}

static uint8_t Steering_TestLimitsClear(void)
{
  if ((debug_limit_r_max.initialized == 0U) ||
      (debug_limit_r_min.initialized == 0U) ||
      (debug_limit_l_min.initialized == 0U) ||
      (debug_limit_l_max.initialized == 0U) ||
      (debug_limit_active_mask != 0U) ||
      (debug_limit_raw_mask != 0U) ||
      (Steering_TestRawLimitMask() != 0U))
  {
    return 0U;
  }
  return 1U;
}

/* Only permit a departure when the exact opposite limit is active:
 * cmd1 LEFT DIR0 can leave L_MAX; cmd2 LEFT DIR1 can leave L_MIN;
 * cmd4 RIGHT DIR1 can leave R_MIN. All other limits must be clear.
 * A nonzero return value is the single allowed initial active bit.
 */
static uint8_t Steering_TestCanEscapeMask(uint8_t cmd)
{
  uint8_t mask = 0U;
  if (cmd == 1U) mask = STEER_LIMIT_L_MAX_MASK;
  else if (cmd == 2U) mask = STEER_LIMIT_L_MIN_MASK;
  else if (cmd == 4U) mask = STEER_LIMIT_R_MIN_MASK;

  if ((mask == 0U) ||
      (debug_limit_r_max.initialized == 0U) ||
      (debug_limit_r_min.initialized == 0U) ||
      (debug_limit_l_min.initialized == 0U) ||
      (debug_limit_l_max.initialized == 0U))
    return 0U;

  return ((debug_limit_active_mask == mask) &&
          (debug_limit_raw_mask == mask) &&
          (Steering_TestRawLimitMask() == mask)) ? mask : 0U;
}

/* During escape only the ORIGINALLY active limit may stay HIGH.
 * Another active/open limit or wire fault cancels this run.
 */
static uint8_t Steering_TestEscapeLimitsSafe(uint8_t mask)
{
  uint8_t raw = Steering_TestRawLimitMask();
  if (mask == 0U) return 0U;
  return ((raw == mask) &&
          ((debug_limit_raw_mask & (uint8_t)~mask) == 0U) &&
          ((debug_limit_active_mask & (uint8_t)~mask) == 0U)) ? 1U : 0U;
}

/* Once a departure is observed, record the released switch and require
 * a separate manual request. Do not start a full run automatically.
 */
static void Steering_TestEscapeSucceeded(void)
{
  uint8_t released_mask = steer_test_escape_mask;
  if ((released_mask == STEER_LIMIT_L_MIN_MASK) &&
      (steer_test_last_command == 2U))
    steer_test_left_dir1_verified = 1U;
  if ((released_mask == STEER_LIMIT_R_MIN_MASK) &&
      (steer_test_last_command == 4U))
    steer_test_right_dir1_verified = 1U;
  steer_test_exit_released_mask = released_mask;
  Steering_TestFinish(7U);
}

static void Steering_TestPulseUpdate(void)
{
  uint8_t cmd = steer_test_request;
  uint32_t now = HAL_GetTick();
  uint32_t previous_primask;
  uint32_t unsafe_mask;
  uint8_t escape_mask = 0U;

  if (cmd != 0U)
  {
    steer_test_request = 0U;
    if (steer_test_running != 0U)
    {
      steer_test_abort_cause = 1U; /* repeated command while still running */
      steer_test_abort_reason_mask = 0U;
      Steering_TestFinish(5U); /* new request aborts instead of extending run */
      steer_test_arm = 0U;
      return;
    }

    /* Direction 4 is enabled only for a short R_MIN release or
     * subsequently verified RIGHT DIR1 full travel. */
    if ((cmd < 1U) || (cmd > 4U))
    {
      steer_test_arm = 0U;
      steer_test_result = 6U;
      steer_test_rejected_count++;
      return;
    }

    unsafe_mask = Steering_TestUnsafeMask();
    escape_mask = Steering_TestCanEscapeMask(cmd);
    if ((steer_test_arm == 0U) ||
        (unsafe_mask != 0U) ||
        ((escape_mask == 0U) &&
         ((Steering_TestLimitsClear() == 0U) ||
          ((cmd == 2U) && (steer_test_left_dir1_verified == 0U)) ||
          ((cmd == 4U) && (steer_test_right_dir1_verified == 0U)))))
    {
      steer_test_arm = 0U;
      steer_test_result = 3U;
      steer_test_rejected_count++;
      return;
    }

    /* One command consumes one arm. Must arm again before next run. */
    steer_test_arm = 0U;
    if ((cmd == 1U) || (escape_mask == STEER_LIMIT_L_MIN_MASK))
      steer_test_left_dir1_verified = 0U;
    if ((cmd == 3U) || (escape_mask == STEER_LIMIT_R_MIN_MASK))
      steer_test_right_dir1_verified = 0U;
    steer_test_escape_mask = escape_mask;
    steer_test_left_escape_active =
        (escape_mask == STEER_LIMIT_L_MIN_MASK) ? 1U : 0U;
    steer_test_exit_released_mask = 0U;
    steer_test_abort_cause = 0U;
    steer_test_abort_reason_mask = 0U;
    steer_test_last_command = cmd;
    steer_test_trip_mask = 0U;
    steer_test_encoder_start = Steering_TestEncoderCount(cmd);
    steer_test_encoder_delta = 0;
    steer_test_start_tick = now;
    steer_test_elapsed_ms = 0U;
    steer_test_running = 1U;
    steer_test_result = 1U;
    steer_test_run_count++;
  }

  if (steer_test_abort != 0U)
  {
    steer_test_abort = 0U;
    steer_test_arm = 0U;
    if (steer_test_running != 0U)
    {
      steer_test_abort_cause = 2U;
      steer_test_abort_reason_mask = 0U;
      Steering_TestFinish(5U);
    }
    return;
  }

  if (steer_test_running == 0U) return;

  steer_test_elapsed_ms = (uint32_t)(now - steer_test_start_tick);
  steer_test_encoder_delta = (int16_t)(
      (uint16_t)(Steering_TestEncoderCount(steer_test_last_command) -
                 steer_test_encoder_start));

  if (steer_test_escape_mask != 0U)
  {
    uint8_t raw = Steering_TestRawLimitMask();
    if ((raw == 0U) &&
        ((debug_limit_active_mask &
          (uint8_t)~steer_test_escape_mask) == 0U))
    {
      Steering_TestEscapeSucceeded(); /* limit released, no auto travel */
      return;
    }
    if (Steering_TestEscapeLimitsSafe(steer_test_escape_mask) == 0U)
    {
      steer_test_trip_mask |= raw;
      steer_test_limit_stop_count++;
      Steering_TestFinish(4U);
      return;
    }
  }

  if (steer_test_elapsed_ms >=
      ((steer_test_escape_mask != 0U) ?
        STEER_TEST_ESCAPE_MAX_MS :
       ((steer_test_last_command <= 2U) ?
        STEER_TEST_LEFT_MAX_RUN_MS : STEER_TEST_RIGHT_MAX_RUN_MS)))
  {
    Steering_TestFinish((steer_test_escape_mask != 0U) ? 8U : 2U);
    return;
  }

  unsafe_mask = Steering_TestUnsafeMask();
  if (unsafe_mask != 0U)
  {
    /* Latch the exact unsafe state BEFORE the control flags can clear. */
    steer_test_abort_cause = 3U;
    steer_test_abort_reason_mask = unsafe_mask;
    Steering_TestFinish(5U);
    return;
  }

  /*
   * Atomic raw-input validation plus PWM write: EXTI cannot intervene
   * between the limit check and compare update. After IRQ unmask any
   * pending rising-edge callback stops the PWM immediately.
   */
  previous_primask = __get_PRIMASK();
  __disable_irq();
  if ((steer_test_running != 0U) &&
      (((steer_test_escape_mask != 0U) ?
        Steering_TestEscapeLimitsSafe(steer_test_escape_mask) :
        Steering_TestLimitsClear()) != 0U))
  {
    GPIO_TypeDef *dir_port = (steer_test_last_command <= 2U) ?
        MOTOR_DIR_TURN_left_GPIO_Port : MOTOR_DIR_TURN_right_GPIO_Port;
    uint16_t dir_pin = (steer_test_last_command <= 2U) ?
        MOTOR_DIR_TURN_left_Pin : MOTOR_DIR_TURN_right_Pin;
    GPIO_PinState dir_level =
        ((steer_test_last_command == 2U) ||
         (steer_test_last_command == 4U)) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(dir_port, dir_pin, dir_level);
    if (steer_test_last_command <= 2U)
    {
      __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, STEER_TEST_LEFT_PWM_DUTY);
      steer_test_pwm_applied = STEER_TEST_LEFT_PWM_DUTY;
    }
    else /* RIGHT DIR0 or DIR1 */
    {
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, STEER_TEST_RIGHT_PWM_DUTY);
      steer_test_pwm_applied = STEER_TEST_RIGHT_PWM_DUTY;
    }
  }
  else
  {
    uint8_t raw = Steering_TestRawLimitMask();
    if ((steer_test_escape_mask != 0U) && (raw == 0U) &&
        ((debug_limit_active_mask &
          (uint8_t)~steer_test_escape_mask) == 0U))
    {
      Steering_TestEscapeSucceeded();
    }
    else
    {
      steer_test_trip_mask |= raw;
      steer_test_limit_stop_count++;
      Steering_TestFinish(4U);
    }
  }
  if (previous_primask == 0U) __enable_irq();
}

/* Invoked from HAL_GPIO_EXTI_Callback on a rising NC-open edge. */
static void Steering_TestOnLimitInterrupt(uint8_t switch_mask)
{
  if (steer_test_running != 0U)
  {
    steer_test_trip_mask |= switch_mask;
    steer_test_limit_stop_count++;
    Steering_TestFinish(4U);
  }
}




/* M7-S05 PS2 direct steering (D-pad L/R and analog RX), separate from R1/R2.
 * Requires manual arming on first bench test. Raw input remains observable
 * even when disarmed. Timer ARR=999; these are CCR ticks, not percentages.
 * LEFT has distinct measured breakaway torque from RIGHT.
 */
volatile uint8_t steer_manual_enable = 0U;
/* Declared early for arbitration/PWM ownership. Initialized in the
 * select-calibration section below and defaults DISARMED on reset.
 */
volatile uint8_t steer_cal_active = 0U;
volatile uint8_t steer_manual_active = 0U;
volatile int8_t steer_manual_direction = 0; /* -1 toward MIN; +1 toward MAX */
volatile uint8_t steer_manual_neutral_required = 0U;
/* 0 none, 1 disarmed, 2 priority/diag, 4 wheels not stopped,
 * 5 neutral required, 6 limits not initialized, 7 contradictory D-pad.
 */
volatile uint8_t steer_manual_block_reason = 0U;
volatile uint16_t steer_manual_left_soft_target = 0U;
volatile uint16_t steer_manual_right_soft_target = 0U;
volatile int16_t steer_manual_left_soft_remaining = 0;
volatile int16_t steer_manual_right_soft_remaining = 0;
volatile uint32_t steer_manual_left_soft_stop_count = 0U;
volatile uint32_t steer_manual_right_soft_stop_count = 0U;
/* Sticky snapshots record the exact position/NC bits at last per-axis stop. */
volatile uint16_t steer_manual_left_stop_raw = 0U;
volatile uint16_t steer_manual_right_stop_raw = 0U;
volatile uint8_t steer_manual_left_stop_limit_mask = 0U;
volatile uint8_t steer_manual_right_stop_limit_mask = 0U;
volatile uint8_t steer_manual_left_state = 0U;
volatile uint8_t steer_manual_right_state = 0U;
volatile uint8_t steer_manual_left_result = 0U;
volatile uint8_t steer_manual_right_result = 0U;
volatile uint16_t steer_manual_left_pwm = 0U;
volatile uint16_t steer_manual_right_pwm = 0U;
volatile uint16_t steer_manual_left_dpad_pwm = 300U;
volatile uint16_t steer_manual_right_dpad_pwm = 100U;
volatile uint16_t steer_manual_left_rx_min_pwm = 175U;
volatile uint16_t steer_manual_right_rx_min_pwm = 50U;
volatile uint16_t steer_manual_left_rx_max_pwm = 300U;
volatile uint16_t steer_manual_right_rx_max_pwm = 100U;
/* RIGHT DIR0 toward MIN was established before M7-S05. Permit only a
 * short, AS5600-verified departure from R_MAX; NEVER a full stroke from
 * an active NC limit. This remains an observable runtime safety gate.
 */
volatile uint8_t steer_manual_allow_right_max_escape = 1U;
/* Combined LY wheel drive + RX steering is a later bench validation gate. */
volatile uint8_t steer_manual_allow_wheel_motion = 0U;
volatile uint32_t steer_manual_start_count = 0U;
volatile uint32_t steer_manual_stop_count = 0U;
volatile uint32_t steer_manual_reject_count = 0U;
volatile uint32_t steer_manual_irq_count = 0U;
volatile uint8_t steer_manual_irq_trip_mask = 0U;
static volatile uint8_t steer_manual_irq_pending_mask = 0U;

/* Kept in physical-meaning order: -1 = MIN, +1 = MAX. */
static int8_t Steering_ManualRequestDirection(void)
{
  int16_t rx;
  if ((PS2_Pad.Connected != 1U) || (PS2_Pad.Mode != 0x73U))
    return 0;
  if ((PS2_Pad.Left != 0U) && (PS2_Pad.Right != 0U))
    return 0; /* contradiction; require neutral to rearm */
  if (PS2_Pad.Left != 0U) return -1;
  if (PS2_Pad.Right != 0U) return 1;
  rx = (int16_t)PS2_Pad.RX - 128;
  if (rx < -10) return -1;
  if (rx > 10) return 1;
  return 0;
}

static uint8_t Steering_ManualInputsNeutral(void)
{
  int16_t rx = (int16_t)PS2_Pad.RX - 128;
  return ((PS2_Pad.Connected == 1U) &&
          (PS2_Pad.Mode == 0x73U) &&
          (PS2_Pad.Left == 0U) && (PS2_Pad.Right == 0U) &&
          (rx >= -10) && (rx <= 10)) ? 1U : 0U;
}

/*
 * M7-S04B: independent left/right PS2 R1/R2 steering position control.
 * Default DISARMED; enabling requires suspended bench and verified sensors.
 *
 * R1 = CENTER: independent move; if initially touching MIN, do only a
 * short release of that side then stop and require a NEW R1 press.
 * R2 = MIN: each axis independently stops at its own NC MIN limit,
 * or at its AS5600 calibrated MIN ±5 counts (distinct arrival reason).
 * A correctly triggered MIN of one side MUST NOT stop the other side.
 * An unexpected limit/fault stops the affected side; systemic STOP,
 * RF loss, loss of arbitration or manual diagnostic conflict stops both.
 *
 * AS5600 live values are refreshed while steering is active; when idle
 * one of the two sensors is polled per bounded service interval, without
 * requiring debug_as5600_poll_enable or the heavyweight debug probe.
 */
/* M7-S07: calibrated MIN/MAX are runtime values, not preprocessor
 * constants. Defaults are the previously measured bench values.
 * These RAM values change ONLY after a COMPLETE, valid Select scan.
 * CENTER is intentionally unchanged and never learned.
 */
volatile uint16_t steer_cal_left_min_runtime = 2319U;
volatile uint16_t steer_cal_left_max_runtime = 1391U;
volatile uint16_t steer_cal_right_min_runtime = 2934U;
volatile uint16_t steer_cal_right_max_runtime = 3680U;
#define STEER_POS_LEFT_MIN_RAW       (steer_cal_left_min_runtime)
#define STEER_POS_LEFT_CENTER_RAW     1857U
#define STEER_POS_LEFT_MAX_RAW       (steer_cal_left_max_runtime)
#define STEER_POS_RIGHT_MIN_RAW      (steer_cal_right_min_runtime)
#define STEER_POS_RIGHT_CENTER_RAW    3398U
#define STEER_POS_RIGHT_MAX_RAW      (steer_cal_right_max_runtime)
#define STEER_POS_TOL_RAW             5 /* R2 MIN unchanged */
/* M7-S08: R1 CENTER is independent of Select CENTER capture.
 * R1 fine correction accepts at most ±5 AS5600 RAW; Select retains
 * separate 15/20-count calibration capture with no CENTER learning.
 */
volatile uint16_t steer_pos_left_center_tol_raw = 5U;
volatile uint16_t steer_pos_right_center_tol_raw = 5U;
static uint16_t Steering_CenterFineTolClamp(uint16_t value)
{
  if (value < 3U) return 3U;
  return (value > 5U) ? 5U : value;
}
static uint16_t Steering_CenterTolClamp(uint16_t value)
{
  if (value < 8U) return 8U;
  return (value > 30U) ? 30U : value;
}
#define STEER_POS_RANGE_MARGIN      64 /* limited overshoot recovery envelope */
#define STEER_POS_MAX_JUMP_RAW      90
#define STEER_POS_TIMEOUT_MS      5000U /* R2 MIN unchanged */
#define STEER_POS_R1_TIMEOUT_MS   9000U /* coarse + short-pulse trim */
#define STEER_POS_ESCAPE_MS       1000U
#define STEER_POS_STALL_MS         650U
#define STEER_POS_IDLE_POLL_MS      50U
#define STEER_POS_RX_NEUTRAL_DELTA  10U
#define STEER_POS_STABLE_SAMPLES     3U
/* Nonblocking R1-only fine position controller. PWM outputs are OFF
 * while coasting/quiet capture; bounded timed motor pulses then settle.
 * A failed tight ±5 result is reported as error, never claimed as PASS.
 * R2 MIN retains its original controller and ±5 comparator.
 */
#define STEER_POS_FINE_ENTER_RAW       35U
#define STEER_POS_FINE_ABORT_RAW       90U
#define STEER_POS_FINE_OFF_MIN_MS     200U
#define STEER_POS_FINE_QUIET_MS       100U
#define STEER_POS_FINE_CAPTURE_MS     120U
#define STEER_POS_FINE_MAX_PULSES       8U
#define STEER_POS_FINE_PHASE_IDLE       0U
#define STEER_POS_FINE_PHASE_SETTLE     1U
#define STEER_POS_FINE_PHASE_PULSE      2U
#define STEER_POS_FINE_RESULT_FAIL     10U

#define STEER_AXIS_IDLE     0U
#define STEER_AXIS_MOVING   1U
#define STEER_AXIS_ESCAPING 2U
#define STEER_AXIS_DONE     3U
#define STEER_AXIS_ERROR    4U

/* Per-axis result: 0 pending, 2 reached AS5600, 3 hit expected MIN,
 * 4 sensor fault, 5 timeout, 7 escaped MIN (press R1 again), 8 stalled
 * or went the wrong way, 9 unexpected limit / arbitration cancelled.
 */
volatile uint8_t steer_pos_enable = 0U;
volatile uint8_t steer_pos_active = 0U;
volatile uint8_t steer_pos_result = 0U;
/* Why a fresh R1/R2 press was rejected:
 * 0 accepted; 1 disarmed; 2 PS2 lost/digital; 3 STOP/Voice/priority;
 * 4 conflicting PS2 movement inputs; 5 wheels not settled;
 * 6 debug/diagnostic steering active; 7 limit input not initialized;
 * 8 R1+R2 together; 9 manual PWM owner still active.
 */
volatile uint8_t steer_pos_block_reason = 0U;
volatile uint8_t steer_pos_command = 0U;
volatile uint8_t steer_pos_escape_mask = 0U;
volatile uint8_t steer_pos_buttons = 0U;
volatile uint16_t steer_pos_left_raw = 0U;
volatile uint16_t steer_pos_right_raw = 0U;
volatile int16_t steer_pos_left_error = 0;
volatile int16_t steer_pos_right_error = 0;
volatile uint16_t steer_pos_left_pwm = 0U;
volatile uint16_t steer_pos_right_pwm = 0U;
volatile uint32_t steer_pos_run_count = 0U;
volatile uint32_t steer_pos_reject_count = 0U;
volatile uint32_t steer_pos_finish_count = 0U;
volatile uint8_t steer_pos_limit_trip_mask = 0U;

/* Each motor has its OWN measured operating PWM range. Compare ticks,
 * ARR=999, NOT percentages. Independent of old manual M7-S02 PWM.
 * User tested LEFT 175..300 counts; RIGHT retains initial 50..100.
 * These are Live Expressions tunables, clamped below on every use.
 */
volatile uint16_t steer_pos_left_pwm_min = 175U;
volatile uint16_t steer_pos_left_pwm_max = 300U;
volatile uint16_t steer_pos_right_pwm_min = 50U;
volatile uint16_t steer_pos_right_pwm_max = 100U;
/* Fine-tune LEFT and RIGHT independently, compare ticks ARR=999.
 * Short pulses intentionally start low/brief; fine trim is NOT
 * guaranteed on high-static-friction hardware. Escalation must be
 * validated on a supported chassis, not silently forced.
 */
volatile uint16_t steer_pos_left_fine_pwm = 175U;
volatile uint16_t steer_pos_right_fine_pwm = 85U;
volatile uint16_t steer_pos_left_fine_pulse_ms = 30U;
volatile uint16_t steer_pos_right_fine_pulse_ms = 40U;
volatile uint8_t steer_pos_left_fine_phase = 0U;
volatile uint8_t steer_pos_right_fine_phase = 0U;
volatile uint8_t steer_pos_left_fine_pulses = 0U;
volatile uint8_t steer_pos_right_fine_pulses = 0U;
volatile int16_t steer_pos_left_fine_error = 0;
volatile int16_t steer_pos_right_fine_error = 0;
volatile uint16_t steer_pos_left_center_final_raw = 0U;
volatile uint16_t steer_pos_right_center_final_raw = 0U;
volatile uint8_t steer_pos_left_fine_fault = 0U;
volatile uint8_t steer_pos_right_fine_fault = 0U;
volatile uint8_t steer_pos_left_state = STEER_AXIS_IDLE;
volatile uint8_t steer_pos_right_state = STEER_AXIS_IDLE;
volatile uint8_t steer_pos_left_result = 0U;
volatile uint8_t steer_pos_right_result = 0U;
volatile uint32_t steer_pos_left_last_sample_ms = 0U;
volatile uint32_t steer_pos_right_last_sample_ms = 0U;
volatile uint32_t steer_pos_left_new_sample_count = 0U;
volatile uint32_t steer_pos_right_new_sample_count = 0U;

typedef struct
{
  uint16_t last_raw;
  uint16_t best_abs_error;
  uint32_t progress_ms;
  uint32_t escape_start_ms;
  uint16_t escape_start_raw;
  uint8_t escape_origin;  /* own MIN or MAX NC mask at start */
  uint8_t stable_count;
  uint8_t fine_phase;
  uint8_t fine_pulses;
  uint8_t fine_dir0;
  int16_t fine_start_error;
  uint16_t fine_quiet_raw;
  uint32_t fine_phase_start_ms;
  uint32_t fine_quiet_since_ms;
  uint32_t fine_capture_since_ms;
} SteeringPositionAxis_t;

static SteeringPositionAxis_t steer_left_axis = {0};
static SteeringPositionAxis_t steer_right_axis = {0};
static uint8_t steer_pos_previous_buttons = 0U;
static uint32_t steer_pos_start_ms = 0U;
static uint32_t steer_pos_idle_last_ms = 0U;
static uint8_t steer_pos_idle_next_side = 0U;
/* EXTI sets ONLY its own side's bit and zeros its own channel. */
static volatile uint8_t steer_pos_irq_pending_mask = 0U;

static uint16_t Steering_PositionClampDuty(uint16_t value)
{
  return (value > 400U) ? 400U : value;
}

static uint16_t Steering_PositionDuty(int16_t err, uint16_t min_duty,
                                     uint16_t max_duty, uint16_t divisor)
{
  uint32_t v;
  uint16_t magnitude = (uint16_t)((err < 0) ? -err : err);
  min_duty = Steering_PositionClampDuty(min_duty);
  max_duty = Steering_PositionClampDuty(max_duty);
  if (max_duty < min_duty) max_duty = min_duty;
  v = (uint32_t)min_duty + (uint32_t)magnitude / divisor;
  return (uint16_t)((v > max_duty) ? max_duty : v);
}

/* Single bounded HAL transaction. Keep debug RAW/MD/ML/MH fresh both
 * during position control and outside it. No separate debug enable
 * and no IsDeviceReady probe in this fast path.
 */
static uint8_t Steering_PositionReadOne(uint8_t left, uint16_t *raw)
{
  AS5600_HandleTypeDef *sensor = left ? &As5600_Left : &As5600_Right;
  uint8_t status = 0U;
  uint16_t value = 0U;
  uint32_t now = HAL_GetTick();
  HAL_StatusTypeDef result = HAL_ERROR;

  if (((left != 0U) && (as5600_left_probe_enabled != 0U)) ||
      ((left == 0U) && (as5600_right_probe_enabled != 0U)))
  {
    result = AS5600_ReadRawAndStatus(sensor, &value, &status);
  }

  if (left != 0U)
  {
    debug_as5600_left_comm_ok = (result == HAL_OK) ? 1U : 0U;
    debug_as5600_left_read_status = (uint8_t)result;
    steer_pos_left_last_sample_ms = now;
    if (result == HAL_OK)
    {
      debug_as5600_left_raw = value;
      debug_as5600_left_deg = AS5600_RawToDegrees(value);
      debug_as5600_left_status = status;
      debug_as5600_left_magnet_detected =
          ((status & AS5600_STATUS_MD) != 0U) ? 1U : 0U;
      debug_as5600_left_magnet_too_weak =
          ((status & AS5600_STATUS_ML) != 0U) ? 1U : 0U;
      debug_as5600_left_magnet_too_strong =
          ((status & AS5600_STATUS_MH) != 0U) ? 1U : 0U;
      debug_as5600_left_read_ok_count++;
      steer_pos_left_new_sample_count++;
      steer_pos_left_raw = value;
    }
    else
    {
      debug_as5600_left_read_fail_count++;
    }
  }
  else
  {
    debug_as5600_right_comm_ok = (result == HAL_OK) ? 1U : 0U;
    debug_as5600_right_read_status = (uint8_t)result;
    steer_pos_right_last_sample_ms = now;
    if (result == HAL_OK)
    {
      debug_as5600_right_raw = value;
      debug_as5600_right_deg = AS5600_RawToDegrees(value);
      debug_as5600_right_status = status;
      debug_as5600_right_magnet_detected =
          ((status & AS5600_STATUS_MD) != 0U) ? 1U : 0U;
      debug_as5600_right_magnet_too_weak =
          ((status & AS5600_STATUS_ML) != 0U) ? 1U : 0U;
      debug_as5600_right_magnet_too_strong =
          ((status & AS5600_STATUS_MH) != 0U) ? 1U : 0U;
      debug_as5600_right_read_ok_count++;
      steer_pos_right_new_sample_count++;
      steer_pos_right_raw = value;
    }
    else
    {
      debug_as5600_right_read_fail_count++;
    }
  }

  if ((result != HAL_OK) ||
      (AS5600_DebugReadingValid(1U, status) == 0U))
    return 0U;
  *raw = value;
  return 1U;
}

static uint8_t Steering_PositionRawInRange(uint8_t left, uint16_t raw)
{
  int16_t d, lower, upper;
  /* Envelope derives from observed endstop RAW, including limited
   * overshoot behind the switch so an away-only escape remains possible.
   */
  if (left != 0U)
  {
    d = AS5600_ShortestDeltaCounts(STEER_POS_LEFT_CENTER_RAW, raw);
    lower = AS5600_ShortestDeltaCounts(STEER_POS_LEFT_CENTER_RAW,
                                       STEER_POS_LEFT_MAX_RAW);
    upper = AS5600_ShortestDeltaCounts(STEER_POS_LEFT_CENTER_RAW,
                                       STEER_POS_LEFT_MIN_RAW);
  }
  else
  {
    d = AS5600_ShortestDeltaCounts(STEER_POS_RIGHT_CENTER_RAW, raw);
    lower = AS5600_ShortestDeltaCounts(STEER_POS_RIGHT_CENTER_RAW,
                                       STEER_POS_RIGHT_MIN_RAW);
    upper = AS5600_ShortestDeltaCounts(STEER_POS_RIGHT_CENTER_RAW,
                                       STEER_POS_RIGHT_MAX_RAW);
  }
  return ((d >= (lower - STEER_POS_RANGE_MARGIN)) &&
          (d <= (upper + STEER_POS_RANGE_MARGIN))) ? 1U : 0U;
}

static uint8_t Steering_PositionWheelsStopped(void)
{
  if ((wheel_spin_active != 0U) || (wheel_ff_test_active != 0U) ||
      (wheel_target_pwm != 0) || (wheel_current_pwm != 0) ||
      (wheel_pwm_lu != 0) || (wheel_pwm_ld != 0) ||
      (wheel_pwm_ru != 0) || (wheel_pwm_rd != 0) ||
      (wheel_pid_target_rpm > 0.5f) || (wheel_pid_target_rpm < -0.5f) ||
      (Motor_LeftUp.current_rpm > 2.0f) || (Motor_LeftUp.current_rpm < -2.0f) ||
      (Motor_LeftDown.current_rpm > 2.0f) || (Motor_LeftDown.current_rpm < -2.0f) ||
      (Motor_RightUp.current_rpm > 2.0f) || (Motor_RightUp.current_rpm < -2.0f) ||
      (Motor_RightDown.current_rpm > 2.0f) || (Motor_RightDown.current_rpm < -2.0f))
    return 0U;
  return 1U;
}

static void Steering_PositionLeftOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  steer_pos_left_pwm = 0U;
}

static void Steering_PositionRightOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
  steer_pos_right_pwm = 0U;
}

static void Steering_PositionFinish(uint8_t result)
{
  Steering_PositionLeftOff();
  Steering_PositionRightOff();
  if (steer_pos_active != 0U) steer_pos_finish_count++;
  steer_pos_active = 0U;
  steer_pos_escape_mask = 0U;
  steer_pos_command = 0U;
  steer_pos_result = result;
}

/* IRQ must cut only the affected motor for NC limit rising edge.
 * R2 MIN contact is an expected arrival, not a shared fault.
 * Final state is processed by main loop, never inside HAL interrupt.
 */
static void Steering_PositionOnLimitInterrupt(uint8_t limit_bit)
{
  if (steer_pos_active == 0U) return;
  steer_pos_limit_trip_mask |= limit_bit;
  steer_pos_irq_pending_mask |= limit_bit;
  if ((limit_bit & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
    Steering_PositionLeftOff();
  if ((limit_bit & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
    Steering_PositionRightOff();
}

static void Steering_PositionAxisDone(uint8_t left, uint8_t result)
{
  if (left != 0U)
  {
    Steering_PositionLeftOff();
    steer_pos_left_state = STEER_AXIS_DONE;
    steer_pos_left_result = result;
  }
  else
  {
    Steering_PositionRightOff();
    steer_pos_right_state = STEER_AXIS_DONE;
    steer_pos_right_result = result;
  }
}

static void Steering_PositionAxisFault(uint8_t left, uint8_t result)
{
  if (left != 0U)
  {
    Steering_PositionLeftOff();
    steer_pos_left_state = STEER_AXIS_ERROR;
    steer_pos_left_result = result;
  }
  else
  {
    Steering_PositionRightOff();
    steer_pos_right_state = STEER_AXIS_ERROR;
    steer_pos_right_result = result;
  }
}

/* R1 CENTER (or R2 MIN departing MAX) may continue after own NC
 * physically releases only when AS5600 confirms >=3 counts of movement
 * in the known AWAY direction. Stops PWM for one entire control cycle
 * during transition; stall, magnet, and total timeout checks still apply.
 * If the switch opens/closes without measured motion, STOP this axis with
 * result=7 rather than treating contact bounce as a valid release.
 */
static void Steering_PositionEscapeReleased(uint8_t left, uint32_t now)
{
  SteeringPositionAxis_t *axis = left ? &steer_left_axis : &steer_right_axis;
  uint8_t min_switch = left ? STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_R_MIN_MASK;
  int16_t traveled = AS5600_ShortestDeltaCounts(axis->escape_start_raw,
                                                axis->last_raw);
  uint8_t must_increase =
      ((left != 0U) && (axis->escape_origin != min_switch)) ||
      ((left == 0U) && (axis->escape_origin == min_switch));
  if (((must_increase != 0U) && (traveled < 3)) ||
      ((must_increase == 0U) && (traveled > -3)))
  {
    Steering_PositionAxisDone(left, 7U);
    return;
  }

  axis->progress_ms = now;
  axis->best_abs_error = 32767U;
  axis->stable_count = 0U;
  if (left != 0U) steer_pos_left_state = STEER_AXIS_MOVING;
  else steer_pos_right_state = STEER_AXIS_MOVING;
}

/* R1 fine correction. All calls are AFTER the normal I2C/NC/sensor
 * plausibility checks, BEFORE the existing atomic CCR/late-IRQ guards.
 * Each axis owns only its own PWM. Phase 1 holds CCR=0 until mechanical
 * coasting has stopped; phase 2 emits ONE short direction-checked pulse.
 * No unbounded hunt or automatic power escalation is permitted.
 */
static uint16_t Steering_PositionFinePulseMs(uint16_t v)
{
  if (v < 12U) return 12U;
  return (v > 60U) ? 60U : v;
}

static void Steering_PositionFineFault(uint8_t left, uint8_t reason)
{
  if (left != 0U)
  {
    steer_pos_left_fine_fault = reason;
    steer_pos_left_fine_phase = 4U;
  }
  else
  {
    steer_pos_right_fine_fault = reason;
    steer_pos_right_fine_phase = 4U;
  }
  Steering_PositionAxisFault(left, STEER_POS_FINE_RESULT_FAIL);
}

static void Steering_PositionFineUpdate(uint8_t left,
                                        SteeringPositionAxis_t *axis,
                                        uint16_t raw, int16_t err,
                                        int16_t step, uint32_t now,
                                        uint16_t *duty,
                                        GPIO_PinState *direction)
{
  uint16_t magnitude = (uint16_t)((err < 0) ? -err : err);
  uint16_t tol = Steering_CenterFineTolClamp(
      left ? steer_pos_left_center_tol_raw :
             steer_pos_right_center_tol_raw);
  uint16_t pulse_ms = Steering_PositionFinePulseMs(
      left ? steer_pos_left_fine_pulse_ms :
             steer_pos_right_fine_pulse_ms);
  uint16_t pwm = Steering_PositionClampDuty(
      left ? steer_pos_left_fine_pwm : steer_pos_right_fine_pwm);
  uint16_t pwm_cap = Steering_PositionClampDuty(
      left ? steer_pos_left_pwm_max : steer_pos_right_pwm_max);
  uint8_t dir0;
  int16_t quiet_drift;

  *duty = 0U;
  if (left != 0U)
  {
    steer_pos_left_fine_error = err;
    steer_pos_left_fine_pulses = axis->fine_pulses;
  }
  else
  {
    steer_pos_right_fine_error = err;
    steer_pos_right_fine_pulses = axis->fine_pulses;
  }
  if (magnitude > STEER_POS_FINE_ABORT_RAW)
  {
    Steering_PositionFineFault(left, 2U); /* unexpected overshoot */
    return;
  }

  /* Bounded per-motor compare: no more than existing tested max.
   * A very weak motor may not respond to short pulses: 8 attempts
   * then report a local fault instead of claiming ±5 was reached.
   */
  if (pwm > pwm_cap) pwm = pwm_cap;
  if (left != 0U)
  {
    if (pwm > 300U) pwm = 300U;
  }
  else
  {
    if (pwm > 100U) pwm = 100U;
  }
  if (pwm == 0U)
  {
    Steering_PositionFineFault(left, 3U); /* explicitly disabled pulse */
    return;
  }

  if (axis->fine_phase == STEER_POS_FINE_PHASE_IDLE)
  {
    axis->fine_phase = STEER_POS_FINE_PHASE_SETTLE;
    axis->fine_phase_start_ms = now;
    axis->fine_quiet_since_ms = now;
    axis->fine_quiet_raw = raw;
  }

  if (axis->fine_phase == STEER_POS_FINE_PHASE_PULSE)
  {
    uint8_t expect_increase = left ? axis->fine_dir0 :
                                    (uint8_t)(!axis->fine_dir0);
    /* Only diagnose wrong direction while actual PWM is pulsing,
     * after the previous quiet interval, not while the motor coasts.
     */
    if (((expect_increase != 0U) && (step < -5)) ||
        ((expect_increase == 0U) && (step > 5)))
    {
      Steering_PositionFineFault(left, 4U);
      return;
    }

    if ((magnitude <= tol) ||
        ((axis->fine_start_error > 0) && (err <= 0)) ||
        ((axis->fine_start_error < 0) && (err >= 0)) ||
        ((uint32_t)(now - axis->fine_phase_start_ms) >= pulse_ms))
    {
      axis->fine_phase = STEER_POS_FINE_PHASE_SETTLE;
      axis->fine_phase_start_ms = now;
      axis->fine_quiet_since_ms = now;
      axis->fine_quiet_raw = raw;
      /* PWM is zero THIS frame. No immediate powered reversal. */
    }
    else
    {
      *direction = (axis->fine_dir0 != 0U) ?
                   GPIO_PIN_RESET : GPIO_PIN_SET;
      *duty = pwm;
    }
  }
  else if (axis->fine_phase == STEER_POS_FINE_PHASE_SETTLE)
  {
    quiet_drift = AS5600_ShortestDeltaCounts(axis->fine_quiet_raw, raw);
    if ((step > 1) || (step < -1) ||
        (quiet_drift > 2) || (quiet_drift < -2))
    {
      axis->fine_quiet_since_ms = now;
      axis->fine_quiet_raw = raw;
    }

    /* Never re-power during coasting. ±5 succeeds only if the
     * measured sensor is still inside the band AFTER the quiet hold.
     */
    if (((uint32_t)(now - axis->fine_phase_start_ms) >=
         STEER_POS_FINE_OFF_MIN_MS) &&
        ((uint32_t)(now - axis->fine_quiet_since_ms) >=
         STEER_POS_FINE_QUIET_MS))
    {
      if (magnitude <= tol)
      {
        if ((uint32_t)(now - axis->fine_quiet_since_ms) >=
            STEER_POS_FINE_CAPTURE_MS)
        {
          if (left != 0U)
          {
            steer_pos_left_center_final_raw = raw;
            steer_pos_left_fine_phase = 3U;
          }
          else
          {
            steer_pos_right_center_final_raw = raw;
            steer_pos_right_fine_phase = 3U;
          }
          Steering_PositionAxisDone(left, 2U);
          return;
        }
      }
      else
      {
        if (axis->fine_pulses >= STEER_POS_FINE_MAX_PULSES)
        {
          Steering_PositionFineFault(left, 1U); /* exhausted trim pulses */
          return;
        }

        /* A new pulse begins ONLY after zero-PWM settle; right DIR0
         * decreases RAW, left DIR0 increases RAW.
         */
        dir0 = left ? (err > 0) : (err < 0);
        axis->fine_dir0 = dir0;
        axis->fine_start_error = err;
        axis->fine_phase = STEER_POS_FINE_PHASE_PULSE;
        axis->fine_phase_start_ms = now;
        axis->fine_pulses++;
        *direction = (dir0 != 0U) ?
                     GPIO_PIN_RESET : GPIO_PIN_SET;
        *duty = pwm;
      }
    }
  }
  else
  {
    Steering_PositionFineFault(left, 5U); /* invalid internal phase */
    return;
  }

  if (left != 0U)
  {
    steer_pos_left_fine_phase = axis->fine_phase;
    steer_pos_left_fine_pulses = axis->fine_pulses;
  }
  else
  {
    steer_pos_right_fine_phase = axis->fine_phase;
    steer_pos_right_fine_pulses = axis->fine_pulses;
  }
}

/* Independent axis servo. No checks on the other side's limit inputs.
 * Uses actual observed progress, not only encoder noise / timeout.
 */
static void Steering_PositionAxisUpdate(uint8_t left, uint8_t limit_mask,
                                        uint32_t now, uint16_t *duty,
                                        GPIO_PinState *direction)
{
  uint8_t state = left ? steer_pos_left_state : steer_pos_right_state;
  uint8_t own_min = left ? STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_R_MIN_MASK;
  uint8_t own_max = left ? STEER_LIMIT_L_MAX_MASK : STEER_LIMIT_R_MAX_MASK;
  SteeringPositionAxis_t *axis = left ? &steer_left_axis : &steer_right_axis;
  uint16_t raw = 0U, goal;
  int16_t err, step;
  uint16_t abs_error;

  *duty = 0U;
  if ((state != STEER_AXIS_MOVING) && (state != STEER_AXIS_ESCAPING))
  {
    /* A finished/faulted axis stays PWM=0 but its AS5600 RAW telemetry
     * must continue updating while the partner is still moving.
     */
    (void)Steering_PositionReadOne(left, &raw);
    return;
  }

  /* Expected MIN wins over a sensor fault at the endpoint for R2;
   * always cut the PWM for that axis, never for its partner.
   */
  /* A motor may leave its OWN initially asserted NC endstop in the
   * safe direction for <=1s. A different active limit is always a fault.
   * The opposite side's limit state must never stop this axis.
   */
  if ((limit_mask & (own_min | own_max)) == (own_min | own_max))
  {
    Steering_PositionAxisFault(left, 9U);
    return;
  }
  if (state == STEER_AXIS_ESCAPING)
  {
    /* Do not mistake an unexpected opposite endstop for a safe release.
     * When both bits are zero, validate AS5600 movement after reading.
     */
    if (((limit_mask & (own_min | own_max)) != 0U) &&
        ((limit_mask & axis->escape_origin) == 0U))
    {
      Steering_PositionAxisFault(left, 9U);
      return;
    }
  }
  else
  {
    if ((steer_pos_command == 2U) && ((limit_mask & own_min) != 0U))
    {
      Steering_PositionAxisDone(left, 3U);
      return;
    }
    if ((limit_mask & (own_min | own_max)) != 0U)
    {
      Steering_PositionAxisFault(left, 9U);
      return;
    }
  }

  if (Steering_PositionReadOne(left, &raw) == 0U ||
      Steering_PositionRawInRange(left, raw) == 0U)
  {
    Steering_PositionAxisFault(left, 4U);
    return;
  }

  step = AS5600_ShortestDeltaCounts(axis->last_raw, raw);
  if ((step > STEER_POS_MAX_JUMP_RAW) || (step < -STEER_POS_MAX_JUMP_RAW))
  {
    Steering_PositionAxisFault(left, 4U);
    return;
  }
  axis->last_raw = raw;

  if (state == STEER_AXIS_ESCAPING)
  {
    uint8_t from_max = (axis->escape_origin == own_max) ? 1U : 0U;
    /* LEFT RAW: leaving MIN decreases, leaving MAX increases.
     * RIGHT RAW: leaving MIN increases, leaving MAX decreases.
     * After verified switch release, transition through one zero-PWM
     * cycle and then continue the requested R1/R2 target.
     */
    if ((limit_mask & axis->escape_origin) == 0U)
    {
      Steering_PositionEscapeReleased(left, now);
      return;
    }
    if ((uint32_t)(now - axis->escape_start_ms) >= STEER_POS_ESCAPE_MS)
    {
      Steering_PositionAxisFault(left, 5U);
      return;
    }
    if (((left != 0U) && (from_max == 0U) && (step > 5)) ||
        ((left == 0U) && (from_max == 0U) && (step < -5)) ||
        ((left != 0U) && (from_max != 0U) && (step < -5)) ||
        ((left == 0U) && (from_max != 0U) && (step > 5)))
    {
      Steering_PositionAxisFault(left, 8U);
      return;
    }
    if ((step >= 2) || (step <= -2)) axis->progress_ms = now;
    if ((uint32_t)(now - axis->progress_ms) > STEER_POS_STALL_MS)
    {
      Steering_PositionAxisFault(left, 8U);
      return;
    }
    *direction = (from_max != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    *duty = left ? Steering_PositionClampDuty(steer_pos_left_pwm_max)
                 : Steering_PositionClampDuty(steer_pos_right_pwm_max);
    return;
  }

  goal = (steer_pos_command == 1U) ?
      (left ? STEER_POS_LEFT_CENTER_RAW : STEER_POS_RIGHT_CENTER_RAW) :
      (left ? STEER_POS_LEFT_MIN_RAW : STEER_POS_RIGHT_MIN_RAW);
  err = AS5600_ShortestDeltaCounts(raw, goal);
  if (left != 0U) steer_pos_left_error = err;
  else steer_pos_right_error = err;

  abs_error = (uint16_t)((err < 0) ? -err : err);
  /* R1: coarse approach to ±35, then independently pulsed ±5 trim.
   * Never declare R1 DONE merely because error is within Select's
   * larger 15/20-count coarse acceptance band.
   * R2 MIN remains its previously validated ±5 controller.
   */
  if (steer_pos_command == 1U)
  {
    if ((axis->fine_phase != STEER_POS_FINE_PHASE_IDLE) ||
        (abs_error <= STEER_POS_FINE_ENTER_RAW))
    {
      Steering_PositionFineUpdate(left, axis, raw, err, step, now,
                                  duty, direction);
      return;
    }
  }
  else if (abs_error <= STEER_POS_TOL_RAW)
  {
    if (axis->stable_count < STEER_POS_STABLE_SAMPLES)
      axis->stable_count++;
    if (axis->stable_count >= STEER_POS_STABLE_SAMPLES)
      Steering_PositionAxisDone(left, 2U);
    return;
  }

  axis->stable_count = 0U;

  if (abs_error + 2U <= axis->best_abs_error)
  {
    axis->best_abs_error = abs_error;
    axis->progress_ms = now;
  }
  if (((uint32_t)abs_error > ((uint32_t)axis->best_abs_error + 20U)) ||
      ((uint32_t)(now - axis->progress_ms) > STEER_POS_STALL_MS))
  {
    Steering_PositionAxisFault(left, 8U);
    return;
  }

  if (left != 0U)
  {
    /* LEFT angle toward MIN increases: DIR0; toward MAX decreases: DIR1. */
    *direction = (err > 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    *duty = Steering_PositionDuty(err, steer_pos_left_pwm_min,
                                  steer_pos_left_pwm_max, 2U);
  }
  else
  {
    /* RIGHT angle toward MIN decreases: DIR0; toward MAX increases: DIR1. */
    *direction = (err < 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    *duty = Steering_PositionDuty(err, steer_pos_right_pwm_min,
                                  steer_pos_right_pwm_max, 4U);
  }
}

/* Legacy full-probe debug can still be enabled manually, but the normal
 * Live Expressions telemetry is automatically serviced with no enable.
 * One sensor per visit (<=2ms HAL read), alternating approximately 50ms
 * apart to avoid old double-bus polling jitter in M6 wheel PI.
 */
static void Steering_PositionTelemetryIdleUpdate(void)
{
  uint16_t raw;
  uint32_t now = HAL_GetTick();
  if ((steer_pos_active != 0U) || (steer_manual_active != 0U) ||
      (steer_cal_active != 0U) || (debug_as5600_poll_enable != 0U))
    return;
  if ((uint32_t)(now - steer_pos_idle_last_ms) < STEER_POS_IDLE_POLL_MS)
    return;
  steer_pos_idle_last_ms = now;
  (void)Steering_PositionReadOne(steer_pos_idle_next_side, &raw);
  steer_pos_idle_next_side ^= 1U;
}

static void Steering_PositionUpdate(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t irq_state;
  uint8_t buttons = 0U;
  uint8_t raw_mask, pending, late_pending;
  uint8_t allowed;
  uint16_t left_cmd = 0U, right_cmd = 0U;
  GPIO_PinState left_dir = GPIO_PIN_RESET;
  GPIO_PinState right_dir = GPIO_PIN_RESET;

  if ((PS2_Pad.Connected == 1U) && (PS2_Pad.R1 != 0U)) buttons |= 1U;
  if ((PS2_Pad.Connected == 1U) && (PS2_Pad.R2 != 0U)) buttons |= 2U;
  steer_pos_buttons = buttons;

  allowed = ((PS2_Pad.Connected == 1U) && (PS2_Pad.Mode == 0x73U) &&
             (control_mode == CONTROL_MODE_PS2) &&
             (control_stop_latched == 0U) && (ps2_cancel_hold == 0U) &&
             (ps2_spin_rf_latched == 0U) &&
             (voice_execution_active == 0U) && (voice_command_ready == 0U) &&
             (PS2_Pad.L1 == 0U) && (PS2_Pad.L2 == 0U) &&
             (PS2_Pad.Up == 0U) && (PS2_Pad.Down == 0U) &&
             (PS2_Pad.Left == 0U) && (PS2_Pad.Right == 0U) &&
             (PS2_Pad.LY >= PS2_LY_FORWARD_LIMIT) &&
             (PS2_Pad.LY <= PS2_LY_REVERSE_LIMIT) &&
             (PS2_Pad.RX >= (128U - STEER_POS_RX_NEUTRAL_DELTA)) &&
             (PS2_Pad.RX <= (128U + STEER_POS_RX_NEUTRAL_DELTA)) &&
             (steer_pos_enable == 1U) &&
             (steer_manual_active == 0U) &&
             (steer_cal_active == 0U) &&
             (PS2_Pad.Select == 0U) &&
             (debug_as5600_poll_enable == 0U) &&
             (steer_test_running == 0U) && (steer_test_request == 0U) &&
             (steer_test_arm == 0U) &&
             (Steering_PositionWheelsStopped() != 0U));

  if (steer_pos_active == 0U)
  {
    if ((steer_test_running == 0U) && (steer_manual_active == 0U) &&
        (steer_cal_active == 0U))
    {
      Steering_PositionLeftOff();
      Steering_PositionRightOff();
    }
    if ((buttons == 0U) || (steer_pos_previous_buttons != 0U))
    {
      steer_pos_previous_buttons = buttons;
      return;
    }
    steer_pos_previous_buttons = buttons;

    if ((buttons == 3U) || (allowed == 0U) ||
        (debug_limit_r_max.initialized == 0U) ||
        (debug_limit_r_min.initialized == 0U) ||
        (debug_limit_l_min.initialized == 0U) ||
        (debug_limit_l_max.initialized == 0U))
    {
      if (buttons == 3U) steer_pos_block_reason = 8U;
      else if (steer_pos_enable != 1U) steer_pos_block_reason = 1U;
      else if ((PS2_Pad.Connected != 1U) ||
               (PS2_Pad.Mode != 0x73U))
        steer_pos_block_reason = 2U;
      else if ((debug_limit_r_max.initialized == 0U) ||
               (debug_limit_r_min.initialized == 0U) ||
               (debug_limit_l_min.initialized == 0U) ||
               (debug_limit_l_max.initialized == 0U))
        steer_pos_block_reason = 7U;
      else if (steer_manual_active != 0U) steer_pos_block_reason = 9U;
      else if (Steering_PositionWheelsStopped() == 0U)
        steer_pos_block_reason = 5U;
      else if ((debug_as5600_poll_enable != 0U) ||
               (steer_test_running != 0U) ||
               (steer_test_request != 0U) ||
               (steer_test_arm != 0U))
        steer_pos_block_reason = 6U;
      else if ((PS2_Pad.Left != 0U) || (PS2_Pad.Right != 0U) ||
               (PS2_Pad.L1 != 0U) || (PS2_Pad.L2 != 0U) ||
               (PS2_Pad.Up != 0U) || (PS2_Pad.Down != 0U) ||
               (PS2_Pad.LY < PS2_LY_FORWARD_LIMIT) ||
               (PS2_Pad.LY > PS2_LY_REVERSE_LIMIT) ||
               (PS2_Pad.RX < (128U - STEER_POS_RX_NEUTRAL_DELTA)) ||
               (PS2_Pad.RX > (128U + STEER_POS_RX_NEUTRAL_DELTA)))
        steer_pos_block_reason = 4U;
      else steer_pos_block_reason = 3U;

      steer_pos_result = 6U;
      steer_pos_reject_count++;
      return;
    }

    steer_pos_block_reason = 0U;
    raw_mask = Steering_TestRawLimitMask();
    /* Each asserted NC endstop may be departed only for <=1s in the
     * known safe opposite direction. A dual active limit on one axis is
     * a local fault; the other axis can still handle its own command.
     */
    steer_pos_active = 1U;
    steer_pos_command = buttons;
    steer_pos_result = 1U;
    steer_pos_start_ms = now;
    steer_pos_irq_pending_mask = 0U;
    steer_pos_limit_trip_mask = 0U;
    steer_pos_left_state = STEER_AXIS_MOVING;
    steer_pos_right_state = STEER_AXIS_MOVING;
    steer_pos_left_result = 0U;
    steer_pos_right_result = 0U;
    steer_left_axis.progress_ms = now;
    steer_right_axis.progress_ms = now;
    steer_left_axis.escape_start_ms = now;
    steer_right_axis.escape_start_ms = now;
    steer_left_axis.escape_origin = 0U;
    steer_right_axis.escape_origin = 0U;
    steer_left_axis.best_abs_error = 32767U;
    steer_right_axis.best_abs_error = 32767U;
    steer_left_axis.stable_count = 0U;
    steer_right_axis.stable_count = 0U;
    steer_left_axis.fine_phase = STEER_POS_FINE_PHASE_IDLE;
    steer_right_axis.fine_phase = STEER_POS_FINE_PHASE_IDLE;
    steer_left_axis.fine_pulses = 0U;
    steer_right_axis.fine_pulses = 0U;
    steer_pos_left_fine_phase = 0U;
    steer_pos_right_fine_phase = 0U;
    steer_pos_left_fine_pulses = 0U;
    steer_pos_right_fine_pulses = 0U;
    steer_pos_left_fine_error = 0;
    steer_pos_right_fine_error = 0;
    steer_pos_left_fine_fault = 0U;
    steer_pos_right_fine_fault = 0U;
    steer_pos_left_center_final_raw = 0U;
    steer_pos_right_center_final_raw = 0U;
    steer_pos_run_count++;

    if ((raw_mask & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) ==
                    (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK))
      Steering_PositionAxisFault(1U, 9U);
    else if ((raw_mask & STEER_LIMIT_L_MIN_MASK) != 0U)
    {
      if (buttons == 2U) Steering_PositionAxisDone(1U, 3U);
      else
      {
        steer_left_axis.escape_origin = STEER_LIMIT_L_MIN_MASK;
        steer_pos_left_state = STEER_AXIS_ESCAPING;
      }
    }
    else if ((raw_mask & STEER_LIMIT_L_MAX_MASK) != 0U)
    {
      steer_left_axis.escape_origin = STEER_LIMIT_L_MAX_MASK;
      steer_pos_left_state = STEER_AXIS_ESCAPING;
    }

    if ((raw_mask & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) ==
                    (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK))
      Steering_PositionAxisFault(0U, 9U);
    else if ((raw_mask & STEER_LIMIT_R_MIN_MASK) != 0U)
    {
      if (buttons == 2U) Steering_PositionAxisDone(0U, 3U);
      else
      {
        steer_right_axis.escape_origin = STEER_LIMIT_R_MIN_MASK;
        steer_pos_right_state = STEER_AXIS_ESCAPING;
      }
    }
    else if ((raw_mask & STEER_LIMIT_R_MAX_MASK) != 0U)
    {
      steer_right_axis.escape_origin = STEER_LIMIT_R_MAX_MASK;
      steer_pos_right_state = STEER_AXIS_ESCAPING;
    }

    steer_pos_escape_mask = (uint8_t)(
        ((steer_pos_left_state == STEER_AXIS_ESCAPING) ?
           steer_left_axis.escape_origin : 0U) |
        ((steer_pos_right_state == STEER_AXIS_ESCAPING) ?
           steer_right_axis.escape_origin : 0U));

    /* At start collect independent baselines, reject each invalid side.
     * Do not let a fault on one channel force the other to stop.
     */
    if (steer_pos_left_state != STEER_AXIS_DONE)
    {
      uint16_t v;
      if ((Steering_PositionReadOne(1U, &v) == 0U) ||
          (Steering_PositionRawInRange(1U, v) == 0U))
        Steering_PositionAxisFault(1U, 4U);
      else
      {
        steer_left_axis.last_raw = v;
        steer_left_axis.escape_start_raw = v;
      }
    }
    if (steer_pos_right_state != STEER_AXIS_DONE)
    {
      uint16_t v;
      if ((Steering_PositionReadOne(0U, &v) == 0U) ||
          (Steering_PositionRawInRange(0U, v) == 0U))
        Steering_PositionAxisFault(0U, 4U);
      else
      {
        steer_right_axis.last_raw = v;
        steer_right_axis.escape_start_raw = v;
      }
    }
  }
  else
  {
    steer_pos_previous_buttons = buttons;
    if ((allowed == 0U) ||
        ((buttons != 0U) && (buttons != steer_pos_command)))
    {
      /* Switching from auto R1/R2 to manual steering requires a fresh
       * neutral->direction stroke; never instantly reverse a moving axis.
       */
      if (Steering_ManualRequestDirection() != 0)
        steer_manual_neutral_required = 1U;
      Steering_PositionFinish(9U);
      return;
    }
  }

  raw_mask = Steering_TestRawLimitMask();
  irq_state = __get_PRIMASK();
  __disable_irq();
  pending = steer_pos_irq_pending_mask;
  steer_pos_irq_pending_mask = 0U;
  if (irq_state == 0U) __enable_irq();

  /* Apply per-axis IRQ hits; one axis reaches its own MIN without
   * disturbing the partner motor. Unexpected hits are local faults.
   */
  if ((pending & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
  {
    if ((steer_pos_command == 2U) &&
        ((pending & STEER_LIMIT_L_MIN_MASK) != 0U) &&
        ((pending & STEER_LIMIT_L_MAX_MASK) == 0U))
      Steering_PositionAxisDone(1U, 3U);
    else
      Steering_PositionAxisFault(1U, 9U);
  }
  if ((pending & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
  {
    if ((steer_pos_command == 2U) &&
        ((pending & STEER_LIMIT_R_MIN_MASK) != 0U) &&
        ((pending & STEER_LIMIT_R_MAX_MASK) == 0U))
      Steering_PositionAxisDone(0U, 3U);
    else
      Steering_PositionAxisFault(0U, 9U);
  }

  if ((uint32_t)(now - steer_pos_start_ms) >=
      ((steer_pos_command == 1U) ?
       STEER_POS_R1_TIMEOUT_MS : STEER_POS_TIMEOUT_MS))
  {
    if ((steer_pos_left_state == STEER_AXIS_MOVING) ||
        (steer_pos_left_state == STEER_AXIS_ESCAPING))
      Steering_PositionAxisFault(1U, 5U);
    if ((steer_pos_right_state == STEER_AXIS_MOVING) ||
        (steer_pos_right_state == STEER_AXIS_ESCAPING))
      Steering_PositionAxisFault(0U, 5U);
  }

  Steering_PositionAxisUpdate(1U, raw_mask, now, &left_cmd, &left_dir);
  Steering_PositionAxisUpdate(0U, raw_mask, now, &right_cmd, &right_dir);
  steer_pos_escape_mask = (uint8_t)(
      ((steer_pos_left_state == STEER_AXIS_ESCAPING) ?
         steer_left_axis.escape_origin : 0U) |
      ((steer_pos_right_state == STEER_AXIS_ESCAPING) ?
         steer_right_axis.escape_origin : 0U));

  /* ATOMIC recheck: no limit IRQ may arrive between checking GPIO and
   * writing compare; when interrupts unmask, pending limit callback
   * immediately kills only the axis whose NC contact rose.
   */
  irq_state = __get_PRIMASK();
  __disable_irq();
  raw_mask = Steering_TestRawLimitMask();
  /* An NC rising edge may have occurred after the earlier snapshot,
   * possibly bounced low by the time we read GPIO here. Honor it
   * BEFORE re-energizing any PWM. Consume while interrupts are masked.
   */
  late_pending = steer_pos_irq_pending_mask;
  steer_pos_irq_pending_mask = 0U;
  if ((late_pending & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
  {
    if ((steer_pos_left_state == STEER_AXIS_MOVING) &&
        (steer_pos_command == 2U) &&
        ((late_pending & STEER_LIMIT_L_MIN_MASK) != 0U) &&
        ((late_pending & STEER_LIMIT_L_MAX_MASK) == 0U))
      Steering_PositionAxisDone(1U, 3U);
    else
      Steering_PositionAxisFault(1U, 9U);
  }
  if ((late_pending & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
  {
    if ((steer_pos_right_state == STEER_AXIS_MOVING) &&
        (steer_pos_command == 2U) &&
        ((late_pending & STEER_LIMIT_R_MIN_MASK) != 0U) &&
        ((late_pending & STEER_LIMIT_R_MAX_MASK) == 0U))
      Steering_PositionAxisDone(0U, 3U);
    else
      Steering_PositionAxisFault(0U, 9U);
  }

  /* Escape paths must also recheck the originating switch after I2C
   * reads: no side may be reenergized if its NC was released, if both
   * NC lines opened, or if the unexpected opposite limit activated.
   */
  if (steer_pos_left_state == STEER_AXIS_ESCAPING)
  {
    uint8_t own_limits = (uint8_t)
        (raw_mask & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK));
    if (own_limits == 0U)
    {
      left_cmd = 0U;
      Steering_PositionEscapeReleased(1U, now);
    }
    else if (own_limits != steer_left_axis.escape_origin)
      Steering_PositionAxisFault(1U, 9U);
  }
  if (steer_pos_right_state == STEER_AXIS_ESCAPING)
  {
    uint8_t own_limits = (uint8_t)
        (raw_mask & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK));
    if (own_limits == 0U)
    {
      right_cmd = 0U;
      Steering_PositionEscapeReleased(0U, now);
    }
    else if (own_limits != steer_right_axis.escape_origin)
      Steering_PositionAxisFault(0U, 9U);
  }

  if ((steer_pos_left_state == STEER_AXIS_MOVING) &&
      ((raw_mask & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U))
  {
    if ((steer_pos_command == 2U) &&
        ((raw_mask & STEER_LIMIT_L_MIN_MASK) != 0U) &&
        ((raw_mask & STEER_LIMIT_L_MAX_MASK) == 0U))
      Steering_PositionAxisDone(1U, 3U);
    else
      Steering_PositionAxisFault(1U, 9U);
  }
  if ((steer_pos_right_state == STEER_AXIS_MOVING) &&
      ((raw_mask & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U))
  {
    if ((steer_pos_command == 2U) &&
        ((raw_mask & STEER_LIMIT_R_MIN_MASK) != 0U) &&
        ((raw_mask & STEER_LIMIT_R_MAX_MASK) == 0U))
      Steering_PositionAxisDone(0U, 3U);
    else
      Steering_PositionAxisFault(0U, 9U);
  }

  if ((steer_pos_left_state == STEER_AXIS_MOVING) ||
      (steer_pos_left_state == STEER_AXIS_ESCAPING))
  {
    if ((steer_pos_left_state != STEER_AXIS_ESCAPING) ||
        ((raw_mask & steer_left_axis.escape_origin) != 0U))
    {
      if (left_cmd != 0U)
        HAL_GPIO_WritePin(MOTOR_DIR_TURN_left_GPIO_Port,
                          MOTOR_DIR_TURN_left_Pin, left_dir);
      __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, left_cmd);
      steer_pos_left_pwm = left_cmd;
    }
    else
      Steering_PositionAxisDone(1U, 7U);
  }
  else
    Steering_PositionLeftOff();

  if ((steer_pos_right_state == STEER_AXIS_MOVING) ||
      (steer_pos_right_state == STEER_AXIS_ESCAPING))
  {
    if ((steer_pos_right_state != STEER_AXIS_ESCAPING) ||
        ((raw_mask & steer_right_axis.escape_origin) != 0U))
    {
      if (right_cmd != 0U)
        HAL_GPIO_WritePin(MOTOR_DIR_TURN_right_GPIO_Port,
                          MOTOR_DIR_TURN_right_Pin, right_dir);
      __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, right_cmd);
      steer_pos_right_pwm = right_cmd;
    }
    else
      Steering_PositionAxisDone(0U, 7U);
  }
  else
    Steering_PositionRightOff();

  if (irq_state == 0U) __enable_irq();

  if ((steer_pos_left_state >= STEER_AXIS_DONE) &&
      (steer_pos_right_state >= STEER_AXIS_DONE))
  {
    uint8_t result = 2U;
    if ((steer_pos_left_state == STEER_AXIS_ERROR) ||
        (steer_pos_right_state == STEER_AXIS_ERROR))
      result = 8U;
    else if ((steer_pos_left_result == 7U) ||
             (steer_pos_right_result == 7U))
      result = 7U;
    Steering_PositionFinish(result);
  }
}


/*
 * M7-S05: direct steering on D-pad L/R or right-stick RX.
 * Follows independent L/R limits, AS5600 safety and conservative PWM
 * values. Neutral cuts PWM in the same control iteration.
 * Turning around from a touched limit requires short escape and NEW press.
 */
/* Conservative start after 20 RAW was not enough to avoid NC contact.
 * Clamp live tuning to [30..120] counts. Not a mechanical braking guarantee.
 */
volatile uint16_t steer_manual_soft_margin_raw = 40U;
#define STEER_MANUAL_SOFT_MARGIN_RAW  \
    ((steer_manual_soft_margin_raw < 30U) ? 30U : \
     ((steer_manual_soft_margin_raw > 120U) ? 120U : \
      steer_manual_soft_margin_raw))
#define STEER_MANUAL_APPROACH_WINDOW_RAW 80U
#define STEER_MANUAL_LEFT_SOFT_MIN  (STEER_POS_LEFT_MIN_RAW - STEER_MANUAL_SOFT_MARGIN_RAW)
#define STEER_MANUAL_LEFT_SOFT_MAX  (STEER_POS_LEFT_MAX_RAW + STEER_MANUAL_SOFT_MARGIN_RAW)
#define STEER_MANUAL_RIGHT_SOFT_MIN (STEER_POS_RIGHT_MIN_RAW + STEER_MANUAL_SOFT_MARGIN_RAW)
#define STEER_MANUAL_RIGHT_SOFT_MAX (STEER_POS_RIGHT_MAX_RAW - STEER_MANUAL_SOFT_MARGIN_RAW)
#define STEER_MANUAL_RESULT_SOFT    11U
#define STEER_MANUAL_RESULT_WRONG_DIRECTION 12U
#define STEER_MANUAL_RESULT_STALL   13U
#define STEER_MANUAL_RUN_MAX_MS       5000U
#define STEER_MANUAL_ESCAPE_MAX_MS    1000U
#define STEER_MANUAL_STALL_MS          650U
#define STEER_MANUAL_PROGRESS_DELTA     2U
#define STEER_MANUAL_PWM_HARD_MAX     400U

#define STEER_MANUAL_IDLE       0U
#define STEER_MANUAL_MOVING     1U
#define STEER_MANUAL_ESCAPE     2U
#define STEER_MANUAL_DONE       3U
#define STEER_MANUAL_FAULT      4U

typedef struct
{
  uint16_t previous_raw;
  uint16_t best_distance;
  uint32_t progress_ms;
  uint32_t escape_start_ms;
} SteeringManualAxis_t;

static SteeringManualAxis_t steer_manual_left_axis = {0};
static SteeringManualAxis_t steer_manual_right_axis = {0};
static uint32_t steer_manual_begin_ms = 0U;

static uint16_t Steering_ManualClampPWM(uint16_t pwm)
{
  return (pwm > STEER_MANUAL_PWM_HARD_MAX) ?
      STEER_MANUAL_PWM_HARD_MAX : pwm;
}

static uint16_t Steering_ManualRXDuty(uint16_t minimum, uint16_t maximum,
                                      uint16_t amount)
{
  uint32_t scaled;
  minimum = Steering_ManualClampPWM(minimum);
  maximum = Steering_ManualClampPWM(maximum);
  if (maximum < minimum) maximum = minimum;
  if (amount > 117U) amount = 117U;
  scaled = (uint32_t)minimum +
      ((uint32_t)(maximum - minimum) * amount) / 117U;
  return (uint16_t)scaled;
}

static void Steering_ManualLeftOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  steer_manual_left_pwm = 0U;
}

static void Steering_ManualRightOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
  steer_manual_right_pwm = 0U;
}

static void Steering_ManualStop(void)
{
  /* If manual does not own the PWM, do NOT overwrite R1/R2 or
   * the manually-armed M7-S02 diagnostic pulse outputs.
   */
  if (steer_manual_active == 0U)
  {
    steer_manual_direction = 0;
    return;
  }
  Steering_ManualLeftOff();
  Steering_ManualRightOff();
  steer_manual_stop_count++;
  steer_manual_active = 0U;
  steer_manual_direction = 0;
}

/* An NC opening always kills the corresponding motor's PWM in EXTI,
 * then main-loop logic decides arrival versus fault.
 */
static void Steering_ManualOnLimitInterrupt(uint8_t bit)
{
  if (steer_manual_active == 0U) return;
  steer_manual_irq_pending_mask |= bit;
  steer_manual_irq_trip_mask |= bit;
  steer_manual_irq_count++;
  if ((bit & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
    Steering_ManualLeftOff();
  if ((bit & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
    Steering_ManualRightOff();
}

static void Steering_ManualAxisDone(uint8_t left, uint8_t reason)
{
  if (left != 0U)
  {
    Steering_ManualLeftOff();
    steer_manual_left_state = STEER_MANUAL_DONE;
    steer_manual_left_result = reason;
    steer_manual_left_stop_raw = debug_as5600_left_raw;
    steer_manual_left_stop_limit_mask = Steering_TestRawLimitMask();
  }
  else
  {
    Steering_ManualRightOff();
    steer_manual_right_state = STEER_MANUAL_DONE;
    steer_manual_right_result = reason;
    steer_manual_right_stop_raw = debug_as5600_right_raw;
    steer_manual_right_stop_limit_mask = Steering_TestRawLimitMask();
  }
}

static void Steering_ManualAxisFault(uint8_t left, uint8_t reason)
{
  if (left != 0U)
  {
    Steering_ManualLeftOff();
    steer_manual_left_state = STEER_MANUAL_FAULT;
    steer_manual_left_result = reason;
    steer_manual_left_stop_raw = debug_as5600_left_raw;
    steer_manual_left_stop_limit_mask = Steering_TestRawLimitMask();
  }
  else
  {
    Steering_ManualRightOff();
    steer_manual_right_state = STEER_MANUAL_FAULT;
    steer_manual_right_result = reason;
    steer_manual_right_stop_raw = debug_as5600_right_raw;
    steer_manual_right_stop_limit_mask = Steering_TestRawLimitMask();
  }
}

/* Execute one side independently. Direction is toward MIN (-1) or MAX (+1).
 * Own expected endstop is an arrival; unrelated side's switch is ignored.
 * If opposite endstop is active, depart for at most 1s and stop when its
 * NC line closes; require full input neutral before further travel.
 */
static void Steering_ManualAxisUpdate(uint8_t left, int8_t direction,
                                      uint8_t limits, uint32_t now,
                                      uint16_t pwm, uint16_t *apply)
{
  uint8_t state = left ? steer_manual_left_state : steer_manual_right_state;
  uint8_t minbit = left ? STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_R_MIN_MASK;
  uint8_t maxbit = left ? STEER_LIMIT_L_MAX_MASK : STEER_LIMIT_R_MAX_MASK;
  uint8_t ownbit = (direction < 0) ? minbit : maxbit;
  uint8_t opposite = (direction < 0) ? maxbit : minbit;
  SteeringManualAxis_t *axis = left ? &steer_manual_left_axis : &steer_manual_right_axis;
  uint16_t raw = 0U, goal, distance;
  int16_t delta, error;
  uint8_t correct_sign;

  *apply = 0U;
  if ((state != STEER_MANUAL_MOVING) && (state != STEER_MANUAL_ESCAPE))
  {
    /* Keep RAW/STATUS live even after a partner motor reaches the limit. */
    (void)Steering_PositionReadOne(left, &raw);
    return;
  }

  if ((limits & (minbit | maxbit)) == (minbit | maxbit))
  {
    Steering_ManualAxisFault(left, 9U);
    return;
  }
  if ((limits & ownbit) != 0U)
  {
    Steering_ManualAxisDone(left, 3U);
    return;
  }
  if (((limits & opposite) != 0U) && (state != STEER_MANUAL_ESCAPE))
  {
    Steering_ManualAxisFault(left, 9U);
    return;
  }

  if ((Steering_PositionReadOne(left, &raw) == 0U) ||
      (Steering_PositionRawInRange(left, raw) == 0U))
  {
    Steering_ManualAxisFault(left, 4U);
    return;
  }
  delta = AS5600_ShortestDeltaCounts(axis->previous_raw, raw);
  if ((delta > STEER_POS_MAX_JUMP_RAW) || (delta < -STEER_POS_MAX_JUMP_RAW))
  {
    Steering_ManualAxisFault(left, 4U);
    return;
  }
  axis->previous_raw = raw;

  /* LEFT raw increases when going MIN; RIGHT raw decreases going MIN. */
  correct_sign = (uint8_t)(((left != 0U) && (direction < 0)) ||
                           ((left == 0U) && (direction > 0)));
  if (((correct_sign != 0U) && (delta < -5)) ||
      ((correct_sign == 0U) && (delta > 5)))
  {
    Steering_ManualAxisFault(left, STEER_MANUAL_RESULT_WRONG_DIRECTION);
    return;
  }

  if (state == STEER_MANUAL_ESCAPE)
  {
    if ((limits & opposite) == 0U)
    {
      Steering_ManualAxisDone(left, 7U);
      return;
    }
    if ((uint32_t)(now - axis->escape_start_ms) >= STEER_MANUAL_ESCAPE_MAX_MS)
    {
      Steering_ManualAxisFault(left, 5U);
      return;
    }
    if ((delta >= (int16_t)STEER_MANUAL_PROGRESS_DELTA) ||
        (delta <= -(int16_t)STEER_MANUAL_PROGRESS_DELTA))
      axis->progress_ms = now;
    if ((uint32_t)(now - axis->progress_ms) > STEER_MANUAL_STALL_MS)
    {
      Steering_ManualAxisFault(left, STEER_MANUAL_RESULT_STALL);
      return;
    }
    *apply = Steering_ManualClampPWM(pwm);
    return;
  }

  /* D-pad/RX are MANUAL: stop 20 encoder counts *before* the
   * mechanical MIN/MAX, not near the physical endstop as previously.
   * No need to press NC. If already past soft boundary, the safe
   * choice is to stop moving INTO it, not fault/latch both motors.
   */
  goal = (direction < 0) ?
      ((left != 0U) ? STEER_MANUAL_LEFT_SOFT_MIN :
                      STEER_MANUAL_RIGHT_SOFT_MIN) :
      ((left != 0U) ? STEER_MANUAL_LEFT_SOFT_MAX :
                      STEER_MANUAL_RIGHT_SOFT_MAX);
  error = AS5600_ShortestDeltaCounts(raw, goal);
  if (left != 0U)
  {
    steer_manual_left_soft_target = goal;
    steer_manual_left_soft_remaining = error;
  }
  else
  {
    steer_manual_right_soft_target = goal;
    steer_manual_right_soft_remaining = error;
  }
  /* Left MIN grows / MAX falls; Right MIN falls / MAX grows.
   * Compare in the real physical direction so even an overshot
   * endpoint is a normal blocked boundary, never a wrong-way fault.
   */
  if (((left != 0U) && (direction < 0) && (raw >= goal)) ||
      ((left != 0U) && (direction > 0) && (raw <= goal)) ||
      ((left == 0U) && (direction < 0) && (raw <= goal)) ||
      ((left == 0U) && (direction > 0) && (raw >= goal)))
  {
    if (left != 0U) steer_manual_left_soft_stop_count++;
    else steer_manual_right_soft_stop_count++;
    Steering_ManualAxisDone(left, STEER_MANUAL_RESULT_SOFT);
    return;
  }
  distance = (uint16_t)((error < 0) ? -error : error);
  if (distance + 2U <= axis->best_distance)
  {
    axis->best_distance = distance;
    axis->progress_ms = now;
  }
  if (((uint32_t)distance > ((uint32_t)axis->best_distance + 20U)) ||
      ((uint32_t)(now - axis->progress_ms) > STEER_MANUAL_STALL_MS))
  {
    Steering_ManualAxisFault(left, STEER_MANUAL_RESULT_STALL);
    return;
  }
  /* Lower PWM during the last 80 RAW before the 20-count soft stop to
   * reduce coasting beyond the boundary. No active motor brake exists:
   * verify physical stopping distance; increase the soft margin if needed.
   * Never raise a low joystick PWM to the configured approach floor.
   */
  pwm = Steering_ManualClampPWM(pwm);
  if (distance < STEER_MANUAL_APPROACH_WINDOW_RAW)
  {
    uint16_t floor_pwm = left ? steer_manual_left_rx_min_pwm :
                                 steer_manual_right_rx_min_pwm;
    floor_pwm = Steering_ManualClampPWM(floor_pwm);
    if (pwm > floor_pwm)
    {
      pwm = (uint16_t)((uint32_t)floor_pwm +
          ((uint32_t)(pwm - floor_pwm) * distance) /
          STEER_MANUAL_APPROACH_WINDOW_RAW);
    }
  }
  *apply = pwm;
}

static void Steering_ManualUpdate(void)
{
  int8_t command = Steering_ManualRequestDirection();
  uint16_t lpwm, rpwm, la = 0U, ra = 0U;
  uint16_t magnitude = 0U;
  uint8_t lstate, rstate, mask, pending;
  uint32_t now = HAL_GetTick();
  uint32_t primask;

  if (command == 0)
  {
    steer_manual_block_reason =
        ((PS2_Pad.Left != 0U) && (PS2_Pad.Right != 0U)) ? 7U : 0U;
    Steering_ManualStop();
    /* Contradictory D-pad left+right is NOT considered neutral. */
    if (Steering_ManualInputsNeutral() != 0U)
      steer_manual_neutral_required = 0U;
    else if ((PS2_Pad.Left != 0U) && (PS2_Pad.Right != 0U))
      steer_manual_neutral_required = 1U;
    return;
  }

  /* If disarmed, a held input must be released before rearming: changing
   * Live Expressions steer_manual_enable from 0 to 1 cannot start a motor
   * until a NEW neutral->direction stroke has been observed.
   */
  if (steer_manual_enable != 1U)
  {
    steer_manual_block_reason = 1U;
    steer_manual_neutral_required = 1U;
    Steering_ManualStop();
    return;
  }

  /* An unexpected mode/priority conflict cannot authorize actuator PWM. */
  if (
      (control_mode != CONTROL_MODE_PS2) ||
      (control_stop_latched != 0U) ||
      (ps2_cancel_hold != 0U) ||
      (ps2_spin_rf_latched != 0U) ||
      (steer_pos_active != 0U) ||
      (steer_cal_active != 0U) ||
      (PS2_Pad.Select != 0U) ||
      (PS2_Pad.R1 != 0U) || (PS2_Pad.R2 != 0U) ||
      (PS2_Pad.L1 != 0U) || (PS2_Pad.L2 != 0U) ||
      (WheelControl_SpinIsActive() != 0U) ||
      ((steer_manual_allow_wheel_motion == 0U) &&
       (Steering_PositionWheelsStopped() == 0U)) ||
      (debug_as5600_poll_enable != 0U) ||
      (steer_test_running != 0U) ||
      (steer_test_arm != 0U) || (steer_test_request != 0U) ||
      (voice_execution_active != 0U) || (voice_command_ready != 0U))
  {
    /* A blocked held command must not auto-start after a transient
     * condition (e.g., wheel deceleration or Voice cancel) clears.
     */
    steer_manual_block_reason =
        ((steer_manual_allow_wheel_motion == 0U) &&
         (Steering_PositionWheelsStopped() == 0U)) ? 4U : 2U;
    steer_manual_neutral_required = 1U;
    Steering_ManualStop();
    return;
  }
  if (steer_manual_neutral_required != 0U)
  {
    steer_manual_block_reason = 5U;
    Steering_ManualStop();
    return;
  }
  /* Changing steering direction without first releasing requires neutral.
   * This prevents a hard reverse on a 20-ms joystick frame boundary.
   */
  if ((steer_manual_active != 0U) &&
      (command != steer_manual_direction))
  {
    steer_manual_block_reason = 5U;
    steer_manual_neutral_required = 1U;
    Steering_ManualStop();
    return;
  }

  if ((PS2_Pad.Left != 0U) || (PS2_Pad.Right != 0U))
  {
    lpwm = Steering_ManualClampPWM(steer_manual_left_dpad_pwm);
    rpwm = Steering_ManualClampPWM(steer_manual_right_dpad_pwm);
  }
  else
  {
    int16_t offset = (int16_t)PS2_Pad.RX - 128;
    magnitude = (uint16_t)((offset < 0) ? -offset : offset);
    if (magnitude <= 10U)
    {
      Steering_ManualStop();
      return;
    }
    magnitude -= 10U;
    if (magnitude > 117U) magnitude = 117U;
    lpwm = Steering_ManualRXDuty(steer_manual_left_rx_min_pwm,
                                 steer_manual_left_rx_max_pwm, magnitude);
    rpwm = Steering_ManualRXDuty(steer_manual_right_rx_min_pwm,
                                 steer_manual_right_rx_max_pwm, magnitude);
  }

  if (steer_manual_active == 0U)
  {
    uint16_t raw = 0U;
    if ((debug_limit_r_max.initialized == 0U) ||
        (debug_limit_r_min.initialized == 0U) ||
        (debug_limit_l_min.initialized == 0U) ||
        (debug_limit_l_max.initialized == 0U))
    {
      steer_manual_block_reason = 6U;
      steer_manual_reject_count++;
      steer_manual_neutral_required = 1U;
      return;
    }

    steer_manual_begin_ms = now;
    steer_manual_direction = command;
    steer_manual_active = 1U;
    steer_manual_irq_pending_mask = 0U;
    steer_manual_irq_trip_mask = 0U;
    steer_manual_left_state = STEER_MANUAL_MOVING;
    steer_manual_right_state = STEER_MANUAL_MOVING;
    steer_manual_left_result = 0U;
    steer_manual_right_result = 0U;
    steer_manual_left_axis.progress_ms = now;
    steer_manual_right_axis.progress_ms = now;
    steer_manual_left_axis.best_distance = 32767U;
    steer_manual_right_axis.best_distance = 32767U;
    steer_manual_start_count++;
    steer_manual_block_reason = 0U;

    mask = Steering_TestRawLimitMask();
    if ((mask & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) ==
          (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK))
      Steering_ManualAxisFault(1U, 9U);
    else if ((mask & ((command < 0) ? STEER_LIMIT_L_MIN_MASK :
                        STEER_LIMIT_L_MAX_MASK)) != 0U)
      Steering_ManualAxisDone(1U, 3U);
    else if ((mask & ((command < 0) ? STEER_LIMIT_L_MAX_MASK :
                        STEER_LIMIT_L_MIN_MASK)) != 0U)
    {
      steer_manual_left_state = STEER_MANUAL_ESCAPE;
      steer_manual_left_axis.escape_start_ms = now;
    }
    if ((mask & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) ==
          (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK))
      Steering_ManualAxisFault(0U, 9U);
    else if ((mask & ((command < 0) ? STEER_LIMIT_R_MIN_MASK :
                        STEER_LIMIT_R_MAX_MASK)) != 0U)
      Steering_ManualAxisDone(0U, 3U);
    else if ((mask & ((command < 0) ? STEER_LIMIT_R_MAX_MASK :
                        STEER_LIMIT_R_MIN_MASK)) != 0U)
    {
      /* R_MAX departure had not passed a separate DIR0 limit-escape
       * bench test. Keep it opt-in; never assume pressing D-pad is enough.
       */
      if ((command < 0) &&
          (steer_manual_allow_right_max_escape == 0U))
        Steering_ManualAxisFault(0U, 10U);
      else
      {
        steer_manual_right_state = STEER_MANUAL_ESCAPE;
        steer_manual_right_axis.escape_start_ms = now;
      }
    }

    /* Baseline current AS5600 per axis before energizing its channel. */
    if ((steer_manual_left_state == STEER_MANUAL_MOVING) ||
        (steer_manual_left_state == STEER_MANUAL_ESCAPE))
    {
      if ((Steering_PositionReadOne(1U, &raw) == 0U) ||
          (Steering_PositionRawInRange(1U, raw) == 0U))
        Steering_ManualAxisFault(1U, 4U);
      else
        steer_manual_left_axis.previous_raw = raw;
    }
    if ((steer_manual_right_state == STEER_MANUAL_MOVING) ||
        (steer_manual_right_state == STEER_MANUAL_ESCAPE))
    {
      if ((Steering_PositionReadOne(0U, &raw) == 0U) ||
          (Steering_PositionRawInRange(0U, raw) == 0U))
        Steering_ManualAxisFault(0U, 4U);
      else
        steer_manual_right_axis.previous_raw = raw;
    }
  }

  /* Consume rising limit EXTI and preserve own-side isolation. */
  primask = __get_PRIMASK();
  __disable_irq();
  pending = steer_manual_irq_pending_mask;
  steer_manual_irq_pending_mask = 0U;
  if (primask == 0U) __enable_irq();

  if ((pending & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
  {
    uint8_t expect = (command < 0) ? STEER_LIMIT_L_MIN_MASK :
                      STEER_LIMIT_L_MAX_MASK;
    if ((pending & expect) != 0U &&
        (pending & (uint8_t)~expect &
         (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) == 0U)
      Steering_ManualAxisDone(1U, 3U);
    else
      Steering_ManualAxisFault(1U, 9U);
  }
  if ((pending & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
  {
    uint8_t expect = (command < 0) ? STEER_LIMIT_R_MIN_MASK :
                      STEER_LIMIT_R_MAX_MASK;
    if ((pending & expect) != 0U &&
        (pending & (uint8_t)~expect &
         (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) == 0U)
      Steering_ManualAxisDone(0U, 3U);
    else
      Steering_ManualAxisFault(0U, 9U);
  }

  if ((uint32_t)(now - steer_manual_begin_ms) > STEER_MANUAL_RUN_MAX_MS)
  {
    if ((steer_manual_left_state == STEER_MANUAL_MOVING) ||
        (steer_manual_left_state == STEER_MANUAL_ESCAPE))
      Steering_ManualAxisFault(1U, 5U);
    if ((steer_manual_right_state == STEER_MANUAL_MOVING) ||
        (steer_manual_right_state == STEER_MANUAL_ESCAPE))
      Steering_ManualAxisFault(0U, 5U);
  }

  mask = Steering_TestRawLimitMask();
  Steering_ManualAxisUpdate(1U, command, mask, now, lpwm, &la);
  Steering_ManualAxisUpdate(0U, command, mask, now, rpwm, &ra);

  primask = __get_PRIMASK();
  __disable_irq();
  mask = Steering_TestRawLimitMask();
  /* Include IRQ edges that arrived after the earlier pending snapshot;
   * a noisy NC rising edge must NEVER be followed by re-energizing PWM.
   */
  pending = steer_manual_irq_pending_mask;
  steer_manual_irq_pending_mask = 0U;
  if ((pending & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
    Steering_ManualAxisFault(1U, 9U);
  if ((pending & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
    Steering_ManualAxisFault(0U, 9U);
  lstate = steer_manual_left_state;
  rstate = steer_manual_right_state;

  /* NC inputs must be re-evaluated ATOMICALLY before CCR writes.
   * At the expected target stop only this side; when escaping accept
   * ONLY its originating opposite limit (or stop on release).
   * Two asserted switches on one side are NEVER an escape condition.
   */
  if ((lstate == STEER_MANUAL_MOVING) ||
      (lstate == STEER_MANUAL_ESCAPE))
  {
    uint8_t own = (uint8_t)
        (mask & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK));
    uint8_t expected = (command < 0) ?
        STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_L_MAX_MASK;
    uint8_t opposite = (command < 0) ?
        STEER_LIMIT_L_MAX_MASK : STEER_LIMIT_L_MIN_MASK;
    if (lstate == STEER_MANUAL_ESCAPE)
    {
      if (own == 0U) Steering_ManualAxisDone(1U, 7U);
      else if (own != opposite) Steering_ManualAxisFault(1U, 9U);
    }
    else
    {
      if (own == expected) Steering_ManualAxisDone(1U, 3U);
      else if (own != 0U) Steering_ManualAxisFault(1U, 9U);
    }
  }
  if ((rstate == STEER_MANUAL_MOVING) ||
      (rstate == STEER_MANUAL_ESCAPE))
  {
    uint8_t own = (uint8_t)
        (mask & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK));
    uint8_t expected = (command < 0) ?
        STEER_LIMIT_R_MIN_MASK : STEER_LIMIT_R_MAX_MASK;
    uint8_t opposite = (command < 0) ?
        STEER_LIMIT_R_MAX_MASK : STEER_LIMIT_R_MIN_MASK;
    if (rstate == STEER_MANUAL_ESCAPE)
    {
      if (own == 0U) Steering_ManualAxisDone(0U, 7U);
      else if (own != opposite) Steering_ManualAxisFault(0U, 9U);
    }
    else
    {
      if (own == expected) Steering_ManualAxisDone(0U, 3U);
      else if (own != 0U) Steering_ManualAxisFault(0U, 9U);
    }
  }

  if ((steer_manual_left_state == STEER_MANUAL_MOVING) ||
      (steer_manual_left_state == STEER_MANUAL_ESCAPE))
  {
    HAL_GPIO_WritePin(MOTOR_DIR_TURN_left_GPIO_Port, MOTOR_DIR_TURN_left_Pin,
                      (command < 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, la);
    steer_manual_left_pwm = la;
  }
  else Steering_ManualLeftOff();

  if ((steer_manual_right_state == STEER_MANUAL_MOVING) ||
      (steer_manual_right_state == STEER_MANUAL_ESCAPE))
  {
    HAL_GPIO_WritePin(MOTOR_DIR_TURN_right_GPIO_Port, MOTOR_DIR_TURN_right_Pin,
                      (command < 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ra);
    steer_manual_right_pwm = ra;
  }
  else Steering_ManualRightOff();

  if (primask == 0U) __enable_irq();

  /* Do not re-arm automatically if held against a stop or stalled.
   * Preserve per-axis result for Live Expressions until next command.
   */
  if ((steer_manual_left_state >= STEER_MANUAL_DONE) &&
      (steer_manual_right_state >= STEER_MANUAL_DONE))
  {
    steer_manual_neutral_required = 1U;
    Steering_ManualStop();
  }
}


/* M7-S07 Select calibration — guarded, one-shot, two independent axes.
 * Safety rule: each endstop's rising NC-open EXTI cuts only its own CCR.
 * No motor may drive TOWARD an already asserted NC switch. Calibration
 * may depart an initially asserted switch ONLY away and <= 1000 ms.
 * Only commit new MIN/MAX when both axes reach the unchanged CENTER.
 *
 * NOTE: runtime RAM calibration only; reset returns to measured defaults.
 * No unapproved flash sector writes or unbounded automatic retries.
 */
#define STEER_CAL_SEEK_MIN       1U
#define STEER_CAL_EXIT_MIN       2U
#define STEER_CAL_SEEK_MAX       3U
#define STEER_CAL_EXIT_MAX       4U
#define STEER_CAL_TO_CENTER      5U
#define STEER_CAL_DONE           6U
#define STEER_CAL_PRE_EXIT_MAX   7U
/* Each axis can reach its own NC at a different time and torque. */
volatile uint16_t steer_cal_left_seek_timeout_ms = 12000U;
volatile uint16_t steer_cal_right_seek_timeout_ms = 9000U;
volatile uint16_t steer_cal_left_center_timeout_ms = 7000U;
volatile uint16_t steer_cal_right_center_timeout_ms = 6500U;
volatile uint16_t steer_cal_left_stall_timeout_ms = 1100U;
volatile uint16_t steer_cal_right_stall_timeout_ms = 850U;
#define STEER_CAL_EXIT_TIMEOUT_MS 1000U
#define STEER_CAL_OVERALL_TIMEOUT_MS 45000U
#define STEER_CAL_STALL_TIMEOUT_MS 850U
/* Hold steering CCR=0 after touching NC before reversing a motor.
 * This guards against interpreting residual INBOUND inertia as a
 * wrong-direction response to the OUTBOUND command.
 */
/* Independent settling windows: gear ratios and coasting differ. */
volatile uint16_t steer_cal_left_reverse_settle_ms = 150U;
volatile uint16_t steer_cal_right_reverse_settle_ms = 220U;
/* In addition to minimum settle time, both sides must show a quiet
 * AS5600 interval before any opposite-direction PWM is permitted.
 * Do not use a shared wait/torque assumption across the two motors.
 */
volatile uint16_t steer_cal_left_reverse_quiet_ms = 60U;
volatile uint16_t steer_cal_right_reverse_quiet_ms = 100U;
volatile uint8_t steer_cal_left_reverse_ready = 0U;
volatile uint8_t steer_cal_right_reverse_ready = 0U;
volatile uint32_t steer_cal_left_reverse_wait_ms = 0U;
volatile uint32_t steer_cal_right_reverse_wait_ms = 0U;
/* S07F: Return-to-CENTER polarity changes need a fresh motor-off,
 * AS5600-quiet transition. LEFT/RIGHT cannot share coast settings.
 */
volatile uint8_t steer_cal_left_center_reverse_wait = 0U;
volatile uint8_t steer_cal_right_center_reverse_wait = 0U;
volatile uint8_t steer_cal_left_center_reverse_count = 0U;
volatile uint8_t steer_cal_right_center_reverse_count = 0U;
volatile uint32_t steer_cal_left_center_reverse_elapsed_ms = 0U;
volatile uint32_t steer_cal_right_center_reverse_elapsed_ms = 0U;
/* A SINGLE reversed AS5600 sample near pressed NC stops CCR at once.
 * Only verified away motion may recover this one-sample ambiguity.
 * Otherwise fail closed; never disable true wrong-way protection.
 */
#define STEER_CAL_NOISE_HOLD_MAX_MS 220U
/* Hold PWM OFF until raw has genuinely settled. A stationary rotor
 * cannot spontaneously move AWAY while PWM is zero, so do NOT require
 * progress in the commanded direction just to unlock this guard.
 * One bounded re-drive is allowed only in the phase's SAFE direction.
 */
#define STEER_CAL_NOISE_HOLD_MIN_MS 80U
#define STEER_CAL_NOISE_QUIET_MS 60U
volatile uint8_t steer_cal_left_noise_hold = 0U;
volatile uint8_t steer_cal_right_noise_hold = 0U;
volatile uint32_t steer_cal_left_noise_hold_count = 0U;
volatile uint32_t steer_cal_right_noise_hold_count = 0U;
volatile uint32_t steer_cal_left_noise_unlock_count = 0U;
volatile uint32_t steer_cal_right_noise_unlock_count = 0U;
/* Subreason=3 means sensor did not settle during the powered-OFF
 * verification hold. Unlike reason1 it does not prove wrong-way drive.
 */
#define STEER_CAL_JUMP_MAX_RAW     90
#define STEER_CAL_STABLE_REQUIRED  3U /* legacy, not center capture */
/* Stop on approaching CENTER band and wait for AS5600 to become
 * physically quiet instead of driving back and forth around ±5 RAW.
 */
#define STEER_CAL_CENTER_QUIET_MS 120U

/* 0=idle, 1=active, 2=success, 3=reject/interlock, 4=sensor,
 * 5=stage/overall timeout, 6=limit/edge mismatch,
 * 7=no progress/wrong direction, 8=invalid measured span,
 * 9=operator STOP/PS2 loss/conflicting controls.
 */
volatile uint8_t steer_cal_arm = 0U;
volatile uint8_t steer_cal_result = 0U;
/* These diagnostics distinguish an actual completed scan from a later
 * SELECT press with no arm, which previously overwrote cal_result = 3.
 * A new run clears reject_reason; last_run_result remains sticky.
 */
volatile uint8_t steer_cal_last_run_result = 0U;
volatile uint8_t steer_cal_reject_reason = 0U;
/* 1 unarmed SELECT; 2 PS2 inputs not neutral; 3 STOP/Voice/priority;
 * 4 other steering mode active; 5 wheel motion; 6 debug/pulse conflict;
 * 7 NC initialization incomplete.
 */
volatile uint32_t steer_cal_unarmed_select_count = 0U;
volatile uint32_t steer_cal_rejected_start_count = 0U;
volatile uint32_t steer_cal_start_count = 0U;
volatile uint8_t steer_cal_left_stage = 0U;
volatile uint8_t steer_cal_right_stage = 0U;
volatile uint8_t steer_cal_left_fault = 0U;
volatile uint8_t steer_cal_right_fault = 0U;
volatile uint16_t steer_cal_left_min_found = 0U;
volatile uint16_t steer_cal_left_max_found = 0U;
volatile uint16_t steer_cal_right_min_found = 0U;
volatile uint16_t steer_cal_right_max_found = 0U;
volatile uint16_t steer_cal_left_pwm = 0U;
volatile uint16_t steer_cal_right_pwm = 0U;
/* Separate motor RPM/torque -> no shared seek/exit/center PWM or
 * unintended near-switch reduction below individual breakaway floor.
 * Live Expressions can tune each; hardware-tested bounds below remain.
 */
volatile uint16_t steer_cal_left_seek_duty = 200U;
volatile uint16_t steer_cal_right_seek_duty = 85U;
volatile uint16_t steer_cal_left_escape_duty = 200U;
volatile uint16_t steer_cal_right_escape_duty = 100U; /* within prior RIGHT cap */
volatile uint16_t steer_cal_left_center_duty = 175U;
volatile uint16_t steer_cal_right_center_duty = 85U;
/* Individual calibrated CENTER acceptance bands, never redefine CENTER.
 * 15 counts ~1.3 degrees, 20 counts ~1.8 degrees (4096 counts/turn).
 * Values are clamped 8..30 RAW; runtime tuning for each motor separately.
 */
volatile uint16_t steer_cal_left_center_tol_raw = 15U;
volatile uint16_t steer_cal_right_center_tol_raw = 20U;
volatile int16_t steer_cal_left_center_error = 0;
volatile int16_t steer_cal_right_center_error = 0;
volatile uint8_t steer_cal_left_center_capturing = 0U;
volatile uint8_t steer_cal_right_center_capturing = 0U;
volatile uint16_t steer_cal_left_center_accepted_raw = 0U;
volatile uint16_t steer_cal_right_center_accepted_raw = 0U;
volatile uint16_t steer_cal_left_approach_floor = 175U;
volatile uint16_t steer_cal_right_approach_floor = 85U;
/* Sticky diagnostics: source and affected axis for result 6.
 * 1 dual limits, 2 contradictory/bouncing IRQ, 3 wrong active limit,
 * 4 late contact while driving, 5 invalid release without movement.
 */
volatile uint8_t steer_cal_limit_fault_reason = 0U;
volatile uint8_t steer_cal_limit_fault_side = 0U; /* 1 left, 2 right */
volatile uint8_t steer_cal_last_irq_snapshot = 0U;
volatile uint8_t steer_cal_last_gpio_snapshot = 0U;
/* First actual limit fault snapshot (stable until a NEW accepted Select).
 * Old last_* snapshots were overwritten every pass and did not report
 * the first (pre-I2C) edge/GPIO sample when AxisUpdate returned 6.
 * side=1 LEFT / 2 RIGHT; bits RMAX=1 RMIN=2 LMIN=4 LMAX=8.
 */
volatile uint8_t steer_cal_limit_fault_stage = 0U;
volatile uint8_t steer_cal_limit_fault_irq = 0U;
volatile uint8_t steer_cal_limit_fault_gpio = 0U;
volatile uint16_t steer_cal_limit_fault_raw = 0U;
volatile uint32_t steer_cal_limit_fault_elapsed_ms = 0U;
volatile uint32_t steer_cal_limit_fault_scan_id = 0U;
/* A context copy is updated BEFORE both per-axis and final CCR checks.
 * Never use a later main-loop debug variable as the failure snapshot.
 */
static uint8_t steer_cal_eval_irq_mask = 0U;
static uint8_t steer_cal_eval_gpio_mask = 0U;
/* Motion result 7 is now attributed to one axis and one mechanism:
 * reason 1=wrong-way under last applied DIR, 2=no directional progress.
 * A sticky snapshot survives later unarmed SELECT presses.
 */
volatile uint8_t steer_cal_motion_fault_side = 0U;   /* 1 left, 2 right */
volatile uint8_t steer_cal_motion_fault_reason = 0U;
volatile uint8_t steer_cal_motion_fault_stage = 0U;
volatile uint16_t steer_cal_motion_fault_raw = 0U;
volatile int16_t steer_cal_motion_fault_delta = 0;
volatile uint16_t steer_cal_motion_fault_pwm = 0U;
volatile uint32_t steer_cal_motion_fault_age_ms = 0U;
volatile uint8_t steer_cal_motion_fault_limit_mask = 0U;
/* A no-motion escape may hit the 1s hard stage deadline before the
 * independent stall timer; preserve side/stage for result 5 as well.
 */
volatile uint8_t steer_cal_timeout_side = 0U;
volatile uint8_t steer_cal_timeout_stage = 0U;
volatile uint16_t steer_cal_timeout_raw = 0U;
volatile uint32_t steer_cal_timeout_elapsed_ms = 0U;
volatile uint16_t steer_cal_timeout_pwm = 0U;
volatile uint16_t steer_cal_left_last_raw = 0U;
volatile uint16_t steer_cal_right_last_raw = 0U;
volatile int16_t steer_cal_left_last_delta = 0;
volatile int16_t steer_cal_right_last_delta = 0;
volatile uint32_t steer_cal_left_progress_age_ms = 0U;
volatile uint32_t steer_cal_right_progress_age_ms = 0U;
volatile uint32_t steer_cal_completed_count = 0U;
volatile uint32_t steer_cal_failed_count = 0U;
volatile uint8_t steer_cal_irq_trip_mask = 0U;
static volatile uint8_t steer_cal_irq_pending_mask = 0U;
static uint8_t steer_cal_select_previous = 0U;
static uint32_t steer_cal_begin_ms = 0U;

typedef struct
{
  uint8_t stage;
  uint8_t stable;
  uint16_t last_raw;
  uint16_t start_raw;
  uint16_t min_raw;
  uint16_t max_raw;
  uint32_t stage_begin;
  uint32_t last_progress_ms;
  uint8_t last_drive_dir0;
  uint8_t last_drive_valid;
  uint8_t reverse_ready;
  uint8_t last_nonzero_dir0;
  uint8_t last_nonzero_valid;
  uint8_t center_reverse_wait;
  uint8_t center_reverse_count;
  uint8_t noise_hold;
  uint8_t noise_hold_count;
  uint8_t noise_expected_increase;
  uint32_t last_drive_ms;
  uint32_t reverse_quiet_since_ms;
  uint32_t center_reverse_start_ms;
  uint32_t center_quiet_since_ms;
  uint8_t center_capturing;
  uint32_t center_capture_quiet_since_ms;
  uint32_t noise_hold_start_ms;
  uint32_t noise_quiet_since_ms;
  uint16_t noise_quiet_start_raw;
} SteeringCalAxis_t;

static SteeringCalAxis_t steer_cal_left_axis = {0};
static SteeringCalAxis_t steer_cal_right_axis = {0};

static void Steering_CalLeftOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  steer_cal_left_pwm = 0U;
}

static void Steering_CalRightOff(void)
{
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
  steer_cal_right_pwm = 0U;
}

static void Steering_CalStop(uint8_t result)
{
  Steering_CalLeftOff();
  Steering_CalRightOff();
  if ((steer_cal_active != 0U) && (result != 2U))
    steer_cal_failed_count++;
  if (steer_cal_active != 0U)
    steer_cal_last_run_result = result;
  steer_cal_active = 0U;
  steer_cal_result = result;
}

/* Called in EXTI context. Never perform HAL I2C inside an IRQ. */
static void Steering_CalOnLimitInterrupt(uint8_t bit)
{
  if (steer_cal_active == 0U) return;
  steer_cal_irq_trip_mask |= bit;
  steer_cal_irq_pending_mask |= bit;
  if ((bit & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) != 0U)
    Steering_CalLeftOff();
  if ((bit & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)
    Steering_CalRightOff();
}

static uint8_t Steering_CalAxisValidSpan(uint8_t left,
                                        const SteeringCalAxis_t *axis)
{
  /* Must bracket the fixed CENTER with a physically meaningful span. */
  if (left != 0U)
  {
    return ((axis->max_raw >= 1000U) &&
            (axis->max_raw + 60U < STEER_POS_LEFT_CENTER_RAW) &&
            (axis->min_raw > STEER_POS_LEFT_CENTER_RAW + 60U) &&
            (axis->min_raw > axis->max_raw + 250U) &&
            (axis->min_raw < axis->max_raw + 1250U)) ? 1U : 0U;
  }
  return ((axis->min_raw >= 2200U) &&
          (axis->min_raw + 60U < STEER_POS_RIGHT_CENTER_RAW) &&
          (axis->max_raw > STEER_POS_RIGHT_CENTER_RAW + 60U) &&
          (axis->max_raw > axis->min_raw + 250U) &&
          (axis->max_raw < axis->min_raw + 1250U)) ? 1U : 0U;
}

static void Steering_CalAxisStage(SteeringCalAxis_t *axis, uint8_t next,
                                  uint16_t raw, uint32_t now)
{
  axis->stage = next;
  axis->stage_begin = now;
  axis->last_progress_ms = now;
  axis->start_raw = raw;
  axis->last_raw = raw;
  axis->stable = 0U;
  axis->last_drive_valid = 0U;
  axis->last_drive_ms = now;
  axis->reverse_ready = 0U;
  axis->reverse_quiet_since_ms = now;
  axis->center_reverse_wait = 0U;
  axis->center_capturing = 0U;
  axis->center_capture_quiet_since_ms = now;
  axis->noise_hold = 0U;
  axis->noise_hold_count = 0U;
  axis->noise_quiet_since_ms = now;
  axis->noise_quiet_start_raw = raw;
  /* Preserve the last physically applied direction for the transition
   * from EXIT_MAX to CENTER; otherwise a center overshoot could
   * reverse PWM immediately and trip the wrong-way guard.
   */
  if (next == STEER_CAL_SEEK_MIN)
    axis->last_nonzero_valid = 0U;
  if (next == STEER_CAL_TO_CENTER)
    axis->center_reverse_count = 0U;
  if (axis == &steer_cal_left_axis)
  {
    steer_cal_left_reverse_ready = 0U;
    steer_cal_left_reverse_wait_ms = 0U;
    steer_cal_left_center_reverse_wait = 0U;
    steer_cal_left_center_capturing = 0U;
    steer_cal_left_noise_hold = 0U;
    if (next == STEER_CAL_TO_CENTER)
      steer_cal_left_center_reverse_count = 0U;
  }
  else
  {
    steer_cal_right_reverse_ready = 0U;
    steer_cal_right_reverse_wait_ms = 0U;
    steer_cal_right_center_reverse_wait = 0U;
    steer_cal_right_center_capturing = 0U;
    steer_cal_right_noise_hold = 0U;
    if (next == STEER_CAL_TO_CENTER)
      steer_cal_right_center_reverse_count = 0U;
  }
}

static uint8_t Steering_CalLimitFault(uint8_t left, uint8_t reason)
{
  /* Keep FIRST actual failure; another axis can be evaluated in the
   * same cycle, but must not destroy the primary diagnosis.
   */
  if (steer_cal_limit_fault_reason == 0U)
  {
    steer_cal_limit_fault_side = left ? 1U : 2U;
    steer_cal_limit_fault_reason = reason;
    steer_cal_limit_fault_stage = left ? steer_cal_left_axis.stage :
                                         steer_cal_right_axis.stage;
    steer_cal_limit_fault_irq = steer_cal_eval_irq_mask;
    steer_cal_limit_fault_gpio = steer_cal_eval_gpio_mask;
    steer_cal_limit_fault_raw = left ? debug_as5600_left_raw :
                                      debug_as5600_right_raw;
    steer_cal_limit_fault_elapsed_ms =
        (uint32_t)(HAL_GetTick() - steer_cal_begin_ms);
    steer_cal_limit_fault_scan_id = steer_cal_start_count;
  }
  return 6U;
}

/* Expected duplicate EXTI from the same NC when backing away can be
 * suppressed for one control frame. Mismatched NC edges are fatal.
 */
static uint8_t Steering_CalEdgeSafe(uint8_t stage, uint8_t edge,
                                    uint8_t limit, uint8_t min_bit,
                                    uint8_t max_bit)
{
  if (edge == 0U) return 1U;
  if (stage == STEER_CAL_SEEK_MIN &&
      edge == min_bit && limit == min_bit) return 1U;
  if (stage == STEER_CAL_SEEK_MAX &&
      edge == max_bit && limit == max_bit) return 1U;
  if (stage == STEER_CAL_EXIT_MIN && edge == min_bit &&
      (limit == min_bit || limit == 0U)) return 1U;
  if ((stage == STEER_CAL_EXIT_MAX || stage == STEER_CAL_PRE_EXIT_MAX) &&
      edge == max_bit && (limit == max_bit || limit == 0U)) return 1U;
  return 0U;
}

/* A sticky, per-motor snapshot of the FIRST actual motion fault.
 * Never infer which axis failed from stage values alone.
 */
static uint8_t Steering_CalMotionFault(uint8_t left, uint8_t reason,
                                       uint8_t stage, uint16_t raw,
                                       int16_t delta, uint32_t now)
{
  /* First fault wins if both axes fail in the same loop.
   * The control loop already stops both; don't overwrite primary cause.
   */
  if (steer_cal_motion_fault_side == 0U)
  {
    steer_cal_motion_fault_side = left ? 1U : 2U;
    steer_cal_motion_fault_reason = reason;
    steer_cal_motion_fault_stage = stage;
    steer_cal_motion_fault_raw = raw;
    steer_cal_motion_fault_delta = delta;
    steer_cal_motion_fault_pwm =
        left ? steer_cal_left_pwm : steer_cal_right_pwm;
    steer_cal_motion_fault_age_ms =
        (uint32_t)(now - (left ? steer_cal_left_axis.last_progress_ms :
                                    steer_cal_right_axis.last_progress_ms));
    steer_cal_motion_fault_limit_mask = Steering_TestRawLimitMask();
  }
  return 7U;
}

static uint8_t Steering_CalStageTimeout(uint8_t left, uint8_t stage,
                                         uint16_t raw, uint32_t elapsed)
{
  if (steer_cal_timeout_side == 0U)
  {
    steer_cal_timeout_side = left ? 1U : 2U;
    steer_cal_timeout_stage = stage;
    steer_cal_timeout_raw = raw;
    steer_cal_timeout_elapsed_ms = elapsed;
    steer_cal_timeout_pwm = left ? steer_cal_left_pwm :
                                   steer_cal_right_pwm;
  }
  return 5U;
}

/* 0 = healthy. Nonzero result code aborts the entire calibration.
 * Each side has separate MIN/MAX, movement and time monitoring.
 */
static uint8_t Steering_CalAxisUpdate(uint8_t left, uint8_t raw_mask,
                                      uint8_t irq_mask, uint32_t now,
                                      uint16_t *pwm, GPIO_PinState *dir)
{
  SteeringCalAxis_t *axis = left ? &steer_cal_left_axis :
                                    &steer_cal_right_axis;
  uint8_t min_bit = left ? STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_R_MIN_MASK;
  uint8_t max_bit = left ? STEER_LIMIT_L_MAX_MASK : STEER_LIMIT_R_MAX_MASK;
  uint8_t limit = (uint8_t)(raw_mask & (min_bit | max_bit));
  uint8_t edge = (uint8_t)(irq_mask & (min_bit | max_bit));
  uint8_t stage = axis->stage;
  uint16_t raw = 0U, target = 0U, magnitude;
  int16_t movement, error;
  uint8_t direction_zero = 1U;
  uint32_t deadline;
  uint16_t requested = 0U;
  if ((stage == STEER_CAL_EXIT_MIN) ||
      (stage == STEER_CAL_EXIT_MAX) ||
      (stage == STEER_CAL_PRE_EXIT_MAX))
    requested = left ? steer_cal_left_escape_duty :
                       steer_cal_right_escape_duty;
  else if (stage == STEER_CAL_TO_CENTER)
    requested = left ? steer_cal_left_center_duty :
                       steer_cal_right_center_duty;
  else
    requested = left ? steer_cal_left_seek_duty :
                       steer_cal_right_seek_duty;

  *pwm = 0U;
  *dir = GPIO_PIN_RESET;
  if (stage == STEER_CAL_DONE)
  {
    /* While waiting for the other motor, keep this AS5600 live and
     * ensure a finished axis is still centered with all NCs closed.
     */
    if ((edge != 0U) || (limit != 0U))
      return Steering_CalLimitFault(left, 3U);
    if (Steering_PositionReadOne(left, &raw) == 0U) return 4U;
    /* DONE isn't permission to accept an off-center result after
     * waiting for a slower partner. Keep monitoring the finished
     * axis and reject the entire scan if it subsequently drifts.
     */
    {
      int16_t delta = AS5600_ShortestDeltaCounts(
          raw, left ? STEER_POS_LEFT_CENTER_RAW :
                      STEER_POS_RIGHT_CENTER_RAW);
      uint16_t abs_delta = (uint16_t)((delta < 0) ? -delta : delta);
      if (abs_delta > Steering_CenterTolClamp(
              left ? steer_cal_left_center_tol_raw :
                     steer_cal_right_center_tol_raw))
        return 8U; /* no actuator retry while partner is moving */
    }
    return 0U;
  }

  if ((Steering_PositionReadOne(left, &raw) == 0U) || (raw > 4095U))
    return 4U;
  if (limit == (min_bit | max_bit))
    return Steering_CalLimitFault(left, 1U);
  if (Steering_CalEdgeSafe(stage, edge, limit,
                           min_bit, max_bit) == 0U)
    return Steering_CalLimitFault(left, 2U);

  movement = AS5600_ShortestDeltaCounts(axis->last_raw, raw);
  if ((movement > STEER_CAL_JUMP_MAX_RAW) ||
      (movement < -STEER_CAL_JUMP_MAX_RAW))
    return 4U;
  axis->last_raw = raw;
  if (left != 0U)
  {
    steer_cal_left_last_raw = raw;
    steer_cal_left_last_delta = movement;
    steer_cal_left_progress_age_ms =
        (uint32_t)(now - axis->last_progress_ms);
  }
  else
  {
    steer_cal_right_last_raw = raw;
    steer_cal_right_last_delta = movement;
    steer_cal_right_progress_age_ms =
        (uint32_t)(now - axis->last_progress_ms);
  }

  if ((stage == STEER_CAL_EXIT_MIN) ||
      (stage == STEER_CAL_EXIT_MAX) ||
      (stage == STEER_CAL_PRE_EXIT_MAX))
    deadline = STEER_CAL_EXIT_TIMEOUT_MS;
  else if (stage == STEER_CAL_TO_CENTER)
    deadline = left ? steer_cal_left_center_timeout_ms :
                      steer_cal_right_center_timeout_ms;
  else
    deadline = left ? steer_cal_left_seek_timeout_ms :
                      steer_cal_right_seek_timeout_ms;
  if (deadline > 14000U) deadline = 14000U;
  if ((stage != STEER_CAL_EXIT_MIN) && (stage != STEER_CAL_EXIT_MAX) &&
      (stage != STEER_CAL_PRE_EXIT_MAX) && (deadline < 1500U))
    deadline = 1500U;

  if ((uint32_t)(now - axis->stage_begin) >= deadline)
    return Steering_CalStageTimeout(left, stage, raw,
                                     (uint32_t)(now - axis->stage_begin));

  if (stage == STEER_CAL_SEEK_MIN)
  {
    if (limit == min_bit)
    {
      axis->min_raw = raw;
      if (left != 0U) steer_cal_left_min_found = raw;
      else steer_cal_right_min_found = raw;
      Steering_CalAxisStage(axis, STEER_CAL_EXIT_MIN, raw, now);
      return 0U; /* zero-PWM cycle when changing direction */
    }
    if (limit == max_bit)
    {
      /* At startup we may already be on MAX: escape it safely before
       * proceeding MIN. Unrequested MAX during a running scan fails.
       */
      if ((uint32_t)(now - steer_cal_begin_ms) > 300U)
        return Steering_CalLimitFault(left, 3U);
      Steering_CalAxisStage(axis, STEER_CAL_PRE_EXIT_MAX, raw, now);
      return 0U;
    }
    direction_zero = 1U; /* DIR0 = both sides toward MIN */
  }
  else if ((stage == STEER_CAL_EXIT_MIN) ||
           (stage == STEER_CAL_EXIT_MAX) ||
           (stage == STEER_CAL_PRE_EXIT_MAX))
  {
    uint8_t from_min = (stage == STEER_CAL_EXIT_MIN) ? 1U : 0U;
    uint8_t own = from_min ? min_bit : max_bit;
    uint8_t unwanted = from_min ? max_bit : min_bit;
    int16_t traveled = AS5600_ShortestDeltaCounts(axis->start_raw, raw);
    uint8_t inc = (uint8_t)(((left != 0U) && (from_min == 0U)) ||
                            ((left == 0U) && (from_min != 0U)));
    if (limit == unwanted) return Steering_CalLimitFault(left, 3U);
    if (limit == 0U)
    {
      /* NC release must correspond to real measured away movement.
       * A flickering or broken NC is not a valid endpoint release.
       */
      if (((inc != 0U) && (traveled < 3)) ||
          ((inc == 0U) && (traveled > -3)))
        return Steering_CalLimitFault(left, 5U);
      if (stage == STEER_CAL_EXIT_MIN)
        Steering_CalAxisStage(axis, STEER_CAL_SEEK_MAX, raw, now);
      else if (stage == STEER_CAL_PRE_EXIT_MAX)
        Steering_CalAxisStage(axis, STEER_CAL_SEEK_MIN, raw, now);
      else
      {
        if (Steering_CalAxisValidSpan(left, axis) == 0U) return 8U;
        Steering_CalAxisStage(axis, STEER_CAL_TO_CENTER, raw, now);
      }
      return 0U; /* at least one full control frame with zero CCR */
    }
    if (limit != own) return Steering_CalLimitFault(left, 3U);
    /* A time-only wait can reverse while the motor is still COASTING
     * toward the switch. Wait for both independently tuned minimum dwell
     * and an AS5600 quiet window, with CCR zero the entire time.
     * Do not silently relax direction or NC safety checks after driving.
     */
    if (axis->reverse_ready == 0U)
    {
      uint16_t settle_ms = left ? steer_cal_left_reverse_settle_ms :
                                  steer_cal_right_reverse_settle_ms;
      uint16_t quiet_ms = left ? steer_cal_left_reverse_quiet_ms :
                                 steer_cal_right_reverse_quiet_ms;
      if (settle_ms < 80U) settle_ms = 80U;
      if (settle_ms > 350U) settle_ms = 350U;
      if (quiet_ms < 40U) quiet_ms = 40U;
      if (quiet_ms > 180U) quiet_ms = 180U;
      if ((movement > 1) || (movement < -1))
        axis->reverse_quiet_since_ms = now;
      if (left != 0U)
        steer_cal_left_reverse_wait_ms =
            (uint32_t)(now - axis->stage_begin);
      else
        steer_cal_right_reverse_wait_ms =
            (uint32_t)(now - axis->stage_begin);
      axis->last_progress_ms = now;
      if (((uint32_t)(now - axis->stage_begin) < settle_ms) ||
          ((uint32_t)(now - axis->reverse_quiet_since_ms) < quiet_ms))
        return 0U;
      axis->reverse_ready = 1U;
      axis->last_progress_ms = now;
      if (left != 0U) steer_cal_left_reverse_ready = 1U;
      else steer_cal_right_reverse_ready = 1U;
    }
    /* NC bounced during safe departure. Do not energize in this frame. */
    if (edge == own) return 0U;
    direction_zero = (from_min != 0U) ? 0U : 1U;
  }
  else if (stage == STEER_CAL_SEEK_MAX)
  {
    if (limit == max_bit)
    {
      axis->max_raw = raw;
      if (left != 0U) steer_cal_left_max_found = raw;
      else steer_cal_right_max_found = raw;
      Steering_CalAxisStage(axis, STEER_CAL_EXIT_MAX, raw, now);
      return 0U;
    }
    if (limit == min_bit) return Steering_CalLimitFault(left, 3U);
    direction_zero = 0U; /* DIR1 = both sides toward MAX */
  }
  else if (stage == STEER_CAL_TO_CENTER)
  {
    uint16_t center = left ? STEER_POS_LEFT_CENTER_RAW :
                             STEER_POS_RIGHT_CENTER_RAW;
    if (limit != 0U) return Steering_CalLimitFault(left, 3U);
    error = AS5600_ShortestDeltaCounts(raw, center);
    if (left != 0U) steer_cal_left_center_error = error;
    else steer_cal_right_center_error = error;
    {
      uint16_t tol = Steering_CenterTolClamp(
          left ? steer_cal_left_center_tol_raw :
                 steer_cal_right_center_tol_raw);
      uint16_t absolute = (uint16_t)((error < 0) ? -error : error);
      /* Entirely unpowered capture: don't correct 5-20-count
       * backlash with repeated opposite motor polarity. Motor may
       * coast slightly after entering the band. Accept only if
       * final RAW is still inside tolerance and has been quiet.
       */
      if (absolute <= tol)
      {
        if (axis->center_capturing == 0U)
        {
          axis->center_capturing = 1U;
          axis->center_capture_quiet_since_ms = now;
        }
        if ((movement > 1) || (movement < -1))
          axis->center_capture_quiet_since_ms = now;
        axis->last_progress_ms = now;
        if (left != 0U) steer_cal_left_center_capturing = 1U;
        else steer_cal_right_center_capturing = 1U;
        if ((uint32_t)(now - axis->center_capture_quiet_since_ms) >=
            STEER_CAL_CENTER_QUIET_MS)
        {
          axis->stage = STEER_CAL_DONE;
          if (left != 0U)
          {
            steer_cal_left_center_accepted_raw = raw;
            steer_cal_left_center_capturing = 0U;
          }
          else
          {
            steer_cal_right_center_accepted_raw = raw;
            steer_cal_right_center_capturing = 0U;
          }
        }
        return 0U;
      }
      if (axis->center_capturing != 0U)
      {
        axis->center_capturing = 0U;
        axis->last_progress_ms = now;
        if (left != 0U) steer_cal_left_center_capturing = 0U;
        else steer_cal_right_center_capturing = 0U;
      }
    }
    /* LEFT raw grows in DIR0; RIGHT raw shrinks in DIR0.
     * Reaching or passing CENTER can change the requested DIR while
     * the rotor/gearbox still coasts the old way. For both motors,
     * hold PWM at zero until the measured RAW is QUIET. This prevents
     * a perfectly valid last -7 count of RIGHT travel from being read
     * as failure of its newly commanded DIR1 correction.
     */
    direction_zero = left ? (error > 0) : (error < 0);
    if ((axis->center_reverse_wait == 0U) &&
        (axis->last_nonzero_valid != 0U) &&
        (direction_zero != axis->last_nonzero_dir0))
    {
      axis->center_reverse_wait = 1U;
      axis->center_reverse_start_ms = now;
      axis->center_quiet_since_ms = now;
      axis->center_reverse_count++;
      if (axis->center_reverse_count > 4U)
        return Steering_CalMotionFault(left, 2U, stage, raw,
                                       movement, now);
      if (left != 0U)
      {
        steer_cal_left_center_reverse_wait = 1U;
        steer_cal_left_center_reverse_count = axis->center_reverse_count;
      }
      else
      {
        steer_cal_right_center_reverse_wait = 1U;
        steer_cal_right_center_reverse_count = axis->center_reverse_count;
      }
    }
    if (axis->center_reverse_wait != 0U)
    {
      uint16_t settle_ms = left ? steer_cal_left_reverse_settle_ms :
                                  steer_cal_right_reverse_settle_ms;
      uint16_t quiet_ms = left ? steer_cal_left_reverse_quiet_ms :
                                 steer_cal_right_reverse_quiet_ms;
      if (settle_ms < 80U) settle_ms = 80U;
      if (settle_ms > 350U) settle_ms = 350U;
      if (quiet_ms < 40U) quiet_ms = 40U;
      if (quiet_ms > 180U) quiet_ms = 180U;
      if (movement > 1 || movement < -1)
        axis->center_quiet_since_ms = now;
      if (left != 0U)
        steer_cal_left_center_reverse_elapsed_ms =
            (uint32_t)(now - axis->center_reverse_start_ms);
      else
        steer_cal_right_center_reverse_elapsed_ms =
            (uint32_t)(now - axis->center_reverse_start_ms);
      axis->last_progress_ms = now;
      if (((uint32_t)(now - axis->center_reverse_start_ms) < settle_ms) ||
          ((uint32_t)(now - axis->center_quiet_since_ms) < quiet_ms))
        return 0U; /* CCR stays zero during entire wait */
      axis->center_reverse_wait = 0U;
      axis->last_nonzero_valid = 0U;
      if (left != 0U) steer_cal_left_center_reverse_wait = 0U;
      else steer_cal_right_center_reverse_wait = 0U;
      return 0U; /* another full zero-PWM frame before DIR change */
    }
  }
  else return Steering_CalLimitFault(left, 3U);

  /* Progress is only motion TOWARD the previously commanded
   * direction. Jitter or small reverse movement must not reset the
   * watchdog and mask a stuck or miswired axis.
   */
  if (axis->noise_hold != 0U)
  {
    /* Before S07G this branch required >=2 AS5600 counts of AWAY
     * motion with PWM=0. A motor at an asserted NC switch cannot
     * satisfy that condition while stationary: result7 after 220ms
     * was an unavoidable state-machine deadlock (delta=0, PWM=0).
     *
     * Instead, hold both the relevant CCR and last_drive_valid at zero
     * until a fresh measured quiet interval. Then permit ONE bounded
     * re-drive in the already validated stage direction. No movement
     * toward an asserted own NC is EVER authorized here.
     */
    uint8_t bad = ((axis->noise_expected_increase != 0U) ?
                    (movement < -5) : (movement > 5)) ? 1U : 0U;
    if (bad != 0U)
      return Steering_CalMotionFault(left, 1U, stage, raw,
                                      movement, now);
    /* Small 1-count-per-frame coasting must not masquerade as a
     * stationary encoder. Require BOTH low instantaneous delta and
     * <=2 counts TOTAL travel since the quiet window began.
     */
    {
      int16_t quiet_drift =
          AS5600_ShortestDeltaCounts(axis->noise_quiet_start_raw, raw);
      if ((movement > 1) || (movement < -1) ||
          (quiet_drift > 2) || (quiet_drift < -2))
      {
        axis->noise_quiet_since_ms = now;
        axis->noise_quiet_start_raw = raw;
      }
    }
    if ((uint32_t)(now - axis->noise_hold_start_ms) >=
        STEER_CAL_NOISE_HOLD_MAX_MS)
      return Steering_CalMotionFault(left, 3U, stage, raw,
                                      movement, now);
    if (((uint32_t)(now - axis->noise_hold_start_ms) <
         STEER_CAL_NOISE_HOLD_MIN_MS) ||
        ((uint32_t)(now - axis->noise_quiet_since_ms) <
         STEER_CAL_NOISE_QUIET_MS))
      return 0U; /* still PWM=0 */

    /* The output remains zero this entire frame; the NEXT cycle
     * may re-energize only after full NC/AS5600/stage and atomic
     * IRQ checks. The existing per-stage noise_hold_count=1
     * makes a second reverse-sign event a hard fault (not a loop).
     */
    axis->noise_hold = 0U;
    axis->last_drive_valid = 0U;
    axis->last_progress_ms = now;
    if (left != 0U)
    {
      steer_cal_left_noise_hold = 0U;
      steer_cal_left_noise_unlock_count++;
    }
    else
    {
      steer_cal_right_noise_hold = 0U;
      steer_cal_right_noise_unlock_count++;
    }
    return 0U;
  }
  if (axis->last_drive_valid != 0U)
  {
    uint8_t should_increase = left ? axis->last_drive_dir0 :
                                     (uint8_t)(!axis->last_drive_dir0);
    if (((should_increase != 0U) && (movement < -5)) ||
        ((should_increase == 0U) && (movement > 5)))
    {
      /* One suspect sample => PWM zero NOW, never keep pushing.
       * Recovery requires subsequent AS5600 progress in the
       * previously applied safe direction, not a timer alone.
       */
      /* One recovery attempt per stage only. Repeated apparent
       * reverse motion at an asserted NC is a real safety concern:
       * fail without another motor restart.
       */
      if (axis->noise_hold_count != 0U)
        return Steering_CalMotionFault(left, 1U, stage, raw,
                                        movement, now);
      axis->noise_hold_count = 1U;
      axis->noise_hold = 1U;
      axis->noise_expected_increase = should_increase;
      axis->noise_hold_start_ms = now;
      axis->noise_quiet_since_ms = now;
      axis->noise_quiet_start_raw = raw;
      if (left != 0U)
      {
        steer_cal_left_noise_hold = 1U;
        steer_cal_left_noise_hold_count++;
      }
      else
      {
        steer_cal_right_noise_hold = 1U;
        steer_cal_right_noise_hold_count++;
      }
      return 0U;
    }
    if (((should_increase != 0U) && (movement >= 2)) ||
        ((should_increase == 0U) && (movement <= -2)))
      axis->last_progress_ms = now;
  }
  {
    uint16_t stall_ms = left ? steer_cal_left_stall_timeout_ms :
                               steer_cal_right_stall_timeout_ms;
    if (stall_ms < 500U) stall_ms = 500U;
    if (stall_ms > 1600U) stall_ms = 1600U;
    if ((uint32_t)(now - axis->last_progress_ms) >= stall_ms)
      return Steering_CalMotionFault(left, 2U, stage, raw,
                                      movement, now);
  }

  /* Never let calibration exceed the independently tested PWM ceilings. */
  if (left != 0U)
  {
    if (requested > 300U) requested = 300U;
    if (requested < 100U) requested = 100U;
  }
  else
  {
    if (requested > 100U) requested = 100U;
    if (requested < 40U) requested = 40U;
  }
  /* Seek slowly near the previous physical estimate, but do not require
   * the old hardstop RAW to match: this routine calibrates a changed stop.
   */
  target = (stage == STEER_CAL_SEEK_MIN) ?
      (left ? steer_cal_left_min_runtime : steer_cal_right_min_runtime) :
      (stage == STEER_CAL_SEEK_MAX) ?
      (left ? steer_cal_left_max_runtime : steer_cal_right_max_runtime) :
      (left ? STEER_POS_LEFT_CENTER_RAW : STEER_POS_RIGHT_CENTER_RAW);
  error = AS5600_ShortestDeltaCounts(raw, target);
  magnitude = (uint16_t)((error < 0) ? -error : error);
  if ((magnitude < 80U) &&
      ((stage == STEER_CAL_SEEK_MIN) ||
       (stage == STEER_CAL_SEEK_MAX)))
  {
    uint16_t low = left ? steer_cal_left_approach_floor :
                           steer_cal_right_approach_floor;
    if (left != 0U)
    {
      if (low > 300U) low = 300U;
      if (low < 100U) low = 100U;
    }
    else
    {
      if (low > 100U) low = 100U;
      if (low < 40U) low = 40U;
    }
    if (requested > low)
      requested = (uint16_t)(low +
          ((uint32_t)(requested - low) * magnitude) / 80U);
  }
  *dir = direction_zero ? GPIO_PIN_RESET : GPIO_PIN_SET;
  *pwm = requested;
  return 0U;
}

/* Called inside one IRQ-masked final CCR commit. Any new NC edge
 * cancels that side's PWM THIS CYCLE, but an expected contact in SEEK
 * or repeated contact while EXITING is NOT a global fault.
 * The axis services the physical switch/transition next cycle.
 */
static uint8_t Steering_CalCommitAxisSafe(uint8_t left,
                                          uint8_t raw_mask,
                                          uint8_t late_edge,
                                          uint16_t *pwm,
                                          GPIO_PinState requested_dir)
{
  SteeringCalAxis_t *axis = left ? &steer_cal_left_axis :
                                    &steer_cal_right_axis;
  uint8_t min_bit = left ? STEER_LIMIT_L_MIN_MASK : STEER_LIMIT_R_MIN_MASK;
  uint8_t max_bit = left ? STEER_LIMIT_L_MAX_MASK : STEER_LIMIT_R_MAX_MASK;
  uint8_t limit = (uint8_t)(raw_mask & (min_bit | max_bit));
  uint8_t edge = (uint8_t)(late_edge & (min_bit | max_bit));
  uint8_t stage = axis->stage;

  if (limit == (min_bit | max_bit))
    return Steering_CalLimitFault(left, 1U);
  if (Steering_CalEdgeSafe(stage, edge, limit, min_bit, max_bit) == 0U)
    return Steering_CalLimitFault(left, 4U);

  if (stage == STEER_CAL_SEEK_MIN)
  {
    if ((limit == max_bit) ||
        ((*pwm != 0U) && (requested_dir != GPIO_PIN_RESET)))
      return Steering_CalLimitFault(left, 3U);
    if (limit == min_bit) *pwm = 0U;
  }
  else if (stage == STEER_CAL_SEEK_MAX)
  {
    if ((limit == min_bit) ||
        ((*pwm != 0U) && (requested_dir != GPIO_PIN_SET)))
      return Steering_CalLimitFault(left, 3U);
    if (limit == max_bit) *pwm = 0U;
  }
  else if ((stage == STEER_CAL_EXIT_MIN) ||
           (stage == STEER_CAL_EXIT_MAX) ||
           (stage == STEER_CAL_PRE_EXIT_MAX))
  {
    uint8_t origin = (stage == STEER_CAL_EXIT_MIN) ?
                       min_bit : max_bit;
    if ((limit != 0U) && (limit != origin))
      return Steering_CalLimitFault(left, 3U);
    /* Extra atomic direction interlock when retrying near a physically
     * asserted switch: EXIT_MAX may ONLY drive DIR0; EXIT_MIN DIR1.
     * The GPIO mode/polarity does not depend on AS5600 noise here.
     */
    if ((*pwm != 0U) &&
        (requested_dir != ((origin == max_bit) ?
                            GPIO_PIN_RESET : GPIO_PIN_SET)))
      return Steering_CalLimitFault(left, 3U);
    if (limit == 0U) *pwm = 0U; /* release: next frame transitions */
  }
  else if ((stage == STEER_CAL_TO_CENTER) ||
           (stage == STEER_CAL_DONE))
  {
    if (limit != 0U) return Steering_CalLimitFault(left, 3U);
    if (stage == STEER_CAL_DONE) *pwm = 0U;
  }
  else return Steering_CalLimitFault(left, 3U);

  /* A just-arrived switch rising edge must never be followed by
   * an immediate PWM re-energization even when the contact bounced.
   */
  if (edge != 0U) *pwm = 0U;
  return 0U;
}

static uint8_t Steering_CalInputsNeutral(void)
{
  int16_t rx = (int16_t)PS2_Pad.RX - 128;
  return (PS2_Pad.Connected == 1U &&
          PS2_Pad.Mode == 0x73U &&
          PS2_Pad.R1 == 0U && PS2_Pad.R2 == 0U &&
          PS2_Pad.L1 == 0U && PS2_Pad.L2 == 0U &&
          PS2_Pad.Start == 0U &&
          PS2_Pad.Up == 0U && PS2_Pad.Down == 0U &&
          PS2_Pad.Left == 0U && PS2_Pad.Right == 0U &&
          PS2_Pad.LY >= PS2_LY_FORWARD_LIMIT &&
          PS2_Pad.LY <= PS2_LY_REVERSE_LIMIT &&
          rx >= -10 && rx <= 10) ? 1U : 0U;
}

/* Called after Manual and R1/R2 services; owns PWM only when active.
 * SELECT edge starts, release does not cancel, any other PS2 command
 * or STOP aborts. This is NOT a background task.
 */
static void Steering_CalUpdate(void)
{
  uint8_t select_now = (PS2_Pad.Connected == 1U &&
                        PS2_Pad.Mode == 0x73U &&
                        PS2_Pad.Select != 0U) ? 1U : 0U;
  uint8_t new_press = (select_now != 0U &&
                       steer_cal_select_previous == 0U) ? 1U : 0U;
  uint8_t raw_limits, pending, lerr, rerr;
  uint16_t lraw, rraw, lpwm = 0U, rpwm = 0U;
  GPIO_PinState ldir = GPIO_PIN_RESET, rdir = GPIO_PIN_RESET;
  uint32_t primask;
  uint32_t now = HAL_GetTick();

  steer_cal_select_previous = select_now;
  if (steer_cal_active == 0U)
  {
    if (new_press == 0U) return;
    if (steer_cal_arm != 1U)
    {
      /* Do not erase the outcome of the preceding real scan merely
       * because SELECT is pressed again without an explicit re-arm.
       */
      steer_cal_reject_reason = 1U;
      steer_cal_unarmed_select_count++;
      return;
    }
    /* Select consumes the arm whether it succeeds or fails. */
    steer_cal_arm = 0U;
    if ((Steering_CalInputsNeutral() == 0U) ||
        (control_mode != CONTROL_MODE_PS2) ||
        (control_stop_latched != 0U) ||
        (ps2_cancel_hold != 0U) || (ps2_spin_rf_latched != 0U) ||
        (voice_execution_active != 0U) ||
        (voice_command_ready != 0U) ||
        (steer_manual_active != 0U) ||
        (steer_pos_active != 0U) ||
        (steer_test_running != 0U) ||
        (steer_test_arm != 0U) ||
        (steer_test_request != 0U) ||
        (debug_as5600_poll_enable != 0U) ||
        (Steering_PositionWheelsStopped() == 0U) ||
        (debug_limit_l_min.initialized == 0U) ||
        (debug_limit_l_max.initialized == 0U) ||
        (debug_limit_r_min.initialized == 0U) ||
        (debug_limit_r_max.initialized == 0U))
    {
      if (Steering_CalInputsNeutral() == 0U)
        steer_cal_reject_reason = 2U;
      else if ((control_mode != CONTROL_MODE_PS2) ||
               (control_stop_latched != 0U) ||
               (ps2_cancel_hold != 0U) ||
               (ps2_spin_rf_latched != 0U) ||
               (voice_execution_active != 0U) ||
               (voice_command_ready != 0U))
        steer_cal_reject_reason = 3U;
      else if ((steer_manual_active != 0U) ||
               (steer_pos_active != 0U))
        steer_cal_reject_reason = 4U;
      else if (Steering_PositionWheelsStopped() == 0U)
        steer_cal_reject_reason = 5U;
      else if ((steer_test_running != 0U) ||
               (steer_test_arm != 0U) ||
               (steer_test_request != 0U) ||
               (debug_as5600_poll_enable != 0U))
        steer_cal_reject_reason = 6U;
      else steer_cal_reject_reason = 7U;
      steer_cal_rejected_start_count++;
      steer_cal_result = 3U;
      return;
    }
    raw_limits = Steering_TestRawLimitMask();
    if (((raw_limits & (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) ==
         (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) ||
        ((raw_limits & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) ==
         (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)))
    {
      steer_cal_result = 6U; /* contradictory NC contacts */
      steer_cal_limit_fault_reason = 1U;
      steer_cal_limit_fault_side =
          ((raw_limits & (STEER_LIMIT_L_MIN_MASK |
                          STEER_LIMIT_L_MAX_MASK)) ==
           (STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_L_MAX_MASK)) ?
          1U : 2U;
      steer_cal_last_gpio_snapshot = raw_limits;
      return;
    }
    if ((Steering_PositionReadOne(1U, &lraw) == 0U) ||
        (Steering_PositionReadOne(0U, &rraw) == 0U))
    {
      steer_cal_result = 4U; /* AS5600 quality/communication */
      return;
    }

    steer_cal_left_axis.stage = STEER_CAL_SEEK_MIN;
    steer_cal_right_axis.stage = STEER_CAL_SEEK_MIN;
    steer_cal_begin_ms = now;
    Steering_CalAxisStage(&steer_cal_left_axis, STEER_CAL_SEEK_MIN,
                          lraw, now);
    Steering_CalAxisStage(&steer_cal_right_axis, STEER_CAL_SEEK_MIN,
                          rraw, now);
    steer_cal_left_min_found = 0U;
    steer_cal_left_max_found = 0U;
    steer_cal_right_min_found = 0U;
    steer_cal_right_max_found = 0U;
    steer_cal_left_stage = STEER_CAL_SEEK_MIN;
    steer_cal_right_stage = STEER_CAL_SEEK_MIN;
    steer_cal_left_fault = 0U;
    steer_cal_right_fault = 0U;
    steer_cal_irq_trip_mask = 0U;
    steer_cal_irq_pending_mask = 0U;
    steer_cal_limit_fault_reason = 0U;
    steer_cal_limit_fault_side = 0U;
    steer_cal_limit_fault_stage = 0U;
    steer_cal_limit_fault_irq = 0U;
    steer_cal_limit_fault_gpio = 0U;
    steer_cal_limit_fault_raw = 0U;
    steer_cal_limit_fault_elapsed_ms = 0U;
    steer_cal_limit_fault_scan_id = 0U;
    steer_cal_eval_gpio_mask = 0U;
    steer_cal_eval_irq_mask = 0U;
    steer_cal_motion_fault_side = 0U;
    steer_cal_motion_fault_reason = 0U;
    steer_cal_motion_fault_stage = 0U;
    steer_cal_motion_fault_raw = 0U;
    steer_cal_motion_fault_delta = 0;
    steer_cal_motion_fault_pwm = 0U;
    steer_cal_motion_fault_age_ms = 0U;
    steer_cal_motion_fault_limit_mask = 0U;
    steer_cal_timeout_side = 0U;
    steer_cal_timeout_stage = 0U;
    steer_cal_timeout_raw = 0U;
    steer_cal_timeout_elapsed_ms = 0U;
    steer_cal_timeout_pwm = 0U;
    steer_cal_left_last_raw = lraw;
    steer_cal_right_last_raw = rraw;
    steer_cal_left_last_delta = 0;
    steer_cal_right_last_delta = 0;
    steer_cal_left_progress_age_ms = 0U;
    steer_cal_right_progress_age_ms = 0U;
    steer_cal_left_reverse_ready = 0U;
    steer_cal_right_reverse_ready = 0U;
    steer_cal_left_reverse_wait_ms = 0U;
    steer_cal_right_reverse_wait_ms = 0U;
    steer_cal_left_center_reverse_wait = 0U;
    steer_cal_right_center_reverse_wait = 0U;
    steer_cal_left_center_reverse_count = 0U;
    steer_cal_right_center_reverse_count = 0U;
    steer_cal_left_center_error = 0;
    steer_cal_right_center_error = 0;
    steer_cal_left_center_accepted_raw = 0U;
    steer_cal_right_center_accepted_raw = 0U;
    steer_cal_left_center_capturing = 0U;
    steer_cal_right_center_capturing = 0U;
    steer_cal_left_center_reverse_elapsed_ms = 0U;
    steer_cal_right_center_reverse_elapsed_ms = 0U;
    steer_cal_left_noise_hold = 0U;
    steer_cal_right_noise_hold = 0U;
    steer_cal_left_noise_hold_count = 0U;
    steer_cal_right_noise_hold_count = 0U;
    steer_cal_left_noise_unlock_count = 0U;
    steer_cal_right_noise_unlock_count = 0U;
    steer_cal_last_irq_snapshot = 0U;
    steer_cal_last_gpio_snapshot = 0U;
    steer_cal_reject_reason = 0U;
    steer_cal_result = 1U;
    steer_cal_active = 1U;
    steer_cal_start_count++;
  }

  /* A radio fault never authorizes calibration to continue from a
   * stale frame. Any operator steering input aborts and zeros CCRs.
   */
  if ((control_mode != CONTROL_MODE_PS2) ||
      (control_stop_latched != 0U) ||
      (ps2_cancel_hold != 0U) ||
      (ps2_spin_rf_latched != 0U) ||
      (Steering_CalInputsNeutral() == 0U) ||
      (steer_cal_arm != 0U) ||
      (steer_manual_active != 0U) ||
      (steer_pos_active != 0U) ||
      (steer_test_running != 0U) ||
      (Steering_PositionWheelsStopped() == 0U))
  {
    Steering_CalStop(9U);
    return;
  }
  if ((uint32_t)(now - steer_cal_begin_ms) >
      STEER_CAL_OVERALL_TIMEOUT_MS)
  {
    Steering_CalStop(5U);
    return;
  }

  /* Snapshot NC edge flags. Even if a contact bounced low afterward,
   * its EXTI edge cannot be ignored and followed by fresh PWM.
   */
  primask = __get_PRIMASK();
  __disable_irq();
  pending = steer_cal_irq_pending_mask;
  steer_cal_irq_pending_mask = 0U;
  if (primask == 0U) __enable_irq();

  raw_limits = Steering_TestRawLimitMask();
  steer_cal_eval_irq_mask = pending;
  steer_cal_eval_gpio_mask = raw_limits;
  steer_cal_last_irq_snapshot = pending;
  steer_cal_last_gpio_snapshot = raw_limits;
  lerr = Steering_CalAxisUpdate(1U, raw_limits, pending, now, &lpwm, &ldir);
  rerr = Steering_CalAxisUpdate(0U, raw_limits, pending, now, &rpwm, &rdir);
  steer_cal_left_stage = steer_cal_left_axis.stage;
  steer_cal_right_stage = steer_cal_right_axis.stage;
  steer_cal_left_fault = lerr;
  steer_cal_right_fault = rerr;
  if ((lerr != 0U) || (rerr != 0U))
  {
    Steering_CalStop((lerr != 0U) ? lerr : rerr);
    return;
  }

  /* Independently validate LEFT and RIGHT before PWM writes, while
   * IRQs are masked. A normal right MIN arrival no longer aborts the
   * still-seeking LEFT. Unexpected NC faults still STOP BOTH axes.
   */
  primask = __get_PRIMASK();
  __disable_irq();
  pending = steer_cal_irq_pending_mask;
  steer_cal_irq_pending_mask = 0U;
  raw_limits = Steering_TestRawLimitMask();
  steer_cal_eval_irq_mask = pending;
  steer_cal_eval_gpio_mask = raw_limits;
  steer_cal_last_irq_snapshot = pending;
  steer_cal_last_gpio_snapshot = raw_limits;
  lerr = Steering_CalCommitAxisSafe(1U, raw_limits, pending, &lpwm,
                                    ldir);
  rerr = Steering_CalCommitAxisSafe(0U, raw_limits, pending, &rpwm,
                                    rdir);
  if ((lerr != 0U) || (rerr != 0U))
  {
    if (lerr != 0U) steer_cal_left_fault = lerr;
    if (rerr != 0U) steer_cal_right_fault = rerr;
    Steering_CalStop(6U);
  }
  else
  {
    if (lpwm != 0U)
      HAL_GPIO_WritePin(MOTOR_DIR_TURN_left_GPIO_Port,
                        MOTOR_DIR_TURN_left_Pin, ldir);
    if (rpwm != 0U)
      HAL_GPIO_WritePin(MOTOR_DIR_TURN_right_GPIO_Port,
                        MOTOR_DIR_TURN_right_Pin, rdir);
    __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, lpwm);
    __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, rpwm);
    steer_cal_left_pwm = lpwm;
    steer_cal_right_pwm = rpwm;
    steer_cal_left_axis.last_drive_valid = (lpwm != 0U) ? 1U : 0U;
    steer_cal_left_axis.last_drive_dir0 =
        (ldir == GPIO_PIN_RESET) ? 1U : 0U;
    if (lpwm != 0U)
    {
      steer_cal_left_axis.last_drive_ms = now;
      steer_cal_left_axis.last_nonzero_valid = 1U;
      steer_cal_left_axis.last_nonzero_dir0 =
          (ldir == GPIO_PIN_RESET) ? 1U : 0U;
    }
    steer_cal_right_axis.last_drive_valid = (rpwm != 0U) ? 1U : 0U;
    steer_cal_right_axis.last_drive_dir0 =
        (rdir == GPIO_PIN_RESET) ? 1U : 0U;
    if (rpwm != 0U)
    {
      steer_cal_right_axis.last_drive_ms = now;
      steer_cal_right_axis.last_nonzero_valid = 1U;
      steer_cal_right_axis.last_nonzero_dir0 =
          (rdir == GPIO_PIN_RESET) ? 1U : 0U;
    }
  }
  if (primask == 0U) __enable_irq();
  if (steer_cal_active == 0U) return;

  if ((steer_cal_left_axis.stage == STEER_CAL_DONE) &&
      (steer_cal_right_axis.stage == STEER_CAL_DONE))
  {
    /* Atomic (relative to this loop) runtime acceptance; never commit a
     * partially scanned pair. No CENTER calibration is performed.
     */
    if ((Steering_CalAxisValidSpan(1U, &steer_cal_left_axis) == 0U) ||
        (Steering_CalAxisValidSpan(0U, &steer_cal_right_axis) == 0U))
    {
      Steering_CalStop(8U);
      return;
    }
    steer_cal_left_min_runtime = steer_cal_left_axis.min_raw;
    steer_cal_left_max_runtime = steer_cal_left_axis.max_raw;
    steer_cal_right_min_runtime = steer_cal_right_axis.min_raw;
    steer_cal_right_max_runtime = steer_cal_right_axis.max_raw;
    steer_cal_completed_count++;
    Steering_CalStop(2U);
  }
}

static int16_t PS2_Convert_LY_To_PWM(uint8_t ly)
{
  if (ly < PS2_LY_FORWARD_LIMIT)
  {
    return (int16_t)(((uint32_t)(PS2_LY_FORWARD_LIMIT - ly) * MOTOR_PWM_MAX) /
                     PS2_LY_FORWARD_LIMIT);
  }

  if (ly > PS2_LY_REVERSE_LIMIT)
  {
    return (int16_t)(-((int32_t)(ly - PS2_LY_REVERSE_LIMIT) * MOTOR_PWM_MAX /
                      (255 - PS2_LY_REVERSE_LIMIT)));
  }

  return 0;
}


static void Turn_Motors_Stop(void)
{
  /*
   * M2 chưa điều khiển vị trí đánh lái.
   * Hai kênh PWM đánh lái chưa được HAL_TIM_PWM_Start(),
   * đồng thời compare luôn được giữ bằng 0.
   */
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
}

/*
 * STOP khẩn cấp cấp xe: wheel_control cắt 4 motor bánh và reset
 * state điều khiển; main giữ trách nhiệm dừng riêng 2 motor đánh lái.
 */
static void Wheel_EmergencyStop(void)
{
  WheelControl_EmergencyStop();
  Turn_Motors_Stop();
}


/* ============================================================
 * VC-02 M1 DEBUG
 * ============================================================
 */
static void VC02_M1_RecordEvent(const VC02_Event_t *event)
{
  if (event == NULL)
  {
    return;
  }

  vc02_event_count++;

  vc02_last_event_type = event->type;
  vc02_last_value = event->value;
  vc02_last_raw0 = event->raw0;
  vc02_last_raw1 = event->raw1;
}


/* ============================================================
 * VC-02 M2 - reset phần command đang xâu chuỗi.
 *
 * Không reset voice_speed_mode vì speed là mode độc lập.
 * ============================================================
 */
static void Voice_ResetPendingCommand(void)
{
  voice_pending_command.movement = VOICE_MOVE_NONE;
  voice_pending_command.turn = VOICE_TURN_NONE;
  voice_pending_command.angle_deg = 0U;

  voice_pending_command.movement_valid = 0U;
  voice_pending_command.turn_valid = 0U;
  voice_pending_command.angle_valid = 0U;

  voice_pending_command.movement_conflict = 0U;
  voice_pending_command.steering_conflict = 0U;
}


/*
 * Movement có thể đến ở bất kỳ vị trí nào trước TIME.
 * Lặp lại đúng cùng movement thì bỏ qua.
 * Movement khác với movement đã có -> đánh dấu mâu thuẫn,
 * không tự ghi đè. Muốn sửa phải dùng Clear command.
 */
static void Voice_SetMovement(VoiceMovement_t movement)
{
  if (voice_pending_command.movement_valid == 0U)
  {
    voice_pending_command.movement = movement;
    voice_pending_command.movement_valid = 1U;
    return;
  }

  if (voice_pending_command.movement != movement)
  {
    voice_pending_command.movement_conflict = 1U;
  }
}


/*
 * Turn có thể đến trước hoặc sau Angle/Movement.
 * Lặp lại cùng hướng thì bỏ qua.
 * Trái <-> phải mâu thuẫn thì không tự ghi đè.
 */
static void Voice_SetTurn(VoiceTurn_t turn)
{
  if (voice_pending_command.turn_valid == 0U)
  {
    voice_pending_command.turn = turn;
    voice_pending_command.turn_valid = 1U;
    return;
  }

  if (voice_pending_command.turn != turn)
  {
    voice_pending_command.steering_conflict = 1U;
  }
}


/*
 * Angle > 0 có thể đến trước hoặc sau Turn/Movement.
 * Hai góc khác nhau trong cùng chuỗi được coi là mâu thuẫn.
 */
static void Voice_SetAngle(uint8_t angle_deg)
{
  if (voice_pending_command.angle_valid == 0U)
  {
    voice_pending_command.angle_deg = angle_deg;
    voice_pending_command.angle_valid = 1U;
    return;
  }

  if (voice_pending_command.angle_deg != angle_deg)
  {
    voice_pending_command.steering_conflict = 1U;
  }
}


/*
 * Zero degrees là lệnh sửa riêng phần steering.
 * Nó xóa Turn + Angle pending và xóa cả mâu thuẫn steering,
 * nhưng giữ nguyên Movement và Speed.
 */
static void Voice_SetStraight(void)
{
  voice_pending_command.turn = VOICE_TURN_NONE;
  voice_pending_command.angle_deg = 0U;

  voice_pending_command.turn_valid = 0U;
  voice_pending_command.angle_valid = 0U;

  voice_pending_command.steering_conflict = 0U;
}


/*
 * TIME là trigger chốt chuỗi.
 * Nếu chuỗi chưa hợp lệ: không thực thi, không lưu TIME,
 * và giữ pending để người dùng có thể bổ sung hoặc Clear command.
 */
static uint8_t Voice_FinalizeCommand(uint8_t duration_sec)
{
  VoiceTurn_t final_turn = VOICE_TURN_NONE;
  uint8_t final_angle = 0U;

  voice_command_complete = 0U;

  if (voice_pending_command.movement_valid == 0U)
  {
    voice_last_build_result = VOICE_BUILD_ERROR_NO_MOVEMENT;
    voice_invalid_count++;
    return 0U;
  }

  if (voice_pending_command.movement_conflict != 0U)
  {
    voice_last_build_result = VOICE_BUILD_ERROR_MOVEMENT_CONFLICT;
    voice_invalid_count++;
    return 0U;
  }

  if (voice_pending_command.steering_conflict != 0U)
  {
    voice_last_build_result = VOICE_BUILD_ERROR_STEERING_CONFLICT;
    voice_invalid_count++;
    return 0U;
  }

  /*
   * Rotate in place hiện chưa dùng Turn/Angle.
   * Nếu người dùng đã đưa steering > 0 vào chuỗi rotate thì báo sai.
   */
  if (voice_pending_command.movement == VOICE_MOVE_ROTATE)
  {
    if ((voice_pending_command.turn_valid != 0U) ||
        (voice_pending_command.angle_valid != 0U))
    {
      voice_last_build_result = VOICE_BUILD_ERROR_ROTATE_WITH_STEERING;
      voice_invalid_count++;
      return 0U;
    }
  }
  else
  {
    /*
     * Chạy thường:
     * - không Turn, không Angle -> đi thẳng 0 độ: hợp lệ.
     * - có Turn và có Angle -> hợp lệ.
     * - chỉ có một trong hai -> thiếu thành phần steering.
     */
    if ((voice_pending_command.turn_valid != 0U) !=
        (voice_pending_command.angle_valid != 0U))
    {
      voice_last_build_result = VOICE_BUILD_ERROR_STEERING_INCOMPLETE;
      voice_invalid_count++;
      return 0U;
    }

    if ((voice_pending_command.turn_valid != 0U) &&
        (voice_pending_command.angle_valid != 0U))
    {
      final_turn = voice_pending_command.turn;
      final_angle = voice_pending_command.angle_deg;
    }
  }

  voice_completed_command.movement = voice_pending_command.movement;
  voice_completed_command.turn = final_turn;
  voice_completed_command.angle_deg = final_angle;
  voice_completed_command.speed_level = voice_speed_mode;
  voice_completed_command.duration_sec = duration_sec;

  voice_command_complete = 1U;
  voice_command_ready = 1U;
  voice_completed_count++;
  voice_last_build_result = VOICE_BUILD_RESULT_OK;

  /* Command đã chốt xong; tránh TIME sau đó làm sống lại command cũ. */
  Voice_ResetPendingCommand();

  return 1U;
}


/* ============================================================
 * M3 - kiểm tra pending có dữ liệu voice hay không.
 * Speed không tính vì speed là mode độc lập, được giữ lại.
 * ============================================================
 */
static uint8_t Voice_HasPendingData(void)
{
  if ((voice_pending_command.movement_valid != 0U) ||
      (voice_pending_command.turn_valid != 0U) ||
      (voice_pending_command.angle_valid != 0U) ||
      (voice_pending_command.movement_conflict != 0U) ||
      (voice_pending_command.steering_conflict != 0U))
  {
    return 1U;
  }

  return 0U;
}


/*
 * Hủy voice do PS2 giành quyền.
 * Không reset voice_speed_mode.
 */
static void Voice_CancelByPS2(void)
{
  uint8_t had_voice = 0U;

  if ((Voice_HasPendingData() != 0U) ||
      (voice_command_ready != 0U) ||
      (voice_execution_active != 0U))
  {
    had_voice = 1U;
  }

  Voice_ResetPendingCommand();

  voice_command_complete = 0U;
  voice_command_ready = 0U;
  voice_execution_active = 0U;
  voice_last_build_result = VOICE_BUILD_RESULT_NONE;

  if (had_voice != 0U)
  {
    voice_cancel_by_ps2_count++;
  }
}


/*
 * STOP ưu tiên cao nhất.
 * Dừng motor ngay, hủy pending/ready/active voice.
 *
 * Ở M3 chưa có điều khiển vị trí steering bằng AS5600 nên
 * chỉ dừng motor đánh lái. Lệnh home 0 độ sẽ được bổ sung
 * ở M7/M9 khi vòng điều khiển góc đã tồn tại.
 */
static void Control_RequestStop(void)
{
  Voice_ResetPendingCommand();

  voice_command_complete = 0U;
  voice_command_ready = 0U;
  voice_execution_active = 0U;
  voice_last_build_result = VOICE_BUILD_RESULT_NONE;

  control_stop_latched = 1U;
  control_stop_count++;

  voice_execution_pwm = 0;

  Wheel_EmergencyStop();
}


/*
 * Đổi 5 mức speed Voice sang command scale -999..999.
 * Khi PID bật, wheel_control sẽ ánh xạ command này sang target RPM.
 */
static uint16_t Voice_SpeedLevelToPWM(uint8_t speed_level)
{
  switch (speed_level)
  {
    case VC02_SPEED_VERY_SLOW:
      return VOICE_PWM_VERY_SLOW;

    case VC02_SPEED_SLOW:
      return VOICE_PWM_SLOW;

    case VC02_SPEED_MEDIUM:
      return VOICE_PWM_MEDIUM;

    case VC02_SPEED_FAST:
      return VOICE_PWM_FAST;

    case VC02_SPEED_FULL:
      return VOICE_PWM_FULL;

    default:
      return 0U;
  }
}


/*
 * M4' chỉ cho chạy thẳng tiến/lùi.
 * Command có Rotate hoặc steering khác 0 chưa được phép chạy.
 */
static uint8_t Voice_IsOpenLoopCommandSupported(
    volatile const VoiceVehicleCommand_t *command)
{
  if (command == NULL)
  {
    return 0U;
  }

  if ((command->movement != VOICE_MOVE_FORWARD) &&
      (command->movement != VOICE_MOVE_BACKWARD))
  {
    return 0U;
  }

  if ((command->turn != VOICE_TURN_NONE) ||
      (command->angle_deg != 0U))
  {
    return 0U;
  }

  if ((command->duration_sec == 0U) ||
      (Voice_SpeedLevelToPWM(command->speed_level) == 0U))
  {
    return 0U;
  }

  return 1U;
}


/*
 * Nhận command đã TIME chốt vào vùng execution và bắt đầu timer.
 * Không dùng HAL_Delay(duration), vì STOP/PS2 vẫn phải phản hồi ngay.
 */
static void Voice_StartExecution(void)
{
  uint16_t pwm_magnitude;

  voice_active_command.movement = voice_completed_command.movement;
  voice_active_command.turn = voice_completed_command.turn;
  voice_active_command.angle_deg = voice_completed_command.angle_deg;
  voice_active_command.speed_level = voice_completed_command.speed_level;
  voice_active_command.duration_sec = voice_completed_command.duration_sec;

  voice_command_ready = 0U;

  if (Voice_IsOpenLoopCommandSupported(
          &voice_active_command) == 0U)
  {
    voice_execution_active = 0U;
    voice_unsupported_command_count++;
    return;
  }

  /* Voice hiện là lệnh tốc độ rời rạc; dùng cùng profile FIXED với autonomous. */
  WheelControl_SetCommandProfile(WHEEL_COMMAND_PROFILE_FIXED);

  pwm_magnitude = Voice_SpeedLevelToPWM(voice_active_command.speed_level);

  if (voice_active_command.movement == VOICE_MOVE_BACKWARD)
  {
    voice_execution_pwm = -(int16_t)pwm_magnitude;
  }
  else
  {
    voice_execution_pwm = (int16_t)pwm_magnitude;
  }

  voice_execution_start_tick = HAL_GetTick();
  voice_execution_duration_ms =
      (uint32_t)voice_active_command.duration_sec * 1000U;
  voice_execution_elapsed_ms = 0U;

  voice_execution_active = 1U;
}


/*
 * Phát command truyền động 4 bánh, không blocking.
 * wheel_control quyết định open-loop hay closed-loop theo wheel_pid_enable.
 */
static void Voice_ExecuteMotorControl(void)
{
  uint32_t now;

  if (voice_execution_active == 0U)
  {
    return;
  }

  now = HAL_GetTick();
  voice_execution_elapsed_ms =
      (uint32_t)(now - voice_execution_start_tick);

  if (voice_execution_elapsed_ms >= voice_execution_duration_ms)
  {
    /* Hết TIME là dừng bình thường -> giảm tốc bằng ramp. */
    WheelControl_SetTargetPWM(0);
    Turn_Motors_Stop();

    voice_execution_pwm = 0;
    voice_execution_active = 0U;
    voice_execution_finish_count++;
    return;
  }

  WheelControl_SetTargetPWM(voice_execution_pwm);
  Turn_Motors_Stop();
}



/*
 * M7 / PS2 mapping - decoded request telemetry.
 * Values reflect manual-steering requests; the separate arbitration and
 * limit-protected services below decide if PWM actually reaches motors.
 * Right-hand joystick RX is the selected steering axis; LY stays wheel drive.
 *
 * Steering direction semantic: D-pad LEFT / RX<128 => both towards MIN,
 * D-pad RIGHT / RX>128 => both towards MAX.
 * PWM units are timer compare counts (ARR=999).
 */
#define PS2_RX_DEADZONE_COUNTS 10
volatile uint8_t debug_ps2_new_mapping_valid = 0U;
volatile uint8_t debug_ps2_request_center = 0U;
volatile uint8_t debug_ps2_request_min = 0U;
volatile int8_t debug_ps2_spin_direction = 0; /* +1 L1, -1 L2 */
volatile int8_t debug_ps2_steer_direction = 0; /* -1 MIN, +1 MAX */
volatile uint16_t debug_ps2_steer_left_pwm = 0U;
volatile uint16_t debug_ps2_steer_right_pwm = 0U;
volatile uint8_t debug_ps2_rx_value = 128U;
volatile uint8_t debug_ps2_input_conflict = 0U; /* bit0 R1+R2, bit1 L1+L2, bit2 left+right */
volatile uint32_t debug_ps2_mapping_sample_count = 0U;

static void PS2_ObserveNewMapping(void)
{
  int16_t rx_offset;
  uint16_t magnitude;

  debug_ps2_new_mapping_valid = 0U;
  debug_ps2_request_center = 0U;
  debug_ps2_request_min = 0U;
  debug_ps2_spin_direction = 0;
  debug_ps2_steer_direction = 0;
  debug_ps2_steer_left_pwm = 0U;
  debug_ps2_steer_right_pwm = 0U;
  debug_ps2_input_conflict = 0U;
  debug_ps2_rx_value = PS2_Pad.RX;

  if ((PS2_Pad.Connected != 1U) || (PS2_Pad.Mode != 0x73U))
  {
    return;
  }

  debug_ps2_new_mapping_valid = 1U;
  debug_ps2_mapping_sample_count++;

  if ((PS2_Pad.R1 != 0U) && (PS2_Pad.R2 != 0U))
    debug_ps2_input_conflict |= 1U;
  else if (PS2_Pad.R1 != 0U)
    debug_ps2_request_center = 1U;
  else if (PS2_Pad.R2 != 0U)
    debug_ps2_request_min = 1U;

  if ((PS2_Pad.L1 != 0U) && (PS2_Pad.L2 != 0U))
    debug_ps2_input_conflict |= 2U;
  else if (PS2_Pad.L1 != 0U)
    debug_ps2_spin_direction = 1;
  else if (PS2_Pad.L2 != 0U)
    debug_ps2_spin_direction = -1;

  if ((PS2_Pad.Left != 0U) && (PS2_Pad.Right != 0U))
  {
    debug_ps2_input_conflict |= 4U;
    return;
  }

  /* D-pad has priority over RX when one direction is held. */
  if (PS2_Pad.Left != 0U)
  {
    debug_ps2_steer_direction = -1;
    debug_ps2_steer_left_pwm =
        Steering_ManualClampPWM(steer_manual_left_dpad_pwm);
    debug_ps2_steer_right_pwm =
        Steering_ManualClampPWM(steer_manual_right_dpad_pwm);
    return;
  }
  if (PS2_Pad.Right != 0U)
  {
    debug_ps2_steer_direction = 1;
    debug_ps2_steer_left_pwm =
        Steering_ManualClampPWM(steer_manual_left_dpad_pwm);
    debug_ps2_steer_right_pwm =
        Steering_ManualClampPWM(steer_manual_right_dpad_pwm);
    return;
  }

  rx_offset = (int16_t)PS2_Pad.RX - 128;
  if ((rx_offset >= -PS2_RX_DEADZONE_COUNTS) &&
      (rx_offset <= PS2_RX_DEADZONE_COUNTS))
    return;

  debug_ps2_steer_direction = (rx_offset < 0) ? -1 : 1;
  magnitude = (uint16_t)((rx_offset < 0) ? -rx_offset : rx_offset);
  if (magnitude > 127U) magnitude = 127U;
  magnitude -= PS2_RX_DEADZONE_COUNTS;

  /* Mapping debug reflects the currently configured manual steering
   * request; applied PWM may still be zero at an AS5600 or NC limit.
   */
  debug_ps2_steer_left_pwm = Steering_ManualRXDuty(
      steer_manual_left_rx_min_pwm, steer_manual_left_rx_max_pwm, magnitude);
  debug_ps2_steer_right_pwm = Steering_ManualRXDuty(
      steer_manual_right_rx_min_pwm, steer_manual_right_rx_max_pwm, magnitude);
}

/*
 * M3 hiện công nhận các thao tác PS2 đang được project sử dụng:
 * - LY tiến/lùi
 * - D-pad Up/Down
 * - D-pad Left/Right cũng được tính là manual override để dành
 *   cho steering ở milestone sau.
 *
 * Chỉ kết nối PS2 nhưng không chạm điều khiển KHÔNG được phép
 * cướp quyền của voice.
 */
static uint8_t PS2_IsManualControlActive(void)
{
  if ((PS2_Pad.Connected != 1U) ||
      (PS2_Pad.Mode != 0x73U))
  {
    return 0U;
  }

  if ((PS2_Pad.LY < PS2_LY_FORWARD_LIMIT) ||
      (PS2_Pad.LY > PS2_LY_REVERSE_LIMIT) ||
      (PS2_Pad.Up != 0U) ||
      (PS2_Pad.Down != 0U) ||
      (PS2_Pad.Left != 0U) ||
      (PS2_Pad.Right != 0U) ||
      (PS2_Pad.L1 != 0U) ||
      (PS2_Pad.L2 != 0U) ||
      (PS2_Pad.R1 != 0U) ||
      (PS2_Pad.R2 != 0U) ||
      (steer_pos_active != 0U) ||
      (steer_manual_active != 0U) ||
      (steer_cal_active != 0U) ||
      (PS2_Pad.Select != 0U) ||
      (Steering_ManualRequestDirection() != 0) ||
      (WheelControl_SpinIsActive() != 0U))
  {
    return 1U;
  }

  return 0U;
}


/*
 * Điều khiển PS2 hiện tại: chỉ 4 motor bánh.
 * Steering PS2 sẽ được thêm khi motor đánh lái được triển khai.
 */
static void PS2_ApplyManualControl(void)
{
  int16_t pwm_duty = PS2_Convert_LY_To_PWM(PS2_Pad.LY);

  /* R1/R2 own steering only; wheel drive cannot overlap.
   * Position PWM is serviced separately after arbitration.
   */
  if ((PS2_Pad.R1 != 0U) || (PS2_Pad.R2 != 0U) ||
      (PS2_Pad.Select != 0U) || (steer_cal_active != 0U) ||
      (steer_pos_active != 0U))
  {
    WheelControl_SetTargetPWM(0);
    if ((steer_pos_active == 0U) && (steer_cal_active == 0U))
      Turn_Motors_Stop();
    return;
  }

  /* L1: LU/LD forward, RU/RD reverse; L2: opposite.
   * The spin shaper latches until stop; no unrelated wheel mode can
   * instantly replace the opposite-side target during coast-down.
   */
  if ((PS2_Pad.L1 != 0U) || (PS2_Pad.L2 != 0U) ||
      (WheelControl_SpinIsActive() != 0U))
  {
    WheelControl_SetCommandProfile(WHEEL_COMMAND_PROFILE_FIXED);
    if ((PS2_Pad.L1 != 0U) && (PS2_Pad.L2 == 0U))
      WheelControl_SetSpinDirection(1, MOTOR_DPAD_PWM);
    else if ((PS2_Pad.L2 != 0U) && (PS2_Pad.L1 == 0U))
      WheelControl_SetSpinDirection(-1, MOTOR_DPAD_PWM);
    else
    {
      /* Both buttons held or released: request a ramped stop. */
      WheelControl_SetSpinDirection(0, 0);
      WheelControl_SetTargetPWM(0);
    }

    Turn_Motors_Stop(); /* steering is not yet PS2-actuated */
    return;
  }

  /*
   * LY analog = MANUAL profile.
   * D-pad Up/Down = một cấp tốc độ cố định nên dùng FIXED profile.
   * Như vậy bài test nút bấm thật sự kiểm tra đúng shaper fixed-speed.
   */
  if (pwm_duty != 0)
  {
    WheelControl_SetCommandProfile(WHEEL_COMMAND_PROFILE_MANUAL);
  }
  else if (PS2_Pad.Up == 1U)
  {
    WheelControl_SetCommandProfile(WHEEL_COMMAND_PROFILE_FIXED);
    pwm_duty = MOTOR_DPAD_PWM;
  }
  else if (PS2_Pad.Down == 1U)
  {
    WheelControl_SetCommandProfile(WHEEL_COMMAND_PROFILE_FIXED);
    pwm_duty = -MOTOR_DPAD_PWM;
  }

  WheelControl_SetTargetPWM(pwm_duty);
  /* The steering service below owns TIM12/TIM14 during direct steering.
   * Stopping them here on each ~20ms PS2 loop formerly defeated RX/D-pad.
   * Neutral/release is cut in Steering_ManualUpdate(), not here.
   */
  if ((steer_manual_active == 0U) &&
      (Steering_ManualRequestDirection() == 0))
    Turn_Motors_Stop();
}


/*
 * ============================================================
 * M3 - ARBITRATION
 *
 * Priority: STOP > PS2 > VOICE > IDLE
 * ============================================================
 */
static void Control_Arbitrate(void)
{
  ps2_manual_active = PS2_IsManualControlActive();

  /*
   * 1) STOP cao nhất.
   *
   * STOP vẫn là dừng khẩn cấp: bỏ qua ramp và cắt PWM ngay.
   * Nếu PS2 đang bị giữ lệch/nhấn, latch giữ STOP cho tới khi
   * người dùng nhả tay về neutral.
   */
  if (control_stop_latched != 0U)
  {
    control_mode = CONTROL_MODE_STOP;
    Wheel_EmergencyStop();

    if (ps2_manual_active == 0U)
    {
      control_stop_latched = 0U;
    }

    return;
  }

  /*
   * M7 fail-safe: during in-place spin, radio loss is not a normal
   * joystick release. Cut all wheel PWM and wait for reconnection
   * followed by a neutral PS2 packet before enabling manual control.
   * This does not modify the pre-existing Voice/STOP latch behavior.
   */
  if (((PS2_Pad.Connected != 1U) || (PS2_Pad.Mode != 0x73U)) &&
      ((WheelControl_SpinIsActive() != 0U) ||
       (steer_pos_active != 0U) || (steer_manual_active != 0U) ||
       (steer_cal_active != 0U)))
    ps2_spin_rf_latched = 1U;

  if (ps2_spin_rf_latched != 0U)
  {
    control_mode = CONTROL_MODE_STOP;
    Wheel_EmergencyStop();
    if ((PS2_Pad.Connected == 1U) && (PS2_Pad.Mode == 0x73U) &&
        (ps2_manual_active == 0U))
      ps2_spin_rf_latched = 0U;
    return;
  }

  /*
   * 2) PS2 CANCEL HOLD.
   *
   * Khi thao tác PS2 vừa được dùng để hủy Voice, thao tác đó chỉ
   * có chức năng CANCEL + STOP, KHÔNG được phép điều khiển motor.
   *
   * Giữ trạng thái này cho tới khi toàn bộ input PS2 trở về neutral.
   * Sau khi đã neutral, một thao tác PS2 MỚI mới được quyền điều khiển.
   *
   * Dừng ở đây là dừng bình thường qua ramp để tránh giật cơ khí.
   */
  if (ps2_cancel_hold != 0U)
  {
    control_mode = CONTROL_MODE_IDLE;

    WheelControl_SetTargetPWM(0);
    Turn_Motors_Stop();

    if (ps2_manual_active == 0U)
    {
      ps2_cancel_hold = 0U;
    }

    return;
  }

  /*
   * 3) Có thao tác PS2.
   */
  if (ps2_manual_active != 0U)
  {
    /*
     * Nếu Voice đang pending / ready / active thì thao tác PS2
     * đầu tiên CHỈ hủy Voice và yêu cầu xe giảm tốc về 0.
     *
     * Không gọi PS2_ApplyManualControl() trong nhánh này.
     */
    if ((voice_execution_active != 0U) ||
        (voice_command_ready != 0U) ||
        (Voice_HasPendingData() != 0U))
    {
      Voice_CancelByPS2();

      ps2_cancel_hold = 1U;
      ps2_cancel_stop_count++;

      control_mode = CONTROL_MODE_IDLE;

      WheelControl_SetTargetPWM(0);
      Turn_Motors_Stop();

      return;
    }

    /*
     * Không còn Voice và PS2 không bị cancel-hold:
     * PS2 được điều khiển bình thường.
     */
    control_mode = CONTROL_MODE_PS2;
    PS2_ApplyManualControl();
    return;
  }

  /*
   * 4) Không có PS2 manual. Nếu có command Voice vừa TIME chốt
   * và chưa có Voice đang active thì nhận command đó.
   */
  if ((voice_execution_active == 0U) &&
      (voice_command_ready != 0U))
  {
    Voice_StartExecution();
  }

  if ((voice_execution_active != 0U) ||
      (voice_command_ready != 0U))
  {
    control_mode = CONTROL_MODE_VOICE;

    Voice_ExecuteMotorControl();

    /*
     * Hàm trên có thể vừa kết thúc command vì hết TIME.
     */
    if (voice_execution_active == 0U)
    {
      control_mode = CONTROL_MODE_IDLE;

      /* Dừng bình thường: ramp về 0 thay vì cắt PWM đột ngột. */
      WheelControl_SetTargetPWM(0);
      Turn_Motors_Stop();
    }

    return;
  }

  /*
   * 5) Không ai có quyền điều khiển.
   */
  control_mode = CONTROL_MODE_IDLE;

  WheelControl_SetTargetPWM(0);
  /* M7-S02: a manually armed 200-ms pulse owns the steering PWM in IDLE.
   * Do not blank its output on every arbitration iteration; the pulse
   * service and limit EXTI stop it. All STOP/PS2/Voice paths above still
   * call Turn_Motors_Stop() immediately as before.
   */
  if (steer_test_running == 0U)
  {
    Turn_Motors_Stop();
  }
}

/* ============================================================
 * VC-02 M2 - nhận event và gom command không phụ thuộc thứ tự.
 * ============================================================
 */
static void VC02_M2_ProcessEvent(const VC02_Event_t *event)
{
  if (event == NULL)
  {
    return;
  }

  /* Giữ lại toàn bộ biến debug của M1. */
  VC02_M1_RecordEvent(event);

  /*
   * M4' - khi một voice command đã READY hoặc đang chạy:
   * - STOP vẫn dừng ngay.
   * - SPEED được phép đổi cho command kế tiếp.
   * - CLEAR chỉ xóa pending, không dừng command đã chốt/đang chạy.
   * - Movement/Turn/Angle/Time mới không được xếp hàng tự chạy.
   */
  if ((voice_execution_active != 0U) ||
      (voice_command_ready != 0U))
  {
    if (event->type == VC02_EVENT_STOP)
    {
      Control_RequestStop();
      voice_stop_count++;
      return;
    }

    if (event->type == VC02_EVENT_SPEED)
    {
      if ((event->value >= VC02_SPEED_VERY_SLOW) &&
          (event->value <= VC02_SPEED_FULL))
      {
        voice_speed_mode = event->value;
      }
      return;
    }

    if (event->type == VC02_EVENT_CLEAR)
    {
      /* Clear chỉ xóa chuỗi pending; không hủy command đã chốt/đang chạy. */
      Voice_ResetPendingCommand();
      voice_clear_count++;
      return;
    }

    if (event->type == VC02_EVENT_WAKEUP)
    {
      return;
    }

    voice_ignored_during_execution_count++;
    return;
  }

  switch (event->type)
  {
    case VC02_EVENT_WAKEUP:
      /* Wake-up không thay đổi chuỗi pending. */
      break;

    case VC02_EVENT_FORWARD:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      Voice_SetMovement(VOICE_MOVE_FORWARD);
      break;

    case VC02_EVENT_BACKWARD:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      Voice_SetMovement(VOICE_MOVE_BACKWARD);
      break;

    case VC02_EVENT_ROTATE_IN_PLACE:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      Voice_SetMovement(VOICE_MOVE_ROTATE);
      break;

    case VC02_EVENT_TURN_LEFT:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      Voice_SetTurn(VOICE_TURN_LEFT);
      break;

    case VC02_EVENT_TURN_RIGHT:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      Voice_SetTurn(VOICE_TURN_RIGHT);
      break;

    case VC02_EVENT_ANGLE:
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;

      if (event->value == 0U)
      {
        Voice_SetStraight();
      }
      else
      {
        Voice_SetAngle(event->value);
      }
      break;

    case VC02_EVENT_SPEED:
      /*
       * Speed là mode độc lập và có thể nói ở bất kỳ lúc nào.
       * Không xóa pending, không chốt command.
       */
      if ((event->value >= VC02_SPEED_VERY_SLOW) &&
          (event->value <= VC02_SPEED_FULL))
      {
        voice_speed_mode = event->value;
      }
      break;

    case VC02_EVENT_DURATION:
      /* TIME là lệnh cuối dùng để kiểm tra và chốt command. */
      (void)Voice_FinalizeCommand(event->value);
      break;

    case VC02_EVENT_CLEAR:
      Voice_ResetPendingCommand();
      voice_command_complete = 0U;
      voice_last_build_result = VOICE_BUILD_RESULT_NONE;
      voice_clear_count++;
      break;

    case VC02_EVENT_STOP:
      /*
       * STOP cao nhất trong arbitration M3.
       * Dừng ngay và hủy pending/ready/active voice.
       */
      Control_RequestStop();
      voice_stop_count++;
      break;

    default:
      break;
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_SPI1_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();
  MX_TIM1_Init();
  MX_TIM9_Init();
  MX_TIM2_Init();
  MX_LPTIM1_Init();
  MX_TIM8_Init();
  MX_TIM10_Init();
  MX_TIM11_Init();
  MX_TIM12_Init();
  MX_TIM14_Init();
  MX_USART6_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  /*
   * Allow the externally-powered AS5600 module/pull-ups to settle first.
   * Recover/check BOTH buses before any AS5600 transaction is allowed.
   * This lets one known-good module be moved between LEFT and RIGHT during
   * manual steering calibration without changing firmware again.
   */
  HAL_Delay(AS5600_STARTUP_SETTLE_MS);
  AS5600_I2C1_BusRecover();
  AS5600_I2C2_BusRecover();

  /*
   * AS5600 steering sensors:
   *   RIGHT -> I2C1
   *   LEFT  -> I2C2
   * Chỉ đọc góc để manual calibration; steering motor vẫn chưa được start PWM.
   */
  AS5600_Init(&As5600_Right, &hi2c1);
  AS5600_Init(&As5600_Left, &hi2c2);
  AS5600_DebugResetCalibration();

  PS2_Init(&hspi1, PS2_CS_GPIO_Port, PS2_CS_Pin);

  if (HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  /* M7-S02: start steering PWM channels at ZERO duty only.
   * No steering motion can occur without manual Live Expressions request.
   */
  __HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, 0U);
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0U);
  if (HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL) != HAL_OK ||
      HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL) != HAL_OK ||
      HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL) != HAL_OK ||
      HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK)
  {
    Error_Handler();
  }

  /*
   * Mốc ban đầu cho bộ đếm mở rộng 32-bit.
   */
  debug_enc_lu_prev = (uint16_t)__HAL_TIM_GET_COUNTER(&htim8);
  debug_enc_ru_prev = (uint16_t)__HAL_TIM_GET_COUNTER(&htim1);
  debug_enc_ld_prev = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
  debug_enc_rd_prev = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

  debug_enc_lu_total = 0;
  debug_enc_ru_total = 0;
  debug_enc_ld_total = 0;
  debug_enc_rd_total = 0;

  /*
   * Encoder 2 motor đánh lái:
   *
   * LEFT  -> TIM4 encoder mode
   * RIGHT -> LPTIM1 encoder mode
   *
   * Không đưa vào Error_Handler nếu start steering encoder lỗi để vẫn
   * có thể chạy chương trình và xem trực tiếp mức A/B trong debugger.
   */
  debug_turn_left_start_ok =
      (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) == HAL_OK) ? 1U : 0U;

  debug_turn_right_start_ok =
      (HAL_LPTIM_Encoder_Start(&hlptim1, 0xFFFFU) == HAL_OK) ? 1U : 0U;

  Motor_Init_TIM(&Motor_LeftUp, &htim9, TIM_CHANNEL_1,
                 MOTOR_DIR_left_up_GPIO_Port, MOTOR_DIR_left_up_Pin,
                 &htim8, WHEEL_ENCODER_COUNTS_PER_REV_LU,
                 MOTOR_LEFT_UP_DIR_INVERTED);

  Motor_Init_TIM(&Motor_RightUp, &htim11, TIM_CHANNEL_1,
                 MOTOR_DIR_right_up_GPIO_Port, MOTOR_DIR_right_up_Pin,
                 &htim1, WHEEL_ENCODER_COUNTS_PER_REV_OTHER,
                 MOTOR_RIGHT_UP_DIR_INVERTED);

  Motor_Init_TIM(&Motor_LeftDown, &htim10, TIM_CHANNEL_1,
                 MOTOR_DIR_left_down_GPIO_Port, MOTOR_DIR_left_down_Pin,
                 &htim2, WHEEL_ENCODER_COUNTS_PER_REV_OTHER,
                 MOTOR_LEFT_DOWN_DIR_INVERTED);

  Motor_Init_TIM(&Motor_RightDown, &htim9, TIM_CHANNEL_2,
                 MOTOR_DIR_right_down_GPIO_Port, MOTOR_DIR_right_down_Pin,
                 &htim3, WHEEL_ENCODER_COUNTS_PER_REV_OTHER,
                 MOTOR_RIGHT_DOWN_DIR_INVERTED);

  /*
   * Chuẩn hóa dấu encoder độc lập với cờ đảo DIR motor.
   * Sau đoạn này: chiều xe tiến được quy ước là RPM dương.
   */
  Motor_SetEncoderDirectionInverted(&Motor_LeftUp,
                                    ENCODER_LEFT_UP_DIR_INVERTED);
  Motor_SetEncoderDirectionInverted(&Motor_RightUp,
                                    ENCODER_RIGHT_UP_DIR_INVERTED);
  Motor_SetEncoderDirectionInverted(&Motor_LeftDown,
                                    ENCODER_LEFT_DOWN_DIR_INVERTED);
  Motor_SetEncoderDirectionInverted(&Motor_RightDown,
                                    ENCODER_RIGHT_DOWN_DIR_INVERTED);

  /*
   * Gắn 4 Motor_HandleTypeDef vào module wheel_control.
   * LU đã có encoder 16 P/R; closed-loop LU được chuẩn bị trong wheel_control.
   * Cờ wheel_encoder_available_lu khởi động ở 0 để kiểm tra dấu encoder trước,
   * sau đó có thể bật =1 trong Live Expressions mà không cần flash lại.
   */
  WheelControl_Init(&Motor_LeftUp,
                    &Motor_RightUp,
                    &Motor_LeftDown,
                    &Motor_RightDown,
                    MOTOR_PWM_MAX);

  RamLogger_Init(&huart3,
                 &Motor_LeftUp,
                 &Motor_LeftDown,
                 &Motor_RightUp,
                 &Motor_RightDown);

  /* Capture reset cause and restore validated completed log if SRAM survived. */
  {
    uint32_t reset_flags = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
    RamLogger_BootRecover(reset_flags);
  }

  Wheel_EmergencyStop();

  /* Khởi tạo command builder M2; speed mặc định đã là Medium. */
  Voice_ResetPendingCommand();

  /* Bắt đầu nhận từng byte lệnh từ VC-02 bằng ngắt USART6. */
  if (VC02_Begin(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* M7-S01 limit switch inputs only: sample regardless of control mode. */
    SteeringLimits_DebugUpdate();

    /*
     * ==========================================================
     * VC-02 - M2 BUILDER + M3 ARBITRATION + M6 WHEEL CONTROL
     * ==========================================================
     *
     * Lấy hết event trong FIFO và gom command theo kiểu
     * không phụ thuộc thứ tự. TIME là trigger chốt cuối.
     * TIME tạo command hoàn chỉnh; arbitration cấp quyền cho VOICE,
     * còn wheel_control thực thi open-loop hoặc PID tùy mode M6.
     */
    while (VC02_GetEvent(&vc02_event) != 0U)
    {
      VC02_M2_ProcessEvent(&vc02_event);
    }

    vc02_dropped_events = VC02_GetDroppedEventCount();


    /*
     * ==========================================================
     * ENCODER
     * ==========================================================
     */
    Motor_Update_RPM(&Motor_LeftUp);
    Motor_Update_RPM(&Motor_RightUp);
    Motor_Update_RPM(&Motor_LeftDown);
    Motor_Update_RPM(&Motor_RightDown);

    /*
     * Copy counter phần cứng ra biến thường để Live Expressions
     * đọc ổn định, tránh lỗi "Failed to evaluate expression"
     * khi nhập trực tiếp TIMx->CNT.
     */
    debug_enc_lu_cnt = __HAL_TIM_GET_COUNTER(&htim8);
    debug_enc_ru_cnt = __HAL_TIM_GET_COUNTER(&htim1);
    debug_enc_ld_cnt = __HAL_TIM_GET_COUNTER(&htim2);
    debug_enc_rd_cnt = __HAL_TIM_GET_COUNTER(&htim3);

    /*
     * Mở rộng counter 16-bit thành total 32-bit có dấu.
     * Ép về int16_t xử lý đúng cả wrap 65535->0 và 0->65535.
     * RU/RD được đảo dấu để chiều xe tiến có total dương như LD.
     */
    {
      uint16_t current;

      current = (uint16_t)debug_enc_lu_cnt;
      {
        int16_t d = (int16_t)(current - debug_enc_lu_prev);
        debug_enc_lu_total += (Motor_LeftUp.encoder_direction_inverted != 0U) ? -d : d;
      }
      debug_enc_lu_prev = current;

      current = (uint16_t)debug_enc_ru_cnt;
      {
        int16_t d = (int16_t)(current - debug_enc_ru_prev);
        debug_enc_ru_total += (Motor_RightUp.encoder_direction_inverted != 0U) ? -d : d;
      }
      debug_enc_ru_prev = current;

      current = (uint16_t)debug_enc_ld_cnt;
      {
        int16_t d = (int16_t)(current - debug_enc_ld_prev);
        debug_enc_ld_total += (Motor_LeftDown.encoder_direction_inverted != 0U) ? -d : d;
      }
      debug_enc_ld_prev = current;

      current = (uint16_t)debug_enc_rd_cnt;
      {
        int16_t d = (int16_t)(current - debug_enc_rd_prev);
        debug_enc_rd_total += (Motor_RightDown.encoder_direction_inverted != 0U) ? -d : d;
      }
      debug_enc_rd_prev = current;
    }

    /*
     * Encoder 2 motor đánh lái.
     */
    debug_turn_left_cnt = __HAL_TIM_GET_COUNTER(&htim4);
    debug_turn_right_cnt = (uint32_t)hlptim1.Instance->CNT;

    /*
     * Đọc trực tiếp mức A/B để phân biệt lỗi encoder/dây với lỗi peripheral.
     * GPIO vẫn đọc được IDR khi chân đang ở Alternate Function.
     */
    debug_turn_left_a =
        (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12) == GPIO_PIN_SET) ? 1U : 0U;
    debug_turn_left_b =
        (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13) == GPIO_PIN_SET) ? 1U : 0U;

    debug_turn_right_a =
        (HAL_GPIO_ReadPin(GPIOG, GPIO_PIN_12) == GPIO_PIN_SET) ? 1U : 0U;
    debug_turn_right_b =
        (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_SET) ? 1U : 0U;

    /*
     * AS5600 telemetry/calibration tạm tắt mặc định trong giai đoạn tune 4 bánh.
     * Các giao dịch I2C debug là blocking và trước đây đã kéo chu kỳ main
     * từ ~24-25 ms lên ~50 ms khi một bus/sensor không sẵn sàng.
     *
     * Chỉ bật debug_as5600_poll_enable = 1 khi cần đọc AS5600 và xe đang dừng.
     */
    if (debug_as5600_poll_enable != 0U)
    {
      AS5600_DebugUpdate();
    }


    /*
     * ==========================================================
     * PS2 + M3 ARBITRATION
     * ==========================================================
     *
     * Đọc PS2 trước, sau đó arbitration quyết định ai có quyền:
     * STOP > PS2 manual > Voice > Idle.
     *
     * Khi Voice đang chạy, thao tác PS2 đầu tiên chỉ HỦY Voice và
     * yêu cầu xe ramp về 0. Phải nhả PS2 về neutral rồi thao tác lại
     * thì PS2 mới được phép điều khiển motor.
     *
     * PS2 chỉ "được kết nối" nhưng đang neutral sẽ không đè Voice.
     */
    PS2_Read_Data();
    PS2_ObserveNewMapping(); /* decoded requests; output gating is separate */

    Control_Arbitrate();

    /*
     * USER button posts only a request from EXTI.
     * Process it here, before wheel-control, so a manual validation
     * start can enable PID in the same control cycle.
     */
    RamLogger_ButtonUpdate();


    /*
     * M6 wheel-control module:
     * - ramp + reverse dead-time;
     * - PID OFF: giữ open-loop cũ;
     * - PID ON: feedforward + PID tốc độ từng bánh có encoder.
     */
    WheelControl_Update();

    /*
     * Auto feedforward-calibration test:
     * chỉ cần đặt wheel_ff_test_start = 1 trong Live Expressions.
     * Wheel-control module yêu cầu logger start/stop qua 2 pulse flag này.
     */
    if (wheel_ff_test_log_start_request != 0U)
    {
      wheel_ff_test_log_start_request = 0U;
      wheel_log_start = 1U;
    }

    if (wheel_ff_test_log_stop_request != 0U)
    {
      wheel_ff_test_log_stop_request = 0U;
      wheel_log_stop = 1U;
    }

    /*
     * Ghi dữ liệu chạy xe vào RAM.
     * Logger chỉ thực sự ghi khi wheel_log_active = 1.
     */
    RamLogger_Update(PS2_Pad.LY, (uint8_t)control_mode);

    /* M7-S02: left 20%, right 10%: manual direction tests <=5s.
     * Leaving an active L_MIN, L_MAX, or R_MIN: <=1s and require
     * a NEW manual request after verified switch release.
     */
    Steering_TestPulseUpdate();
    /* Release the previous manual PWM owner BEFORE checking a fresh
     * R1/R2 edge. Otherwise the position command gets rejected simply
     * because steer_manual_active was still set for this same cycle.
     */
    Steering_ManualUpdate();   /* D-pad LEFT/RIGHT and joystick RX */
    Steering_PositionUpdate(); /* R1/R2 independent closed-loop */
    Steering_CalUpdate();      /* SELECT: guarded auto MIN/MAX scan */
    Steering_PositionTelemetryIdleUpdate(); /* AS5600 live without poll flag */

    /* CSV dump is accepted only after logging is complete and PWM = 0. */
    RamLogger_DumpCsvUpdate();

    HAL_Delay(20);  /* Chu kỳ điều khiển khoảng 50 Hz */

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  /* 48-MHz I2C kernel clock, Standard-mode ~100 kHz (same as I2C2). */
  hi2c1.Init.Timing = 0xB0420F13;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  /* 48-MHz I2C kernel clock, Standard-mode ~100 kHz (RM0410 Table 217). */
  hi2c2.Init.Timing = 0xB0420F13;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief LPTIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{

  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
  hlptim1.Init.UltraLowPowerClock.Polarity = LPTIM_CLOCKPOLARITY_RISING_FALLING;
  hlptim1.Init.UltraLowPowerClock.SampleTime = LPTIM_CLOCKSAMPLETIME_4TRANSITIONS;
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_EXTERNAL;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 4;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 4;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 4;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 4;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 4;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 4;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 4;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 4;
  if (HAL_TIM_Encoder_Init(&htim4, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 4;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 4;
  if (HAL_TIM_Encoder_Init(&htim8, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief TIM9 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM9_Init(void)
{

  /* USER CODE BEGIN TIM9_Init 0 */

  /* USER CODE END TIM9_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM9_Init 1 */

  /* USER CODE END TIM9_Init 1 */
  htim9.Instance = TIM9;
  htim9.Init.Prescaler = 5;
  htim9.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim9.Init.Period = 999;
  htim9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim9.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim9, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim9) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim9, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM9_Init 2 */

  /* USER CODE END TIM9_Init 2 */
  HAL_TIM_MspPostInit(&htim9);

}

/**
  * @brief TIM10 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM10_Init(void)
{

  /* USER CODE BEGIN TIM10_Init 0 */

  /* USER CODE END TIM10_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM10_Init 1 */

  /* USER CODE END TIM10_Init 1 */
  htim10.Instance = TIM10;
  htim10.Init.Prescaler = 5;
  htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim10.Init.Period = 999;
  htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim10) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim10, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM10_Init 2 */

  /* USER CODE END TIM10_Init 2 */
  HAL_TIM_MspPostInit(&htim10);

}

/**
  * @brief TIM11 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM11_Init(void)
{

  /* USER CODE BEGIN TIM11_Init 0 */

  /* USER CODE END TIM11_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM11_Init 1 */

  /* USER CODE END TIM11_Init 1 */
  htim11.Instance = TIM11;
  htim11.Init.Prescaler = 5;
  htim11.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim11.Init.Period = 999;
  htim11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim11.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim11) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim11, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM11_Init 2 */

  /* USER CODE END TIM11_Init 2 */
  HAL_TIM_MspPostInit(&htim11);

}

/**
  * @brief TIM12 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM12_Init(void)
{

  /* USER CODE BEGIN TIM12_Init 0 */

  /* USER CODE END TIM12_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM12_Init 1 */

  /* USER CODE END TIM12_Init 1 */
  htim12.Instance = TIM12;
  htim12.Init.Prescaler = 5;
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim12.Init.Period = 999;
  htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim12, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM12_Init 2 */

  /* USER CODE END TIM12_Init 2 */
  HAL_TIM_MspPostInit(&htim12);

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 5;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 999;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */
  HAL_TIM_MspPostInit(&htim14);

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, MOTOR_DIR_left_up_Pin|MOTOR_DIR_right_down_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, MOTOR_DIR_TURN_right_Pin|MOTOR_DIR_right_up_Pin|MOTOR_DIR_left_down_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MOTOR_DIR_TURN_left_GPIO_Port, MOTOR_DIR_TURN_left_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : MOTOR_DIR_left_up_Pin MOTOR_DIR_right_down_Pin */
  GPIO_InitStruct.Pin = MOTOR_DIR_left_up_Pin|MOTOR_DIR_right_down_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LIMIT_R_MAX_Pin LIMIT_R_MIN_Pin */
  GPIO_InitStruct.Pin = LIMIT_R_MAX_Pin|LIMIT_R_MIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : MOTOR_DIR_TURN_right_Pin MOTOR_DIR_right_up_Pin MOTOR_DIR_left_down_Pin */
  GPIO_InitStruct.Pin = MOTOR_DIR_TURN_right_Pin|MOTOR_DIR_right_up_Pin|MOTOR_DIR_left_down_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PS2_CS_Pin */
  GPIO_InitStruct.Pin = PS2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(PS2_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LIMIT_L_MIN_Pin LIMIT_L_MAX_Pin */
  GPIO_InitStruct.Pin = LIMIT_L_MIN_Pin|LIMIT_L_MAX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : MOTOR_DIR_TURN_left_Pin */
  GPIO_InitStruct.Pin = MOTOR_DIR_TURN_left_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MOTOR_DIR_TURN_left_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == USER_Btn_Pin)
  {
    RamLogger_OnUserButtonInterrupt(HAL_GetTick());
  }
  else if (GPIO_Pin == LIMIT_R_MAX_Pin)
  {
    debug_limit_r_max.exti_rise_count++;
    Steering_TestOnLimitInterrupt(0x01U);
    Steering_PositionOnLimitInterrupt(0x01U);
    Steering_ManualOnLimitInterrupt(0x01U);
    Steering_CalOnLimitInterrupt(0x01U);
  }
  else if (GPIO_Pin == LIMIT_R_MIN_Pin)
  {
    debug_limit_r_min.exti_rise_count++;
    Steering_TestOnLimitInterrupt(0x02U);
    Steering_PositionOnLimitInterrupt(0x02U);
    Steering_ManualOnLimitInterrupt(0x02U);
    Steering_CalOnLimitInterrupt(0x02U);
  }
  else if (GPIO_Pin == LIMIT_L_MIN_Pin)
  {
    debug_limit_l_min.exti_rise_count++;
    Steering_TestOnLimitInterrupt(0x04U);
    Steering_PositionOnLimitInterrupt(0x04U);
    Steering_ManualOnLimitInterrupt(0x04U);
    Steering_CalOnLimitInterrupt(0x04U);
  }
  else if (GPIO_Pin == LIMIT_L_MAX_Pin)
  {
    debug_limit_l_max.exti_rise_count++;
    Steering_TestOnLimitInterrupt(0x08U);
    Steering_PositionOnLimitInterrupt(0x08U);
    Steering_ManualOnLimitInterrupt(0x08U);
    Steering_CalOnLimitInterrupt(0x08U);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  VC02_UART_RxCpltCallback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  VC02_UART_ErrorCallback(huart);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
