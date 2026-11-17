#include "bmi088.h"
#include "ti_msp_dl_config.h"

#define IMU_SCLK_H()   DL_GPIO_setPins(GPIO_IMU_A_PORT, GPIO_IMU_A_SCLK_PIN)
#define IMU_SCLK_L()   DL_GPIO_clearPins(GPIO_IMU_A_PORT, GPIO_IMU_A_SCLK_PIN)
#define IMU_MOSI_H()   DL_GPIO_setPins(GPIO_IMU_B_PORT, GPIO_IMU_B_MOSI_PIN)
#define IMU_MOSI_L()   DL_GPIO_clearPins(GPIO_IMU_B_PORT, GPIO_IMU_B_MOSI_PIN)
#define IMU_CSB1_H()   DL_GPIO_setPins(GPIO_IMU_B_PORT, GPIO_IMU_B_CSB1_PIN)
#define IMU_CSB1_L()   DL_GPIO_clearPins(GPIO_IMU_B_PORT, GPIO_IMU_B_CSB1_PIN)
#define IMU_CSB2_H()   DL_GPIO_setPins(GPIO_IMU_A_PORT, GPIO_IMU_A_CSB2_PIN)
#define IMU_CSB2_L()   DL_GPIO_clearPins(GPIO_IMU_A_PORT, GPIO_IMU_A_CSB2_PIN)

#define IMU_MISO()     ((DL_GPIO_readPins(GPIO_IMU_B_PORT, GPIO_IMU_B_MISO_PIN) != 0) ? 1 : 0)

/* 8 个周期 @32MHz ≈ 250ns，SCLK 约 800kHz，远低于 BMI088 的 10MHz 上限 */
#define IMU_SPI_DLY    (8)

#define DELAY_1MS       (32000)

//SCLK: 主时钟，低电平有效
//tx: 发送的字节，rx: 接收的字节
static uint8_t imu_spi_xfer(uint8_t tx)
{
    uint8_t i;
    uint8_t rx = 0;

    for (i = 0; i < 8; i++) {
        IMU_SCLK_L();
        if (tx & 0x80) {
            IMU_MOSI_H();
        } else {
            IMU_MOSI_L();
        }
        tx <<= 1;
        delay_cycles(IMU_SPI_DLY);
        IMU_SCLK_H();
        delay_cycles(IMU_SPI_DLY);
        rx = (uint8_t)((rx << 1) | IMU_MISO());
    }
    return rx;
}
/*0x80: 读取寄存器，最高位为 1 表示读取*/
/* 加速度计读寄存器 */
static uint8_t acc_read_reg(uint8_t reg)
{
    uint8_t val;
    //低电平有效
    IMU_CSB1_L();
    delay_cycles(IMU_SPI_DLY);
    imu_spi_xfer((uint8_t)(reg | 0x80));
    imu_spi_xfer(0x00);
    val = imu_spi_xfer(0x00);
    IMU_CSB1_H();

    return val;
}
/* 陀螺仪读寄存器 */
static uint8_t gyro_read_reg(uint8_t reg)
{
    uint8_t val;
    //低电平有效
    IMU_CSB2_L();
    delay_cycles(IMU_SPI_DLY);
    imu_spi_xfer((uint8_t)(reg | 0x80));
    val = imu_spi_xfer(0x00);
    IMU_CSB2_H();

    return val;
}
/* 读加速度计 ID，正常返回 0x1E */
uint8_t bmi088_read_acc_id(void)
{
    acc_read_reg(0x00);        
    return acc_read_reg(0x00);
}
/* 读陀螺仪 ID，正常返回 0x0F */
uint8_t bmi088_read_gyro_id(void)
{
    return gyro_read_reg(0x00);
}
/*0x7F: 寄存器地址，最高位为 0 表示写入*/
/* 陀螺仪写寄存器 */
static void gyro_write_reg(uint8_t reg, uint8_t val)
{
    IMU_CSB2_L();
    delay_cycles(IMU_SPI_DLY);
    imu_spi_xfer((uint8_t)(reg & 0x7F));
    imu_spi_xfer(val);
    IMU_CSB2_H();
    delay_cycles(IMU_SPI_DLY);
}

/* 连续读多个寄存器，陀螺仪地址会自动加一 */
static void gyro_read_burst(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    IMU_CSB2_L();
    delay_cycles(IMU_SPI_DLY);
    imu_spi_xfer((uint8_t)(reg | 0x80));
    for (i = 0; i < len; i++) {
        buf[i] = imu_spi_xfer(0x00);
    }
    IMU_CSB2_H();
}

void bmi088_init_gyro(void)
{
    gyro_write_reg(0x14, 0xB6);        /* 软复位，清掉上电残留的配置 */
    delay_cycles(DELAY_1MS * 30);      /* 手册要求复位后至少等 30ms */

    /* 0x0F 高 4 位 ODR/带宽 = 0x2 -> 400Hz / 116Hz
     *      低 3 位量程     = 0x2 -> ±500°/s */
    gyro_write_reg(0x0F, 0x22);
    delay_cycles(DELAY_1MS * 5);
}

void bmi088_read_gyro_raw(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t b[6];

    gyro_read_burst(0x02, b, 6);       /* 0x02~0x07 依次为 GX GY GZ，低字节在前 */

    *gx = (int16_t)(((uint16_t)b[1] << 8) | b[0]);
    *gy = (int16_t)(((uint16_t)b[3] << 8) | b[2]);
    *gz = (int16_t)(((uint16_t)b[5] << 8) | b[4]);
}

static int32_t gyro_bias[3];

void bmi088_calib_gyro(uint16_t samples)
{
    int32_t sx = 0, sy = 0, sz = 0;
    int16_t x, y, z;
    uint16_t i;

    if (samples == 0) {
        samples = 200;
    }

    for (i = 0; i < samples; i++) {
        bmi088_read_gyro_raw(&x, &y, &z);
        sx += x;
        sy += y;
        sz += z;
        delay_cycles(DELAY_1MS * 2);
    }

    gyro_bias[0] = sx / (int32_t)samples;
    gyro_bias[1] = sy / (int32_t)samples;
    gyro_bias[2] = sz / (int32_t)samples;
}

float bmi088_gyro_z_dps(void)
{
    int16_t x, y, z;

    bmi088_read_gyro_raw(&x, &y, &z);

    /* ±500°/s 时 1°/s = 32768 / 500 = 65.536 LSB */
    return (float)(z - gyro_bias[2]) / 65.536f;
}