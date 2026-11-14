/*
 * bmi088.c - BMI088 六轴 IMU 驱动（软件 SPI / bit-bang）
 * 详见 bmi088.h 中的接线与时序说明。
 */

#include "bmi088.h"

/* Mode 3 空闲态：SCLK 高、两个片选都高、MOSI 低 */
static void imu_idle(void)
{
    CSB1_H();
    CSB2_H();
    MOSI_L();
    SCLK_H();
}

/* Mode 3 收发一个字节：MSB 先出，上升沿采样 */
static uint8_t spi_rw(uint8_t tx)
{
    uint8_t rx = 0;

    for (uint8_t i = 0; i < 8; i++) {
        /* SCLK 处于高相，此时改 MOSI（CPHA=1 要求数据在高相改变） */
        if (tx & 0x80) {
            MOSI_H();
        } else {
            MOSI_L();
        }
        tx <<= 1;
        delay_cycles(IMU_SPI_DLY);

        /* 下降沿：从机把 MISO 推出来 */
        SCLK_L();
        delay_cycles(IMU_SPI_DLY);

        /* 低相采样（此时数据已稳定），先左移再填最低位 => MSB 先入 */
        rx <<= 1;
        if (MISO_READ()) {
            rx |= 0x01;
        }
        delay_cycles(IMU_SPI_DLY);

        /* 上升沿：从机采样 MOSI，本 bit 结束 */
        SCLK_H();
        delay_cycles(IMU_SPI_DLY);
    }

    return rx;
}

/* 加速度计写寄存器（CSB1），bit7=0 表示写 */
static void acc_write(uint8_t reg, uint8_t val)
{
    CSB2_H();                 /* 两个片选任何时刻最多一个为低 */
    CSB1_L();
    spi_rw(reg & 0x7F);
    spi_rw(val);
    CSB1_H();
}

/* 加速度计读寄存器（CSB1），地址后必须插 1 字节 dummy */
static void acc_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSB2_H();
    CSB1_L();
    spi_rw(reg | 0x80);       /* bit7=1 表示读 */
    spi_rw(0x00);             /* 加速度计专属：1 字节 dummy */
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_rw(0x00);
    }
    CSB1_H();
}

/* 陀螺仪写寄存器（CSB2），bit7=0 表示写 */
static void gyro_write(uint8_t reg, uint8_t val)
{
    CSB1_H();
    CSB2_L();
    spi_rw(reg & 0x7F);
    spi_rw(val);
    CSB2_H();
}

/* 陀螺仪读寄存器（CSB2），地址后直接吐数据，无 dummy */
static void gyro_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSB1_H();
    CSB2_L();
    spi_rw(reg | 0x80);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_rw(0x00);
    }
    CSB2_H();
}

void bmi088_begin(void)
{
    imu_idle();
}

uint8_t bmi088_loopback(uint8_t tx)
{
    return spi_rw(tx);
}

uint8_t bmi088_read_acc_id(void)
{
    uint8_t id = 0;

    acc_read(0x00, &id, 1);   /* 第一次丢弃：靠 CSB1 上升沿完成 I2C -> SPI 切换 */
    acc_read(0x00, &id, 1);   /* 第二次才是真正的 CHIP_ID，期望 0x1E */

    return id;
}

uint8_t bmi088_read_gyro_id(void)
{
    uint8_t id = 0;

    gyro_read(0x00, &id, 1);  /* 第一次丢弃：靠 CSB2 上升沿完成 I2C -> SPI 切换 */
    gyro_read(0x00, &id, 1);  /* 第二次才是真正的 GYRO_CHIP_ID，期望 0x0F */

    return id;
}

/* ---------- 陀螺仪数据 ---------- */

#define GYRO_REG_RANGE      (0x0F)
#define GYRO_REG_BANDWIDTH  (0x10)
#define GYRO_REG_X_L        (0x02)

#define GYRO_LSB_PER_DPS    (65.536f)   /* ±500°/s 量程 */
#define YAW_SCALE           (1.08f)     /* 实测修正：手转一圈读数偏低约 8% */

static int16_t gz_bias = 0;

void bmi088_init_gyro(void)
{
    uint8_t tmp;

    gyro_read(0x00, &tmp, 1);              /* 丢弃：靠 CSB 上升沿切到 SPI 模式 */
    gyro_write(GYRO_REG_RANGE,     0x02);  /* ±500 °/s */
    gyro_write(GYRO_REG_BANDWIDTH, 0x02);  /* 1000Hz ODR / 116Hz 带宽 */
    delay_cycles(32000 * 10);              /* 等 10ms 让陀螺仪稳定 */
}

void bmi088_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t b[6];

    gyro_read(GYRO_REG_X_L, b, 6);         /* 连续 6 字节，低字节在前 */

    *gx = (int16_t)(((uint16_t)b[1] << 8) | b[0]);
    *gy = (int16_t)(((uint16_t)b[3] << 8) | b[2]);
    *gz = (int16_t)(((uint16_t)b[5] << 8) | b[4]);
}

void bmi088_calib_gyro(uint16_t samples)
{
    int32_t sum = 0;
    int16_t gx, gy, gz;

    if (samples == 0) { samples = 1; }

    for (uint16_t i = 0; i < samples; i++) {
        bmi088_read_gyro(&gx, &gy, &gz);
        sum += gz;
        delay_cycles(32000 * 2);           /* 2ms 采一次 */
    }

    gz_bias = (int16_t)(sum / (int32_t)samples);
}

uint8_t bmi088_read_gyro_reg(uint8_t reg)
{
    uint8_t v = 0;

    gyro_read(reg, &v, 1);

    return v;
}

float bmi088_gyro_z_dps(void)
{
    int16_t gx, gy, gz;

    bmi088_read_gyro(&gx, &gy, &gz);

    return ((float)(gz - gz_bias)) / GYRO_LSB_PER_DPS * YAW_SCALE;
}
