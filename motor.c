#include "motor.h"
#include "ti_msp_dl_config.h"

/* 上电初始化：先把四个轮子置为停止 */
void motor_init(void)
{
    motor_stop_all();
}

/* 每个轮子的死区 duty*/
static const uint16_t dead_duty[MOTOR_NUM] = { 85, 85, 85, 85 };

/* speed: -1000 ~ +1000，正数前进，0 停止 */
void motor_set(motor_id_t id, int16_t speed)
{
    uint16_t mag;//速度绝对值
    uint16_t dead;//死区duty
    uint32_t duty;//占空比
    uint8_t  fwd;//前进方向

    /* 限幅 */
    if (speed >  MOTOR_UNIT_MAX) { speed =  MOTOR_UNIT_MAX; }
    if (speed < -MOTOR_UNIT_MAX) { speed = -MOTOR_UNIT_MAX; }

    /* 正数前进，负数后退 */
    fwd = (speed > 0) ? 1 : 0;

    /* 速度单位 → PWM 占空比 */
    if (speed == 0) {
        duty = 0;
    } else {
        mag  = (uint16_t)(fwd ? speed : -speed);
        dead = dead_duty[id];
        duty = dead + (uint32_t)(MOTOR_DUTY_MAX - dead) * mag / MOTOR_UNIT_MAX;
    }

    switch (id) {
    case MOTOR_RF:   /* 右前：AIN1/AIN2 */
        if (speed == 0) {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_AIN1_PIN | GPIO_DIRA_AIN2_PIN);
        } else if (fwd) {
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_AIN1_PIN);
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_AIN2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_AIN1_PIN);
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_AIN2_PIN);
        }
        DL_TimerA_setCaptureCompareValue(PWM_RIGHT_INST, duty, GPIO_PWM_RIGHT_C1_IDX);
        break;

    case MOTOR_LF:   /* 左前：DIN1/DIN2 */
        if (speed == 0) {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_DIN1_PIN | GPIO_DIRA_DIN2_PIN);
        } else if (fwd) {
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_DIN1_PIN);
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_DIN2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_DIN1_PIN);
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_DIN2_PIN);
        }
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, duty, GPIO_PWM_LEFT_C0_IDX);
        break;

    case MOTOR_RB:   /* 右后：BIN1/BIN2，装反，前进时给 BIN2 */
        if (speed == 0) {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_BIN1_PIN | GPIO_DIRA_BIN2_PIN);
        } else if (fwd) {
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_BIN1_PIN);
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_BIN2_PIN);
        } else {
            DL_GPIO_setPins  (GPIO_DIRA_PORT, GPIO_DIRA_BIN1_PIN);
            DL_GPIO_clearPins(GPIO_DIRA_PORT, GPIO_DIRA_BIN2_PIN);
        }
        DL_TimerA_setCaptureCompareValue(PWM_RIGHT_INST, duty, GPIO_PWM_RIGHT_C0_IDX);
        break;

    case MOTOR_LB:   /* 左后：CIN1/CIN2，装反，前进时给 CIN2 */
        if (speed == 0) {
            DL_GPIO_clearPins(GPIO_DIRB_PORT, GPIO_DIRB_CIN1_PIN | GPIO_DIRB_CIN2_PIN);
        } else if (fwd) {
            DL_GPIO_clearPins(GPIO_DIRB_PORT, GPIO_DIRB_CIN1_PIN);
            DL_GPIO_setPins  (GPIO_DIRB_PORT, GPIO_DIRB_CIN2_PIN);
        } else {
            DL_GPIO_setPins  (GPIO_DIRB_PORT, GPIO_DIRB_CIN1_PIN);
            DL_GPIO_clearPins(GPIO_DIRB_PORT, GPIO_DIRB_CIN2_PIN);
        }
        DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST, duty, GPIO_PWM_LEFT_C1_IDX);
        break;

    default:
        break;
    }
}

/* 停止所有轮子：滑行 */
void motor_stop_all(void)
{
    for (uint8_t i = 0; i < MOTOR_NUM; i++) {
        motor_set((motor_id_t)i, 0);
    }
}

/* 刹车：IN1 = IN2 = 1 时电机两端短接，转得越快制动力越大 */
void motor_brake_all(void)
{
    DL_GPIO_setPins(GPIO_DIRA_PORT,GPIO_DIRA_AIN1_PIN | GPIO_DIRA_AIN2_PIN);
    DL_GPIO_setPins(GPIO_DIRA_PORT,GPIO_DIRA_BIN1_PIN | GPIO_DIRA_BIN2_PIN);
    DL_GPIO_setPins(GPIO_DIRA_PORT,GPIO_DIRA_DIN1_PIN | GPIO_DIRA_DIN2_PIN);
    DL_GPIO_setPins(GPIO_DIRB_PORT,GPIO_DIRB_CIN1_PIN | GPIO_DIRB_CIN2_PIN);

    DL_TimerA_setCaptureCompareValue(PWM_RIGHT_INST, 0, GPIO_PWM_RIGHT_C1_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_RIGHT_INST, 0, GPIO_PWM_RIGHT_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST,  0, GPIO_PWM_LEFT_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_LEFT_INST,  0, GPIO_PWM_LEFT_C1_IDX);
}

/* 左侧两个轮子同速 */
void motor_left(int16_t speed)
{
    motor_set(MOTOR_LF, speed);
    motor_set(MOTOR_LB, speed);
}

/* 右侧两个轮子同速 */
void motor_right(int16_t speed)
{
    motor_set(MOTOR_RF, speed);
    motor_set(MOTOR_RB, speed);
}

/* 左右两侧差速：left 单侧速度，right 单侧速度 */
void motor_drive(int16_t left, int16_t right)
{
    motor_left(left);
    motor_right(right);
}
