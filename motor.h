#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#define MOTOR_DUTY_MAX   (2133)   /* PWM 周期，对应 SysConfig 的 timerCount */
#define MOTOR_UNIT_MAX   (1000)   /* 上层速度单位上限：-1000 ~ +1000 */

typedef enum {
    MOTOR_RF = 0,   /* 右前 */
    MOTOR_LF,       /* 左前 */
    MOTOR_RB,       /* 右后 */
    MOTOR_LB,       /* 左后 */
    MOTOR_NUM
} motor_id_t;

void motor_init(void);
void motor_set(motor_id_t id, int16_t speed);
void motor_stop_all(void);          /* 四轮滑行停车（断电，靠阻力减速） */
void motor_brake_all(void);         /* 四轮刹车（电机两端短接，立即停） */

void motor_left (int16_t speed);
void motor_right(int16_t speed);
void motor_drive(int16_t left, int16_t right);   /* 左右两侧差速 */

#endif