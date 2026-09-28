#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f7xx_hal.h"

/*
 * Loại bộ timer được sử dụng để đọc encoder.
 *
 * Dự án hiện tại có:
 * - 5 encoder sử dụng TIM1, TIM2, TIM3, TIM4 và TIM8.
 * - 1 encoder sử dụng LPTIM1.
 *
 * motor.c dựa vào giá trị này để chọn đúng hàm đọc counter.
 */
typedef enum {
    MOTOR_ENCODER_TIM = 0,  // Encoder sử dụng timer TIM thông thường
    MOTOR_ENCODER_LPTIM     // Encoder sử dụng Low-Power Timer LPTIM
} Motor_EncoderTypeDef;


/*
 * Cấu trúc lưu toàn bộ cấu hình và trạng thái của một động cơ.
 *
 * Mỗi motor có:
 * - Một kênh PWM để điều khiển tốc độ.
 * - Một chân DIR để điều khiển chiều quay.
 * - Một timer đọc encoder.
 * - Các biến lưu trạng thái để tính tốc độ RPM.
 */
typedef struct {

    /* =========================================================
     * 1. CẤU HÌNH PHẦN CỨNG ĐIỀU KHIỂN MOTOR
     * ========================================================= */

    /*
     * Con trỏ tới timer tạo PWM cho motor.
     *
     * Ví dụ:
     * &htim9, &htim10, &htim11, &htim12 hoặc &htim14.
     */
    TIM_HandleTypeDef *htim_pwm;

    /*
     * Kênh PWM nằm trong timer htim_pwm.
     *
     * Ví dụ:
     * TIM_CHANNEL_1 hoặc TIM_CHANNEL_2.
     */
    uint32_t channel_pwm;

    /*
     * Cổng GPIO chứa chân điều khiển chiều quay DIR.
     *
     * Ví dụ:
     * GPIOE, GPIOF hoặc GPIOD.
     */
    GPIO_TypeDef *port_dir;

    /*
     * Chân GPIO điều khiển chiều quay DIR.
     *
     * Ví dụ:
     * GPIO_PIN_3, GPIO_PIN_12 hoặc GPIO_PIN_14.
     */
    uint16_t pin_dir;


    /* =========================================================
     * 2. CẤU HÌNH PHẦN CỨNG ĐỌC ENCODER
     * ========================================================= */

    /*
     * Con trỏ tới TIM dùng để đọc encoder.
     *
     * Biến này được sử dụng khi:
     * encoder_type == MOTOR_ENCODER_TIM.
     *
     * Ví dụ:
     * &htim1, &htim2, &htim3, &htim4 hoặc &htim8.
     *
     * Nếu motor sử dụng LPTIM1 thì biến này bằng NULL.
     */
    TIM_HandleTypeDef *htim_enc;

    /*
     * Con trỏ tới LPTIM dùng để đọc encoder.
     *
     * Biến này được sử dụng khi:
     * encoder_type == MOTOR_ENCODER_LPTIM.
     *
     * Trong dự án hiện tại, giá trị này là &hlptim1.
     *
     * Nếu motor sử dụng TIM thông thường thì biến này bằng NULL.
     */
    LPTIM_HandleTypeDef *hlptim_enc;

    /*
     * Cho biết encoder của motor sử dụng TIM hay LPTIM.
     *
     * Giá trị có thể là:
     * - MOTOR_ENCODER_TIM
     * - MOTOR_ENCODER_LPTIM
     */
    Motor_EncoderTypeDef encoder_type;


    /* =========================================================
     * 3. THÔNG SỐ ĐẶC TÍNH CỦA MOTOR VÀ ENCODER
     * ========================================================= */

    /*
     * Tổng số count encoder trong một vòng quay.
     *
     * Đây là số count sau khi tính hệ số giải mã quadrature,
     * không phải PPR gốc được nhà sản xuất ghi trên encoder.
     *
     * Giá trị danh định đang dùng:
     * - Motor 110 RPM, encoder 11 PPR:
     *   11 x 4 = 44 count/vòng.
     *
     * - Motor 32 RPM, encoder 16 PPR:
     *   16 x 4 = 64 count/vòng.
     *
     * Các giá trị 44 và 64 chưa tính thêm tỉ số truyền và cần
     * được đo lại bằng thực nghiệm.
     */
    float counts_per_revolution;

    /*
     * Cờ đảo logic chân DIR riêng cho từng motor.
     *
     * direction_inverted = 0:
     * - Lệnh tốc độ dương xuất DIR ở mức mặc định.
     *
     * direction_inverted = 1:
     * - Đảo mức logic chân DIR so với mặc định.
     *
     * Cờ này chỉ đảo chiều điều khiển motor.
     * Nó không đảo dấu số đếm encoder hoặc RPM.
     */
    uint8_t direction_inverted;


    /* =========================================================
     * 4. TRẠNG THÁI VẬN HÀNH CỦA MOTOR
     * ========================================================= */

    /*
     * Giá trị counter encoder tại lần đọc trước.
     *
     * Counter encoder hiện là bộ đếm 16-bit nên giá trị nằm trong
     * khoảng từ 0 đến 65535.
     *
     * Biến này được dùng cùng current_count để tính delta_count.
     */
    uint16_t last_count;

    /*
     * Thời điểm thực hiện lần cập nhật RPM gần nhất.
     *
     * Đơn vị: millisecond.
     * Giá trị được lấy từ HAL_GetTick().
     */
    uint32_t last_time;

    /*
     * Số count encoder thay đổi kể từ lần cập nhật trước.
     *
     * delta_count > 0:
     * Encoder quay theo chiều đếm tăng.
     *
     * delta_count < 0:
     * Encoder quay theo chiều đếm giảm.
     *
     * motor.c xử lý cả trường hợp counter 16-bit bị tràn từ
     * 65535 về 0 hoặc quay ngược từ 0 về 65535.
     */
    int32_t delta_count;

    /*
     * Tốc độ quay được tính từ encoder.
     *
     * Đơn vị: vòng/phút (RPM).
     *
     * current_rpm > 0:
     * Encoder quay theo chiều đếm tăng.
     *
     * current_rpm < 0:
     * Encoder quay theo chiều đếm giảm.
     */
    float current_rpm;

} Motor_HandleTypeDef;


/* =============================================================
 * CÁC HÀM ĐIỀU KHIỂN MOTOR
 * ============================================================= */

/*
 * Khởi tạo đối tượng motor có encoder sử dụng TIM thông thường.
 *
 * Hàm này được dùng cho encoder nối với:
 * TIM1, TIM2, TIM3, TIM4 hoặc TIM8.
 *
 * Tham số:
 * motor                 : Địa chỉ đối tượng motor cần khởi tạo.
 * hpwm                  : Timer tạo PWM.
 * ch_pwm                : Kênh PWM trong timer hpwm.
 * p_dir                 : Cổng GPIO của chân DIR.
 * pin_dir               : Số chân GPIO của chân DIR.
 * henc                  : Timer đọc encoder.
 * counts_per_revolution : Số count encoder trong một vòng.
 * direction_inverted    : Cờ đảo logic chân DIR, nhận 0 hoặc 1.
 *
 * Lưu ý:
 * Hàm này không tự start PWM hoặc encoder.
 * PWM và encoder phải được start từ main.c.
 */
void Motor_Init_TIM(Motor_HandleTypeDef *motor,
                    TIM_HandleTypeDef *hpwm,
                    uint32_t ch_pwm,
                    GPIO_TypeDef *p_dir,
                    uint16_t pin_dir,
                    TIM_HandleTypeDef *henc,
                    float counts_per_revolution,
                    uint8_t direction_inverted);

/*
 * Khởi tạo đối tượng motor có encoder sử dụng LPTIM.
 *
 * Trong dự án hiện tại, hàm này được dùng cho encoder nối với
 * LPTIM1.
 *
 * Tham số:
 * motor                 : Địa chỉ đối tượng motor cần khởi tạo.
 * hpwm                  : Timer tạo PWM.
 * ch_pwm                : Kênh PWM trong timer hpwm.
 * p_dir                 : Cổng GPIO của chân DIR.
 * pin_dir               : Số chân GPIO của chân DIR.
 * henc                  : LPTIM đọc encoder.
 * counts_per_revolution : Số count encoder trong một vòng.
 * direction_inverted    : Cờ đảo logic chân DIR, nhận 0 hoặc 1.
 *
 * Lưu ý:
 * Hàm này không tự start PWM hoặc encoder.
 * PWM và LPTIM phải được start từ main.c.
 */
void Motor_Init_LPTIM(Motor_HandleTypeDef *motor,
                      TIM_HandleTypeDef *hpwm,
                      uint32_t ch_pwm,
                      GPIO_TypeDef *p_dir,
                      uint16_t pin_dir,
                      LPTIM_HandleTypeDef *henc,
                      float counts_per_revolution,
                      uint8_t direction_inverted);

/*
 * Đọc counter encoder và cập nhật tốc độ motor.
 *
 * Hàm thực hiện:
 * - Đọc counter từ TIM hoặc LPTIM.
 * - Tính delta_count.
 * - Xử lý tràn counter 16-bit.
 * - Tính current_rpm.
 *
 * RPM được cập nhật khi đã qua ít nhất 50 ms kể từ lần cập nhật
 * trước. Vì vậy hàm này có thể được gọi liên tục trong vòng lặp.
 */
void Motor_Update_RPM(Motor_HandleTypeDef *motor);

/*
 * Điều khiển chiều quay và độ rộng xung PWM của motor.
 *
 * speed > 0:
 * - Motor quay theo chiều dương.
 *
 * speed < 0:
 * - Motor quay theo chiều âm.
 *
 * speed == 0:
 * - PWM được đưa về 0, motor dừng nhận lệnh chạy.
 *
 * Trị tuyệt đối của speed là giá trị compare PWM.
 * Nếu vượt quá ARR của timer, motor.c sẽ giới hạn về ARR.
 */
void Motor_SetSpeed_PWM(Motor_HandleTypeDef *motor, int16_t speed);

/*
 * Dừng motor bằng cách đặt giá trị compare PWM về 0.
 *
 * Hàm này không thay đổi trạng thái chân DIR và không dừng timer.
 */
void Motor_Stop(Motor_HandleTypeDef *motor);

#endif /* MOTOR_H */
