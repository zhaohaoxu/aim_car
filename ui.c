#include "ui.h"
#include "oled.h"
#include "ctrl.h"

/* 有符号整数转字符串，buf 至少 8 字节 */
static void i16_to_str(int16_t v, char *buf)
{
    char     tmp[6];
    int      n = 0;
    int      i = 0;
    uint16_t u;

    if (v < 0) { buf[i++] = '-'; u = (uint16_t)(-v); }
    else       {                 u = (uint16_t)v;   }

    if (u == 0) { buf[i++] = '0'; buf[i] = '\0'; return; }

    while (u > 0) { tmp[n++] = (char)('0' + (u % 10)); u /= 10; }
    while (n > 0) { buf[i++] = tmp[--n]; }
    buf[i] = '\0';
}

/* 画一行 "KEY:数值" */
static void draw_item(uint8_t page, const char *key, int16_t v)
{
    char line[16];
    char num[8];
    int  i = 0;
    int  j = 0;

    while (key[i] != '\0' && i < 8) { line[i] = key[i]; i++; }

    i16_to_str(v, num);
    while (num[j] != '\0') { line[i++] = num[j++]; }
    line[i] = '\0';

    oled_draw_string(0, page, line);
}

void ui_show(uint8_t running)
{
    oled_clear();

    draw_item(0, "YAW:",  (int16_t)ctrl_yaw());
    draw_item(2, "PID:",  (int16_t)ctrl_straight_out());

    oled_draw_string(0, 4, running ? "RUN" : "READY");

    draw_item(6, "PEAK:", (int16_t)ctrl_peak());

    oled_update();
}
