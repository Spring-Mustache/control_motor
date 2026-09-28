#include "motor.h"

/*
 * Đọc giá trị counter hiện tại của encoder.
 *
 * Đây là hàm static:
 * - Chỉ được sử dụng bên trong motor.c.
 * - Không cần khai báo trong motor.h.
 *
 * Mỗi motor chỉ sử dụng một trong hai loại encoder:
 * - MOTOR_ENCODER_TIM:
 *   Đọc counter từ TIM1, TIM2, TIM3, TIM4 hoặc TIM8.
 *
 * - MOTOR_ENCODER_LPTIM:
 *   Đọc counter từ LPTIM1.
 *
 * Tất cả encoder trong project hiện được cấu hình với chu kỳ
 * đếm từ 0 đến 65535, nên kết quả được lưu dưới dạng uint16_t.
 *
 * Mặc dù TIM2 về phần cứng có thể hỗ trợ counter 32-bit,
 * project hiện đã cấu hình TIM2 với Period = 65535.
 */
static uint16_t Motor_Read_Encoder_Count(
        const Motor_HandleTypeDef *motor)
{
    /*
     * Nếu motor sử dụng LPTIM1, đọc counter bằng API riêng
     * của LPTIM.
     */
    if (motor->encoder_type == MOTOR_ENCODER_LPTIM) {
        return (uint16_t)HAL_LPTIM_ReadCounter(
                motor->hlptim_enc);
    }

    /*
     * Các motor còn lại sử dụng TIM thông thường.
     *
     * Macro __HAL_TIM_GET_COUNTER() đọc trực tiếp thanh ghi CNT
     * của timer encoder tương ứng.
     */
    return (uint16_t)__HAL_TIM_GET_COUNTER(
            motor->htim_enc);
}


/*
 * Khởi tạo các thành phần chung của một motor.
 *
 * Hàm này được gọi bởi:
 * - Motor_Init_TIM()
 * - Motor_Init_LPTIM()
 *
 * Trước khi gọi hàm này, một trong hai hàm phía trên phải:
 * - Gán encoder_type.
 * - Gán đúng htim_enc hoặc hlptim_enc.
 *
 * Tham số:
 * motor:
 *     Địa chỉ cấu trúc motor cần khởi tạo.
 *
 * hpwm:
 *     Địa chỉ timer tạo PWM cho motor.
 *
 * ch_pwm:
 *     Kênh PWM nằm trong timer hpwm.
 *
 * p_dir:
 *     Cổng GPIO chứa chân DIR.
 *
 * pin_dir:
 *     Chân GPIO điều khiển DIR.
 *
 * counts_per_revolution:
 *     Tổng số count encoder trong một vòng quay.
 *     Đây là count/vòng sau giải mã quadrature.
 *
 * direction_inverted:
 *     Cờ đảo logic chân DIR riêng của motor.
 */
static void Motor_Init_Common(
        Motor_HandleTypeDef *motor,
        TIM_HandleTypeDef *hpwm,
        uint32_t ch_pwm,
        GPIO_TypeDef *p_dir,
        uint16_t pin_dir,
        float counts_per_revolution,
        uint8_t direction_inverted)
{
    /*
     * Lưu cấu hình timer và kênh tạo PWM.
     */
    motor->htim_pwm = hpwm;
    motor->channel_pwm = ch_pwm;

    /*
     * Lưu cổng và chân GPIO điều khiển chiều quay.
     */
    motor->port_dir = p_dir;
    motor->pin_dir = pin_dir;

    /*
     * Lưu số count encoder trong một vòng quay.
     *
     * Giá trị 44 hoặc 64 sẽ được truyền vào từ main.c.
     * motor.c không hard-code thông số của từng motor.
     */
    motor->counts_per_revolution =
            counts_per_revolution;

    /*
     * Chuẩn hóa cờ đảo chiều về đúng 0 hoặc 1.
     *
     * Nếu truyền vào:
     * - 0: lưu 0.
     * - Bất kỳ giá trị khác 0: lưu 1.
     */
    motor->direction_inverted =
            (direction_inverted != 0U) ? 1U : 0U;

    /*
     * Lấy counter hiện tại làm mốc ban đầu.
     *
     * Không đặt cứng last_count = 0 vì encoder có thể đã phát
     * sinh count trước khi Motor_Init_*() được gọi. Nếu luôn đặt
     * bằng 0, lần tính RPM đầu tiên có thể xuất hiện giá trị giả.
     *
     * Trong main.c đề xuất, các encoder được start trước khi gọi
     * Motor_Init_TIM() hoặc Motor_Init_LPTIM().
     */
    motor->last_count =
            Motor_Read_Encoder_Count(motor);

    /*
     * Lưu thời điểm bắt đầu theo HAL tick.
     *
     * Đơn vị: millisecond.
     */
    motor->last_time = HAL_GetTick();

    /*
     * Chưa có chuyển động được đo tại thời điểm khởi tạo.
     */
    motor->delta_count = 0;
    motor->current_rpm = 0.0f;
}


/*
 * Khởi tạo motor sử dụng encoder TIM thông thường.
 *
 * Hàm này dùng cho encoder nối với:
 * - TIM1
 * - TIM2
 * - TIM3
 * - TIM4
 * - TIM8
 *
 * Hàm chỉ lưu cấu hình và đọc trạng thái encoder ban đầu.
 * Nó không tự gọi HAL_TIM_PWM_Start() hoặc
 * HAL_TIM_Encoder_Start().
 */
void Motor_Init_TIM(
        Motor_HandleTypeDef *motor,
        TIM_HandleTypeDef *hpwm,
        uint32_t ch_pwm,
        GPIO_TypeDef *p_dir,
        uint16_t pin_dir,
        TIM_HandleTypeDef *henc,
        float counts_per_revolution,
        uint8_t direction_inverted)
{
    /*
     * Đánh dấu motor này sử dụng encoder TIM thông thường.
     */
    motor->encoder_type = MOTOR_ENCODER_TIM;

    /*
     * Lưu timer encoder TIM.
     */
    motor->htim_enc = henc;

    /*
     * Motor này không sử dụng LPTIM.
     */
    motor->hlptim_enc = NULL;

    /*
     * Khởi tạo phần PWM, DIR, thông số encoder và trạng thái.
     */
    Motor_Init_Common(
            motor,
            hpwm,
            ch_pwm,
            p_dir,
            pin_dir,
            counts_per_revolution,
            direction_inverted);
}


/*
 * Khởi tạo motor sử dụng encoder LPTIM.
 *
 * Trong project hiện tại, hàm này được dùng cho encoder nối
 * với LPTIM1.
 *
 * Hàm chỉ lưu cấu hình và đọc trạng thái encoder ban đầu.
 * Nó không tự gọi HAL_TIM_PWM_Start() hoặc
 * HAL_LPTIM_Encoder_Start().
 */
void Motor_Init_LPTIM(
        Motor_HandleTypeDef *motor,
        TIM_HandleTypeDef *hpwm,
        uint32_t ch_pwm,
        GPIO_TypeDef *p_dir,
        uint16_t pin_dir,
        LPTIM_HandleTypeDef *henc,
        float counts_per_revolution,
        uint8_t direction_inverted)
{
    /*
     * Đánh dấu motor này sử dụng encoder LPTIM.
     */
    motor->encoder_type = MOTOR_ENCODER_LPTIM;

    /*
     * Motor này không sử dụng encoder TIM thông thường.
     */
    motor->htim_enc = NULL;

    /*
     * Lưu handle của LPTIM encoder.
     */
    motor->hlptim_enc = henc;

    /*
     * Khởi tạo phần PWM, DIR, thông số encoder và trạng thái.
     */
    Motor_Init_Common(
            motor,
            hpwm,
            ch_pwm,
            p_dir,
            pin_dir,
            counts_per_revolution,
            direction_inverted);
}


/*
 * Đọc encoder và cập nhật tốc độ motor.
 *
 * Hàm có thể được gọi liên tục trong vòng lặp while(1),
 * nhưng chỉ tính lại RPM khi đã qua ít nhất 50 ms.
 *
 * Các biến được cập nhật:
 * - delta_count
 * - current_rpm
 * - last_count
 * - last_time
 */
void Motor_Update_RPM(Motor_HandleTypeDef *motor)
{
    /*
     * Thời điểm hiện tại.
     *
     * Đơn vị: millisecond.
     */
    uint32_t current_time = HAL_GetTick();

    /*
     * Khoảng thời gian kể từ lần tính RPM trước.
     *
     * Phép trừ uint32_t này vẫn hoạt động đúng khi HAL tick
     * tràn từ 0xFFFFFFFF về 0, miễn khoảng thời gian giữa hai
     * lần cập nhật không vượt quá nửa miền uint32_t.
     */
    uint32_t dt = current_time - motor->last_time;

    /*
     * Chỉ cập nhật khi đã qua ít nhất 50 ms.
     *
     * Nếu chưa đủ 50 ms, hàm kết thúc và giữ nguyên kết quả RPM
     * của lần đo trước.
     */
    if (dt >= 50U) {
        /*
         * Giá trị counter encoder ở thời điểm hiện tại.
         */
        uint16_t current_count =
                Motor_Read_Encoder_Count(motor);

        /*
         * Hiệu counter theo modulo 65536.
         *
         * Việc ép về uint16_t giúp giữ đúng phần thấp 16-bit khi:
         * - Counter tràn từ 65535 về 0.
         * - Counter quay ngược từ 0 về 65535.
         */
        uint16_t wrapped_delta =
                (uint16_t)(current_count -
                           motor->last_count);

        /*
         * Chuyển hiệu modulo thành số count có dấu.
         *
         * 0x0000 đến 0x7FFF:
         * - Được hiểu là encoder đếm tăng.
         *
         * 0x8000 đến 0xFFFF:
         * - Được hiểu là encoder đếm giảm.
         *
         * Điều kiện sử dụng:
         * Encoder phải thay đổi ít hơn 32768 count giữa hai
         * lần cập nhật.
         */
        if (wrapped_delta <= 0x7FFFU) {
            motor->delta_count =
                    (int32_t)wrapped_delta;
        } else {
            motor->delta_count =
                    (int32_t)wrapped_delta - 65536;
        }

        /*
         * Chỉ tính RPM khi counts_per_revolution hợp lệ.
         *
         * Công thức:
         *
         * RPM = delta_count / count_per_revolution
         *       x 60000 / dt
         *
         * Trong đó:
         * - delta_count / count_per_revolution:
         *   Số vòng quay trong khoảng đo.
         *
         * - dt:
         *   Khoảng thời gian đo, đơn vị millisecond.
         *
         * - 60000:
         *   Số millisecond trong một phút.
         */
        if (motor->counts_per_revolution > 0.0f) {
            motor->current_rpm =
                    ((float)motor->delta_count * 60000.0f) /
                    (motor->counts_per_revolution *
                     (float)dt);
        } else {
            /*
             * Bảo vệ phép chia cho 0 hoặc thông số CPR âm.
             */
            motor->current_rpm = 0.0f;
        }

        /*
         * Lưu counter và thời điểm hiện tại để làm mốc cho lần
         * cập nhật tiếp theo.
         */
        motor->last_count = current_count;
        motor->last_time = current_time;
    }
}


/*
 * Điều khiển chiều quay và độ rộng xung PWM.
 *
 * Ý nghĩa speed:
 * - speed > 0: quay theo chiều dương.
 * - speed < 0: quay theo chiều âm.
 * - speed = 0: PWM bằng 0.
 *
 * Trị tuyệt đối của speed là giá trị compare PWM.
 *
 * Với cấu hình hiện tại:
 * - ARR của các timer PWM bằng 999.
 * - Miền PWM hợp lệ là 0 đến 999.
 */
void Motor_SetSpeed_PWM(
        Motor_HandleTypeDef *motor,
        int16_t speed)
{
    /*
     * Dùng int32_t để lấy trị tuyệt đối an toàn kể cả khi
     * speed bằng INT16_MIN (-32768).
     */
    int32_t magnitude = speed;

    /*
     * forward = 1:
     * Lệnh tốc độ dương.
     *
     * forward = 0:
     * Lệnh tốc độ âm.
     */
    uint8_t forward = 1U;

    /*
     * Đọc giá trị ARR thực tế của timer PWM.
     *
     * Giá trị này là compare tối đa được phép ghi.
     */
    uint32_t pwm_max =
            __HAL_TIM_GET_AUTORELOAD(motor->htim_pwm);

    /*
     * Mức GPIO cuối cùng sẽ được xuất ra chân DIR.
     */
    GPIO_PinState dir_level;

    /*
     * Nếu speed âm:
     * - Đánh dấu chiều quay âm.
     * - Chuyển magnitude thành trị dương.
     */
    if (magnitude < 0) {
        forward = 0U;
        magnitude = -magnitude;
    }

    /*
     * Giới hạn magnitude không vượt quá ARR.
     *
     * Ví dụ ARR = 999:
     * - speed = 800  -> PWM = 800.
     * - speed = 1200 -> PWM bị giới hạn còn 999.
     */
    if ((uint32_t)magnitude > pwm_max) {
        magnitude = (int32_t)pwm_max;
    }

    /*
     * Nếu lệnh bằng 0, đưa compare PWM về 0.
     *
     * Không thay đổi chân DIR khi dừng để tránh chân DIR đổi
     * trạng thái không cần thiết.
     */
    if (magnitude == 0) {
        __HAL_TIM_SET_COMPARE(
                motor->htim_pwm,
                motor->channel_pwm,
                0U);
        return;
    }

    /*
     * Tính mức logic của chân DIR.
     *
     * direction_inverted = 0:
     * - forward = 1 -> GPIO_PIN_SET.
     * - forward = 0 -> GPIO_PIN_RESET.
     *
     * direction_inverted = 1:
     * - Kết quả trên được đảo lại.
     *
     * Toán tử XOR chỉ hoạt động đúng theo bảng này vì hai biến
     * forward và direction_inverted luôn được chuẩn hóa về 0/1.
     */
    dir_level =
            ((forward ^ motor->direction_inverted) != 0U) ?
            GPIO_PIN_SET :
            GPIO_PIN_RESET;

    /*
     * Xuất chiều quay trước, sau đó mới xuất PWM.
     */
    HAL_GPIO_WritePin(
            motor->port_dir,
            motor->pin_dir,
            dir_level);

    /*
     * Ghi trị tuyệt đối của speed vào compare PWM.
     */
    __HAL_TIM_SET_COMPARE(
            motor->htim_pwm,
            motor->channel_pwm,
            (uint32_t)magnitude);
}


/*
 * Dừng cấp PWM cho motor.
 *
 * Hàm chỉ đặt compare PWM về 0:
 * - Không dừng timer PWM.
 * - Không thay đổi chân DIR.
 * - Không reset counter encoder.
 *
 * Trạng thái cơ khí là phanh hay thả trôi còn phụ thuộc vào
 * driver motor và cách đấu chân điều khiển; motor.c không thể
 * tự quyết định điều đó.
 */
void Motor_Stop(Motor_HandleTypeDef *motor)
{
    __HAL_TIM_SET_COMPARE(
            motor->htim_pwm,
            motor->channel_pwm,
            0U);
}
