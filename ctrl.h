/*
 * ctrl.h - 航向控制模块
 *
 * 职责：把陀螺仪的角速度积分成航向角，并用 PID 保持车头朝向。
 * 上层只需要：每 5ms 调 ctrl_update()，再把 ctrl_straight_out() 的返回值
 * 加到左右轮上，就能走直线。
 *
 * 符号约定：逆时针（车头往左）为正，顺时针为负。
 */

#ifndef CTRL_H
#define CTRL_H

#include <stdint.h>

/* ---------- PID 参数：调参就改这里 ---------- */
#define CTRL_KP         (6.0f)     /* 比例：偏 1 度给多少修正 */
#define CTRL_KI         (0.0f)     /* 积分：先给 0，有稳态偏差再加 */
#define CTRL_KD         (0.0f)     /* 微分：先给 0，超调再加 */
#define CTRL_OUT_MAX    (200.0f)   /* 修正量限幅 */
#define CTRL_INT_MAX    (300.0f)   /* 积分限幅，防饱和 */

/* 清零航向、积分、峰值。每次开始新动作前调用 */
void ctrl_reset(void);

/* 采样一次陀螺仪并推进 1 个控制周期，dt 单位秒 */
void ctrl_update(float dt);

/* 当前航向角（度），左转为正 */
float ctrl_yaw(void);

/* 本次动作中 |航向| 的最大值（度），用于评估直线度 */
float ctrl_peak(void);

/* 直行时的修正量：加号给左轮、减号给右轮 */
float ctrl_straight_out(void);

#endif /* CTRL_H */
