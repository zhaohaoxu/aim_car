/*
 * bmi088.h - BMI088 六轴 IMU 驱动（软件 SPI / bit-bang）
 *
 * 接线（立创 LCKFB-TMX-MSPM0G3507 底板 U12）：
 *   VCC  3.3V   （严禁 5V）
 *   GND  GND
 *   MISO PB18   （模块输出 -> MCU 输入）
 *   MOSI PB17   （MCU 输出 -> 模块输入）
 *   SCLK PA21   （PA21 无硬件 SPI 功能，只能软件模拟）
 *   CSB1 PB19   （加速度计片选，低有效）
 *   CSB2 PA23   （陀螺仪片选，低有效）
 *
 * 时序：SPI Mode 3（CPOL=1, CPHA=1）
 *   SCLK 空闲为高，数据在上升沿采样，下降沿由从机推出数据。
 *   加速度计只支持 Mode 3；陀螺仪 Mode 0/3 均可 —— 所以统一用 Mode 3。
 */

#ifndef BMI088_H
#define BMI088_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/* ---------- SPI 位延时 ----------
 * 8 cycle @32MHz = 250ns，每 bit 4 段延时约 1.25us -> SCLK 约 800kHz。
 * 若回环自检读回值不稳定，可先把它加大到 20 再排查。 */
#define IMU_SPI_DLY (8)

/* ---------- 引脚操作 ----------
 * 端口与引脚必须配对：GPIO_IMU_A_PORT 只配 SCLK / CSB2，
 * GPIO_IMU_B_PORT 只配 MOSI / MISO / CSB1。 */
#define SCLK_H()    DL_GPIO_setPins  (GPIO_IMU_A_PORT, GPIO_IMU_A_SCLK_PIN)
#define SCLK_L()    DL_GPIO_clearPins(GPIO_IMU_A_PORT, GPIO_IMU_A_SCLK_PIN)
#define MOSI_H()    DL_GPIO_setPins  (GPIO_IMU_B_PORT, GPIO_IMU_B_MOSI_PIN)
#define MOSI_L()    DL_GPIO_clearPins(GPIO_IMU_B_PORT, GPIO_IMU_B_MOSI_PIN)
#define MISO_READ() (DL_GPIO_readPins (GPIO_IMU_B_PORT, GPIO_IMU_B_MISO_PIN) != 0)
#define CSB1_H()    DL_GPIO_setPins  (GPIO_IMU_B_PORT, GPIO_IMU_B_CSB1_PIN)
#define CSB1_L()    DL_GPIO_clearPins(GPIO_IMU_B_PORT, GPIO_IMU_B_CSB1_PIN)
#define CSB2_H()    DL_GPIO_setPins  (GPIO_IMU_A_PORT, GPIO_IMU_A_CSB2_PIN)
#define CSB2_L()    DL_GPIO_clearPins(GPIO_IMU_A_PORT, GPIO_IMU_A_CSB2_PIN)

/* ---------- 对外接口 ---------- */

/* 把总线拉到 Mode 3 空闲态（SYSCFG_DL_init 之后调用一次） */
void bmi088_begin(void);

/* 回环自检：PB17 与 PB18 短接时，bmi088_loopback(0xA5) 应返回 0xA5 */
uint8_t bmi088_loopback(uint8_t tx);

/* 读加速度计的芯片 ID（寄存器 0x00），正常值 0x1E
 * 注意：加速度计上电默认是 I2C 模式，需要 CSB1 的一次上升沿才切到 SPI，
 * 本函数内部已自动丢弃第一次读取。 */
uint8_t bmi088_read_acc_id(void);

/* 读陀螺仪的芯片 ID（寄存器 0x00），正常值 0x0F */
uint8_t bmi088_read_gyro_id(void);

/* 陀螺仪初始化：量程 ±500°/s，带宽 116Hz */
void bmi088_init_gyro(void);

/* 读三轴原始值（LSB），±500°/s 量程下 65.536 LSB = 1°/s */
void bmi088_read_gyro(int16_t *gx, int16_t *gy, int16_t *gz);

/* 静止零偏标定：采样 samples 次求 z 轴平均，标定期间车必须完全静止 */
void bmi088_calib_gyro(uint16_t samples);

/* 读陀螺仪任意寄存器，用于回读配置是否写成功 */
uint8_t bmi088_read_gyro_reg(uint8_t reg);

/* 读 z 轴角速度，单位 °/s，已扣除零偏；逆时针为正 */
float bmi088_gyro_z_dps(void);

#endif /* BMI088_H */
