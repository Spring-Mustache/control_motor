#include "wheel_control.h"
#include <stddef.h>

/* ============================================================
 * M6 - WHEEL CONTROL CONFIG
 * ============================================================ */
#define WC_TARGET_RPM_MAX                 100.0f
#define WC_FF_K_LU_INIT                       8.3042f
#define WC_FF_K_LD_INIT                       8.39f
#define WC_FF_K_RU_INIT                       8.3050f
#define WC_FF_K_RD_INIT                       8.1250f

#define WC_FF_OFFSET_LU_INIT                 63.39f
#define WC_FF_OFFSET_LD_INIT                 72.00f
#define WC_FF_OFFSET_RU_INIT                 80.00f
#define WC_FF_OFFSET_RD_INIT                 67.61f
#define WC_FF_OFFSET_RD_REVERSE_INIT         79.50f

/*
 * Dynamic / acceleration feedforward - FIXED BASELINE:
 *
 *   PWM_accel = Ka * d(target_rpm)/dt
 *
 * Ka = 0.18 đã được A/B với 0.22 và được chọn làm baseline:
 * 0.22 không giảm ramp error tổng thể, trong khi overshoot/spread tăng.
 *
 * Với command shaper mới, Ka vẫn giữ 0.18. Ví dụ:
 *   +50 rpm/s   ->  +9.0 PWM
 *   +60 rpm/s   -> +10.8 PWM
 *   -100 rpm/s  -> -18.0 PWM (release-to-zero)
 *
 * Ka được áp cứng trong actuator loop; Live Expressions chỉ dùng
 * để quan sát trạng thái, tránh thay đổi ngoài ý muốn giữa các test.
 */
#define WC_FF_ACCEL_K_FIXED                  0.18f
#define WC_FF_ACCEL_MAX_PWM_INIT            40.0f

/*
 * Cross-wheel synchronization - RAMP ONLY + SPREAD HYSTERESIS.
 *
 * Mục tiêu: chỉ can thiệp khi các bánh có encoder thực sự lệch nhau trong lúc ramp,
 * tránh đuổi theo lượng tử hóa/nhiễu nhỏ khi ba bánh vốn đã đồng bộ.
 *
 *   rpm_ref    = mean(rpm_fb của các bánh có encoder)
 *   rpm_spread = max(rpm_fb) - min(rpm_fb)
 *   e_sync_i   = rpm_ref - rpm_fb_i
 *   pwm_sync   = clamp(Ksync * e_sync_i, +/- max_pwm)
 *
 * Hysteresis mặc định:
 *   spread >= 1.5 rpm  -> bật sync
 *   spread <= 0.8 rpm  -> tắt sync
 *   0.8 < spread < 1.5 -> giữ trạng thái trước đó
 *
 * Correction dùng chính feedback MA5 của PID. Giá trị baseline:
 *   Ksync = 1.5 PWM/rpm, max = +/-8 PWM.
 */
#define WC_SYNC_K_PWM_PER_RPM_INIT           1.5f
#define WC_SYNC_MAX_PWM_INIT                 8.0f
#define WC_SYNC_SPREAD_ENTER_RPM_INIT        1.5f
#define WC_SYNC_SPREAD_EXIT_RPM_INIT         0.8f
#define WC_SYNC_MIN_TARGET_ACCEL_RPM_PER_S   1.0f

#define WC_PID_KP_INIT                      4.5f
#define WC_PID_KI_INIT                      3.5f
#define WC_PID_KD_INIT                      0.0f
#define WC_PID_INTEGRAL_LIMIT_PWM         300.0f

/*
 * M11 architecture experiment A1: encoder-based plateau integral adaptation.
 * The static FF and normal Kp/Ki/Kd remain untouched. After FIXED target
 * slew completes, a persistent large RPM error gets a faster (bounded) I
 * correction until steady-hold is entered. This works without battery ADC.
 * An explicit enable switch permits A/B comparisons with the same binary.
 * This first experiment intentionally does NOT modify steady-hold itself.
 */
#define WC_PLATEAU_ADAPT_I_ENABLE_INIT            0U
#define WC_PLATEAU_ADAPT_I_WHEELS_INIT            0x0FU
#define WC_PLATEAU_ADAPT_I_KI_INIT               20.0f
#define WC_PLATEAU_ADAPT_I_MIN_ERROR_RPM_INIT     0.8f
#define WC_PLATEAU_ADAPT_I_MAX_STEP_PWM_INIT      1.5f
#define WC_PLATEAU_ADAPT_I_MIN_TARGET_RPM        20.0f

#define WC_PID_DT_MIN_S                     0.020f
#define WC_PID_DT_MAX_S                     0.250f

/*
 * PID feedback conditioning baseline.
 *
 * RPM encoder vẫn được cập nhật khoảng mỗi 24 ms trong motor.c.
 * PID dùng rolling / moving average của 5 mẫu gần nhất:
 *
 *   rpm_fb[k] = (rpm_raw[k] + rpm_raw[k-1] + rpm_raw[k-2]
 *              + rpm_raw[k-3] + rpm_raw[k-4]) / 5
 *
 * Khi chưa đủ 5 mẫu sau reset, dùng trung bình của số mẫu hiện có.
 * Bộ lọc được cập nhật mỗi sample PID; KHÔNG chờ 5 mẫu rồi mới update PID.
 */
#define WC_PID_FEEDBACK_MA_SAMPLES             5U
#define WC_PID_ZERO_EPS_RPM                 0.01f
#define WC_PID_INTEGRAL_HOLD_GAP_RPM_INIT      0.5f
#define WC_PID_HUNT_ENABLE_INIT                    0U
#define WC_PID_HUNT_ENTER_ERROR_RPM_INIT         0.8f
#define WC_PID_HUNT_EXIT_ERROR_RPM_INIT          1.5f
#define WC_PID_HUNT_ENTER_SAMPLES_INIT           3U
#define WC_PID_HUNT_EXIT_SAMPLES_INIT            2U
#define WC_PID_HUNT_I_DEADBAND_RPM_INIT          0.35f
#define WC_PID_HUNT_KI_SCALE_INIT                0.25f

/*
 * RU plateau capture assist - targeted transient gain, FIXED profile only.
 *
 * Bench 18 showed RU warm steady-state is already good, but the first fixed
 * +/-40 rpm plateau can need 1.6-2.3 s before entering steady-hold. Static FF
 * is therefore kept unchanged; this assist only adds a bounded correction in
 * the short plateau-capture window.
 *
 *   PWM_capture = clamp(Kcapture * (target - MA5_feedback), +/- max_pwm)
 *
 * It starts exactly when FIXED target finishes its slew (integral_hold 1->0),
 * stops immediately when RU enters steady-hold, and has a 600 ms safety
 * timeout. On HOLD entry the assist is removed before WC_ClosedLoopPWM(); the
 * existing bumpless transfer moves the previous correction into I. On timeout
 * the current assist is explicitly transferred into I before removal so PWM
 * does not step.
 */
#define WC_RU_CAPTURE_ASSIST_ENABLE_INIT              0U
#define WC_RU_CAPTURE_ASSIST_K_PWM_PER_RPM_INIT      6.0f
#define WC_RU_CAPTURE_ASSIST_MAX_PWM_INIT            8.0f
#define WC_RU_CAPTURE_ASSIST_TIMEOUT_MS_INIT          600U
#define WC_RU_CAPTURE_ASSIST_MIN_TARGET_RPM_INIT     20.0f

/*
 * Integral hold during target slew.
 *
 * Khi raw target còn cách setpoint đã slew quá ngưỡng này, target vẫn đang
 * ramp. P + feedforward vẫn hoạt động bình thường nhưng I được giữ nguyên,
 * tránh tích lũy trong lúc motor đang tăng/giảm tốc rồi gây overshoot ở plateau.
 * Khi setpoint bắt kịp raw target, I tự hoạt động lại để sửa sai số tĩnh.
 */

/*
 * Target-RPM command shaper.
 *
 * Dùng CHUNG một lõi FF + PI + sync nhưng tách cách tạo setpoint theo nguồn:
 *
 * MANUAL (PS2 analog LY):
 *   - accel mềm để hạn chế phản lực làm rung/đảo thân xe khi kéo cần nhanh;
 *   - giảm cùng chiều dùng rate mềm và soft-landing ở ~10 rpm cuối để tránh
 *     chuyển mô-men đột ngột từ decel sang giữ tốc độ;
 *   - hysteresis nhỏ trên raw joystick để nhiễu tay/ADC không làm shaper đổi
 *     accel <-> decel liên tục;
 *   - release-to-zero có rate riêng; 15 rpm cuối có zero soft-landing;
 *   - trong đúng vùng zero soft-landing, offset của static feedforward được
 *     taper tuyến tính về 0 để tránh còn giữ PWM lớn ngay trước STOP.
 *
 * FIXED (Voice / autonomous fixed-speed):
 *   - target sạch, ít thay đổi nên có profile riêng;
 *   - accel vẫn được giới hạn để tránh step cơ khí;
 *   - stop-to-zero dùng rate nhanh nhưng có kiểm soát.
 *
 * Các giá trị dưới đây là baseline trước ground/load validation. Hai profile
 * tách riêng để sau này có thể tune độc lập mà KHÔNG đổi Kp/Ki/Ka/sync.
 */
#define WC_MANUAL_ACCEL_RPM_PER_S_INIT      50.0f
#define WC_MANUAL_DECEL_RPM_PER_S_INIT      50.0f
#define WC_MANUAL_ZERO_RPM_PER_S_INIT      100.0f
#define WC_MANUAL_SOFT_LANDING_WINDOW_RPM_INIT   10.0f
#define WC_MANUAL_SOFT_LANDING_MIN_RATE_INIT     20.0f
#define WC_MANUAL_ZERO_SOFT_LANDING_WINDOW_RPM_INIT   15.0f
#define WC_MANUAL_ZERO_SOFT_LANDING_MIN_RATE_INIT     35.0f
#define WC_MANUAL_TARGET_HYSTERESIS_RPM_INIT      1.5f

#define WC_FIXED_ACCEL_RPM_PER_S_INIT       60.0f
#define WC_FIXED_DECEL_RPM_PER_S_INIT       80.0f
#define WC_FIXED_ZERO_RPM_PER_S_INIT       100.0f

/* M6-S02 experimental FIXED-speed S-curve. MANUAL and FF calibration stay legacy. */
#define WC_FIXED_SCURVE_ENABLE_INIT              1U
#define WC_FIXED_SCURVE_40RPM_S_INIT             1.0f
#define WC_FIXED_SCURVE_REFERENCE_RPM            40.0f
#define WC_FIXED_SCURVE_MIN_SEGMENT_S            0.20f
#define WC_FIXED_SCURVE_MAX_SEGMENT_S            5.00f

#define WC_TARGET_DT_MAX_S                   0.100f
#define WC_REVERSE_DEADTIME_MS             150U

/*
 * AUTO FEEDFORWARD CALIBRATION TEST
 *
 * Chỉ cần đặt wheel_ff_test_start = 1 khi xe đang đứng yên.
 * Controller tự chạy:
 *   0 -> 30 -> 50 -> 70 -> 85 -> 0 rpm
 *
 * Mỗi plateau chỉ bắt đầu tính HOLD sau khi target RPM đã ramp tới mức yêu cầu.
 * Logger được main.c tự start/stop qua hai request bên dưới.
 */
#define WC_FF_TEST_REACH_TOL_RPM             0.5f
#define WC_FF_TEST_STOP_RPM_MAX              2.0f

#define WC_FF_TEST_STAGE_IDLE                0U
#define WC_FF_TEST_STAGE_PREP_ZERO           1U
#define WC_FF_TEST_STAGE_RAMP_LEVEL          2U
#define WC_FF_TEST_STAGE_HOLD_LEVEL          3U
#define WC_FF_TEST_STAGE_RAMP_ZERO           4U
#define WC_FF_TEST_STAGE_FINAL_ZERO          5U

/* Motor handles được gắn một lần trong WheelControl_Init(). */
static Motor_HandleTypeDef *wc_motor_lu = NULL;
static Motor_HandleTypeDef *wc_motor_ru = NULL;
static Motor_HandleTypeDef *wc_motor_ld = NULL;
static Motor_HandleTypeDef *wc_motor_rd = NULL;

static int16_t wc_pwm_max = 999;
static uint32_t wc_reverse_wait_start_tick = 0U;
static uint32_t wc_target_last_tick = 0U;

/* Private quintic segment state; only used for FIXED commands. */
static float wc_fixed_scurve_start_rpm = 0.0f;
static float wc_fixed_scurve_goal_rpm = 0.0f;
static float wc_fixed_scurve_elapsed_s = 0.0f;
static float wc_fixed_scurve_duration_s = 0.0f;
static uint8_t wc_pid_enable_prev = 0U;
static float wc_pid_prev_target_rpm = 0.0f;

/*
 * Target manual đã qua hysteresis. Raw joystick vẫn được giữ riêng để logger
 * thấy đúng đầu vào thật; biến này chỉ ngăn jitter nhỏ làm command shaper
 * liên tục đổi trạng thái accel/decel.
 */
static float wc_manual_accepted_target_rpm = 0.0f;

/* Previous target sampled đúng theo nhịp PID để tính d(target)/dt. */
static float wc_ff_accel_prev_target_rpm = 0.0f;
static uint8_t wc_ff_accel_prev_valid = 0U;

/* 5-sample rolling average state for PID feedback. */
static float wc_pid_feedback_lu_history[WC_PID_FEEDBACK_MA_SAMPLES] = {0.0f};
static float wc_pid_feedback_ld_history[WC_PID_FEEDBACK_MA_SAMPLES] = {0.0f};
static float wc_pid_feedback_ru_history[WC_PID_FEEDBACK_MA_SAMPLES] = {0.0f};
static float wc_pid_feedback_rd_history[WC_PID_FEEDBACK_MA_SAMPLES] = {0.0f};
static uint8_t wc_pid_feedback_ma_index = 0U;
static uint8_t wc_pid_feedback_ma_count = 0U;

static uint8_t wc_pid_hunt_lu_active = 0U;
static uint8_t wc_pid_hunt_ld_active = 0U;
static uint8_t wc_pid_hunt_ru_active = 0U;
static uint8_t wc_pid_hunt_rd_active = 0U;

static uint8_t wc_pid_hunt_lu_enter_count = 0U;
static uint8_t wc_pid_hunt_ld_enter_count = 0U;
static uint8_t wc_pid_hunt_ru_enter_count = 0U;
static uint8_t wc_pid_hunt_rd_enter_count = 0U;

static uint8_t wc_pid_hunt_lu_exit_count = 0U;
static uint8_t wc_pid_hunt_ld_exit_count = 0U;
static uint8_t wc_pid_hunt_ru_exit_count = 0U;
static uint8_t wc_pid_hunt_rd_exit_count = 0U;

/* RU targeted capture-assist private state. */
static uint8_t wc_ru_capture_prev_integral_hold = 0U;
static uint32_t wc_ru_capture_start_tick = 0U;

static float wc_ff_test_target_rpm = 0.0f;
static uint32_t wc_ff_test_stage_start_tick = 0U;
static uint32_t wc_ff_test_run_start_tick = 0U;
static uint8_t wc_ff_test_level_index = 0U;

/* ============================================================
 * PUBLIC DEBUG / TUNING STATE
 * ============================================================ */
volatile int16_t wheel_target_pwm = 0;
volatile int16_t wheel_current_pwm = 0;
volatile uint8_t wheel_reverse_wait_active = 0U;
volatile uint32_t wheel_reverse_count = 0U;

/* Normal drive uses encoder speed PI + FF from boot.
 * Set wheel_pid_enable=0 explicitly only for diagnostic open-loop tests.
 */
volatile uint8_t wheel_pid_enable = 1U;
volatile uint8_t wheel_pid_active = 0U;

/* Integral-hold debug/tuning.
 * Các symbol global này xem/chỉnh trực tiếp bằng Live Expressions.
 * Không module nào khác cần truy cập nên không expose qua wheel_control.h.
 */
volatile uint8_t wheel_pid_integral_hold_enable = 1U;
volatile uint8_t wheel_pid_integral_hold_active = 0U;
volatile float wheel_pid_integral_hold_gap_rpm =
    WC_PID_INTEGRAL_HOLD_GAP_RPM_INIT;
volatile uint32_t wheel_pid_integral_hold_count = 0U;

/* A1: shared, bounded, plateau-only adaptive I. Wheel mask bits: LU=1,
 * LD=2, RU=4, RD=8. All tunables are public for Live Expressions. */
volatile uint8_t wheel_plateau_adapt_i_enable =
    WC_PLATEAU_ADAPT_I_ENABLE_INIT;
volatile uint8_t wheel_plateau_adapt_i_wheels =
    WC_PLATEAU_ADAPT_I_WHEELS_INIT;
volatile float wheel_plateau_adapt_i_ki = WC_PLATEAU_ADAPT_I_KI_INIT;
volatile float wheel_plateau_adapt_i_min_error_rpm =
    WC_PLATEAU_ADAPT_I_MIN_ERROR_RPM_INIT;
volatile float wheel_plateau_adapt_i_max_step_pwm =
    WC_PLATEAU_ADAPT_I_MAX_STEP_PWM_INIT;
volatile uint8_t wheel_plateau_adapt_i_mask = 0U;
volatile uint32_t wheel_plateau_adapt_i_sample_count = 0U;

/*
 * Plateau steady-hold / anti-hunting:
 * - Chỉ hoạt động khi target đã hết ramp (integral_hold = 0).
 * - Vào vùng HOLD khi |error| <= enter threshold đủ số mẫu liên tiếp.
 * - Thoát HOLD khi |error| >= exit threshold đủ số mẫu liên tiếp.
 * - Vừa ENTER HOLD: chuyển integral để FF + I giữ đúng output chu kỳ trước
 *   (bumpless transfer, tránh hụt PWM do bỏ P đột ngột).
 * - Trong HOLD: P/D = 0. Nếu |error| <= I-deadband thì freeze I.
 *   Nếu error nằm giữa I-deadband và exit threshold thì I trim chậm với
 *   Ki_hold = Ki * wheel_pid_hunt_ki_scale.
 *
 * Hai threshold tạo hysteresis để tránh bật/tắt liên tục quanh biên.
 */
volatile uint8_t wheel_pid_hunt_enable = WC_PID_HUNT_ENABLE_INIT;
volatile float wheel_pid_hunt_enter_error_rpm =
    WC_PID_HUNT_ENTER_ERROR_RPM_INIT;
volatile float wheel_pid_hunt_exit_error_rpm =
    WC_PID_HUNT_EXIT_ERROR_RPM_INIT;

volatile uint8_t wheel_pid_hunt_enter_samples =
    WC_PID_HUNT_ENTER_SAMPLES_INIT;
volatile uint8_t wheel_pid_hunt_exit_samples =
    WC_PID_HUNT_EXIT_SAMPLES_INIT;

volatile float wheel_pid_hunt_i_deadband_rpm =
    WC_PID_HUNT_I_DEADBAND_RPM_INIT;
volatile float wheel_pid_hunt_ki_scale =
    WC_PID_HUNT_KI_SCALE_INIT;

volatile uint8_t wheel_pid_hunt_active_lu = 0U;
volatile uint8_t wheel_pid_hunt_active_ld = 0U;
volatile uint8_t wheel_pid_hunt_active_ru = 0U;
volatile uint8_t wheel_pid_hunt_active_rd = 0U;
volatile uint32_t wheel_pid_hunt_entry_count = 0U;


/* RU FIXED-plateau capture assist tuning/debug. */
volatile uint8_t wheel_ru_capture_assist_enable =
    WC_RU_CAPTURE_ASSIST_ENABLE_INIT;
volatile float wheel_ru_capture_assist_k_pwm_per_rpm =
    WC_RU_CAPTURE_ASSIST_K_PWM_PER_RPM_INIT;
volatile float wheel_ru_capture_assist_max_pwm =
    WC_RU_CAPTURE_ASSIST_MAX_PWM_INIT;
volatile uint32_t wheel_ru_capture_assist_timeout_ms =
    WC_RU_CAPTURE_ASSIST_TIMEOUT_MS_INIT;
volatile uint8_t wheel_ru_capture_assist_active = 0U;
volatile float wheel_ru_capture_assist_pwm = 0.0f;
volatile uint32_t wheel_ru_capture_assist_entry_count = 0U;
volatile uint32_t wheel_ru_capture_assist_timeout_count = 0U;

/* ============================================================
 * AUTO FEEDFORWARD CALIBRATION TEST
 *
 * Normal use:
 *   1) Xe đứng yên, joystick neutral.
 *   2) Ghi wheel_ff_test_start = 1 trong Live Expressions.
 *   3) Không chạm joystick/nút trong lúc test.
 *
 * Test tự bật PID, tự yêu cầu logger start, chạy profile 4 cấp tốc độ,
 * về 0 và tự stop logger.
 *
 * status:
 *   0 = chưa chạy / idle
 *   1 = đang chạy
 *   2 = hoàn tất bình thường
 *   3 = từ chối start vì xe chưa đứng yên
 *   4 = bị hủy bởi command ngoài hoặc EmergencyStop
 */
volatile uint8_t wheel_ff_test_start = 0U;
volatile uint8_t wheel_ff_test_active = 0U;
volatile uint8_t wheel_ff_test_done = 0U;
volatile uint8_t wheel_ff_test_status = 0U;
volatile uint8_t wheel_ff_test_stage = WC_FF_TEST_STAGE_IDLE;
volatile uint8_t wheel_ff_test_level = 0U;

volatile float wheel_ff_test_rpm_1 = 30.0f;
volatile float wheel_ff_test_rpm_2 = 50.0f;
volatile float wheel_ff_test_rpm_3 = 70.0f;
volatile float wheel_ff_test_rpm_4 = 85.0f;

volatile uint32_t wheel_ff_test_prep_ms = 1000U;
volatile uint32_t wheel_ff_test_hold_ms = 4000U;
volatile uint32_t wheel_ff_test_final_ms = 1000U;

volatile float wheel_ff_test_current_command_rpm = 0.0f;
volatile uint32_t wheel_ff_test_stage_elapsed_ms = 0U;
volatile uint32_t wheel_ff_test_elapsed_ms = 0U;
volatile uint32_t wheel_ff_test_run_count = 0U;

/* Main logger consumes and auto-clears these request flags. */
volatile uint8_t wheel_ff_test_log_start_request = 0U;
volatile uint8_t wheel_ff_test_log_stop_request = 0U;

/*
 * raw_target_rpm: target trực tiếp sau khi map command -> RPM.
 * wheel_pid_target_rpm: target đã qua slew-rate limiter và là setpoint
 * thực sự đưa vào feedforward + PID.
 */
volatile float wheel_target_raw_rpm = 0.0f;
volatile float wheel_pid_target_rpm = 0.0f;

/*
 * Command-shaper profile. FIXED là mặc định an toàn lúc boot; PS2 chuyển sang
 * MANUAL ngay khi một thao tác manual thực sự được nhận. Khi PS2 về neutral,
 * profile MANUAL được giữ để release-to-zero vẫn dùng đúng manual_zero rate.
 */
volatile WheelCommandProfile_t wheel_command_profile = WHEEL_COMMAND_PROFILE_FIXED;

/* Tuning riêng cho PS2/manual. */
volatile float wheel_manual_accel_rpm_per_s = WC_MANUAL_ACCEL_RPM_PER_S_INIT;
volatile float wheel_manual_decel_rpm_per_s = WC_MANUAL_DECEL_RPM_PER_S_INIT;
volatile float wheel_manual_zero_rpm_per_s = WC_MANUAL_ZERO_RPM_PER_S_INIT;
volatile float wheel_manual_soft_landing_window_rpm =
    WC_MANUAL_SOFT_LANDING_WINDOW_RPM_INIT;
volatile float wheel_manual_soft_landing_min_rate_rpm_per_s =
    WC_MANUAL_SOFT_LANDING_MIN_RATE_INIT;
volatile float wheel_manual_zero_soft_landing_window_rpm =
    WC_MANUAL_ZERO_SOFT_LANDING_WINDOW_RPM_INIT;
volatile float wheel_manual_zero_soft_landing_min_rate_rpm_per_s =
    WC_MANUAL_ZERO_SOFT_LANDING_MIN_RATE_INIT;
volatile float wheel_manual_target_hysteresis_rpm =
    WC_MANUAL_TARGET_HYSTERESIS_RPM_INIT;

/* Target sau hysteresis (MANUAL) / bằng raw target (FIXED), trước slew. */
volatile float wheel_command_target_rpm = 0.0f;
volatile uint8_t wheel_manual_soft_landing_active = 0U;
volatile uint8_t wheel_manual_zero_soft_landing_active = 0U;

/*
 * Zero feedforward taper (MANUAL STOP only).
 * scale = 1 ngoài vùng zero soft-landing; trong 15 rpm cuối giảm tuyến tính
 * về 0. Chỉ taper OFFSET của static FF; phần Kff*RPM vẫn giảm tự nhiên theo RPM.
 */
volatile float wheel_manual_zero_ff_taper_scale = 1.0f;

/* Tuning riêng cho fixed-speed / autonomous. */
volatile float wheel_fixed_accel_rpm_per_s = WC_FIXED_ACCEL_RPM_PER_S_INIT;
volatile float wheel_fixed_decel_rpm_per_s = WC_FIXED_DECEL_RPM_PER_S_INIT;
volatile float wheel_fixed_zero_rpm_per_s = WC_FIXED_ZERO_RPM_PER_S_INIT;

/* M6-S02 opt-in test path for FIXED commands, default ON only on this branch.
 * Set enable=0 to run the exact M6-S01 linear FIXED shaper for A/B comparison.
 * Duration is for a 40 RPM change; e.g. 0 -> 40 in 2 s and 40 -> 0 in 2 s.
 */
volatile uint8_t wheel_fixed_scurve_enable = WC_FIXED_SCURVE_ENABLE_INIT;
volatile float wheel_fixed_scurve_40rpm_s = WC_FIXED_SCURVE_40RPM_S_INIT;
volatile uint8_t wheel_fixed_scurve_active = 0U;

/* Rate ĐANG được dùng; giữ tên cũ để Live Expressions/logger vẫn dễ theo dõi. */
volatile float wheel_target_accel_rpm_per_s = WC_FIXED_ACCEL_RPM_PER_S_INIT;
volatile float wheel_target_decel_rpm_per_s = WC_FIXED_DECEL_RPM_PER_S_INIT;
volatile float wheel_target_zero_rpm_per_s = WC_FIXED_ZERO_RPM_PER_S_INIT;
/*
 * Feedforward riêng từng bánh:
 *   PWMff = sign(target) * (Kff * |target_rpm| + offset)
 *
 * LU/LD/RU vẫn dùng cùng tham số cho tiến/lùi.
 * RD đã tách riêng OFFSET chiều lùi vì bench 17 cho thấy cùng target 40 rpm
 * nhưng chiều lùi cần thêm khoảng 10-12 PWM so với chiều tiến; dùng chung
 * offset khiến RD mất 1.3-3.8 s mới vào steady-hold trong khi chiều tiến
 * sau warm-up vào hold khoảng 0.15 s.
 *
 * Chỉnh các biến này khi xe đang dừng để thử hệ số mới mà không cần build lại.
 */
volatile float wheel_ff_k_lu = WC_FF_K_LU_INIT;
volatile float wheel_ff_k_ld = WC_FF_K_LD_INIT;
volatile float wheel_ff_k_ru = WC_FF_K_RU_INIT;
volatile float wheel_ff_k_rd = WC_FF_K_RD_INIT;

volatile float wheel_ff_offset_lu = WC_FF_OFFSET_LU_INIT;
volatile float wheel_ff_offset_ld = WC_FF_OFFSET_LD_INIT;
volatile float wheel_ff_offset_ru = WC_FF_OFFSET_RU_INIT;
volatile float wheel_ff_offset_rd = WC_FF_OFFSET_RD_INIT;
volatile float wheel_ff_offset_rd_reverse = WC_FF_OFFSET_RD_REVERSE_INIT;

/*
 * Acceleration feedforward status/debug.
 * Dynamic FF được bật cố định theo baseline Ka=0.18.
 * Hai biến enable/k được giữ để quan sát trong Live Expressions;
 * actuator loop sẽ ép chúng về cấu hình cố định và không dùng chúng để tune runtime.
 */
volatile uint8_t wheel_ff_accel_enable = 1U;
volatile float wheel_ff_accel_k = WC_FF_ACCEL_K_FIXED;
volatile float wheel_ff_accel_max_pwm = WC_FF_ACCEL_MAX_PWM_INIT;

/* Debug runtime */
volatile float wheel_ff_target_accel_rpm_per_s = 0.0f;
volatile float wheel_ff_accel_pwm = 0.0f;

/*
 * Cross-wheel synchronization debug/tuning.
 * Default ON, nhưng chỉ ACTIVE khi setpoint đang ramp VÀ spread vượt ngưỡng.
 * Có hysteresis ENTER/EXIT để không bật-tắt theo từng nấc lượng tử encoder.
 * Có thể A/B nhanh bằng wheel_sync_enable = 0/1 trong Live Expressions.
 */
volatile uint8_t wheel_sync_enable = 0U;
volatile uint8_t wheel_sync_active = 0U;
volatile float wheel_sync_k_pwm_per_rpm = WC_SYNC_K_PWM_PER_RPM_INIT;
volatile float wheel_sync_max_pwm = WC_SYNC_MAX_PWM_INIT;
volatile float wheel_sync_spread_enter_rpm = WC_SYNC_SPREAD_ENTER_RPM_INIT;
volatile float wheel_sync_spread_exit_rpm = WC_SYNC_SPREAD_EXIT_RPM_INIT;
volatile float wheel_sync_spread_rpm = 0.0f;
volatile float wheel_sync_ref_rpm = 0.0f;
volatile float wheel_sync_error_lu_rpm = 0.0f;
volatile float wheel_sync_error_ld_rpm = 0.0f;
volatile float wheel_sync_error_ru_rpm = 0.0f;
volatile float wheel_sync_error_rd_rpm = 0.0f;
volatile float wheel_sync_pwm_lu = 0.0f;
volatile float wheel_sync_pwm_ld = 0.0f;
volatile float wheel_sync_pwm_ru = 0.0f;
volatile float wheel_sync_pwm_rd = 0.0f;

/* Biến cũ được giữ để Live Expressions cũ không lỗi.
 * Chỉ là Kff trung bình/debug, không dùng trực tiếp để điều khiển.
 */
volatile float wheel_ff_pwm_per_rpm = WC_FF_K_LD_INIT;

volatile int16_t wheel_pwm_lu = 0;
volatile int16_t wheel_pwm_ld = 0;
volatile int16_t wheel_pwm_ru = 0;
volatile int16_t wheel_pwm_rd = 0;

volatile uint32_t wheel_pid_update_count = 0U;
volatile uint32_t wheel_pid_last_sample_tick = 0U;

/*
 * PID feedback rolling-average debug/tuning.
 * - enable = 1: PID dùng MA5.
 * - enable = 0: PID dùng raw RPM trực tiếp (A/B test bằng Live Expressions).
 * Các giá trị *_rpm bên dưới là feedback THỰC SỰ đưa vào PID.
 */
volatile uint8_t wheel_pid_feedback_ma_enable = 1U;
volatile uint8_t wheel_pid_feedback_ma_count = 0U;
volatile float wheel_pid_feedback_lu_rpm = 0.0f;
volatile float wheel_pid_feedback_ld_rpm = 0.0f;
volatile float wheel_pid_feedback_ru_rpm = 0.0f;
volatile float wheel_pid_feedback_rd_rpm = 0.0f;

/* Shared PID tuning controls for Live Expressions. */
volatile float wheel_pid_tune_kp = WC_PID_KP_INIT;
volatile float wheel_pid_tune_ki = WC_PID_KI_INIT;
volatile float wheel_pid_tune_kd = WC_PID_KD_INIT;
volatile float wheel_pid_tune_integral_limit = WC_PID_INTEGRAL_LIMIT_PWM;
volatile uint8_t wheel_pid_apply_tuning = 0U;
volatile uint32_t wheel_pid_tuning_apply_count = 0U;

/*
 * LU đã lắp encoder 16 P/R. Giữ default 0 cho lần kiểm tra đầu tiên để
 * xác nhận đúng dấu/CPR mà không có nguy cơ PID bão hòa nếu A/B bị đảo.
 * Sau khi Motor_LeftUp.current_rpm dương khi xe chạy tiến, đặt biến này = 1
 * trong Live Expressions; LU sẽ tham gia PI + sync ngay lập tức.
 */
volatile uint8_t wheel_encoder_available_lu = 1U;
volatile uint8_t wheel_encoder_available_ld = 1U;
volatile uint8_t wheel_encoder_available_ru = 1U;
volatile uint8_t wheel_encoder_available_rd = 1U;

volatile PID_Controller_t wheel_pid_lu;
volatile PID_Controller_t wheel_pid_ld;
volatile PID_Controller_t wheel_pid_ru;
volatile PID_Controller_t wheel_pid_rd;

volatile WheelControlDebug_t wheel_ctrl_debug;

static float WC_ClampFloat(float value, float min_value, float max_value);


static float WC_AbsFloat(float value)
{
  return (value < 0.0f) ? -value : value;
}

static void WC_ClearSyncOutputs(void)
{
  wheel_sync_ref_rpm = 0.0f;
  wheel_sync_error_lu_rpm = 0.0f;
  wheel_sync_error_ld_rpm = 0.0f;
  wheel_sync_error_ru_rpm = 0.0f;
  wheel_sync_error_rd_rpm = 0.0f;
  wheel_sync_pwm_lu = 0.0f;
  wheel_sync_pwm_ld = 0.0f;
  wheel_sync_pwm_ru = 0.0f;
  wheel_sync_pwm_rd = 0.0f;
}

static void WC_ClearSyncState(void)
{
  wheel_sync_active = 0U;
  wheel_sync_spread_rpm = 0.0f;
  WC_ClearSyncOutputs();
}

static float WC_FFTestLevelRPM(uint8_t index)
{
  switch (index)
  {
    case 0U:
      return wheel_ff_test_rpm_1;

    case 1U:
      return wheel_ff_test_rpm_2;

    case 2U:
      return wheel_ff_test_rpm_3;

    case 3U:
      return wheel_ff_test_rpm_4;

    default:
      return 0.0f;
  }
}

static uint8_t WC_IsStoppedForFFTest(void)
{
  if (WC_AbsFloat(wheel_pid_target_rpm) > WC_FF_TEST_STOP_RPM_MAX)
  {
    return 0U;
  }

  if ((wheel_encoder_available_ld != 0U) &&
      (wc_motor_ld != NULL) &&
      (WC_AbsFloat(wc_motor_ld->current_rpm) > WC_FF_TEST_STOP_RPM_MAX))
  {
    return 0U;
  }

  if ((wheel_encoder_available_ru != 0U) &&
      (wc_motor_ru != NULL) &&
      (WC_AbsFloat(wc_motor_ru->current_rpm) > WC_FF_TEST_STOP_RPM_MAX))
  {
    return 0U;
  }

  if ((wheel_encoder_available_rd != 0U) &&
      (wc_motor_rd != NULL) &&
      (WC_AbsFloat(wc_motor_rd->current_rpm) > WC_FF_TEST_STOP_RPM_MAX))
  {
    return 0U;
  }

  return 1U;
}

static void WC_FFTestFinish(uint8_t status)
{
  wc_ff_test_target_rpm = 0.0f;
  wheel_ff_test_current_command_rpm = 0.0f;
  wheel_ff_test_active = 0U;
  wheel_ff_test_done = 1U;
  wheel_ff_test_status = status;
  wheel_ff_test_stage = WC_FF_TEST_STAGE_IDLE;
  wheel_ff_test_level = 0U;
  wheel_ff_test_log_stop_request = 1U;

  if (status == 2U)
  {
    wheel_ff_test_run_count++;
  }
}

static void WC_FFTestUpdate(uint32_t now)
{
  float requested_rpm;
  float target_error;

  if (wheel_ff_test_start != 0U)
  {
    wheel_ff_test_start = 0U;

    if (wheel_ff_test_active != 0U)
    {
      return;
    }

    if (WC_IsStoppedForFFTest() == 0U)
    {
      wheel_ff_test_done = 1U;
      wheel_ff_test_status = 3U;
      wheel_ff_test_stage = WC_FF_TEST_STAGE_IDLE;
      return;
    }

    /* Clamp test parameters so accidental debugger values stay safe. */
    wheel_ff_test_rpm_1 = WC_ClampFloat(wheel_ff_test_rpm_1, 5.0f, 90.0f);
    wheel_ff_test_rpm_2 = WC_ClampFloat(wheel_ff_test_rpm_2, 5.0f, 90.0f);
    wheel_ff_test_rpm_3 = WC_ClampFloat(wheel_ff_test_rpm_3, 5.0f, 90.0f);
    wheel_ff_test_rpm_4 = WC_ClampFloat(wheel_ff_test_rpm_4, 5.0f, 90.0f);

    if (wheel_ff_test_prep_ms < 500U)
    {
      wheel_ff_test_prep_ms = 500U;
    }

    if (wheel_ff_test_hold_ms < 2500U)
    {
      wheel_ff_test_hold_ms = 2500U;
    }

    if (wheel_ff_test_final_ms < 500U)
    {
      wheel_ff_test_final_ms = 500U;
    }

    wheel_pid_enable = 1U;

    wheel_ff_test_active = 1U;
    wheel_ff_test_done = 0U;
    wheel_ff_test_status = 1U;
    wheel_ff_test_stage = WC_FF_TEST_STAGE_PREP_ZERO;
    wheel_ff_test_level = 0U;
    wc_ff_test_level_index = 0U;

    wc_ff_test_target_rpm = 0.0f;
    wheel_ff_test_current_command_rpm = 0.0f;

    wc_ff_test_run_start_tick = now;
    wc_ff_test_stage_start_tick = now;
    wheel_ff_test_stage_elapsed_ms = 0U;
    wheel_ff_test_elapsed_ms = 0U;

    wheel_ff_test_log_stop_request = 0U;
    wheel_ff_test_log_start_request = 1U;
  }

  if (wheel_ff_test_active == 0U)
  {
    return;
  }

  wheel_ff_test_elapsed_ms = (uint32_t)(now - wc_ff_test_run_start_tick);
  wheel_ff_test_stage_elapsed_ms =
      (uint32_t)(now - wc_ff_test_stage_start_tick);

  /*
   * Bất kỳ command ngoài khác 0 trong lúc test đều hủy test.
   * Nhờ vậy joystick/D-pad/Voice không bị "bỏ qua" âm thầm.
   */
  if (wheel_target_pwm != 0)
  {
    WheelControl_EmergencyStop();
    return;
  }

  switch (wheel_ff_test_stage)
  {
    case WC_FF_TEST_STAGE_PREP_ZERO:
      wc_ff_test_target_rpm = 0.0f;
      wheel_ff_test_current_command_rpm = 0.0f;

      if (wheel_ff_test_stage_elapsed_ms >= wheel_ff_test_prep_ms)
      {
        wc_ff_test_level_index = 0U;
        wheel_ff_test_level = 1U;
        wheel_ff_test_stage = WC_FF_TEST_STAGE_RAMP_LEVEL;
        wc_ff_test_stage_start_tick = now;
      }
      break;

    case WC_FF_TEST_STAGE_RAMP_LEVEL:
      requested_rpm = WC_FFTestLevelRPM(wc_ff_test_level_index);
      wc_ff_test_target_rpm = requested_rpm;
      wheel_ff_test_current_command_rpm = requested_rpm;

      target_error = WC_AbsFloat(wheel_pid_target_rpm - requested_rpm);

      if (target_error <= WC_FF_TEST_REACH_TOL_RPM)
      {
        wheel_ff_test_stage = WC_FF_TEST_STAGE_HOLD_LEVEL;
        wc_ff_test_stage_start_tick = now;
      }
      break;

    case WC_FF_TEST_STAGE_HOLD_LEVEL:
      requested_rpm = WC_FFTestLevelRPM(wc_ff_test_level_index);
      wc_ff_test_target_rpm = requested_rpm;
      wheel_ff_test_current_command_rpm = requested_rpm;

      if (wheel_ff_test_stage_elapsed_ms >= wheel_ff_test_hold_ms)
      {
        if (wc_ff_test_level_index < 3U)
        {
          wc_ff_test_level_index++;
          wheel_ff_test_level = (uint8_t)(wc_ff_test_level_index + 1U);
          wheel_ff_test_stage = WC_FF_TEST_STAGE_RAMP_LEVEL;
          wc_ff_test_stage_start_tick = now;
        }
        else
        {
          wheel_ff_test_level = 0U;
          wheel_ff_test_stage = WC_FF_TEST_STAGE_RAMP_ZERO;
          wc_ff_test_stage_start_tick = now;
        }
      }
      break;

    case WC_FF_TEST_STAGE_RAMP_ZERO:
      wc_ff_test_target_rpm = 0.0f;
      wheel_ff_test_current_command_rpm = 0.0f;

      if (WC_AbsFloat(wheel_pid_target_rpm) <= WC_FF_TEST_REACH_TOL_RPM)
      {
        wheel_ff_test_stage = WC_FF_TEST_STAGE_FINAL_ZERO;
        wc_ff_test_stage_start_tick = now;
      }
      break;

    case WC_FF_TEST_STAGE_FINAL_ZERO:
      wc_ff_test_target_rpm = 0.0f;
      wheel_ff_test_current_command_rpm = 0.0f;

      if (wheel_ff_test_stage_elapsed_ms >= wheel_ff_test_final_ms)
      {
        WC_FFTestFinish(2U);
      }
      break;

    default:
      WC_FFTestFinish(4U);
      break;
  }
}

static float WC_ClampFloat(float value, float min_value, float max_value)
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

static int16_t WC_RoundPWM(float value)
{
  value = WC_ClampFloat(value, -(float)wc_pwm_max, (float)wc_pwm_max);

  if (value >= 0.0f)
  {
    return (int16_t)(value + 0.5f);
  }

  return (int16_t)(value - 0.5f);
}

static void WC_ApplyPWM(void)
{
  if (wc_motor_lu != NULL)
  {
    Motor_SetSpeed_PWM(wc_motor_lu, wheel_pwm_lu);
  }

  if (wc_motor_ru != NULL)
  {
    Motor_SetSpeed_PWM(wc_motor_ru, wheel_pwm_ru);
  }

  if (wc_motor_ld != NULL)
  {
    Motor_SetSpeed_PWM(wc_motor_ld, wheel_pwm_ld);
  }

  if (wc_motor_rd != NULL)
  {
    Motor_SetSpeed_PWM(wc_motor_rd, wheel_pwm_rd);
  }
}

static void WC_SetAllPWM(int16_t pwm)
{
  wheel_pwm_lu = pwm;
  wheel_pwm_ld = pwm;
  wheel_pwm_ru = pwm;
  wheel_pwm_rd = pwm;
  WC_ApplyPWM();
}

static void WC_ResetPIDFeedbackMA(void)
{
  uint8_t i;

  for (i = 0U; i < WC_PID_FEEDBACK_MA_SAMPLES; i++)
  {
    wc_pid_feedback_lu_history[i] = 0.0f;
    wc_pid_feedback_ld_history[i] = 0.0f;
    wc_pid_feedback_ru_history[i] = 0.0f;
    wc_pid_feedback_rd_history[i] = 0.0f;
  }

  wc_pid_feedback_ma_index = 0U;
  wc_pid_feedback_ma_count = 0U;
  wheel_pid_feedback_ma_count = 0U;

  wheel_pid_feedback_lu_rpm = 0.0f;
  wheel_pid_feedback_ld_rpm = 0.0f;
  wheel_pid_feedback_ru_rpm = 0.0f;
  wheel_pid_feedback_rd_rpm = 0.0f;
}

static float WC_AverageHistory(const float *history, uint8_t count)
{
  uint8_t i;
  float sum = 0.0f;

  if ((history == NULL) || (count == 0U))
  {
    return 0.0f;
  }

  for (i = 0U; i < count; i++)
  {
    sum += history[i];
  }

  return sum / (float)count;
}

static void WC_UpdatePIDFeedbackMA(void)
{
  uint8_t index = wc_pid_feedback_ma_index;
  uint8_t count;
  float raw_lu = (wc_motor_lu != NULL) ? wc_motor_lu->current_rpm : 0.0f;
  float raw_ld = (wc_motor_ld != NULL) ? wc_motor_ld->current_rpm : 0.0f;
  float raw_ru = (wc_motor_ru != NULL) ? wc_motor_ru->current_rpm : 0.0f;
  float raw_rd = (wc_motor_rd != NULL) ? wc_motor_rd->current_rpm : 0.0f;

  wc_pid_feedback_lu_history[index] = raw_lu;
  wc_pid_feedback_ld_history[index] = raw_ld;
  wc_pid_feedback_ru_history[index] = raw_ru;
  wc_pid_feedback_rd_history[index] = raw_rd;

  if (wc_pid_feedback_ma_count < WC_PID_FEEDBACK_MA_SAMPLES)
  {
    wc_pid_feedback_ma_count++;
  }

  wc_pid_feedback_ma_index++;
  if (wc_pid_feedback_ma_index >= WC_PID_FEEDBACK_MA_SAMPLES)
  {
    wc_pid_feedback_ma_index = 0U;
  }

  count = wc_pid_feedback_ma_count;
  wheel_pid_feedback_ma_count = count;

  if (wheel_pid_feedback_ma_enable != 0U)
  {
    wheel_pid_feedback_lu_rpm =
        WC_AverageHistory(wc_pid_feedback_lu_history, count);
    wheel_pid_feedback_ld_rpm =
        WC_AverageHistory(wc_pid_feedback_ld_history, count);
    wheel_pid_feedback_ru_rpm =
        WC_AverageHistory(wc_pid_feedback_ru_history, count);
    wheel_pid_feedback_rd_rpm =
        WC_AverageHistory(wc_pid_feedback_rd_history, count);
  }
  else
  {
    wheel_pid_feedback_lu_rpm = raw_lu;
    wheel_pid_feedback_ld_rpm = raw_ld;
    wheel_pid_feedback_ru_rpm = raw_ru;
    wheel_pid_feedback_rd_rpm = raw_rd;
  }
}

static void WC_ResetAllPID(void)
{
  PID_Reset(&wheel_pid_lu);
  PID_Reset(&wheel_pid_ld);
  PID_Reset(&wheel_pid_ru);
  PID_Reset(&wheel_pid_rd);

  wc_pid_hunt_lu_active = 0U;
  wc_pid_hunt_ld_active = 0U;
  wc_pid_hunt_ru_active = 0U;
  wc_pid_hunt_rd_active = 0U;

  wc_pid_hunt_lu_enter_count = 0U;
  wc_pid_hunt_ld_enter_count = 0U;
  wc_pid_hunt_ru_enter_count = 0U;
  wc_pid_hunt_rd_enter_count = 0U;

  wc_pid_hunt_lu_exit_count = 0U;
  wc_pid_hunt_ld_exit_count = 0U;
  wc_pid_hunt_ru_exit_count = 0U;
  wc_pid_hunt_rd_exit_count = 0U;

  wheel_pid_hunt_active_lu = 0U;
  wheel_pid_hunt_active_ld = 0U;
  wheel_pid_hunt_active_ru = 0U;
  wheel_pid_hunt_active_rd = 0U;

  wheel_plateau_adapt_i_mask = 0U;
  wheel_ru_capture_assist_active = 0U;
  wheel_ru_capture_assist_pwm = 0.0f;
  wc_ru_capture_prev_integral_hold = 0U;
  wc_ru_capture_start_tick = 0U;

  WC_ResetPIDFeedbackMA();

  wheel_pid_last_sample_tick = 0U;
  wc_pid_prev_target_rpm = 0.0f;

  wc_ff_accel_prev_target_rpm = 0.0f;
  wc_ff_accel_prev_valid = 0U;
  wheel_ff_target_accel_rpm_per_s = 0.0f;
  wheel_ff_accel_pwm = 0.0f;

  WC_ClearSyncState();
}


static void WC_ApplyTuningRequest(void)
{
  float kp;
  float ki;
  float kd;
  float integral_limit;

  if (wheel_pid_apply_tuning == 0U)
  {
    return;
  }

  wheel_pid_apply_tuning = 0U;

  /* Keep debugger mistakes bounded to sane non-negative values. */
  kp = WC_ClampFloat(wheel_pid_tune_kp, 0.0f, 100.0f);
  ki = WC_ClampFloat(wheel_pid_tune_ki, 0.0f, 100.0f);
  kd = WC_ClampFloat(wheel_pid_tune_kd, 0.0f, 20.0f);
  integral_limit = WC_ClampFloat(wheel_pid_tune_integral_limit,
                                 0.0f,
                                 (float)wc_pwm_max);

  wheel_pid_tune_kp = kp;
  wheel_pid_tune_ki = ki;
  wheel_pid_tune_kd = kd;
  wheel_pid_tune_integral_limit = integral_limit;

  wheel_pid_lu.kp = kp;
  wheel_pid_lu.ki = ki;
  wheel_pid_lu.kd = kd;
  wheel_pid_lu.integral_limit = integral_limit;

  wheel_pid_ld.kp = kp;
  wheel_pid_ld.ki = ki;
  wheel_pid_ld.kd = kd;
  wheel_pid_ld.integral_limit = integral_limit;

  wheel_pid_ru.kp = kp;
  wheel_pid_ru.ki = ki;
  wheel_pid_ru.kd = kd;
  wheel_pid_ru.integral_limit = integral_limit;

  wheel_pid_rd.kp = kp;
  wheel_pid_rd.ki = ki;
  wheel_pid_rd.kd = kd;
  wheel_pid_rd.integral_limit = integral_limit;

  WC_ResetAllPID();
  wheel_pid_tuning_apply_count++;
}

static float WC_CommandToTargetRPM(int16_t command)
{
  float target_rpm;

  if (wc_pwm_max <= 0)
  {
    return 0.0f;
  }

  target_rpm =
      ((float)command * WC_TARGET_RPM_MAX) / (float)wc_pwm_max;

  return WC_ClampFloat(target_rpm,
                       -WC_TARGET_RPM_MAX,
                        WC_TARGET_RPM_MAX);
}

static float WC_FeedforwardPWM(float target_rpm,
                               float k_pwm_per_rpm,
                               float offset_pwm)
{
  float magnitude;
  float abs_target;

  k_pwm_per_rpm = WC_ClampFloat(k_pwm_per_rpm, 0.0f, 30.0f);
  offset_pwm = WC_ClampFloat(offset_pwm, 0.0f, (float)wc_pwm_max);

  if ((target_rpm <= WC_PID_ZERO_EPS_RPM) &&
      (target_rpm >= -WC_PID_ZERO_EPS_RPM))
  {
    return 0.0f;
  }

  abs_target = (target_rpm < 0.0f) ? -target_rpm : target_rpm;
  magnitude = (k_pwm_per_rpm * abs_target) + offset_pwm;
  magnitude = WC_ClampFloat(magnitude, 0.0f, (float)wc_pwm_max);

  return (target_rpm < 0.0f) ? -magnitude : magnitude;
}

static float WC_MoveTowardFloat(float current,
                                float target,
                                float max_step)
{
  if (max_step <= 0.0f)
  {
    return current;
  }

  if (current < target)
  {
    current += max_step;

    if (current > target)
    {
      current = target;
    }
  }
  else if (current > target)
  {
    current -= max_step;

    if (current < target)
    {
      current = target;
    }
  }

  return current;
}

/*
 * Quintic smoothstep setpoint (zero first and second derivatives at both ends).
 * Only reshapes the speed target. FF + PI, encoder, pwm limits are unchanged.
 * A new FIXED command starts a new segment from the current setpoint.
 * No sudden setpoint step when the operator changes a FIXED command.
 */
static void WC_FixedSCurveReset(void)
{
  wheel_fixed_scurve_active = 0U;
  wc_fixed_scurve_start_rpm = 0.0f;
  wc_fixed_scurve_goal_rpm = 0.0f;
  wc_fixed_scurve_elapsed_s = 0.0f;
  wc_fixed_scurve_duration_s = 0.0f;
}

static float WC_FixedSCurveStep(float current, float goal, float dt_s)
{
  float distance;
  float scale;
  float progress;
  float smooth;

  distance = WC_AbsFloat(goal - current);

  if (distance <= WC_PID_ZERO_EPS_RPM)
  {
    WC_FixedSCurveReset();
    return goal;
  }

  if ((wheel_fixed_scurve_active == 0U) ||
      (WC_AbsFloat(goal - wc_fixed_scurve_goal_rpm) > WC_PID_ZERO_EPS_RPM))
  {
    wc_fixed_scurve_start_rpm = current;
    wc_fixed_scurve_goal_rpm = goal;
    wc_fixed_scurve_elapsed_s = 0.0f;
    scale = WC_ClampFloat(wheel_fixed_scurve_40rpm_s, 1.0f, 4.0f);
    wheel_fixed_scurve_40rpm_s = scale;
    wc_fixed_scurve_duration_s = WC_ClampFloat(
        (distance / WC_FIXED_SCURVE_REFERENCE_RPM) * scale,
        WC_FIXED_SCURVE_MIN_SEGMENT_S,
        WC_FIXED_SCURVE_MAX_SEGMENT_S);
    wheel_fixed_scurve_active = 1U;
  }

  /* dt_s is already limited to 100 ms: a debugger pause cannot skip the ramp. */
  wc_fixed_scurve_elapsed_s += dt_s;

  if (wc_fixed_scurve_elapsed_s >= wc_fixed_scurve_duration_s)
  {
    WC_FixedSCurveReset();
    return goal;
  }

  progress = wc_fixed_scurve_elapsed_s / wc_fixed_scurve_duration_s;
  smooth = progress * progress * progress *
           (progress * (progress * 6.0f - 15.0f) + 10.0f);

  return wc_fixed_scurve_start_rpm +
         (wc_fixed_scurve_goal_rpm - wc_fixed_scurve_start_rpm) * smooth;
}

static int16_t WC_TargetRPMToCommand(float target_rpm)
{
  float command;

  if (WC_TARGET_RPM_MAX <= 0.0f)
  {
    return 0;
  }

  command = (target_rpm * (float)wc_pwm_max) / WC_TARGET_RPM_MAX;
  return WC_RoundPWM(command);
}

/*
 * Slew-rate limiter trong miền RPM.
 *
 * - Joystick, D-pad và Voice đều chỉ tạo raw target.
 * - Setpoint thật đưa vào PID thay đổi với tốc độ giới hạn.
 * - Khi đảo chiều: giảm target về 0 -> chờ dead-time -> tăng chiều mới.
 * - wheel_current_pwm được giữ như "command tương đương" của target đã
 *   làm mượt để các biến debug/logic cũ vẫn còn ý nghĩa.
 */
static void WC_TargetRPMUpdate(uint32_t now)
{
  uint32_t dt_ms;
  float dt_s;
  float raw_target;
  float command_target;
  float current;
  float accel_rate;
  float decel_rate;
  float zero_rate;
  float max_step;
  float current_abs;
  float command_abs;
  float command_gap;
  float soft_window = 0.0f;
  float soft_min_rate = 0.0f;
  float zero_soft_window = 0.0f;
  float zero_soft_min_rate = 0.0f;
  float hysteresis = 0.0f;
  float effective_decel_rate;
  float effective_zero_rate;
  WheelCommandProfile_t profile;
  uint8_t reversing = 0U;
  uint8_t zero_command = 0U;

  if (wheel_ff_test_active != 0U)
  {
    raw_target = wc_ff_test_target_rpm;
    profile = WHEEL_COMMAND_PROFILE_FIXED;
  }
  else
  {
    raw_target = WC_CommandToTargetRPM(wheel_target_pwm);
    profile = wheel_command_profile;
  }

  /* Raw luôn là đúng lệnh nguồn để logger/debug không che mất jitter joystick. */
  wheel_target_raw_rpm = raw_target;
  wheel_manual_soft_landing_active = 0U;
  wheel_manual_zero_soft_landing_active = 0U;

  /*
   * MANUAL target hysteresis:
   * - zero và đổi dấu được nhận ngay để STOP/reverse không bị trễ;
   * - cùng chiều, thay đổi nhỏ hơn hysteresis bị bỏ qua;
   * - khi vượt ngưỡng, nhận target mới nguyên vẹn.
   * FIXED không cần hysteresis vì target đã sạch.
   */
  if (profile == WHEEL_COMMAND_PROFILE_MANUAL)
  {
    hysteresis = WC_ClampFloat(wheel_manual_target_hysteresis_rpm,
                               0.0f,
                               5.0f);
    wheel_manual_target_hysteresis_rpm = hysteresis;

    if ((raw_target <= WC_PID_ZERO_EPS_RPM) &&
        (raw_target >= -WC_PID_ZERO_EPS_RPM))
    {
      wc_manual_accepted_target_rpm = 0.0f;
    }
    else if (((wc_manual_accepted_target_rpm > WC_PID_ZERO_EPS_RPM) &&
              (raw_target < -WC_PID_ZERO_EPS_RPM)) ||
             ((wc_manual_accepted_target_rpm < -WC_PID_ZERO_EPS_RPM) &&
              (raw_target > WC_PID_ZERO_EPS_RPM)) ||
             ((wc_manual_accepted_target_rpm <= WC_PID_ZERO_EPS_RPM) &&
              (wc_manual_accepted_target_rpm >= -WC_PID_ZERO_EPS_RPM)) ||
             (WC_AbsFloat(raw_target - wc_manual_accepted_target_rpm) >=
              hysteresis))
    {
      wc_manual_accepted_target_rpm = raw_target;
    }

    command_target = wc_manual_accepted_target_rpm;
  }
  else
  {
    wc_manual_accepted_target_rpm = 0.0f;
    command_target = raw_target;
  }

  wheel_command_target_rpm = command_target;

  if (wc_target_last_tick == 0U)
  {
    wc_target_last_tick = now;
    wheel_current_pwm = WC_TargetRPMToCommand(wheel_pid_target_rpm);
    return;
  }

  dt_ms = (uint32_t)(now - wc_target_last_tick);

  if (dt_ms == 0U)
  {
    return;
  }

  wc_target_last_tick = now;
  dt_s = (float)dt_ms / 1000.0f;

  if (dt_s > WC_TARGET_DT_MAX_S)
  {
    dt_s = WC_TARGET_DT_MAX_S;
  }

  /*
   * Chọn profile theo nguồn lệnh. Từng tham số vẫn chỉnh được bằng
   * Live Expressions và được clamp để lỗi nhập số không tạo step nguy hiểm.
   */
  if (profile == WHEEL_COMMAND_PROFILE_MANUAL)
  {
    accel_rate = WC_ClampFloat(wheel_manual_accel_rpm_per_s, 1.0f, 500.0f);
    decel_rate = WC_ClampFloat(wheel_manual_decel_rpm_per_s, 1.0f, 500.0f);
    zero_rate = WC_ClampFloat(wheel_manual_zero_rpm_per_s, 1.0f, 500.0f);
    soft_window = WC_ClampFloat(wheel_manual_soft_landing_window_rpm,
                                0.0f,
                                30.0f);
    soft_min_rate = WC_ClampFloat(wheel_manual_soft_landing_min_rate_rpm_per_s,
                                  1.0f,
                                  decel_rate);
    zero_soft_window = WC_ClampFloat(wheel_manual_zero_soft_landing_window_rpm,
                                     0.0f,
                                     30.0f);
    zero_soft_min_rate = WC_ClampFloat(
        wheel_manual_zero_soft_landing_min_rate_rpm_per_s,
        1.0f,
        zero_rate);

    wheel_manual_accel_rpm_per_s = accel_rate;
    wheel_manual_decel_rpm_per_s = decel_rate;
    wheel_manual_zero_rpm_per_s = zero_rate;
    wheel_manual_soft_landing_window_rpm = soft_window;
    wheel_manual_soft_landing_min_rate_rpm_per_s = soft_min_rate;
    wheel_manual_zero_soft_landing_window_rpm = zero_soft_window;
    wheel_manual_zero_soft_landing_min_rate_rpm_per_s = zero_soft_min_rate;
  }
  else
  {
    profile = WHEEL_COMMAND_PROFILE_FIXED;
    accel_rate = WC_ClampFloat(wheel_fixed_accel_rpm_per_s, 1.0f, 500.0f);
    decel_rate = WC_ClampFloat(wheel_fixed_decel_rpm_per_s, 1.0f, 500.0f);
    zero_rate = WC_ClampFloat(wheel_fixed_zero_rpm_per_s, 1.0f, 500.0f);

    wheel_fixed_accel_rpm_per_s = accel_rate;
    wheel_fixed_decel_rpm_per_s = decel_rate;
    wheel_fixed_zero_rpm_per_s = zero_rate;
  }

  /* Public active-rate aliases for logger/debug. */
  wheel_target_accel_rpm_per_s = accel_rate;
  wheel_target_decel_rpm_per_s = decel_rate;
  wheel_target_zero_rpm_per_s = zero_rate;

  current = wheel_pid_target_rpm;
  command_abs = WC_AbsFloat(command_target);
  current_abs = WC_AbsFloat(current);

  if (command_abs <= WC_PID_ZERO_EPS_RPM)
  {
    zero_command = 1U;
  }

  if (wheel_reverse_wait_active != 0U)
  {
    WC_FixedSCurveReset();
    wheel_pid_target_rpm = 0.0f;
    wheel_current_pwm = 0;

    /* Nếu người dùng đã hủy lệnh đảo chiều thì thoát dead-time ngay. */
    if (zero_command != 0U)
    {
      wheel_reverse_wait_active = 0U;
      return;
    }

    if ((uint32_t)(now - wc_reverse_wait_start_tick) <
        WC_REVERSE_DEADTIME_MS)
    {
      return;
    }

    wheel_reverse_wait_active = 0U;
    current = 0.0f;
    current_abs = 0.0f;
  }

  if (((current > WC_PID_ZERO_EPS_RPM) &&
       (command_target < -WC_PID_ZERO_EPS_RPM)) ||
      ((current < -WC_PID_ZERO_EPS_RPM) &&
       (command_target > WC_PID_ZERO_EPS_RPM)))
  {
    reversing = 1U;
  }

  /*
   * M6-S02 first stage: S-curve for clean FIXED targets only.
   * - MANUAL joystick and FF auto-calibration keep their validated behavior.
   * - Reverse still goes through zero + existing 150 ms dead-time.
   * - EmergencyStop() bypasses this shaper and resets the segment.
   */
  if ((profile == WHEEL_COMMAND_PROFILE_FIXED) &&
      (wheel_fixed_scurve_enable != 0U) &&
      (wheel_ff_test_active == 0U))
  {
    if (reversing != 0U)
    {
      current = WC_FixedSCurveStep(current, 0.0f, dt_s);

      if (WC_AbsFloat(current) <= WC_PID_ZERO_EPS_RPM)
      {
        current = 0.0f;
        WC_FixedSCurveReset();
        wheel_reverse_wait_active = 1U;
        wc_reverse_wait_start_tick = now;
        wheel_reverse_count++;
      }
    }
    else
    {
      current = WC_FixedSCurveStep(current, command_target, dt_s);
    }

    current = WC_ClampFloat(current,
                            -WC_TARGET_RPM_MAX,
                             WC_TARGET_RPM_MAX);

    if ((zero_command != 0U) &&
        (WC_AbsFloat(current) <= WC_PID_ZERO_EPS_RPM))
    {
      current = 0.0f;
      WC_FixedSCurveReset();
    }

    wheel_pid_target_rpm = current;
    wheel_current_pwm = WC_TargetRPMToCommand(current);
    return;
  }

  WC_FixedSCurveReset();

  if (reversing != 0U)
  {
    /* Đảo chiều luôn phải đi qua 0; dùng stop-rate rồi giữ dead-time. */
    max_step = zero_rate * dt_s;
    current = WC_MoveTowardFloat(current, 0.0f, max_step);

    if ((current <= WC_PID_ZERO_EPS_RPM) &&
        (current >= -WC_PID_ZERO_EPS_RPM))
    {
      current = 0.0f;
      wheel_reverse_wait_active = 1U;
      wc_reverse_wait_start_tick = now;
      wheel_reverse_count++;
    }
  }
  else if (zero_command != 0U)
  {
    /*
     * Release joystick / STOP:
     * - ngoài zero_soft_window vẫn giữ stop-rate nhanh 100 rpm/s;
     * - MANUAL trong đoạn cuối về 0 giảm rate tuyến tính để RPM thực có thời
     *   gian bắt kịp target trước khi PID reset/PWM về 0;
     * - FIXED giữ nguyên profile cũ.
     *
     * Với baseline 15 rpm / 35 rpm/s, phần lớn quãng dừng vẫn ở 100 rpm/s;
     * chỉ khoảng 15 rpm cuối được làm mềm nên độ trễ tăng rất ít.
     */
    effective_zero_rate = zero_rate;

    if ((profile == WHEEL_COMMAND_PROFILE_MANUAL) &&
        (zero_soft_window > WC_PID_ZERO_EPS_RPM) &&
        (current_abs > WC_PID_ZERO_EPS_RPM) &&
        (current_abs < zero_soft_window))
    {
      wheel_manual_zero_soft_landing_active = 1U;
      effective_zero_rate = zero_soft_min_rate +
          ((zero_rate - zero_soft_min_rate) *
           (current_abs / zero_soft_window));
    }

    max_step = effective_zero_rate * dt_s;
    current = WC_MoveTowardFloat(current, 0.0f, max_step);
  }
  else
  {
    if (command_abs > current_abs)
    {
      max_step = accel_rate * dt_s;
    }
    else
    {
      effective_decel_rate = decel_rate;

      /*
       * Soft landing CHỈ cho MANUAL, giảm cùng chiều về target khác 0.
       * Trong soft_window cuối, decel rate giảm tuyến tính từ decel_rate
       * xuống soft_min_rate. Vì d(target)/dt cũng giảm mềm, dynamic FF Ka
       * không còn nhảy mạnh từ giá trị phanh sang 0 khi chạm plateau mới.
       */
      if ((profile == WHEEL_COMMAND_PROFILE_MANUAL) &&
          (soft_window > WC_PID_ZERO_EPS_RPM) &&
          (((current > WC_PID_ZERO_EPS_RPM) &&
            (command_target > WC_PID_ZERO_EPS_RPM)) ||
           ((current < -WC_PID_ZERO_EPS_RPM) &&
            (command_target < -WC_PID_ZERO_EPS_RPM))))
      {
        command_gap = WC_AbsFloat(current - command_target);

        if (command_gap < soft_window)
        {
          wheel_manual_soft_landing_active = 1U;
          effective_decel_rate = soft_min_rate +
              ((decel_rate - soft_min_rate) * (command_gap / soft_window));
        }
      }

      max_step = effective_decel_rate * dt_s;
    }

    current = WC_MoveTowardFloat(current, command_target, max_step);
  }

  current = WC_ClampFloat(current,
                          -WC_TARGET_RPM_MAX,
                           WC_TARGET_RPM_MAX);

  if ((current <= WC_PID_ZERO_EPS_RPM) &&
      (current >= -WC_PID_ZERO_EPS_RPM) &&
      (zero_command != 0U))
  {
    current = 0.0f;
  }

  wheel_pid_target_rpm = current;
  wheel_current_pwm = WC_TargetRPMToCommand(current);
}

static uint32_t WC_GetMasterSampleTick(void)
{
  if ((wheel_encoder_available_ld != 0U) && (wc_motor_ld != NULL))
  {
    return wc_motor_ld->last_time;
  }

  if ((wheel_encoder_available_ru != 0U) && (wc_motor_ru != NULL))
  {
    return wc_motor_ru->last_time;
  }

  if ((wheel_encoder_available_rd != 0U) && (wc_motor_rd != NULL))
  {
    return wc_motor_rd->last_time;
  }

  if ((wheel_encoder_available_lu != 0U) && (wc_motor_lu != NULL))
  {
    return wc_motor_lu->last_time;
  }

  return 0U;
}

static uint8_t WC_ShouldHoldIntegral(void)
{
  float gap;
  float threshold;

  if (wheel_pid_integral_hold_enable == 0U)
  {
    return 0U;
  }

  threshold = WC_ClampFloat(wheel_pid_integral_hold_gap_rpm,
                            0.05f,
                            5.0f);
  wheel_pid_integral_hold_gap_rpm = threshold;

  /* So sánh với target sau manual hysteresis, không phải raw ADC jitter. */
  gap = wheel_command_target_rpm - wheel_pid_target_rpm;
  if (gap < 0.0f)
  {
    gap = -gap;
  }

  if ((wheel_reverse_wait_active != 0U) || (gap > threshold))
  {
    return 1U;
  }

  return 0U;
}

static uint8_t WC_UpdateAntiHuntState(uint8_t current_state,
                                      uint8_t encoder_available,
                                      Motor_HandleTypeDef *motor,
                                      float target_rpm,
                                      float feedback_rpm,
                                      uint8_t integral_hold,
                                      uint8_t *enter_count,
                                      uint8_t *exit_count)
{
  float error_abs;
  uint8_t enter_required;
  uint8_t exit_required;

  if ((enter_count == NULL) || (exit_count == NULL))
  {
    return 0U;
  }

  /*
   * Anti-hunting chỉ dành cho plateau.
   * Khi đang ramp / reverse wait / không có encoder:
   * - PID hoạt động theo logic bình thường.
   * - Xóa debounce cũ để không mang trạng thái của plateau trước sang plateau mới.
   */
  if ((wheel_pid_hunt_enable == 0U) ||
      (integral_hold != 0U) ||
      (encoder_available == 0U) ||
      (motor == NULL))
  {
    *enter_count = 0U;
    *exit_count = 0U;
    return 0U;
  }

  enter_required = wheel_pid_hunt_enter_samples;
  exit_required = wheel_pid_hunt_exit_samples;

  if (enter_required < 1U)
  {
    enter_required = 1U;
  }

  if (exit_required < 1U)
  {
    exit_required = 1U;
  }

  error_abs = target_rpm - feedback_rpm;
  if (error_abs < 0.0f)
  {
    error_abs = -error_abs;
  }

  if (current_state != 0U)
  {
    /*
     * Đang HOLD:
     * Chỉ thoát khi error vượt exit threshold đủ N mẫu LIÊN TIẾP.
     * Một spike encoder đơn lẻ không được phép đánh thức PID.
     */
    *enter_count = 0U;

    if (error_abs >= wheel_pid_hunt_exit_error_rpm)
    {
      if (*exit_count < exit_required)
      {
        (*exit_count)++;
      }

      if (*exit_count >= exit_required)
      {
        *exit_count = 0U;
        return 0U;
      }
    }
    else
    {
      *exit_count = 0U;
    }

    return 1U;
  }

  /*
   * Chưa HOLD:
   * Chỉ vào HOLD khi error nằm trong enter threshold đủ N mẫu LIÊN TIẾP.
   */
  *exit_count = 0U;

  if (error_abs <= wheel_pid_hunt_enter_error_rpm)
  {
    if (*enter_count < enter_required)
    {
      (*enter_count)++;
    }

    if (*enter_count >= enter_required)
    {
      *enter_count = 0U;
      return 1U;
    }
  }
  else
  {
    *enter_count = 0U;
  }

  return 0U;
}

static int16_t WC_ClosedLoopPWM(volatile PID_Controller_t *pid,
                                uint8_t encoder_available,
                                Motor_HandleTypeDef *motor,
                                float target_rpm,
                                float feedback_rpm,
                                float feedforward_pwm,
                                float dt_s,
                                float output_min,
                                float output_max,
                                uint8_t integral_hold,
                                uint8_t anti_hunt_active,
                                uint8_t anti_hunt_just_entered,
                                uint8_t wheel_mask_bit,
                                uint8_t boost_allowed)
{
  float output;
  float output_unclamped;
  float error;
  float error_abs;
  float ki_saved;
  float ki_hold;
  float integral_candidate;
  float integral_delta;
  float adaptive_ki;
  float adaptive_step;
  float max_delta;

  if ((encoder_available == 0U) || (motor == NULL))
  {
    return WC_RoundPWM(feedforward_pwm);
  }

  if (anti_hunt_active != 0U)
  {
    /*
     * Plateau anti-hunt HOLD.
     *
     * Mục tiêu:
     * 1) Không cho P đuổi theo từng nấc lượng tử/noise RPM nhỏ.
     * 2) Không làm PWM bị hụt khi vừa bỏ P (bumpless transfer).
     * 3) Vẫn cho I trim CHẬM nếu có sai số tĩnh thật kéo dài.
     */
    error = target_rpm - feedback_rpm;
    error_abs = error;
    if (error_abs < 0.0f)
    {
      error_abs = -error_abs;
    }

    pid->error = error;
    pid->derivative = 0.0f;
    pid->prev_error = error;
    pid->initialized = 1U;

    if (anti_hunt_just_entered != 0U)
    {
      /*
       * Bumpless entry:
       * Chu kỳ trước output còn có P. Khi chuyển sang HOLD (P=0),
       * chuyển phần correction đang có vào integral sao cho:
       *
       *   FF + I_new ~= output_previous
       *
       * Nhờ vậy PWM không bị step xuống ngay lúc ENTER HOLD.
       */
      pid->integral = pid->output - feedforward_pwm;
      pid->integral = WC_ClampFloat(pid->integral,
                                    -pid->integral_limit,
                                     pid->integral_limit);
    }
    else if ((error_abs > wheel_pid_hunt_i_deadband_rpm) &&
             (error_abs < wheel_pid_hunt_exit_error_rpm))
    {
      /*
       * Slow-I trim:
       * - Trong deadband: I freeze hoàn toàn.
       * - Ngoài deadband nhưng chưa đủ lớn để thoát HOLD:
       *   I dịch rất chậm với Ki_hold = Ki * scale.
       * - Nếu error >= exit threshold nhưng chưa đủ debounce để thoát:
       *   I vẫn freeze để một spike encoder không bị tích phân.
       */
      ki_hold = pid->ki * wheel_pid_hunt_ki_scale;
      integral_delta = ki_hold * error * dt_s;
      integral_candidate = pid->integral + integral_delta;

      integral_candidate = WC_ClampFloat(integral_candidate,
                                         -pid->integral_limit,
                                          pid->integral_limit);

      output_unclamped = feedforward_pwm + integral_candidate;

      /*
       * Anti-windup đơn giản:
       * nếu candidate đã vượt saturation và delta tiếp tục đẩy ra xa
       * vùng hợp lệ thì bỏ lần tích phân đó.
       */
      if (!(((output_unclamped > output_max) && (integral_delta > 0.0f)) ||
            ((output_unclamped < output_min) && (integral_delta < 0.0f))))
      {
        pid->integral = integral_candidate;
      }
    }

    output = feedforward_pwm + pid->integral;
    output = WC_ClampFloat(output, output_min, output_max);
    pid->output = output;

    return WC_RoundPWM(output);
  }

  /*
   * Giữ integral mà không sửa pid.c:
   * PID_Update() chỉ cộng integral qua Ki*error*dt. Tạm đặt Ki=0 trong đúng
   * một lần update sẽ giữ nguyên integral hiện có, trong khi P, D, error,
   * derivative và anti-windup vẫn cập nhật bình thường. Khôi phục Ki ngay sau đó.
   */
  ki_saved = pid->ki;

  if (integral_hold != 0U)
  {
    pid->ki = 0.0f;
  }
  else if ((boost_allowed != 0U) &&
           (wheel_plateau_adapt_i_enable != 0U) &&
           ((wheel_plateau_adapt_i_wheels & wheel_mask_bit) != 0U) &&
           (ki_saved > 0.0f) &&
           (anti_hunt_active == 0U) &&
           (wheel_command_profile == WHEEL_COMMAND_PROFILE_FIXED) &&
           (WC_AbsFloat(target_rpm) >= WC_PLATEAU_ADAPT_I_MIN_TARGET_RPM) &&
           (WC_AbsFloat(target_rpm - feedback_rpm) >=
              wheel_plateau_adapt_i_min_error_rpm))
  {
    /* The step cap limits transient I movement even if error is large.
     * PID_Update still applies its original anti-windup and saturation rules.
     * Clamp the effective gain so both conditions can be satisfied. */
    adaptive_ki = wheel_plateau_adapt_i_ki;
    adaptive_step = WC_AbsFloat(target_rpm - feedback_rpm) * dt_s;
    max_delta = wheel_plateau_adapt_i_max_step_pwm;

    if (adaptive_step > 0.0f)
    {
      adaptive_ki = WC_ClampFloat(adaptive_ki,
                                   ki_saved,
                                   max_delta / adaptive_step);
    }

    if (adaptive_ki > ki_saved)
    {
      pid->ki = adaptive_ki;
      wheel_plateau_adapt_i_mask |= wheel_mask_bit;
      wheel_plateau_adapt_i_sample_count++;
    }
  }

  output = PID_Update(pid,
                      target_rpm,
                      feedback_rpm,
                      feedforward_pwm,
                      dt_s,
                      output_min,
                      output_max);

  pid->ki = ki_saved;

  return WC_RoundPWM(output);
}

static void WC_RefreshDebug(float ff_lu,
                            float ff_ld,
                            float ff_ru,
                            float ff_rd)
{
  wheel_ctrl_debug.target_command = wheel_target_pwm;
  wheel_ctrl_debug.ramp_command = wheel_current_pwm;
  wheel_ctrl_debug.raw_target_rpm = wheel_target_raw_rpm;
  wheel_ctrl_debug.command_target_rpm = wheel_command_target_rpm;
  wheel_ctrl_debug.target_rpm = wheel_pid_target_rpm;
  wheel_ctrl_debug.command_profile = (uint8_t)wheel_command_profile;
  wheel_ctrl_debug.soft_landing_active = wheel_manual_soft_landing_active;
  wheel_ctrl_debug.zero_soft_landing_active = wheel_manual_zero_soft_landing_active;
  wheel_ctrl_debug.zero_ff_taper_scale = wheel_manual_zero_ff_taper_scale;
  wheel_ctrl_debug.target_accel_rpm_per_s = wheel_target_accel_rpm_per_s;
  wheel_ctrl_debug.target_decel_rpm_per_s = wheel_target_decel_rpm_per_s;
  wheel_ctrl_debug.target_zero_rpm_per_s = wheel_target_zero_rpm_per_s;
  wheel_ctrl_debug.feedforward_pwm_lu = ff_lu;
  wheel_ctrl_debug.feedforward_pwm_ld = ff_ld;
  wheel_ctrl_debug.feedforward_pwm_ru = ff_ru;
  wheel_ctrl_debug.feedforward_pwm_rd = ff_rd;

  /* Field legacy: trung bình các bánh đang tham gia closed-loop. */
  wheel_ctrl_debug.feedforward_pwm =
      (wheel_encoder_available_lu != 0U)
          ? ((ff_lu + ff_ld + ff_ru + ff_rd) / 4.0f)
          : ((ff_ld + ff_ru + ff_rd) / 3.0f);

  wheel_ctrl_debug.rpm_lu = (wc_motor_lu != NULL) ? wc_motor_lu->current_rpm : 0.0f;
  wheel_ctrl_debug.rpm_ld = (wc_motor_ld != NULL) ? wc_motor_ld->current_rpm : 0.0f;
  wheel_ctrl_debug.rpm_ru = (wc_motor_ru != NULL) ? wc_motor_ru->current_rpm : 0.0f;
  wheel_ctrl_debug.rpm_rd = (wc_motor_rd != NULL) ? wc_motor_rd->current_rpm : 0.0f;

  wheel_ctrl_debug.pwm_lu = wheel_pwm_lu;
  wheel_ctrl_debug.pwm_ld = wheel_pwm_ld;
  wheel_ctrl_debug.pwm_ru = wheel_pwm_ru;
  wheel_ctrl_debug.pwm_rd = wheel_pwm_rd;

  wheel_ctrl_debug.pid_enable = wheel_pid_enable;
  wheel_ctrl_debug.pid_active = wheel_pid_active;
  wheel_ctrl_debug.reverse_wait_active = wheel_reverse_wait_active;
  wheel_ctrl_debug.pid_update_count = wheel_pid_update_count;
  wheel_ctrl_debug.reverse_count = wheel_reverse_count;

  wheel_ctrl_debug.tune_kp = wheel_pid_tune_kp;
  wheel_ctrl_debug.tune_ki = wheel_pid_tune_ki;
  wheel_ctrl_debug.tune_kd = wheel_pid_tune_kd;
  wheel_ctrl_debug.tune_integral_limit = wheel_pid_tune_integral_limit;
  wheel_ff_pwm_per_rpm =
      (wheel_encoder_available_lu != 0U)
          ? ((wheel_ff_k_lu + wheel_ff_k_ld + wheel_ff_k_ru + wheel_ff_k_rd) / 4.0f)
          : ((wheel_ff_k_ld + wheel_ff_k_ru + wheel_ff_k_rd) / 3.0f);
  wheel_ctrl_debug.ff_pwm_per_rpm = wheel_ff_pwm_per_rpm;

  wheel_ctrl_debug.ff_k_lu = wheel_ff_k_lu;
  wheel_ctrl_debug.ff_k_ld = wheel_ff_k_ld;
  wheel_ctrl_debug.ff_k_ru = wheel_ff_k_ru;
  wheel_ctrl_debug.ff_k_rd = wheel_ff_k_rd;

  wheel_ctrl_debug.ff_offset_lu = wheel_ff_offset_lu;
  wheel_ctrl_debug.ff_offset_ld = wheel_ff_offset_ld;
  wheel_ctrl_debug.ff_offset_ru = wheel_ff_offset_ru;
  wheel_ctrl_debug.ff_offset_rd = wheel_ff_offset_rd;
}

void WheelControl_Init(Motor_HandleTypeDef *left_up,
                       Motor_HandleTypeDef *right_up,
                       Motor_HandleTypeDef *left_down,
                       Motor_HandleTypeDef *right_down,
                       int16_t pwm_max)
{
  wc_motor_lu = left_up;
  wc_motor_ru = right_up;
  wc_motor_ld = left_down;
  wc_motor_rd = right_down;

  wc_pwm_max = (pwm_max > 0) ? pwm_max : 999;

  PID_Init(&wheel_pid_lu,
           WC_PID_KP_INIT,
           WC_PID_KI_INIT,
           WC_PID_KD_INIT,
           WC_PID_INTEGRAL_LIMIT_PWM);

  PID_Init(&wheel_pid_ld,
           WC_PID_KP_INIT,
           WC_PID_KI_INIT,
           WC_PID_KD_INIT,
           WC_PID_INTEGRAL_LIMIT_PWM);

  PID_Init(&wheel_pid_ru,
           WC_PID_KP_INIT,
           WC_PID_KI_INIT,
           WC_PID_KD_INIT,
           WC_PID_INTEGRAL_LIMIT_PWM);

  PID_Init(&wheel_pid_rd,
           WC_PID_KP_INIT,
           WC_PID_KI_INIT,
           WC_PID_KD_INIT,
           WC_PID_INTEGRAL_LIMIT_PWM);

  /* Close the speed loop by default for PS2/Voice, independently of logger.
   * EmergencyStop still forces zero PWM without disabling this default mode.
   */
  wheel_pid_enable = 1U;
  wheel_pid_active = 0U;
  wheel_pid_integral_hold_active = 0U;
  wheel_target_raw_rpm = 0.0f;
  wheel_command_target_rpm = 0.0f;
  wheel_pid_target_rpm = 0.0f;
  wheel_manual_soft_landing_active = 0U;
  wheel_manual_zero_soft_landing_active = 0U;
  wheel_manual_zero_ff_taper_scale = 1.0f;
  wc_manual_accepted_target_rpm = 0.0f;

  wheel_command_profile = WHEEL_COMMAND_PROFILE_FIXED;

  wheel_manual_accel_rpm_per_s = WC_MANUAL_ACCEL_RPM_PER_S_INIT;
  wheel_manual_decel_rpm_per_s = WC_MANUAL_DECEL_RPM_PER_S_INIT;
  wheel_manual_zero_rpm_per_s = WC_MANUAL_ZERO_RPM_PER_S_INIT;
  wheel_manual_soft_landing_window_rpm = WC_MANUAL_SOFT_LANDING_WINDOW_RPM_INIT;
  wheel_manual_soft_landing_min_rate_rpm_per_s = WC_MANUAL_SOFT_LANDING_MIN_RATE_INIT;
  wheel_manual_zero_soft_landing_window_rpm = WC_MANUAL_ZERO_SOFT_LANDING_WINDOW_RPM_INIT;
  wheel_manual_zero_soft_landing_min_rate_rpm_per_s = WC_MANUAL_ZERO_SOFT_LANDING_MIN_RATE_INIT;
  wheel_manual_target_hysteresis_rpm = WC_MANUAL_TARGET_HYSTERESIS_RPM_INIT;

  wheel_fixed_accel_rpm_per_s = WC_FIXED_ACCEL_RPM_PER_S_INIT;
  wheel_fixed_decel_rpm_per_s = WC_FIXED_DECEL_RPM_PER_S_INIT;
  wheel_fixed_zero_rpm_per_s = WC_FIXED_ZERO_RPM_PER_S_INIT;
  wheel_fixed_scurve_enable = WC_FIXED_SCURVE_ENABLE_INIT;
  wheel_fixed_scurve_40rpm_s = WC_FIXED_SCURVE_40RPM_S_INIT;
  WC_FixedSCurveReset();

  wheel_target_accel_rpm_per_s = WC_FIXED_ACCEL_RPM_PER_S_INIT;
  wheel_target_decel_rpm_per_s = WC_FIXED_DECEL_RPM_PER_S_INIT;
  wheel_target_zero_rpm_per_s = WC_FIXED_ZERO_RPM_PER_S_INIT;
  wheel_ff_k_lu = WC_FF_K_LU_INIT;
  wheel_ff_k_ld = WC_FF_K_LD_INIT;
  wheel_ff_k_ru = WC_FF_K_RU_INIT;
  wheel_ff_k_rd = WC_FF_K_RD_INIT;

  wheel_ff_offset_lu = WC_FF_OFFSET_LU_INIT;
  wheel_ff_offset_ld = WC_FF_OFFSET_LD_INIT;
  wheel_ff_offset_ru = WC_FF_OFFSET_RU_INIT;
  wheel_ff_offset_rd = WC_FF_OFFSET_RD_INIT;
  wheel_ff_offset_rd_reverse = WC_FF_OFFSET_RD_REVERSE_INIT;

  /* Dynamic FF baseline is fixed ON at Ka=0.18. */
  wheel_ff_accel_enable = 1U;
  wheel_ff_accel_k = WC_FF_ACCEL_K_FIXED;
  wheel_ff_accel_max_pwm = WC_FF_ACCEL_MAX_PWM_INIT;
  wheel_ff_target_accel_rpm_per_s = 0.0f;
  wheel_ff_accel_pwm = 0.0f;
  wc_ff_accel_prev_target_rpm = 0.0f;
  wc_ff_accel_prev_valid = 0U;

  wheel_sync_enable = 0U;
  wheel_sync_k_pwm_per_rpm = WC_SYNC_K_PWM_PER_RPM_INIT;
  wheel_sync_max_pwm = WC_SYNC_MAX_PWM_INIT;
  wheel_sync_spread_enter_rpm = WC_SYNC_SPREAD_ENTER_RPM_INIT;
  wheel_sync_spread_exit_rpm = WC_SYNC_SPREAD_EXIT_RPM_INIT;
  WC_ClearSyncState();

  wheel_ff_pwm_per_rpm =
      (wheel_encoder_available_lu != 0U)
          ? ((wheel_ff_k_lu + wheel_ff_k_ld + wheel_ff_k_ru + wheel_ff_k_rd) / 4.0f)
          : ((wheel_ff_k_ld + wheel_ff_k_ru + wheel_ff_k_rd) / 3.0f);
  wheel_pid_update_count = 0U;
  wheel_reverse_count = 0U;

  wheel_pid_feedback_ma_enable = 1U;
  WC_ResetPIDFeedbackMA();

  wheel_pid_tune_kp = WC_PID_KP_INIT;
  wheel_pid_tune_ki = WC_PID_KI_INIT;
  wheel_pid_tune_kd = WC_PID_KD_INIT;
  wheel_pid_tune_integral_limit = WC_PID_INTEGRAL_LIMIT_PWM;
  wheel_pid_apply_tuning = 0U;
  wheel_pid_tuning_apply_count = 0U;

  wheel_pid_integral_hold_enable = 1U;
  wheel_pid_integral_hold_active = 0U;
  wheel_pid_integral_hold_gap_rpm = WC_PID_INTEGRAL_HOLD_GAP_RPM_INIT;
  wheel_pid_integral_hold_count = 0U;
  wheel_plateau_adapt_i_enable = WC_PLATEAU_ADAPT_I_ENABLE_INIT;
  wheel_plateau_adapt_i_wheels = WC_PLATEAU_ADAPT_I_WHEELS_INIT;
  wheel_plateau_adapt_i_ki = WC_PLATEAU_ADAPT_I_KI_INIT;
  wheel_plateau_adapt_i_min_error_rpm = WC_PLATEAU_ADAPT_I_MIN_ERROR_RPM_INIT;
  wheel_plateau_adapt_i_max_step_pwm = WC_PLATEAU_ADAPT_I_MAX_STEP_PWM_INIT;
  wheel_plateau_adapt_i_mask = 0U;
  wheel_plateau_adapt_i_sample_count = 0U;

  wheel_pid_hunt_enable = WC_PID_HUNT_ENABLE_INIT;
  wheel_pid_hunt_enter_error_rpm = WC_PID_HUNT_ENTER_ERROR_RPM_INIT;
  wheel_pid_hunt_exit_error_rpm = WC_PID_HUNT_EXIT_ERROR_RPM_INIT;
  wheel_pid_hunt_enter_samples = WC_PID_HUNT_ENTER_SAMPLES_INIT;
  wheel_pid_hunt_exit_samples = WC_PID_HUNT_EXIT_SAMPLES_INIT;
  wheel_pid_hunt_i_deadband_rpm = WC_PID_HUNT_I_DEADBAND_RPM_INIT;
  wheel_pid_hunt_ki_scale = WC_PID_HUNT_KI_SCALE_INIT;
  wheel_pid_hunt_active_lu = 0U;
  wheel_pid_hunt_active_ld = 0U;
  wheel_pid_hunt_active_ru = 0U;
  wheel_pid_hunt_active_rd = 0U;
  wheel_pid_hunt_entry_count = 0U;

  wheel_ru_capture_assist_enable = WC_RU_CAPTURE_ASSIST_ENABLE_INIT;
  wheel_ru_capture_assist_k_pwm_per_rpm =
      WC_RU_CAPTURE_ASSIST_K_PWM_PER_RPM_INIT;
  wheel_ru_capture_assist_max_pwm = WC_RU_CAPTURE_ASSIST_MAX_PWM_INIT;
  wheel_ru_capture_assist_timeout_ms = WC_RU_CAPTURE_ASSIST_TIMEOUT_MS_INIT;
  wheel_ru_capture_assist_active = 0U;
  wheel_ru_capture_assist_pwm = 0.0f;
  wheel_ru_capture_assist_entry_count = 0U;
  wheel_ru_capture_assist_timeout_count = 0U;
  wc_ru_capture_prev_integral_hold = 0U;
  wc_ru_capture_start_tick = 0U;

  wheel_ff_test_start = 0U;
  wheel_ff_test_active = 0U;
  wheel_ff_test_done = 0U;
  wheel_ff_test_status = 0U;
  wheel_ff_test_stage = WC_FF_TEST_STAGE_IDLE;
  wheel_ff_test_level = 0U;
  wheel_ff_test_current_command_rpm = 0.0f;
  wheel_ff_test_stage_elapsed_ms = 0U;
  wheel_ff_test_elapsed_ms = 0U;
  wheel_ff_test_run_count = 0U;
  wheel_ff_test_log_start_request = 0U;
  wheel_ff_test_log_stop_request = 0U;
  wc_ff_test_target_rpm = 0.0f;
  wc_ff_test_stage_start_tick = 0U;
  wc_ff_test_run_start_tick = 0U;
  wc_ff_test_level_index = 0U;

  WheelControl_EmergencyStop();
}

void WheelControl_SetCommandProfile(WheelCommandProfile_t profile)
{
  if (profile == WHEEL_COMMAND_PROFILE_MANUAL)
  {
    wheel_command_profile = WHEEL_COMMAND_PROFILE_MANUAL;
  }
  else
  {
    wheel_command_profile = WHEEL_COMMAND_PROFILE_FIXED;
  }
}

void WheelControl_SetTargetPWM(int16_t target_pwm)
{
  if (target_pwm > wc_pwm_max)
  {
    target_pwm = wc_pwm_max;
  }
  else if (target_pwm < -wc_pwm_max)
  {
    target_pwm = -wc_pwm_max;
  }

  wheel_target_pwm = target_pwm;
}

void WheelControl_EmergencyStop(void)
{
  if (wheel_ff_test_active != 0U)
  {
    wheel_ff_test_active = 0U;
    wheel_ff_test_done = 1U;
    wheel_ff_test_status = 4U;
    wheel_ff_test_stage = WC_FF_TEST_STAGE_IDLE;
    wheel_ff_test_level = 0U;
    wheel_ff_test_current_command_rpm = 0.0f;
    wheel_ff_test_log_stop_request = 1U;
    wc_ff_test_target_rpm = 0.0f;
  }

  wheel_target_pwm = 0;
  wheel_current_pwm = 0;

  wheel_reverse_wait_active = 0U;
  wc_reverse_wait_start_tick = 0U;
  wc_target_last_tick = HAL_GetTick();
  WC_FixedSCurveReset();

  wheel_pid_active = 0U;
  wheel_pid_integral_hold_active = 0U;
  wheel_target_raw_rpm = 0.0f;
  wheel_command_target_rpm = 0.0f;
  wheel_pid_target_rpm = 0.0f;
  wheel_manual_soft_landing_active = 0U;
  wheel_manual_zero_soft_landing_active = 0U;
  wheel_manual_zero_ff_taper_scale = 1.0f;
  wc_manual_accepted_target_rpm = 0.0f;
  wc_pid_enable_prev = wheel_pid_enable;

  WC_ResetAllPID();
  WC_SetAllPWM(0);
  WC_RefreshDebug(0.0f, 0.0f, 0.0f, 0.0f);
}

void WheelControl_Update(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t sample_tick;
  uint32_t dt_ms;
  float dt_s;
  float target_rpm;
  float ff_lu;
  float ff_ld;
  float ff_ru;
  float ff_rd;
  float rd_ff_offset;
  float static_ff_offset_scale;
  float zero_ff_taper_window;
  float target_accel_rpm_per_s;
  float accel_ff_pwm;
  float accel_ff_limit;
  float sync_sum_rpm;
  float sync_min_rpm;
  float sync_max_rpm;
  float sync_k;
  float sync_limit;
  float sync_enter_rpm;
  float sync_exit_rpm;
  float output_min;
  float output_max;
  float hunt_enter;
  float hunt_exit;
  float ru_capture_error;
  float ru_capture_k;
  float ru_capture_limit;
  float ru_capture_pwm;
  uint32_t ru_capture_elapsed_ms;
  uint8_t integral_hold;
  uint8_t sync_count;
  uint8_t hunt_lu_prev;
  uint8_t hunt_ld_prev;
  uint8_t hunt_ru_prev;
  uint8_t hunt_rd_prev;
  uint8_t hunt_lu_entered;
  uint8_t hunt_ld_entered;
  uint8_t hunt_ru_entered;
  uint8_t hunt_rd_entered;

  WC_ApplyTuningRequest();
  WC_FFTestUpdate(now);
  WC_TargetRPMUpdate(now);

  target_rpm = wheel_pid_target_rpm;
  integral_hold = WC_ShouldHoldIntegral();

  if ((integral_hold != 0U) &&
      (wheel_pid_integral_hold_active == 0U))
  {
    wheel_pid_integral_hold_count++;
  }

  wheel_pid_integral_hold_active = integral_hold;

  /* Clamp tuning values before use; Live Expressions mistakes stay bounded. */
  wheel_ff_k_lu = WC_ClampFloat(wheel_ff_k_lu, 0.0f, 30.0f);
  wheel_ff_k_ld = WC_ClampFloat(wheel_ff_k_ld, 0.0f, 30.0f);
  wheel_ff_k_ru = WC_ClampFloat(wheel_ff_k_ru, 0.0f, 30.0f);
  wheel_ff_k_rd = WC_ClampFloat(wheel_ff_k_rd, 0.0f, 30.0f);

  wheel_ff_offset_lu = WC_ClampFloat(wheel_ff_offset_lu, 0.0f, (float)wc_pwm_max);
  wheel_ff_offset_ld = WC_ClampFloat(wheel_ff_offset_ld, 0.0f, (float)wc_pwm_max);
  wheel_ff_offset_ru = WC_ClampFloat(wheel_ff_offset_ru, 0.0f, (float)wc_pwm_max);
  wheel_ff_offset_rd = WC_ClampFloat(wheel_ff_offset_rd, 0.0f, (float)wc_pwm_max);
  wheel_ff_offset_rd_reverse = WC_ClampFloat(wheel_ff_offset_rd_reverse, 0.0f, (float)wc_pwm_max);

  /*
   * Ka baseline: ép status/debug về cấu hình cố định mỗi chu kỳ.
   * Như vậy không cần set wheel_ff_accel_enable/k bằng Live Expressions
   * sau reset hoặc trước mỗi test PI.
   */
  wheel_ff_accel_enable = 1U;
  wheel_ff_accel_k = WC_FF_ACCEL_K_FIXED;
  wheel_ff_accel_max_pwm =
      WC_ClampFloat(wheel_ff_accel_max_pwm, 0.0f, (float)wc_pwm_max);

  /*
   * Zero FF taper - MANUAL STOP only.
   *
   * Bản v4 làm mềm 15 rpm cuối giúp RPM thực bám target tốt hơn, nhưng static
   * FF vẫn giữ nguyên offset Coulomb/dead-zone nên ngay sát 0 motor vẫn có thể
   * còn hàng chục PWM rồi bị cắt thẳng khi target chạm 0.
   *
   * Trong đúng vùng zero soft-landing, chỉ taper phần OFFSET:
   *
   *   offset_eff = offset * |target| / zero_soft_window
   *
   * Kff*RPM vẫn giữ nguyên vì bản thân nó đã tự tiến về 0 cùng target. Dynamic
   * FF Ka vẫn giữ nguyên để còn mô-men hãm nhẹ trong lúc target đang giảm.
   */
  static_ff_offset_scale = 1.0f;
  zero_ff_taper_window = WC_ClampFloat(
      wheel_manual_zero_soft_landing_window_rpm,
      WC_PID_ZERO_EPS_RPM,
      30.0f);

  if ((wheel_command_profile == WHEEL_COMMAND_PROFILE_MANUAL) &&
      (wheel_manual_zero_soft_landing_active != 0U))
  {
    static_ff_offset_scale = WC_ClampFloat(
        WC_AbsFloat(target_rpm) / zero_ff_taper_window,
        0.0f,
        1.0f);
  }

  wheel_manual_zero_ff_taper_scale = static_ff_offset_scale;

  ff_lu = WC_FeedforwardPWM(target_rpm,
                             wheel_ff_k_lu,
                             wheel_ff_offset_lu * static_ff_offset_scale);
  ff_ld = WC_FeedforwardPWM(target_rpm,
                             wheel_ff_k_ld,
                             wheel_ff_offset_ld * static_ff_offset_scale);
  ff_ru = WC_FeedforwardPWM(target_rpm,
                             wheel_ff_k_ru,
                             wheel_ff_offset_ru * static_ff_offset_scale);
  /*
   * RD có ma sát/dead-zone bất đối xứng theo chiều. Giữ offset tiến hiện tại
   * để không phá response đã đẹp; chỉ dùng offset riêng khi target âm.
   */
  rd_ff_offset = (target_rpm < -WC_PID_ZERO_EPS_RPM)
                     ? wheel_ff_offset_rd_reverse
                     : wheel_ff_offset_rd;

  ff_rd = WC_FeedforwardPWM(target_rpm,
                             wheel_ff_k_rd,
                             rd_ff_offset * static_ff_offset_scale);

  if (wheel_pid_enable != wc_pid_enable_prev)
  {
    WC_ResetAllPID();
    wc_pid_enable_prev = wheel_pid_enable;
  }

  if (wheel_pid_enable == 0U)
  {
    wheel_pid_active = 0U;
    wheel_pid_integral_hold_active = 0U;
    WC_SetAllPWM(wheel_current_pwm);
    WC_RefreshDebug(ff_lu, ff_ld, ff_ru, ff_rd);
    return;
  }

  wheel_pid_active = 1U;

  if ((target_rpm <= WC_PID_ZERO_EPS_RPM) &&
      (target_rpm >= -WC_PID_ZERO_EPS_RPM))
  {
    wheel_pid_integral_hold_active = 0U;
    WC_ResetAllPID();
    WC_SetAllPWM(0);
    WC_RefreshDebug(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if ((wc_pid_prev_target_rpm * target_rpm) < 0.0f)
  {
    WC_ResetAllPID();
  }

  wc_pid_prev_target_rpm = target_rpm;

  sample_tick = WC_GetMasterSampleTick();

  if ((sample_tick == 0U) ||
      (sample_tick == wheel_pid_last_sample_tick))
  {
    WC_RefreshDebug(ff_lu, ff_ld, ff_ru, ff_rd);
    return;
  }

  if (wheel_pid_last_sample_tick == 0U)
  {
    dt_ms = 50U;
  }
  else
  {
    dt_ms = (uint32_t)(sample_tick - wheel_pid_last_sample_tick);
  }

  wheel_pid_last_sample_tick = sample_tick;
  dt_s = (float)dt_ms / 1000.0f;

  if ((dt_s < WC_PID_DT_MIN_S) || (dt_s > WC_PID_DT_MAX_S))
  {
    WC_ResetAllPID();
    WC_RefreshDebug(ff_lu, ff_ld, ff_ru, ff_rd);
    return;
  }

  /*
   * Cập nhật rolling average 5 mẫu đúng một lần trên mỗi sample PID.
   * PID vẫn chạy theo nhịp sample mới (~24 ms), chỉ feedback được làm mượt.
   */
  WC_UpdatePIDFeedbackMA();

  /*
   * Acceleration feedforward dùng đạo hàm của SETPOINT đã qua slew limiter,
   * không dùng đạo hàm RPM encoder.
   *
   * Vì vậy:
   * - Không khuếch đại noise encoder.
   * - Chỉ tạo thêm torque khi target thực sự đang tăng/giảm.
   * - Khi plateau, d(target)/dt = 0 nên term này tự biến mất.
   */
  target_accel_rpm_per_s = 0.0f;
  accel_ff_pwm = 0.0f;

  if (wc_ff_accel_prev_valid != 0U)
  {
    target_accel_rpm_per_s =
        (target_rpm - wc_ff_accel_prev_target_rpm) / dt_s;

    /*
     * Dynamic FF được áp cứng ON với Ka=0.18 baseline.
     * Không phụ thuộc thao tác kích hoạt/tuning runtime nữa.
     */
    accel_ff_pwm = WC_FF_ACCEL_K_FIXED * target_accel_rpm_per_s;

    accel_ff_limit = wheel_ff_accel_max_pwm;
    accel_ff_pwm = WC_ClampFloat(accel_ff_pwm,
                                 -accel_ff_limit,
                                  accel_ff_limit);
  }

  wc_ff_accel_prev_target_rpm = target_rpm;
  wc_ff_accel_prev_valid = 1U;

  wheel_ff_target_accel_rpm_per_s = target_accel_rpm_per_s;
  wheel_ff_accel_pwm = accel_ff_pwm;

  ff_lu += accel_ff_pwm;
  ff_ld += accel_ff_pwm;
  ff_ru += accel_ff_pwm;
  ff_rd += accel_ff_pwm;

  /*
   * RU FIXED-plateau capture assist.
   *
   * Only runs after the target slew has finished. This is deliberately not a
   * new steady-state FF offset: it is a short transient correction so RU can
   * enter the existing 0.8-rpm steady-hold window quickly without overfeeding
   * the already-good warm plateau.
   */
  ru_capture_pwm = 0.0f;
  ru_capture_k = WC_ClampFloat(wheel_ru_capture_assist_k_pwm_per_rpm,
                               0.0f, 20.0f);
  ru_capture_limit = WC_ClampFloat(wheel_ru_capture_assist_max_pwm,
                                   0.0f, 30.0f);
  wheel_ru_capture_assist_k_pwm_per_rpm = ru_capture_k;
  wheel_ru_capture_assist_max_pwm = ru_capture_limit;

  if (wheel_ru_capture_assist_timeout_ms < 100U)
  {
    wheel_ru_capture_assist_timeout_ms = 100U;
  }
  else if (wheel_ru_capture_assist_timeout_ms > 2000U)
  {
    wheel_ru_capture_assist_timeout_ms = 2000U;
  }

  /* Detect FIXED plateau start on PID-sample timing: integral hold 1 -> 0. */
  if ((wheel_ru_capture_assist_enable != 0U) &&
      (wheel_command_profile == WHEEL_COMMAND_PROFILE_FIXED) &&
      (integral_hold == 0U) &&
      (wc_ru_capture_prev_integral_hold != 0U) &&
      (wheel_encoder_available_ru != 0U) &&
      (wc_motor_ru != NULL) &&
      (WC_AbsFloat(target_rpm) >= WC_RU_CAPTURE_ASSIST_MIN_TARGET_RPM_INIT) &&
      (wc_pid_hunt_ru_active == 0U))
  {
    wheel_ru_capture_assist_active = 1U;
    wc_ru_capture_start_tick = sample_tick;
    wheel_ru_capture_assist_entry_count++;
  }

  if (wheel_ru_capture_assist_active != 0U)
  {
    /* Abort cleanly if the command starts moving again or leaves FIXED mode. */
    if ((wheel_command_profile != WHEEL_COMMAND_PROFILE_FIXED) ||
        (integral_hold != 0U) ||
        (WC_AbsFloat(target_rpm) < WC_RU_CAPTURE_ASSIST_MIN_TARGET_RPM_INIT))
    {
      /* Preserve current output approximately by moving assist into I. */
      wheel_pid_ru.integral = WC_ClampFloat(
          wheel_pid_ru.integral + wheel_ru_capture_assist_pwm,
          -wheel_pid_ru.integral_limit,
           wheel_pid_ru.integral_limit);
      wheel_ru_capture_assist_active = 0U;
      wheel_ru_capture_assist_pwm = 0.0f;
      wc_ru_capture_start_tick = 0U;
    }
    else
    {
      ru_capture_elapsed_ms = (uint32_t)(sample_tick - wc_ru_capture_start_tick);

      if (ru_capture_elapsed_ms >= wheel_ru_capture_assist_timeout_ms)
      {
        /* Timeout is bumpless: transfer the last assist into integral. */
        wheel_pid_ru.integral = WC_ClampFloat(
            wheel_pid_ru.integral + wheel_ru_capture_assist_pwm,
            -wheel_pid_ru.integral_limit,
             wheel_pid_ru.integral_limit);
        wheel_ru_capture_assist_active = 0U;
        wheel_ru_capture_assist_pwm = 0.0f;
        wheel_ru_capture_assist_timeout_count++;
        wc_ru_capture_start_tick = 0U;
      }
      else
      {
        ru_capture_error = target_rpm - wheel_pid_feedback_ru_rpm;
        ru_capture_pwm = WC_ClampFloat(ru_capture_k * ru_capture_error,
                                      -ru_capture_limit,
                                       ru_capture_limit);
        wheel_ru_capture_assist_pwm = ru_capture_pwm;
        ff_ru += ru_capture_pwm;
      }
    }
  }
  else
  {
    wheel_ru_capture_assist_pwm = 0.0f;
  }

  /*
   * Cross-wheel synchronization: ramp-only + spread hysteresis.
   *
   * - Plateau hoặc sync disable -> tắt latch và correction = 0.
   * - Trong ramp, chỉ bật khi spread MA5 >= ENTER.
   * - Khi đã bật, chỉ tắt khi spread MA5 <= EXIT.
   *
   * Mọi bánh có wheel_encoder_available_* = 1 đều tham gia reference/correction.
   */
  sync_k = WC_ClampFloat(wheel_sync_k_pwm_per_rpm, 0.0f, 20.0f);
  sync_limit = WC_ClampFloat(wheel_sync_max_pwm, 0.0f, 50.0f);
  sync_enter_rpm = WC_ClampFloat(wheel_sync_spread_enter_rpm, 0.0f, 20.0f);
  sync_exit_rpm = WC_ClampFloat(wheel_sync_spread_exit_rpm, 0.0f, 20.0f);

  /* EXIT không được lớn hơn ENTER; nếu Live Expressions nhập nhầm thì tự kẹp. */
  if (sync_exit_rpm > sync_enter_rpm)
  {
    sync_exit_rpm = sync_enter_rpm;
  }

  wheel_sync_k_pwm_per_rpm = sync_k;
  wheel_sync_max_pwm = sync_limit;
  wheel_sync_spread_enter_rpm = sync_enter_rpm;
  wheel_sync_spread_exit_rpm = sync_exit_rpm;

  /* Xóa output/debug mỗi chu kỳ nhưng giữ wheel_sync_active làm latch hysteresis. */
  WC_ClearSyncOutputs();
  wheel_sync_spread_rpm = 0.0f;

  if ((wheel_sync_enable != 0U) &&
      (WC_AbsFloat(target_accel_rpm_per_s) >=
       WC_SYNC_MIN_TARGET_ACCEL_RPM_PER_S))
  {
    sync_sum_rpm = 0.0f;
    sync_min_rpm = 0.0f;
    sync_max_rpm = 0.0f;
    sync_count = 0U;

    if ((wheel_encoder_available_lu != 0U) && (wc_motor_lu != NULL))
    {
      sync_sum_rpm += wheel_pid_feedback_lu_rpm;
      sync_min_rpm = wheel_pid_feedback_lu_rpm;
      sync_max_rpm = wheel_pid_feedback_lu_rpm;
      sync_count++;
    }

    if ((wheel_encoder_available_ld != 0U) && (wc_motor_ld != NULL))
    {
      sync_sum_rpm += wheel_pid_feedback_ld_rpm;
      if (sync_count == 0U)
      {
        sync_min_rpm = wheel_pid_feedback_ld_rpm;
        sync_max_rpm = wheel_pid_feedback_ld_rpm;
      }
      else
      {
        if (wheel_pid_feedback_ld_rpm < sync_min_rpm)
        {
          sync_min_rpm = wheel_pid_feedback_ld_rpm;
        }
        if (wheel_pid_feedback_ld_rpm > sync_max_rpm)
        {
          sync_max_rpm = wheel_pid_feedback_ld_rpm;
        }
      }
      sync_count++;
    }

    if ((wheel_encoder_available_ru != 0U) && (wc_motor_ru != NULL))
    {
      sync_sum_rpm += wheel_pid_feedback_ru_rpm;
      if (sync_count == 0U)
      {
        sync_min_rpm = wheel_pid_feedback_ru_rpm;
        sync_max_rpm = wheel_pid_feedback_ru_rpm;
      }
      else
      {
        if (wheel_pid_feedback_ru_rpm < sync_min_rpm)
        {
          sync_min_rpm = wheel_pid_feedback_ru_rpm;
        }
        if (wheel_pid_feedback_ru_rpm > sync_max_rpm)
        {
          sync_max_rpm = wheel_pid_feedback_ru_rpm;
        }
      }
      sync_count++;
    }

    if ((wheel_encoder_available_rd != 0U) && (wc_motor_rd != NULL))
    {
      sync_sum_rpm += wheel_pid_feedback_rd_rpm;
      if (sync_count == 0U)
      {
        sync_min_rpm = wheel_pid_feedback_rd_rpm;
        sync_max_rpm = wheel_pid_feedback_rd_rpm;
      }
      else
      {
        if (wheel_pid_feedback_rd_rpm < sync_min_rpm)
        {
          sync_min_rpm = wheel_pid_feedback_rd_rpm;
        }
        if (wheel_pid_feedback_rd_rpm > sync_max_rpm)
        {
          sync_max_rpm = wheel_pid_feedback_rd_rpm;
        }
      }
      sync_count++;
    }

    if (sync_count >= 2U)
    {
      wheel_sync_spread_rpm = sync_max_rpm - sync_min_rpm;

      if (wheel_sync_active == 0U)
      {
        if (wheel_sync_spread_rpm >= sync_enter_rpm)
        {
          wheel_sync_active = 1U;
        }
      }
      else if (wheel_sync_spread_rpm <= sync_exit_rpm)
      {
        wheel_sync_active = 0U;
      }

      if (wheel_sync_active != 0U)
      {
        wheel_sync_ref_rpm = sync_sum_rpm / (float)sync_count;

        if ((wheel_encoder_available_lu != 0U) && (wc_motor_lu != NULL))
        {
          wheel_sync_error_lu_rpm =
              wheel_sync_ref_rpm - wheel_pid_feedback_lu_rpm;
          wheel_sync_pwm_lu =
              WC_ClampFloat(sync_k * wheel_sync_error_lu_rpm,
                            -sync_limit,
                             sync_limit);
          ff_lu += wheel_sync_pwm_lu;
        }

        if ((wheel_encoder_available_ld != 0U) && (wc_motor_ld != NULL))
        {
          wheel_sync_error_ld_rpm =
              wheel_sync_ref_rpm - wheel_pid_feedback_ld_rpm;
          wheel_sync_pwm_ld =
              WC_ClampFloat(sync_k * wheel_sync_error_ld_rpm,
                            -sync_limit,
                             sync_limit);
          ff_ld += wheel_sync_pwm_ld;
        }

        if ((wheel_encoder_available_ru != 0U) && (wc_motor_ru != NULL))
        {
          wheel_sync_error_ru_rpm =
              wheel_sync_ref_rpm - wheel_pid_feedback_ru_rpm;
          wheel_sync_pwm_ru =
              WC_ClampFloat(sync_k * wheel_sync_error_ru_rpm,
                            -sync_limit,
                             sync_limit);
          ff_ru += wheel_sync_pwm_ru;
        }

        if ((wheel_encoder_available_rd != 0U) && (wc_motor_rd != NULL))
        {
          wheel_sync_error_rd_rpm =
              wheel_sync_ref_rpm - wheel_pid_feedback_rd_rpm;
          wheel_sync_pwm_rd =
              WC_ClampFloat(sync_k * wheel_sync_error_rd_rpm,
                            -sync_limit,
                             sync_limit);
          ff_rd += wheel_sync_pwm_rd;
        }
      }
    }
    else
    {
      WC_ClearSyncState();
    }
  }
  else
  {
    WC_ClearSyncState();
  }

  if (target_rpm > WC_PID_ZERO_EPS_RPM)
  {
    output_min = 0.0f;
    output_max = (float)wc_pwm_max;
  }
  else
  {
    output_min = -(float)wc_pwm_max;
    output_max = 0.0f;
  }

  /*
   * Clamp threshold và bảo đảm có hysteresis thật sự.
   * Nếu Live Expressions nhập exit <= enter, tự ép exit lớn hơn enter 0.05 rpm.
   */
  hunt_enter = WC_ClampFloat(wheel_pid_hunt_enter_error_rpm,
                             0.05f,
                             5.0f);
  hunt_exit = WC_ClampFloat(wheel_pid_hunt_exit_error_rpm,
                            0.10f,
                            10.0f);

  if (hunt_exit <= hunt_enter)
  {
    hunt_exit = hunt_enter + 0.05f;
  }

  wheel_pid_hunt_enter_error_rpm = hunt_enter;
  wheel_pid_hunt_exit_error_rpm = hunt_exit;

  if (wheel_pid_hunt_enter_samples < 1U)
  {
    wheel_pid_hunt_enter_samples = 1U;
  }
  else if (wheel_pid_hunt_enter_samples > 10U)
  {
    wheel_pid_hunt_enter_samples = 10U;
  }

  if (wheel_pid_hunt_exit_samples < 1U)
  {
    wheel_pid_hunt_exit_samples = 1U;
  }
  else if (wheel_pid_hunt_exit_samples > 10U)
  {
    wheel_pid_hunt_exit_samples = 10U;
  }

  wheel_pid_hunt_i_deadband_rpm =
      WC_ClampFloat(wheel_pid_hunt_i_deadband_rpm,
                    0.0f,
                    wheel_pid_hunt_exit_error_rpm);

  wheel_pid_hunt_ki_scale =
      WC_ClampFloat(wheel_pid_hunt_ki_scale,
                    0.0f,
                    1.0f);

  wheel_plateau_adapt_i_ki = WC_ClampFloat(wheel_plateau_adapt_i_ki,
                                            WC_PID_KI_INIT,
                                            40.0f);
  wheel_plateau_adapt_i_min_error_rpm =
      WC_ClampFloat(wheel_plateau_adapt_i_min_error_rpm,
                    0.4f, 3.0f);
  wheel_plateau_adapt_i_max_step_pwm =
      WC_ClampFloat(wheel_plateau_adapt_i_max_step_pwm,
                    0.25f, 3.0f);

  hunt_lu_prev = wc_pid_hunt_lu_active;
  hunt_ld_prev = wc_pid_hunt_ld_active;
  hunt_ru_prev = wc_pid_hunt_ru_active;
  hunt_rd_prev = wc_pid_hunt_rd_active;

  wc_pid_hunt_lu_active =
      WC_UpdateAntiHuntState(wc_pid_hunt_lu_active,
                             wheel_encoder_available_lu,
                             wc_motor_lu,
                             target_rpm,
                             wheel_pid_feedback_lu_rpm,
                             integral_hold,
                             &wc_pid_hunt_lu_enter_count,
                             &wc_pid_hunt_lu_exit_count);

  wc_pid_hunt_ld_active =
      WC_UpdateAntiHuntState(wc_pid_hunt_ld_active,
                             wheel_encoder_available_ld,
                             wc_motor_ld,
                             target_rpm,
                             wheel_pid_feedback_ld_rpm,
                             integral_hold,
                             &wc_pid_hunt_ld_enter_count,
                             &wc_pid_hunt_ld_exit_count);

  wc_pid_hunt_ru_active =
      WC_UpdateAntiHuntState(wc_pid_hunt_ru_active,
                             wheel_encoder_available_ru,
                             wc_motor_ru,
                             target_rpm,
                             wheel_pid_feedback_ru_rpm,
                             integral_hold,
                             &wc_pid_hunt_ru_enter_count,
                             &wc_pid_hunt_ru_exit_count);

  wc_pid_hunt_rd_active =
      WC_UpdateAntiHuntState(wc_pid_hunt_rd_active,
                             wheel_encoder_available_rd,
                             wc_motor_rd,
                             target_rpm,
                             wheel_pid_feedback_rd_rpm,
                             integral_hold,
                             &wc_pid_hunt_rd_enter_count,
                             &wc_pid_hunt_rd_exit_count);

  hunt_lu_entered =
      ((hunt_lu_prev == 0U) && (wc_pid_hunt_lu_active != 0U)) ? 1U : 0U;
  hunt_ld_entered =
      ((hunt_ld_prev == 0U) && (wc_pid_hunt_ld_active != 0U)) ? 1U : 0U;
  hunt_ru_entered =
      ((hunt_ru_prev == 0U) && (wc_pid_hunt_ru_active != 0U)) ? 1U : 0U;
  hunt_rd_entered =
      ((hunt_rd_prev == 0U) && (wc_pid_hunt_rd_active != 0U)) ? 1U : 0U;

  if ((hunt_lu_entered != 0U) ||
      (hunt_ld_entered != 0U) ||
      (hunt_ru_entered != 0U) ||
      (hunt_rd_entered != 0U))
  {
    wheel_pid_hunt_entry_count++;
  }

  wheel_pid_hunt_active_lu = wc_pid_hunt_lu_active;
  wheel_pid_hunt_active_ld = wc_pid_hunt_ld_active;
  wheel_pid_hunt_active_ru = wc_pid_hunt_ru_active;
  wheel_pid_hunt_active_rd = wc_pid_hunt_rd_active;

  if ((hunt_ru_entered != 0U) &&
      (wheel_ru_capture_assist_active != 0U))
  {
    /*
     * Remove capture FF on the exact HOLD-entry sample. WC_ClosedLoopPWM()
     * sees anti_hunt_just_entered=1 and transfers the previous output into I,
     * so there is no PWM step when the temporary assist disappears.
     */
    ff_ru -= wheel_ru_capture_assist_pwm;
    wheel_ru_capture_assist_pwm = 0.0f;
    wheel_ru_capture_assist_active = 0U;
    wc_ru_capture_start_tick = 0U;
  }

  wheel_plateau_adapt_i_mask = 0U;

  wheel_pwm_lu = WC_ClosedLoopPWM(&wheel_pid_lu,
                                   wheel_encoder_available_lu,
                                   wc_motor_lu,
                                   target_rpm,
                                   wheel_pid_feedback_lu_rpm,
                                   ff_lu,
                                   dt_s,
                                   output_min,
                                   output_max,
                                   integral_hold,
                                   wc_pid_hunt_lu_active,
                                   hunt_lu_entered,
                                   1U,
                                   1U);

  wheel_pwm_ld = WC_ClosedLoopPWM(&wheel_pid_ld,
                                   wheel_encoder_available_ld,
                                   wc_motor_ld,
                                   target_rpm,
                                   wheel_pid_feedback_ld_rpm,
                                   ff_ld,
                                   dt_s,
                                   output_min,
                                   output_max,
                                   integral_hold,
                                   wc_pid_hunt_ld_active,
                                   hunt_ld_entered,
                                   2U,
                                   1U);

  wheel_pwm_ru = WC_ClosedLoopPWM(&wheel_pid_ru,
                                   wheel_encoder_available_ru,
                                   wc_motor_ru,
                                   target_rpm,
                                   wheel_pid_feedback_ru_rpm,
                                   ff_ru,
                                   dt_s,
                                   output_min,
                                   output_max,
                                   integral_hold,
                                   wc_pid_hunt_ru_active,
                                   hunt_ru_entered,
                                   4U,
                                   (wheel_ru_capture_assist_active == 0U) ? 1U : 0U);

  wheel_pwm_rd = WC_ClosedLoopPWM(&wheel_pid_rd,
                                   wheel_encoder_available_rd,
                                   wc_motor_rd,
                                   target_rpm,
                                   wheel_pid_feedback_rd_rpm,
                                   ff_rd,
                                   dt_s,
                                   output_min,
                                   output_max,
                                   integral_hold,
                                   wc_pid_hunt_rd_active,
                                   hunt_rd_entered,
                                   8U,
                                   1U);

  WC_ApplyPWM();
  wheel_pid_update_count++;
  wc_ru_capture_prev_integral_hold = integral_hold;
  WC_RefreshDebug(ff_lu, ff_ld, ff_ru, ff_rd);
}
