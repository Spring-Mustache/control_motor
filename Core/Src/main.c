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
#include "vc02.h"
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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
 * Bốn motor bánh xe dùng cùng loại motor, hộp số và encoder.
 * Giá trị 3124 là tổng số count encoder trên một vòng trục ra,
 * giữ thống nhất với chương trình đã dùng để test hai motor trước đó.
 */
#define WHEEL_ENCODER_COUNTS_PER_REV      3124.0f

/*
 * Đổi riêng từ 0U thành 1U nếu sau khi kiểm tra thực tế một motor
 * quay ngược chiều chuyển động mong muốn.
 */
#define MOTOR_LEFT_UP_DIR_INVERTED          0U
#define MOTOR_RIGHT_UP_DIR_INVERTED         0U
#define MOTOR_LEFT_DOWN_DIR_INVERTED        1U
#define MOTOR_RIGHT_DOWN_DIR_INVERTED       0U

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

volatile uint8_t debug_rd_dir_odr = 0U;
volatile uint8_t debug_rd_dir_pin = 0U;
volatile uint32_t debug_rd_pwm = 0U;
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
 * - M2 CHƯA cho voice điều khiển motor.
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

static void Wheel_Motors_SetSpeed(int16_t pwm_duty)
{
  Motor_SetSpeed_PWM(&Motor_LeftUp, pwm_duty);
  Motor_SetSpeed_PWM(&Motor_RightUp, pwm_duty);
  Motor_SetSpeed_PWM(&Motor_LeftDown, pwm_duty);
  Motor_SetSpeed_PWM(&Motor_RightDown, pwm_duty);
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

static void All_Motors_Stop(void)
{
  Wheel_Motors_SetSpeed(0);
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

  All_Motors_Stop();
}


/*
 * M4' - đổi 5 mức speed voice sang PWM open-loop.
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
 * M4' - thực thi 4 bánh open-loop, không blocking.
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
    Wheel_Motors_SetSpeed(0);
    Turn_Motors_Stop();

    voice_execution_active = 0U;
    voice_execution_finish_count++;
    return;
  }

  Wheel_Motors_SetSpeed(voice_execution_pwm);
  Turn_Motors_Stop();
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
      (PS2_Pad.Right != 0U))
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

  if ((pwm_duty == 0) && (PS2_Pad.Up == 1U))
  {
    pwm_duty = MOTOR_DPAD_PWM;
  }
  else if ((pwm_duty == 0) && (PS2_Pad.Down == 1U))
  {
    pwm_duty = -MOTOR_DPAD_PWM;
  }

  Wheel_Motors_SetSpeed(pwm_duty);
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
   * Nếu PS2 đang bị giữ lệch/nhấn, latch vẫn giữ STOP cho tới
   * khi người dùng nhả tay về neutral. Nhờ vậy PS2 không thể
   * làm xe chạy lại ngay 20 ms sau lệnh Stop.
   */
  if (control_stop_latched != 0U)
  {
    control_mode = CONTROL_MODE_STOP;
    All_Motors_Stop();

    if (ps2_manual_active == 0U)
    {
      control_stop_latched = 0U;
    }

    return;
  }

  /*
   * 2) Có thao tác PS2 -> PS2 giành quyền và hủy voice.
   */
  if (ps2_manual_active != 0U)
  {
    Voice_CancelByPS2();

    control_mode = CONTROL_MODE_PS2;
    PS2_ApplyManualControl();
    return;
  }

  /*
   * 3) Không có PS2 manual. Nếu có command voice vừa TIME chốt
   * và chưa có voice đang active thì nhận command đó.
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
      All_Motors_Stop();
    }

    return;
  }

  /* 4) Không ai có quyền điều khiển. */
  control_mode = CONTROL_MODE_IDLE;
  All_Motors_Stop();
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
  PS2_Init(&hspi1, PS2_CS_GPIO_Port, PS2_CS_Pin);

  if (HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim11, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1) != HAL_OK ||
      HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2) != HAL_OK)
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

  Motor_Init_TIM(&Motor_LeftUp, &htim9, TIM_CHANNEL_1,
                 MOTOR_DIR_left_up_GPIO_Port, MOTOR_DIR_left_up_Pin,
                 &htim8, WHEEL_ENCODER_COUNTS_PER_REV,
                 MOTOR_LEFT_UP_DIR_INVERTED);

  Motor_Init_TIM(&Motor_RightUp, &htim11, TIM_CHANNEL_1,
                 MOTOR_DIR_right_up_GPIO_Port, MOTOR_DIR_right_up_Pin,
                 &htim1, WHEEL_ENCODER_COUNTS_PER_REV,
                 MOTOR_RIGHT_UP_DIR_INVERTED);

  Motor_Init_TIM(&Motor_LeftDown, &htim10, TIM_CHANNEL_1,
                 MOTOR_DIR_left_down_GPIO_Port, MOTOR_DIR_left_down_Pin,
                 &htim2, WHEEL_ENCODER_COUNTS_PER_REV,
                 MOTOR_LEFT_DOWN_DIR_INVERTED);

  Motor_Init_TIM(&Motor_RightDown, &htim9, TIM_CHANNEL_2,
                 MOTOR_DIR_right_down_GPIO_Port, MOTOR_DIR_right_down_Pin,
                 &htim3, WHEEL_ENCODER_COUNTS_PER_REV,
                 MOTOR_RIGHT_DOWN_DIR_INVERTED);

  All_Motors_Stop();

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
    /*
     * ==========================================================
     * VC-02 - M2 BUILDER + M3 ARBITRATION + M4' OPEN-LOOP
     * ==========================================================
     *
     * Lấy hết event trong FIFO và gom command theo kiểu
     * không phụ thuộc thứ tự. TIME là trigger chốt cuối.
     * TIME tạo command hoàn chỉnh; M4' thực thi tiến/lùi
     * open-loop khi arbitration cấp quyền cho VOICE.
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
     * ==========================================================
     * PS2 + M3 ARBITRATION
     * ==========================================================
     *
     * Đọc PS2 trước, sau đó arbitration quyết định ai có quyền:
     * STOP > PS2 manual > Voice > Idle.
     *
     * Quan trọng: PS2 chỉ "được kết nối" nhưng đang neutral sẽ
     * KHÔNG còn tự động gọi All_Motors_Stop() để đè voice.
     */
    PS2_Read_Data();

    Control_Arbitrate();
    /*
     * ==========================================================
     * DEBUG MOTOR RIGHT DOWN
     * DIR = PE4
     * PWM = TIM9_CH2
     * ==========================================================
     */

    /* Mức mà STM32 đang yêu cầu xuất ra PE4 */
    debug_rd_dir_odr =
        ((MOTOR_DIR_right_down_GPIO_Port->ODR &
          MOTOR_DIR_right_down_Pin) != 0U) ? 1U : 0U;


    /* Đọc lại mức logic thực tế tại chân PE4 */
    debug_rd_dir_pin =
        (HAL_GPIO_ReadPin(MOTOR_DIR_right_down_GPIO_Port,
                          MOTOR_DIR_right_down_Pin)
         == GPIO_PIN_SET) ? 1U : 0U;


    /* PWM hiện đang xuất cho RightDown */
    debug_rd_pwm =
        __HAL_TIM_GET_COMPARE(&htim9, TIM_CHANNEL_2);
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
  hi2c1.Init.Timing = 0x2010091A;
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
  hi2c2.Init.Timing = 0x2010091A;
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
