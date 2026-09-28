#ifndef VC02_H
#define VC02_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/*
 * VC-02 protocol hiện tại:
 *
 * A1       Wake-up
 * A2       Go forward
 * A3       Go backward
 * A4       Rotate in place
 * A5       Turn left
 * A6       Turn right
 *
 * B0..B9   Steering angle 0..45 degree, step 5 degree
 *
 * C1..C5   Speed mode 1..5
 *          C3 = Medium/default
 *
 * D0 xx    Duration in seconds
 *          xx = 05, 0A, 0F ... 78
 *          tương ứng 5 ... 120 giây
 *
 * F0       Clear command
 * FF       Stop
 *
 * D0 xx là HAI BYTE UART thực.
 */

typedef enum
{
    VC02_EVENT_NONE = 0,

    VC02_EVENT_WAKEUP,
    VC02_EVENT_FORWARD,
    VC02_EVENT_BACKWARD,
    VC02_EVENT_ROTATE_IN_PLACE,

    VC02_EVENT_TURN_LEFT,
    VC02_EVENT_TURN_RIGHT,

    VC02_EVENT_ANGLE,
    VC02_EVENT_SPEED,
    VC02_EVENT_DURATION,

    VC02_EVENT_CLEAR,
    VC02_EVENT_STOP

} VC02_EventType_t;


typedef enum
{
    VC02_SPEED_VERY_SLOW = 1U,
    VC02_SPEED_SLOW      = 2U,
    VC02_SPEED_MEDIUM    = 3U,
    VC02_SPEED_FAST      = 4U,
    VC02_SPEED_FULL      = 5U

} VC02_SpeedLevel_t;


typedef struct
{
    VC02_EventType_t type;

    /*
     * ANGLE    : 0, 5, ... 45
     * SPEED    : 1 ... 5
     * DURATION : 5, 10, ... 120
     * Event khác: 0
     */
    uint8_t value;

    /* Byte UART gốc, dùng để debug. */
    uint8_t raw0;
    uint8_t raw1;

} VC02_Event_t;


/* Bắt đầu nhận UART bằng interrupt. */
HAL_StatusTypeDef VC02_Begin(UART_HandleTypeDef *huart);


/* Gọi từ HAL_UART_RxCpltCallback() trong main.c. */
void VC02_UART_RxCpltCallback(UART_HandleTypeDef *huart);


/* Gọi từ HAL_UART_ErrorCallback() trong main.c. */
void VC02_UART_ErrorCallback(UART_HandleTypeDef *huart);


/*
 * Lấy event tiếp theo.
 *
 * Return:
 * 1U = có event
 * 0U = không có event
 */
uint8_t VC02_GetEvent(VC02_Event_t *event);


/* Số event bị mất nếu FIFO đầy, phục vụ debug. */
uint32_t VC02_GetDroppedEventCount(void);


#ifdef __cplusplus
}
#endif

#endif /* VC02_H */
