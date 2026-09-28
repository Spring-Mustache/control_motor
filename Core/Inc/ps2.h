#ifndef PS2_H_
#define PS2_H_

#include "stm32f7xx_hal.h"

// Cấu trúc lưu trữ trạng thái tay cầm
typedef struct {
    uint8_t Connected; // 1 = Đã kết nối, 0 = Mất sóng
    uint8_t Mode;      // 0x41: Digital, 0x73: Analog

    // Nút bấm (1 = Đang nhấn, 0 = Nhả)
    uint8_t Select, L3, R3, Start, Up, Right, Down, Left;
    uint8_t L2, R2, L1, R1, Triangle, Circle, Cross, Square;

    // Joystick 2 trục (Giá trị từ 0 - 255, Vị trí trung tâm là ~128)
    uint8_t RX, RY; // Cần gạt phải
    uint8_t LX, LY; // Cần gạt trái
} PS2_ControllerTypeDef;

// Khai báo biến toàn cục (extern) để main.c có thể truy cập
extern volatile PS2_ControllerTypeDef PS2_Pad;

// Nguyên mẫu hàm
void PS2_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
void PS2_Read_Data(void);

#endif /* PS2_H_ */
