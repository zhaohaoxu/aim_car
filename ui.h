/*
 * ui.h - OLED 业务显示（航向 / PID / 运行状态 / 峰值）
 *
 * 底层绘图接口在 oled.h，本模块只负责把一帧内容组织好再刷屏。
 * 四行固定布局：
 *   page 0  YAW: 当前航向角（度）
 *   page 2  PID: 直行 PID 输出
 *   page 4  状态 READY / RUN
 *   page 6  PEAK: 本次动作中最大偏离角
 */

#ifndef UI_H
#define UI_H

#include <stdint.h>

/* 整屏刷新一次（内部已调用 oled_update，约 30ms） */
void ui_show(uint8_t running);

#endif /* UI_H */
