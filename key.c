#include "key.h"
#include "ti_msp_dl_config.h"

#define KEY_DEBOUNCE_CNT    (3)     /* 连续 3 次采样一致才认，主循环 10ms 即约 30ms */

/* 读原始电平：按键低电平有效，返回 1 表示按下 */
static uint8_t key_read(key_id_t id)
{
    uint32_t value;

    switch (id) {
    case KEY_1:
        value = DL_GPIO_readPins(GPIO_KEY_B_PORT, GPIO_KEY_B_KEY1_PIN);
        break;
    case KEY_2:
        value = DL_GPIO_readPins(GPIO_KEYA_PORT, GPIO_KEYA_KEY2_PIN);
        break;
    case KEY_3:
        value = DL_GPIO_readPins(GPIO_KEY_B_PORT, GPIO_KEY_B_KEY3_PIN);
        break;
    case KEY_4:
        value = DL_GPIO_readPins(GPIO_KEY_B_PORT, GPIO_KEY_B_KEY4_PIN);
        break;
    default:
        return 0;
    }
    return (value == 0) ? 1 : 0;
}

static uint8_t key_state[KEY_NUM];   /* 消抖后的稳定状态，1 按下 */
static uint8_t key_cnt[KEY_NUM];     /* 连续相同采样次数 */

/* 扫描按键，返回按下按键 ID*/
key_id_t key_scanf(void)
{
    key_id_t pressed = KEY_NUM;
    key_id_t i;
    /* 扫描所有按键 */
    for (i = KEY_1; i < KEY_NUM; i++) {
        uint8_t raw = key_read(i);//raw 1按下，raw 0松开

        if (raw == key_state[i]) {
            key_cnt[i] = 0;             /* 状态没变，计数清零 */
        } else {
            key_cnt[i]++;//状态改变，计数加一
            if (key_cnt[i] >= KEY_DEBOUNCE_CNT) {
                key_state[i] = raw;
                key_cnt[i] = 0;
                if (raw) {
                    pressed = i;        /* 只在按下的瞬间报告一次 */
                }
            }
        }
    }

    return pressed;//返回按下按键 ID，未按下返回 KEY_NUM
}

uint8_t key_down(key_id_t id)
{
    if (id >= KEY_NUM) {
        return 0;
    }
    return key_state[id];//返回按键当前状态，1 按下，0 松开
}