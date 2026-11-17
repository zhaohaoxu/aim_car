#ifndef BMI088_H
#define BMI088_H

#include <stdint.h>

/* 读加速度计 ID，正常返回 0x1E */
uint8_t bmi088_read_acc_id(void);

/* 读陀螺仪 ID，正常返回 0x0F */
uint8_t bmi088_read_gyro_id(void);

/* 陀螺仪初始化：软复位 + 400Hz ODR / 116Hz 带宽 / ±500°/s，耗时约 35ms */
void bmi088_init_gyro(void);

/* 读三轴角速度原始值（LSB），未扣零偏 */
void bmi088_read_gyro_raw(int16_t *gx, int16_t *gy, int16_t *gz);

/* 静止零偏标定，车必须完全静止时调用，建议 200 次 */
void bmi088_calib_gyro(uint16_t samples);

/* z 轴角速度（°/s），已扣零偏，逆时针为正 */
float bmi088_gyro_z_dps(void);

#endif /* BMI088_H */