#ifndef OLED_H
#define OLED_H

#include <stdint.h>

/* 屏幕分辨率与页数（一页 = 8 行像素） */
#define OLED_WIDTH      (128)
#define OLED_HEIGHT     (64)
#define OLED_PAGES      (8)

/* SSD1306 的 7 位从机地址，8 位写地址 0x78 右移一位得到 */
#define OLED_I2C_ADDR   (0x3C)

void OLED_init(void);
void OLED_clear(void);
void OLED_fill(uint8_t pattern);
void OLED_refresh(void);

/* 5x7 英文点阵，只写到显存，需要 OLED_refresh() 才会上屏 */
void OLED_show_char(uint8_t x, uint8_t page, char ch);
void OLED_show_string(uint8_t x, uint8_t page, const char *str);

#endif
