# 01 点灯

> **本篇目标**：板载 LED4 每 1 秒闪烁一次。
> **前置**：完成 [00 环境搭建](00-environment.md)。
> **验收标准**：LED4 稳定闪烁，周期约 1 秒。

## 为什么先点灯

点灯是嵌入式界的 `hello world`。它能一次性验证三件事：**工程能编译、程序能烧进去、GPIO 能输出**。这三件事通了，后面所有外设才有意义。

## 硬件

| 项目 | 说明 |
| --- | --- |
| LED4 | PA29，高电平点亮 |
| 限流电阻 | 1 kΩ，LED 阴极接地 |

## 原理

```mermaid
flowchart LR
    CPU[CPU 写寄存器] --> GPIO[PA29 输出高/低]
    GPIO --> LED[LED 亮/灭]
    DELAY[delay_cycles 延时] -.循环.-> CPU
```

- `DL_GPIO_setPins()` 输出高电平 → LED 亮
- `DL_GPIO_clearPins()` 输出低电平 → LED 灭
- `DL_GPIO_togglePins()` 翻转电平 → 最省事，一句话切换

**延时怎么算**：MCLK = 32 MHz，`delay_cycles(16000000)` 正好 0.5 秒。两个 0.5 秒就是一个完整周期。

## 代码

新建 `led_blink.c`（或直接改 `empty_mspm0g3507.c` 的 `main`）：

```c
#include "ti_msp_dl_config.h"

#define DELAY_500MS (16000000)   /* 32 MHz × 0.5 s = 1600 万拍 */

int main(void)
{
    SYSCFG_DL_init();            /* 按 SysConfig 初始化所有外设 */

    while (1) {
        DL_GPIO_togglePins(GPIO_LED_STATUS_PORT, GPIO_LED_STATUS_LED4_PIN);
        delay_cycles(DELAY_500MS);
    }
}
```

## 编译烧录

1. 保存 `empty_mspm0g3507.syscfg`，确认 GPIO 里 LED4 配置为 **Output**
2. `Project → Build Project`
3. `Run → Debug`

## 验收

| 检查 | 通过标准 |
| --- | --- |
| LED4 | 亮 0.5 s、灭 0.5 s，周而复始 |
| 改延时 | 把 `DELAY_500MS` 改成 `3200000`，闪烁明显变快 |

## 常见问题

| 现象 | 原因 |
| --- | --- |
| 完全不亮 | 引脚宏写错；确认用的是 `GPIO_LED_STATUS_LED4_PIN` 而不是随手写的 `DL_GPIO_PIN_29` |
| 一直常亮 | `while(1)` 里少了 `delay_cycles`，翻转太快肉眼看不出来 |
| 编译报 `GPIO_LED_STATUS_PORT` 未定义 | SysConfig 没生成，重新保存 `.syscfg` 后重编 |
| 下载后没反应 | 烧录成功但程序没跑，点 `Resume`（F8） |

> 想点 LED1~LED3？需要在 SysConfig 的 GPIO 模块里新增引脚（PB23 / PB26 / PB27），保存后会自动生成 `GPIO_xxx_LEDx_PIN` 宏。

## 下一步

[02 电机](02-motor.md) —— 让四个轮子转起来。
