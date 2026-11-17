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

/* 5x7 点阵：每字 5 列，每列 1 字节，bit0 是最上一行，bit6 是最下一行。
 * 这个排布和 SSD1306 的显存一致，可以直接拷进 oled_buf */
static const uint8_t font_digit[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},     /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00},     /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46},     /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31},     /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10},     /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39},     /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30},     /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03},     /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36},     /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E},     /* 9 */
};

static const uint8_t font_upper[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E},     /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36},     /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22},     /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C},     /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41},     /* E */
    {0x7F, 0x09, 0x09, 0x09, 0x01},     /* F */
    {0x3E, 0x41, 0x49, 0x49, 0x7A},     /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F},     /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00},     /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01},     /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41},     /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40},     /* L */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},     /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F},     /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E},     /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06},     /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E},     /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46},     /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31},     /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01},     /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F},     /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F},     /* V */
    {0x3F, 0x40, 0x38, 0x40, 0x3F},     /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63},     /* X */
    {0x07, 0x08, 0x70, 0x08, 0x07},     /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43},     /* Z */
};

static const uint8_t font_space[5]   = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t font_minus[5]   = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t font_dot[5]     = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t font_colon[5]   = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t font_slash[5]   = {0x20, 0x10, 0x08, 0x04, 0x02};
static const uint8_t font_percent[5] = {0x23, 0x13, 0x08, 0x64, 0x62};

/* 按字符取点阵，不认识的字符按空格处理 */
static const uint8_t *oled_glyph(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return font_digit[ch - '0'];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return font_upper[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return font_upper[ch - 'a'];    /* 没有小写，统一显示成大写 */
    }

    switch (ch) {
    case '-': return font_minus;
    case '.': return font_dot;
    case ':': return font_colon;
    case '/': return font_slash;
    case '%': return font_percent;
    default:  return font_space;
    }
}

/* 在指定页的 x 列写一个字符，占 5 列，另留 1 列做字间距 */
void OLED_show_char(uint8_t x, uint8_t page, char ch)
{
    const uint8_t *glyph = oled_glyph(ch);
    uint8_t i;

    if (page >= OLED_PAGES) {
        return;
    }

    for (i = 0; i < 5; i++) {
        if ((uint16_t)x + i < OLED_WIDTH) {
            oled_buf[page][x + i] = glyph[i];
        }
    }

    if ((uint16_t)x + 5 < OLED_WIDTH) {
        oled_buf[page][x + 5] = 0x00;
    }
}

/* 从 x 列开始写字符串，超出屏幕右边界的部分直接截断 */
void OLED_show_string(uint8_t x, uint8_t page, const char *str)
{
    while (*str != '\0' && (uint16_t)x + 5 < OLED_WIDTH) {
        OLED_show_char(x, page, *str);
        x = (uint8_t)(x + 6);
        str++;
    }
}
