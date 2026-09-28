#include "vc02.h"


#define VC02_EVENT_QUEUE_SIZE    16U


static UART_HandleTypeDef *s_vc02_uart = NULL;

static uint8_t s_rx_byte = 0U;


/*
 * FIFO lưu event đã decode.
 */
static VC02_Event_t s_event_queue[VC02_EVENT_QUEUE_SIZE];

static volatile uint8_t s_queue_head = 0U;
static volatile uint8_t s_queue_tail = 0U;

static volatile uint32_t s_dropped_events = 0U;


/*
 * = 1 khi vừa nhận byte D0
 * và đang chờ byte duration phía sau.
 */
static uint8_t s_wait_duration_byte = 0U;


/*
 * ============================================================
 * Kiểm tra duration hợp lệ.
 *
 * Chỉ cho phép:
 *
 * 5, 10, 15, ... 120 giây
 * ============================================================
 */
static uint8_t VC02_IsValidDuration(uint8_t seconds)
{
    if ((seconds < 5U) ||
        (seconds > 120U))
    {
        return 0U;
    }


    if ((seconds % 5U) != 0U)
    {
        return 0U;
    }


    return 1U;
}


/*
 * ============================================================
 * Đưa một event vào FIFO.
 * ============================================================
 */
static void VC02_QueuePush(const VC02_Event_t *event)
{
    uint8_t next_head;


    if (event == NULL)
    {
        return;
    }


    next_head =
        (uint8_t)((s_queue_head + 1U)
                  % VC02_EVENT_QUEUE_SIZE);


    /*
     * FIFO đầy.
     */
    if (next_head == s_queue_tail)
    {
        s_dropped_events++;

        return;
    }


    s_event_queue[s_queue_head] = *event;

    s_queue_head = next_head;
}


/*
 * ============================================================
 * Decode một byte UART.
 *
 * Return:
 *
 * 1 = đã tạo được event hoàn chỉnh
 * 0 = chưa có event / byte không hợp lệ
 * ============================================================
 */
static uint8_t VC02_DecodeByte(uint8_t data,
                              VC02_Event_t *event)
{
    if (event == NULL)
    {
        return 0U;
    }


    /*
     * Giá trị mặc định của event.
     */
    event->type  = VC02_EVENT_NONE;
    event->value = 0U;
    event->raw0  = 0U;
    event->raw1  = 0U;


    /*
     * ========================================================
     * Nếu trước đó đã nhận D0,
     * byte hiện tại phải là duration.
     * ========================================================
     */
    if (s_wait_duration_byte != 0U)
    {
        s_wait_duration_byte = 0U;


        if (VC02_IsValidDuration(data) != 0U)
        {
            event->type  = VC02_EVENT_DURATION;
            event->value = data;

            event->raw0  = 0xD0U;
            event->raw1  = data;

            return 1U;
        }


        /*
         * Nếu byte phía sau D0 không phải duration hợp lệ,
         * tiếp tục kiểm tra byte này như một command mới.
         *
         * Ví dụ:
         *
         * D0 A2
         *
         * D0 bị xem là lỗi,
         * nhưng A2 vẫn có thể được hiểu là Forward.
         */
    }


    /*
     * ========================================================
     * A1 ... A6
     * ========================================================
     */
    switch (data)
    {
        case 0xA1U:

            event->type = VC02_EVENT_WAKEUP;
            event->raw0 = data;

            return 1U;


        case 0xA2U:

            event->type = VC02_EVENT_FORWARD;
            event->raw0 = data;

            return 1U;


        case 0xA3U:

            event->type = VC02_EVENT_BACKWARD;
            event->raw0 = data;

            return 1U;


        case 0xA4U:

            event->type =
                VC02_EVENT_ROTATE_IN_PLACE;

            event->raw0 = data;

            return 1U;


        case 0xA5U:

            event->type = VC02_EVENT_TURN_LEFT;
            event->raw0 = data;

            return 1U;


        case 0xA6U:

            event->type = VC02_EVENT_TURN_RIGHT;
            event->raw0 = data;

            return 1U;


        default:

            break;
    }


    /*
     * ========================================================
     * B0 ... B9
     *
     * B0 =  0 degree
     * B1 =  5 degree
     * ...
     * B9 = 45 degree
     * ========================================================
     */
    if ((data >= 0xB0U) &&
        (data <= 0xB9U))
    {
        event->type = VC02_EVENT_ANGLE;

        event->value =
            (uint8_t)((data - 0xB0U) * 5U);

        event->raw0 = data;

        return 1U;
    }


    /*
     * ========================================================
     * C1 ... C5
     *
     * C1 = Very slow
     * C2 = Slow
     * C3 = Medium
     * C4 = Fast
     * C5 = Full
     * ========================================================
     */
    if ((data >= 0xC1U) &&
        (data <= 0xC5U))
    {
        event->type = VC02_EVENT_SPEED;

        event->value =
            (uint8_t)(data - 0xC0U);

        event->raw0 = data;

        return 1U;
    }


    /*
     * ========================================================
     * D0 xx
     *
     * D0 là prefix duration.
     * Chờ byte tiếp theo.
     * ========================================================
     */
    if (data == 0xD0U)
    {
        s_wait_duration_byte = 1U;

        return 0U;
    }


    /*
     * ========================================================
     * F0 = Clear command
     * ========================================================
     */
    if (data == 0xF0U)
    {
        event->type = VC02_EVENT_CLEAR;
        event->raw0 = data;

        return 1U;
    }


    /*
     * ========================================================
     * FF = Stop
     * ========================================================
     */
    if (data == 0xFFU)
    {
        event->type = VC02_EVENT_STOP;
        event->raw0 = data;

        return 1U;
    }


    /*
     * Byte không thuộc protocol.
     */
    return 0U;
}


/*
 * ============================================================
 * Khởi động VC-02 UART reception.
 * ============================================================
 */
HAL_StatusTypeDef VC02_Begin(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return HAL_ERROR;
    }


    s_vc02_uart = huart;

    s_rx_byte = 0U;


    s_queue_head = 0U;
    s_queue_tail = 0U;

    s_dropped_events = 0U;

    s_wait_duration_byte = 0U;


    return HAL_UART_Receive_IT(s_vc02_uart,
                               &s_rx_byte,
                               1U);
}


/*
 * ============================================================
 * Gọi từ HAL_UART_RxCpltCallback().
 * ============================================================
 */
void VC02_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    VC02_Event_t event;


    if ((s_vc02_uart == NULL) ||
        (huart != s_vc02_uart))
    {
        return;
    }


    /*
     * Decode byte vừa nhận.
     */
    if (VC02_DecodeByte(s_rx_byte,
                        &event) != 0U)
    {
        VC02_QueuePush(&event);
    }


    /*
     * Arm interrupt để nhận byte tiếp theo.
     */
    (void)HAL_UART_Receive_IT(s_vc02_uart,
                              &s_rx_byte,
                              1U);
}


/*
 * ============================================================
 * UART error callback.
 * ============================================================
 */
void VC02_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((s_vc02_uart == NULL) ||
        (huart != s_vc02_uart))
    {
        return;
    }


    if (__HAL_UART_GET_FLAG(huart,
                            UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
    }


    /*
     * Nếu đang đợi byte thứ hai của D0 xx
     * mà UART xảy ra lỗi thì hủy trạng thái đó.
     */
    s_wait_duration_byte = 0U;


    /*
     * Khởi động lại UART reception.
     */
    (void)HAL_UART_Receive_IT(s_vc02_uart,
                              &s_rx_byte,
                              1U);
}


/*
 * ============================================================
 * Lấy event tiếp theo từ FIFO.
 *
 * Return:
 *
 * 1 = có event
 * 0 = FIFO rỗng
 * ============================================================
 */
uint8_t VC02_GetEvent(VC02_Event_t *event)
{
    uint8_t has_event = 0U;

    uint32_t primask;


    if (event == NULL)
    {
        return 0U;
    }


    /*
     * FIFO được ghi trong UART interrupt,
     * vì vậy khóa interrupt trong thời gian rất ngắn
     * khi lấy event ra.
     */
    primask = __get_PRIMASK();

    __disable_irq();


    if (s_queue_tail != s_queue_head)
    {
        *event =
            s_event_queue[s_queue_tail];


        s_queue_tail =
            (uint8_t)((s_queue_tail + 1U)
                      % VC02_EVENT_QUEUE_SIZE);


        has_event = 1U;
    }


    if (primask == 0U)
    {
        __enable_irq();
    }


    return has_event;
}


/*
 * ============================================================
 * Số event bị mất do FIFO đầy.
 * Dùng để debug M1.
 * ============================================================
 */
uint32_t VC02_GetDroppedEventCount(void)
{
    return s_dropped_events;
}
