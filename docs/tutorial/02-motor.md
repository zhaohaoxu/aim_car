# 02 电机

> **本篇目标**：四个轮子正转 2 秒 → 停 1 秒 → 反转 2 秒，循环往复。
> **前置**：完成 [01 点灯](01-led.md)。
> **验收标准**：四轮同向转动，正反转切换正确，停车时完全静止。

> **务必架空车轮测试**，让轮子悬空，避免小车冲下桌子。

## 硬件

| 项目 | 说明 |
| --- | --- |
| 驱动 | TB6612 双路（本工程左右各并联 2 个电机） |
| 电机 | 520 直流减速电机 ×4 |
| 电源 | 12 V 电池，驱动与主控共地 |

方向与 PWM 引脚见 README 的「电机（TB6612）」表。

## 原理

一个直流电机要转，需要两样东西：

| 要素 | 作用 | 本工程 |
| --- | --- | --- |
| 方向 | 两个方向引脚的电平组合决定正转/反转 | AIN1/AIN2 等 8 根 GPIO |
| 速度 | PWM 占空比越大转得越快 | TIMA0 / TIMA1 两路 PWM |

**速度单位**：上层统一用 −1000 ~ +1000，`motor_set()` 内部换算成 PWM 占空比：

```
duty = 85 + (PWM_MAX - 85) × |speed| / 1000
```

那个 **85 是死区**：占空比太小时电机堵转不转，还会发热。所以从 85 起步。

## 代码

```c
#include "ti_msp_dl_config.h"
#include "motor.h"

int main(void)
{
    SYSCFG_DL_init();
    motor_init();                       /* 上电先全部停止 */

    while (1) {
        motor_drive(500, 500);          /* 左右都正转，速度 500 */
        delay_cycles(DELAY_1MS * 2000);

        motor_stop_all();               /* 滑行停车 */
        delay_cycles(DELAY_1MS * 1000);

        motor_drive(-500, -500);        /* 左右都反转 */
        delay_cycles(DELAY_1MS * 2000);

        motor_stop_all();
        delay_cycles(DELAY_1MS * 1000);
    }
}
```

`DELAY_1MS` 定义在 `empty_mspm0g3507.c`：

```c
#define DELAY_1MS (32000)   /* 32 MHz × 1 ms */
```

## 关键接口

| 函数 | 作用 |
| --- | --- |
| `motor_set(id, speed)` | 单个轮子，`id` 取 `MOTOR_LF/LB/RF/RB` |
| `motor_left(speed)` | 左侧两个轮子同速 |
| `motor_right(speed)` | 右侧两个轮子同速 |
| `motor_drive(left, right)` | 左右差速，最常用 |
| `motor_stop_all()` | 全部滑行停止 |
| `motor_brake_all()` | 全部短接刹车（转得越快制动越强） |

## 验收

| 检查 | 通过标准 |
| --- | --- |
| 正转 | 四轮同向，速度 500 时明显转动 |
| 停车 | `motor_stop_all()` 后轮子靠惯性慢慢停 |
| 反转 | 四轮反向 |
| 差速 | 改成 `motor_drive(500, 200)`，左快右慢，车会向右偏 |

## 常见问题

| 现象 | 原因 |
| --- | --- |
| 某个轮子转向反了 | 该电机的两根线接反，或在 `motor_set` 里交换方向引脚 |
| 轮子只嗡嗡响不转 | 速度太低，占空比没过死区，把速度提到 300 以上 |
| 四轮都不转 | 12 V 电池没接或没电；TB6612 的 STBY 引脚没拉高 |
| 一通电就猛转 | `motor_init()` 前没调 `SYSCFG_DL_init()`，PWM 没初始化 |

## 下一步

[03 循迹](03-trace.md) —— 让小车自己跟着黑线走。
