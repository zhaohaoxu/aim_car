#ifndef KEY_H
#define KEY_H

#include <stdint.h>

/* 按键编号，对应 SysConfig 里的 KEY1~KEY4 */
typedef enum {
    KEY_1 = 0,      /* PB10 */
    KEY_2,          /* PA7  */
    KEY_3,          /* PB11 */
    KEY_4,          /* PB14 */
    KEY_NUM
} key_id_t;

/* 每 10ms 调用一次，返回本次新按下的键，没有新按键返回 KEY_NUM */
key_id_t key_scanf(void);

/* 消抖后的当前状态：1 按下，0 松开，适合“按住才动”的用法 */
uint8_t key_down(key_id_t id);

#endif