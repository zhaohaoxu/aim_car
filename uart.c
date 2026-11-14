#include "uart.h"

// 发送一个字符
void uart_bt_send_char(char c)
{
    DL_UART_Main_transmitDataBlocking(UART_BT_INST, (uint8_t)c);
}

// 发送一个字符串
void uart_bt_send_string(const char *s)
{
    while (*s != '\0') {
        uart_bt_send_char(*s++);
    }
}

// 接收一个字符
int uart_bt_get_char(void)
{
    uint8_t c;

    if (DL_UART_Main_receiveDataCheck(UART_BT_INST, &c)) {
        return (int)c;
    }
    return -1;
}
