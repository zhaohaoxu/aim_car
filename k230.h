/*
 * k230.h - K230 视觉模块串口驱动（UART0）
 *
 * 接线（底板 CN5 / U1）：
 *   MSPM0 PA0 (TX) -> K230 IO_4 / GPIO4 (UART1_RXD)
 *   MSPM0 PA1 (RX) <- K230 IO_3 / GPIO3 (UART1_TXD)
 *   GND 必须共地
 *
 * 串口参数：115200，8 数据位，无校验，1 停止位
 *
 * 二进制帧格式：
 *   [0]   0xAA
 *   [1]   0x55
 *   [2]   LEN        负载长度，0 ~ K230_PAYLOAD_MAX
 *   [3..] PAYLOAD    负载数据
 *   [末]  SUM        从 LEN 到 PAYLOAD 最后一个字节的累加和（取低 8 位）
 *
 * 负载（LEN = 7）：
 *   [0]    found     1 = 找到目标，0 = 未找到
 *   [1..2] cx        目标中心 x，小端
 *   [3..4] cy        目标中心 y，小端
 *   [5..6] area      色块面积，小端
 */

#ifndef K230_H
#define K230_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

#define K230_FRAME_HEAD0    (0xAA)
#define K230_FRAME_HEAD1    (0x55)
#define K230_PAYLOAD_MAX    (32)

/* 初始化接收缓冲并打开串口接收中断，在 SYSCFG_DL_init() 之后调用一次 */
void k230_init(void);

/* 取一个原始字节，无数据返回 -1（调试用） */
int k230_get_byte(void);

/* 尝试解析一帧：取到完整且校验通过的帧返回 1，否则返回 0
 * payload 至少 K230_PAYLOAD_MAX 字节，返回时 *len 为实际负载长度 */
uint8_t k230_read_frame(uint8_t *payload, uint8_t *len);

/* 向 K230 发送数据 */
void k230_send_char(char c);
void k230_send_string(const char *s);

#endif /* K230_H */
