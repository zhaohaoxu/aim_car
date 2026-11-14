# MSPM0G3507 四轮智能小车

全国大学生电子设计竞赛 E 题（简易自主循迹小车）的嵌入式工程，基于 TI MSPM0G3507，使用 CCS Theia + SysConfig 开发。

## 硬件平台

| 项目 | 说明 |
| --- | --- |
| 主控 | MSPM0G3507（Cortex-M0+，LQFP-64，MCLK 32 MHz） |
| 底板 | 立创 LCKFB-TMX-MSPM0G3507 扩展板 |
| 循迹 | 8 路灰度传感器 + 74HC4051 八选一模拟开关，GPIO 扫描 |
| 电机 | 四路直流减速电机，双 H 桥驱动，TIMA0 / TIMA1 输出四路 PWM |
| 姿态 | BMI088 六轴 IMU，软件 SPI（bit-bang） |
| 显示 | 0.96 寸 OLED，硬件 I2C（PA10 / PA11） |
| 遥控 | 蓝牙串口模块，UART1（PB6 / PB7） |

## 功能

- **8 路循迹**：每 10 ms 扫描一轮，加权平均计算偏差，差速转向
- **丢线保护**：连续 0.5 s 检测不到黑线自动停车
- **蓝牙遥控**：`W` 前进、`S` 后退、`A` 左转、`D` 右转、`X` 停止
- **双模式互斥**：循迹与蓝牙共用一条 `if / else if / else` 控制链，同一时刻只有一个控制源写电机

## 按键操作

| 按键 | 引脚 | 功能 |
| --- | --- | --- |
| KEY1 | PB10 | 启动循迹 |
| KEY2 | PA7 | 进入蓝牙遥控 |
| KEY3 | PB11 | 停车 |
| KEY4 | PB14 | 预留 |

按键低电平有效，连续 3 次采样一致才确认，只有按下沿触发。

## 工程结构

| 文件 | 说明 |
| --- | --- |
| `empty_mspm0g3507.c` | 主程序：模式切换、循迹控制、丢线保护 |
| `motor.c` / `motor.h` | 电机驱动，速度单位 -1000 ~ +1000，含死区补偿 |
| `trace.c` / `trace.h` | 循迹传感器驱动，74HC4051 通道切换与加权平均 |
| `key.c` / `key.h` | 按键扫描与消抖 |
| `uart.c` / `uart.h` | 蓝牙串口收发 |
| `bmi088.c` / `bmi088.h` | BMI088 六轴 IMU 驱动（未接入主循环） |
| `ctrl.c` / `ctrl.h` | 航向角积分与直行 PID（未接入主循环） |
| `ui.c` / `ui.h` | OLED 业务显示（未接入主循环） |
| `oled.c` / `oled.h` | OLED 底层绘图（未实现） |
| `empty_mspm0g3507.syscfg` | SysConfig 外设配置，生成 `Debug/syscfg/ti_msp_dl_config.[ch]` |
| `vision/01颜色识别.py` | K230 / CanMV 视觉模块红色色块识别脚本（`find_blobs`） |
| `docs/images/` | 原理图、PCB、板卡渲染与算法参考图 |

## 引脚分配

| 外设 | 信号 | 引脚 |
| --- | --- | --- |
| 循迹 | AD0 | PA30 |
| 循迹 | AD1 | PB0 |
| 循迹 | AD2 | PB1 |
| 循迹 | OUT | PB21 |
| 电机方向 | AIN1 / AIN2 | PA14 / PA13 |
| 电机方向 | BIN1 / BIN2 | PA16 / PA17 |
| 电机方向 | CIN1 / CIN2 | PB20 / PB24 |
| 电机方向 | DIN1 / DIN2 | PA25 / PA26 |
| 电机 PWM | PWM_LEFT（TIMA0） | PA8 / PA9 |
| 电机 PWM | PWM_RIGHT（TIMA1） | PB2 / PB3 |
| IMU | MOSI / MISO | PB17 / PB18 |
| IMU | SCLK / CSB2 | PA21 / PA23 |
| IMU | CSB1 | PB19 |
| OLED | SDA / SCL | PA10 / PA11 |
| 蓝牙 | TX / RX | PB6 / PB7 |

## 可调参数

集中在 `empty_mspm0g3507.c` 与 `trace.c` 顶部：

| 宏 | 当前值 | 说明 |
| --- | --- | --- |
| `RUN_SPEED` | 100 | 循迹直行速度（±1000 量纲） |
| `TURN_SPEED` | 350 | 蓝牙原地转向速度 |
| `TURN_GAIN` | 90 | 偏差 1 格对应的左右轮差速 |
| `LOOP_MS` | 10 | 主循环周期（ms） |
| `LOST_STOP_MS` | 500 | 丢线停车阈值（ms） |
| `TRACE_ACTIVE_LEVEL` | 1 | 黑线对应电平（高电平有效） |
| `TRACE_SETTLE_CYCLES` | 64 | 通道切换等待，约 2 µs |
| `MOTOR_DUTY_MAX` | 2133 | PWM 满占空比计数值 |
| 死区 | 85 | 电机起转最小 duty |

## 编译与烧录

环境：CCS Theia 21.0.1 + MSPM0 SDK 2.11.00.07 + tiarmclang 5.1.1。

1. CCS 中 `File → Import → CCS Projects`，选择本目录
2. `Project → Build Project` 生成 `Debug/aim_car.out`
3. 连接 XDS110，`Run → Debug` 烧录（配置见 `targetConfigs/MSPM0G3507.ccxml`）

`Debug/` 已在 `.gitignore` 中排除，克隆后需要重新 Build 一次。

命令行编译：

```bash
<CCS_PATH>/utils/bin/gmake.exe -C Debug all
```

## 参考资料（docs/images）

| 文件 | 内容 |
| --- | --- |
| `schematic-main.png` | 嘉立创 EDA 原理图（main 图页） |
| `pcb-layout.webp` | PCB 布线图 |
| `board-render.png` | LCKFB-TMX-MSPM0G3507 底板 3D 渲染图 |
| `motor-pin-mapping.png` | 四轮方向脚 / 定时器通道映射表 |
| `hc4051-channel-table.png` | 74HC4051 通道选择真值表（AD2 / AD1 / AD0） |
| `differential-drive-radius.png` | 差速转向半径公式与四种转弯模式 |

## 已知限制

- `bmi088`、`ctrl`、`ui`、`oled` 四个模块尚未接入主循环，属于后续 PID 直行功能的预留代码
- 8 路循迹通道与车身左右侧的物理对应关系需按实车接线实测确认
- 蓝牙指令为单字节裸协议，无帧头与校验，长距离通信可能误触发
