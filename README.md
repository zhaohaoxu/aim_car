# MSPM0G3507 四轮智能小车

基于 TI MSPM0G3507 的四轮循迹小车嵌入式工程，开发环境 CCS Theia + SysConfig。

## 硬件平台

| 项目 | 说明 |
| --- | --- |
| 主控 | MSPM0G3507（Cortex-M0+，LQFP-64，MCLK 32 MHz） |
| 底板 | 立创 LCKFB-TMX-MSPM0G3507 扩展板 |
| 循迹 | 8 路灰度传感器 + 74HC4051 八选一模拟开关 |
| 电机 | 四路直流减速电机，TIMA0 / TIMA1 输出四路 PWM |
| 姿态 | BMI088 六轴 IMU（软件 SPI，未接入） |
| 遥控 | 蓝牙串口模块，UART1（PB6 / PB7） |

## 功能

- **8 路循迹**：10 ms 扫描一轮，加权平均求偏差，差速转向
- **丢线保护**：连续 0.5 s 无黑线自动停车
- **蓝牙遥控**：`W` 前进、`S` 后退、`A` 左转、`D` 右转、`X` 停止
- **双模式互斥**：循迹与蓝牙共用一条控制链，同一时刻只有一个控制源写电机

## 按键

| 按键 | 引脚 | 功能 |
| --- | --- | --- |
| KEY1 | PB10 | 启动循迹 |
| KEY2 | PA7 | 进入蓝牙遥控 |
| KEY3 | PB11 | 停车 |
| KEY4 | PB14 | 预留 |

按键低电平有效，连续 3 次采样一致才确认，仅在按下沿触发。

## 目录结构

```
aim_car/
├── empty_mspm0g3507.c/.syscfg   主程序与外设配置
├── motor / trace / key / uart    驱动模块
├── bmi088 / ctrl / ui / oled     预留模块（未接入主循环）
├── targetConfigs/                XDS110 调试配置
├── docs/                         硬件资料与说明
└── vision/01颜色识别.py          K230 视觉识别脚本
```

## 可调参数

集中在 `empty_mspm0g3507.c` 与 `trace.c` 顶部：

| 宏 | 当前值 | 说明 |
| --- | --- | --- |
| `RUN_SPEED` | 100 | 循迹直行速度（±1000 量纲） |
| `TURN_SPEED` | 350 | 蓝牙原地转向速度 |
| `TURN_GAIN` | 90 | 偏差 1 格对应的左右轮差速 |
| `LOST_STOP_MS` | 500 | 丢线停车阈值（ms） |
| `TRACE_ACTIVE_LEVEL` | 1 | 黑线对应电平（高电平有效） |

## 编译与烧录

环境：CCS Theia 21.0.1 + MSPM0 SDK 2.11.00.07 + tiarmclang 5.1.1。

1. `File → Import → CCS Projects` 导入本目录
2. `Project → Build Project` 生成 `Debug/aim_car.out`
3. 连接 XDS110，`Run → Debug` 烧录

`Debug/` 已在 `.gitignore` 中排除，克隆后需重新 Build。

命令行编译：

```bash
<CCS_PATH>/utils/bin/gmake.exe -C Debug all
```

## 说明

- 硬件原理图、PCB 布线图、底板渲染图见 [docs/](docs/README.md)
- `bmi088`、`ctrl`、`ui`、`oled` 为后续 PID 直行功能的预留代码，尚未接入主循环
- 蓝牙指令为单字节裸协议，无帧头与校验
