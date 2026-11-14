#include "trace.h"
#include "ti_msp_dl_config.h"

/* 黑线对应的电平 */
#define TRACE_ACTIVE_LEVEL   (1)

/* 通道切换时间2us */
#define TRACE_SETTLE_CYCLES  (64)

/* 8 路权重：CH0 在左(-7)，CH7 在右(+7)，步长 2 */
static const int16_t weight[TRACE_NUM] = { -7, -5, -3, -1, 1, 3, 5, 7 };

/* 通道选择 ch(0~7) */
static void trace_set_addr(uint8_t ch)
{
    if (ch & 0x01) {
        DL_GPIO_setPins(GPIO_TRACE_A_PORT, GPIO_TRACE_A_AD0_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_TRACE_A_PORT, GPIO_TRACE_A_AD0_PIN);
    }

    if (ch & 0x02) {
        DL_GPIO_setPins(GPIO_TRACE_B_PORT, GPIO_TRACE_B_AD1_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_TRACE_B_PORT, GPIO_TRACE_B_AD1_PIN);
    }

    if (ch & 0x04) {
        DL_GPIO_setPins(GPIO_TRACE_B_PORT, GPIO_TRACE_B_AD2_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_TRACE_B_PORT, GPIO_TRACE_B_AD2_PIN);
    }
}

/* 选中第 ch 路，返回是否为黑线：1=黑线，0=白底 */
static uint8_t trace_read_ch(uint8_t ch)
{
    uint32_t value;
    uint8_t raw;
    /* 选中通道 ch */
    trace_set_addr(ch);
    /* 等待信号建立 */
    delay_cycles(TRACE_SETTLE_CYCLES);
    /* 读取 OUT 电平 */
    value = DL_GPIO_readPins(GPIO_TRACE_B_PORT, GPIO_TRACE_B_OUT_PIN);

    raw = (value != 0) ? 1 : 0;
    /* 1=黑线，0=白底 */
    return (raw == TRACE_ACTIVE_LEVEL) ? 1 : 0;
}

// 初始化循迹传感器
void trace_init(void)
{
    trace_set_addr(0);
}

/* 读取8路状态，返回8位状态字 */
uint8_t trace_bits(void)
{
    uint8_t bits = 0;
    uint8_t ch;

    for (ch = 0; ch < TRACE_NUM; ch++) {
        if (trace_read_ch(ch)) {
            bits |= (uint8_t)(1u << ch);//1u=0b00000001
        }
    }

    return bits;
}

// 计算误差
int16_t trace_error(void)
{
    uint8_t  bits = trace_bits();
    int16_t  sum  = 0;
    int16_t  cnt  = 0;
    uint8_t  ch;

    for (ch = 0; ch < TRACE_NUM; ch++) {
        if (bits & (1u << ch)) {
            sum += weight[ch];
            cnt++;
        }
    }

    if (cnt == 0) {
        return TRACE_LOST;
    }

    return sum / cnt;
}