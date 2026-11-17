#include "k230.h"

#define K230_RX_SIZE    (256)       /* 必须是 2 的幂，便于取模 */

static volatile uint8_t  rx_buf[K230_RX_SIZE];
static volatile uint16_t rx_head;   /* 中断写入位置 */
static volatile uint16_t rx_tail;   /* 主循环读取位置 */

void k230_init(void)
{
    rx_head = 0;
    rx_tail = 0;

    NVIC_ClearPendingIRQ(UART_VISION_INST_INT_IRQN);// 清除中断
    NVIC_EnableIRQ(UART_VISION_INST_INT_IRQN);// 使能中断
}

/* 中断函数 */
void UART_VISION_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_VISION_INST)) {
    case DL_UART_MAIN_IIDX_RX:
        while (!DL_UART_Main_isRXFIFOEmpty(UART_VISION_INST)) {
            uint8_t  c    = DL_UART_Main_receiveData(UART_VISION_INST);
            uint16_t next = (uint16_t)((rx_head + 1) % K230_RX_SIZE);

            if (next != rx_tail) {      /* 缓冲满则丢弃新数据 */
                rx_buf[rx_head] = c;
                rx_head         = next;
            }
        }
        break;

    default:
        break;
    }
}

static int ring_pop(uint8_t *c)
{
    if (rx_tail == rx_head) {
        return 0;
    }
    *c      = rx_buf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1) % K230_RX_SIZE);
    return 1;
}

int k230_get_byte(void)
{
    uint8_t c;

    if (ring_pop(&c)) {
        return (int)c;
    }
    return -1;
}

uint8_t k230_read_frame(uint8_t *payload, uint8_t *len)
{
    static uint8_t state;       /* 0=找帧头1 1=找帧头2 2=读长度 3=读负载 */
    static uint8_t idx;         /* 已收到负载字节数 */
    static uint8_t plen;        /* 本帧负载长度 */
    static uint8_t sum;         /* 累加和 */
    uint8_t        c;

    while (ring_pop(&c)) {
        switch (state) {
        case 0:
            if (c == K230_FRAME_HEAD0) { state = 1; }
            break;

        case 1:
            if (c == K230_FRAME_HEAD1) {
                state = 2;
            } else {
                state = (c == K230_FRAME_HEAD0) ? 1 : 0;
            }
            break;

        case 2:
            if (c > K230_PAYLOAD_MAX) {   /* 长度非法，重新找帧头 */
                state = 0;
            } else {
                plen  = c;
                idx   = 0;
                sum   = c;
                state = 3;
            }
            break;

        case 3:
            if (idx < plen) {
                payload[idx++] = c;
                sum           += c;
            } else {
                state = 0;
                if (c == sum) {
                    *len = plen;
                    return 1;
                }
            }
            break;

        default:
            state = 0;
            break;
        }
    }

    return 0;
}

void k230_send_char(char c)
{
    DL_UART_Main_transmitDataBlocking(UART_VISION_INST, (uint8_t)c);
}

void k230_send_string(const char *s)
{
    while (*s != '\0') {
        k230_send_char(*s++);
    }
}
