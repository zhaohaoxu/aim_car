#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/* 发送字节 */
void uart_bt_send_char(char c);

/* 发送字符串 */
void uart_bt_send_string(const char *s);

/* 读取一个字节 */
int uart_bt_get_char(void);

#endif /* UART_H */
