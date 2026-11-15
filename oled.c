#include "oled.h"
#include "ti_msp_dl_config.h"

#define OLED_CTRL_CMD       (0x00)      /* 命令 */
#define OLED_CTRL_DATA      (0x40)      /* 数据 */
#define OLED_I2C_TIMEOUT    (200000)

/* SSD1306 要求VDD稳定后至少等100ms才能接收命令*/
#define OLED_POWER_ON_CYCLES    (3200000)

static uint8_t oled_buf[OLED_PAGES][OLED_WIDTH];

//复位控制器并清空 FIFO
static void oled_i2c_abort(void)
{
    DL_I2C_resetControllerTransfer(I2C_OLED_INST);
    DL_I2C_flushControllerTXFIFO(I2C_OLED_INST);
}

//发送OLED数据
static uint8_t oled_i2c_send(uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    uint16_t sent    = 0;
    uint32_t timeout = OLED_I2C_TIMEOUT;

    DL_I2C_fillControllerTXFIFO(I2C_OLED_INST, &ctrl, 1);
    DL_I2C_startControllerTransfer(I2C_OLED_INST, OLED_I2C_ADDR,
                                   DL_I2C_CONTROLLER_DIRECTION_TX,
                                   (uint16_t)(len + 1));

    /* 从机不应答时传输会被中止，FIFO 不再被取走，必须带超时 */
    while (sent < len) {
        sent += DL_I2C_fillControllerTXFIFO(I2C_OLED_INST, &data[sent],
                                            (uint16_t)(len - sent));
        if (--timeout == 0) {
            oled_i2c_abort();
            return 0;
        }
    }

    /* 等待传输完成 */
    while (DL_I2C_getControllerStatus(I2C_OLED_INST) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        if (--timeout == 0) {
            oled_i2c_abort();
            return 0;
        }
    }

    /* 总线正常但被 NACK，同样算失败 */
    if (DL_I2C_getControllerStatus(I2C_OLED_INST) &
        DL_I2C_CONTROLLER_STATUS_ERROR) {
        oled_i2c_abort();
        return 0;
    }

    return 1;
}
//发送OLED命令
static void oled_cmd(uint8_t cmd)
{
    oled_i2c_send(OLED_CTRL_CMD, &cmd, 1);
}

static const uint8_t oled_init[] = {
    0xAE,               /* 关显示 */
    0xD5, 0x80,         /* 时钟分频 */
    0xA8, 0x3F,         /* 复用率 1/64 */
    0xD3, 0x00,         /* 显示偏移 0 */
    0x40,               /* 起始行 0 */
    0x8D, 0x14,         /* 电荷泵开 */
    0x20, 0x00,         /* 水平寻址模式 */
    0xA1,               /* 段重映射 */
    0xC8,               /* COM 反向扫描 */
    0xDA, 0x12,         /* COM 引脚配置 */
    0x81, 0xCF,         /* 对比度 */
    0xD9, 0xF1,         /* 预充电周期 */
    0xDB, 0x40,         /* VCOMH */
    0xA4,               /* 输出跟随显存 */
    0xA6,               /* 正常显示 */
    0x2E                /* 关滚动 */
};
//初始化OLED
void OLED_init(void)
{
    delay_cycles(OLED_POWER_ON_CYCLES);
    oled_i2c_send(OLED_CTRL_CMD, oled_init, sizeof(oled_init));
    OLED_clear();
    OLED_refresh();
    oled_cmd(0xAF);     /* 先清屏再开显示，避免上电闪雪花 */
}
//清除屏幕
void OLED_clear(void)
{
    OLED_fill(0x00);
}
//填充屏幕
void OLED_fill(uint8_t pattern)//pattern:一列8像素
{
    uint8_t page, x;

    for (page = 0; page < OLED_PAGES; page++) {
        for (x = 0; x < OLED_WIDTH; x++) {
            oled_buf[page][x] = pattern;
        }
    }
}
//屏幕刷新
void OLED_refresh(void)
{
    uint8_t page;

    for (page = 0; page < OLED_PAGES; page++) {
        oled_cmd((uint8_t)(0xB0 | page));   /* 选页 */
        oled_cmd(0x00);                     /* 列地址低 4 位 */
        oled_cmd(0x10);                     /* 列地址高 4 位 */
        oled_i2c_send(OLED_CTRL_DATA, oled_buf[page], OLED_WIDTH);
    }
}
