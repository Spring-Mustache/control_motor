#include "ps2.h"

/*
 * Thời gian tối đa chờ một lần truyền/nhận SPI 1 byte.
 * Dùng timeout hữu hạn để tránh treo vòng điều khiển nếu SPI gặp lỗi.
 */
#define PS2_SPI_TIMEOUT_MS 10U

volatile PS2_ControllerTypeDef PS2_Pad;

static SPI_HandleTypeDef *ps2_hspi;
static GPIO_TypeDef *ps2_cs_port;
static uint16_t ps2_cs_pin;

// Bộ đệm SPI nội bộ (Giấu kín không cho main.c thấy)
static uint8_t ps2_cmd[9]  = {0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t ps2_data[9] = {0};

// Hàm delay ép buộc trình biên dịch không được xóa (chống tối ưu hóa)
#pragma GCC push_options
#pragma GCC optimize ("O0")
static void PS2_Delay_Slow(uint16_t us) {
    for(volatile uint32_t i = 0; i < (us * 100); i++);
}
#pragma GCC pop_options

void PS2_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    ps2_hspi = hspi;
    ps2_cs_port = cs_port;
    ps2_cs_pin = cs_pin;

    // Kéo CS lên mức HIGH (Trạng thái nghỉ)
    HAL_GPIO_WritePin(ps2_cs_port, ps2_cs_pin, GPIO_PIN_SET);
}

void PS2_Read_Data(void) {
    // 1. Kéo CS xuống LOW để đánh thức bộ thu
    HAL_GPIO_WritePin(ps2_cs_port, ps2_cs_pin, GPIO_PIN_RESET);
    PS2_Delay_Slow(20);

    // 2. Gửi TỪNG BYTE một và ép độ trễ
    for(int i = 0; i < 9; i++) {
        HAL_StatusTypeDef spi_status;

        spi_status = HAL_SPI_TransmitReceive(
                ps2_hspi,
                &ps2_cmd[i],
                &ps2_data[i],
                1,
                PS2_SPI_TIMEOUT_MS);

        if(spi_status != HAL_OK) {
            // Luôn nhả CS trước khi thoát nếu SPI lỗi hoặc timeout
            HAL_GPIO_WritePin(ps2_cs_port, ps2_cs_pin, GPIO_PIN_SET);

            // Báo mất kết nối để main.c dừng motor theo fail-safe
            PS2_Pad.Connected = 0;
            PS2_Pad.Mode = 0;
            return;
        }

        PS2_Delay_Slow(15);
    }

    // 3. Kéo CS lên HIGH để kết thúc
    HAL_GPIO_WritePin(ps2_cs_port, ps2_cs_pin, GPIO_PIN_SET);

    // 4. Giải mã dữ liệu (Bit-shifting)
    if((ps2_data[1] == 0x41 || ps2_data[1] == 0x73) && ps2_data[2] == 0x5A) {
        PS2_Pad.Connected = 1;
        PS2_Pad.Mode = ps2_data[1];

        // Byte 3: Nút chức năng & D-Pad (Đã tích hợp L3, R3)
        PS2_Pad.Select = (~ps2_data[3] >> 0) & 0x01;
        PS2_Pad.L3     = (~ps2_data[3] >> 1) & 0x01;
        PS2_Pad.R3     = (~ps2_data[3] >> 2) & 0x01;
        PS2_Pad.Start  = (~ps2_data[3] >> 3) & 0x01;
        PS2_Pad.Up     = (~ps2_data[3] >> 4) & 0x01;
        PS2_Pad.Right  = (~ps2_data[3] >> 5) & 0x01;
        PS2_Pad.Down   = (~ps2_data[3] >> 6) & 0x01;
        PS2_Pad.Left   = (~ps2_data[3] >> 7) & 0x01;

        // Byte 4: Nút hình học & Nút vai (Shoulder)
        PS2_Pad.L2       = (~ps2_data[4] >> 0) & 0x01;
        PS2_Pad.R2       = (~ps2_data[4] >> 1) & 0x01;
        PS2_Pad.L1       = (~ps2_data[4] >> 2) & 0x01;
        PS2_Pad.R1       = (~ps2_data[4] >> 3) & 0x01;
        PS2_Pad.Triangle = (~ps2_data[4] >> 4) & 0x01;
        PS2_Pad.Circle   = (~ps2_data[4] >> 5) & 0x01;
        PS2_Pad.Cross    = (~ps2_data[4] >> 6) & 0x01;
        PS2_Pad.Square   = (~ps2_data[4] >> 7) & 0x01;

        if(PS2_Pad.Mode == 0x73) {
            PS2_Pad.RX = ps2_data[5];
            PS2_Pad.RY = ps2_data[6];
            PS2_Pad.LX = ps2_data[7];
            PS2_Pad.LY = ps2_data[8];
        } else {
            PS2_Pad.RX = 128; PS2_Pad.RY = 128;
            PS2_Pad.LX = 128; PS2_Pad.LY = 128;
        }
    } else {
        PS2_Pad.Connected = 0;
        PS2_Pad.Mode = 0;
    }
}
