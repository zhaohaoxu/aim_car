#include "ctrl.h"
#include "bmi088.h"

static float yaw   = 0.0f;    /* 航向角，左转为正 */
static float peak  = 0.0f;    /* 本次动作中 |yaw| 的最大值 */
static float integ = 0.0f;    /* 积分累加 */
static float out   = 0.0f;    /* 最近一次 PID 输出 */

void ctrl_reset(void)
{
    yaw   = 0.0f;
    peak  = 0.0f;
    integ = 0.0f;
    out   = 0.0f;
}

void ctrl_update(float dt)
{
    float gz  = bmi088_gyro_z_dps();
    float err;

    /* 角速度积分成角度 */
    yaw += gz * dt;

    /* 记录最大偏离，取绝对值 */
    if (yaw < 0.0f) {
        if (-yaw > peak) { peak = -yaw; }
    } else {
        if (yaw > peak) { peak = yaw; }
    }

    /* 目标是保持出发时的朝向，即 yaw = 0 */
    err = 0.0f - yaw;

    integ += err * dt;
    if (integ >  CTRL_INT_MAX) { integ =  CTRL_INT_MAX; }
    if (integ < -CTRL_INT_MAX) { integ = -CTRL_INT_MAX; }

    out = CTRL_KP * err + CTRL_KI * integ + CTRL_KD * (-gz);

    if (out >  CTRL_OUT_MAX) { out =  CTRL_OUT_MAX; }
    if (out < -CTRL_OUT_MAX) { out = -CTRL_OUT_MAX; }
}

float ctrl_yaw(void)
{
    return yaw;
}

float ctrl_peak(void)
{
    return peak;
}

float ctrl_straight_out(void)
{
    return out;
}
