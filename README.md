# MSPM0G3507 四轮智能小车

基于 TI MSPM0G3507 的四轮麦轮小车嵌入式工程，开发环境 CCS Theia + SysConfig。

## 硬件平台

| 项目 | 说明 |
| --- | --- |
| 主控 | MSPM0G3507（Cortex-M0+，LQFP-64，MCLK 32 MHz） |
| 底板 | 立创 LCKFB-TMX-MSPM0G3507 扩展板 |
| 车体 | 四驱麦轮底盘（双层），305×230×5 mm 亚克力底板 ×2，整车约 307×305×126 mm |
| 电机 | 520 直流编码器电机 ×4，80 mm 麦克纳姆轮 ×4 |
| 电机驱动 | TB6612 双路驱动模块（带稳压版）×1，左右各驱动 2 个并联电机 |
| 电源 | 12V 电池供电 |
| 循迹 | 亚博智能 8 路灰度巡线模块，经 74HC4051 八选一模拟开关扫描 |
| 姿态 | BMI088 六轴 IMU 模块（软件 SPI，已接入陀螺仪 z 轴） |
| 遥控 | HC-05 蓝牙串口模块（HC05-V3.0 带底板），UART1 |
| 显示 | 0.96 寸 OLED SSD1306 128×64（IIC 四针） |
| 视觉 | CanMV-K230-LP4 V3.0（K230 RISC-V，1 GB LPDDR4，双 MIPI CSI），UART0 与主控通信 |

## 引脚配置

引脚以 [empty_mspm0g3507.syscfg](empty_mspm0g3507.syscfg) 为准，改动后需重新 Build。

### 电机（TB6612）

| 轮位 | 方向引脚 | PWM |
| --- | --- | --- |
| 左前 LF | DIN1 = PA25，DIN2 = PA26 | PA8（TIMA0 CCP0） |
| 左后 LB | CIN1 = PB20，CIN2 = PB24 | PA9（TIMA0 CCP1） |
| 右前 RF | AIN1 = PA14，AIN2 = PA13 | PB3（TIMA1 CCP1） |
| 右后 RB | BIN1 = PA16，BIN2 = PA17 | PB2（TIMA1 CCP0） |

PWM 周期 2133，边沿对齐向上计数；上层速度范围 −1000 ~ +1000，含 85 的启动死区。

代码按四路独立方向控制编写。若使用单块 TB6612（双路），左右两侧各并联 2 个电机，只需 A/B 两组方向线 + 两路 PWM。

### 循迹（74HC4051 八选一）

| 信号 | 引脚 | 说明 |
| --- | --- | --- |
| AD0 | PA30 | 通道选择最低位 |
| AD1 | PB0 | 通道选择 |
| AD2 | PB1 | 通道选择最高位 |
| OUT | PB21 | 输入，1 = 黑线 |

CH0 在最左（权重 −7），CH7 在最右（权重 +7）。

### 按键

| 按键 | 引脚 | 功能 |
| --- | --- | --- |
| KEY1 | PB10 | 启动循迹 |
| KEY2 | PA7 | 进入蓝牙遥控 |
| KEY3 | PB11 | 进入视觉追踪 |
| KEY4 | PB14 | 停车 |
| KEY5 | PA18 | 上拉输入，未使用 |

按键低电平有效，连续 3 次采样一致才确认，仅在按下沿触发。

### OLED（I2C0 / SSD1306）

| 信号 | 引脚 |
| --- | --- |
| SDA | PA10 |
| SCL | PA11 |

7 位从机地址 0x3C，Fast 模式（400 kHz）。

### 蓝牙（UART1）

| 信号 | 引脚 |
| --- | --- |
| TX | PB6 |
| RX | PB7 |

### K230 视觉（UART0）

| MSPM0 | 方向 | K230 |
| --- | --- | --- |
| PA0（TX） | → | IO_4（UART1_RXD） |
| PA1（RX） | ← | IO_3（UART1_TXD） |
| GND | — | GND |

115200、8 数据位、无校验、1 停止位，开启 RX 中断与 FIFO。

### BMI088（软件 SPI）

| 信号 | 引脚 |
| --- | --- |
| SCLK | PA21 |
| MOSI | PB17 |
| MISO | PB18 |
| CSB1（加速度计） | PB19 |
| CSB2（陀螺仪） | PA23 |

MISO 内部下拉，两个片选上电默认拉高。

### 调试与指示灯

| 功能 | 引脚 |
| --- | --- |
| SWDIO | PA19 |
| SWCLK | PA20 |
| LED4 | PA29 |

LED4 每成功解析一帧 K230 数据翻转一次，可用来判断串口链路是否正常。

## 功能

- **8 路循迹**：10 ms 扫描一轮，加权平均求偏差，差速转向
- **丢线保护**：连续 0.5 s 无黑线自动停车
- **蓝牙遥控**：`W` 前进、`S` 后退、`A` 左转、`D` 右转、`X` 停止
- **视觉追踪**：接收 K230 的目标中心 x 与面积，按偏差比例转向，对准中心后停车
- **三模式互斥**：循迹 / 蓝牙 / 视觉共用一条控制链，同一时刻只有一个控制源写电机
- **OLED 显示**：空闲时每 200 ms 刷新一次陀螺仪 z 轴角速度
- **陀螺仪**：上电初始化后静止标定 200 次，输出扣零偏的 z 轴角速度

## 通信协议（K230 → MSPM0）

一帧 11 字节：

| 字节 | 内容 |
| --- | --- |
| 0 | 0xAA 帧头 |
| 1 | 0x55 帧头 |
| 2 | LEN，负载长度（当前为 7） |
| 3..9 | PAYLOAD |
| 10 | SUM，从 LEN 到 PAYLOAD 末字节的累加和（取低 8 位） |

负载 7 字节，小端：

| 字节 | 内容 |
| --- | --- |
| 0 | found，1 = 找到目标，0 = 未找到 |
| 1..2 | cx，目标中心 x |
| 3..4 | cy，目标中心 y |
| 5..6 | area，目标像素面积 |

未检测到目标时也持续发送 found = 0 的心跳帧。

## 目录结构

```
aim_car/
├── empty_mspm0g3507.c           主程序入口（模式调度 / 循迹 / 蓝牙 / 视觉追踪）
├── empty_mspm0g3507.syscfg      SysConfig 外设与引脚配置
├── motor                        四路电机 PWM 与方向控制
├── trace                        8 路灰度循迹
├── key                          按键扫描与消抖
├── uart                         蓝牙串口收发
├── k230                         K230 视觉串口协议解析
├── oled                         SSD1306 显示驱动
├── bmi088                       六轴 IMU 驱动（软件 SPI）
├── ui                           OLED 多行布局（预留，未参与编译）
├── targetConfigs/               XDS110 调试配置
├── docs/                        硬件资料与说明
└── vision/
    └── yolo.py                  K230 YOLOv8n 检测脚本
```

## 可调参数

集中在 `empty_mspm0g3507.c` 与 `trace.c` 顶部：

| 宏 | 当前值 | 说明 |
| --- | --- | --- |
| `RUN_SPEED` | 100 | 循迹与蓝牙直行速度（−1000 ~ +1000） |
| `TURN_SPEED` | 350 | 蓝牙原地转向速度 |
| `TURN_GAIN` | 90 | 偏差 1 格对应的左右轮差速 |
| `LOST_STOP_MS` | 500 | 丢线停车阈值（ms） |
| `TRACE_ACTIVE_LEVEL` | 1 | 黑线对应电平（定义在 `trace.c`） |
| `VISION_CENTER_X` | 320 | 图像中心 x，按 640 宽计算 |
| `VISION_DEAD_X` | 40 | 中心死区，±40 像素内视为已对准并停车 |
| `VISION_MIN_AREA` | 900 | 目标面积下限，小于该值视为未找到 |
| `VISION_MIN_TURN` / `VISION_MAX_TURN` | 180 / 350 | 视觉转向速度的上下限 |
| `VISION_GAIN` | 3 | 偏差 1 像素对应的速度 |

> 视觉坐标目前按 640 宽设计。K230 侧的检测分辨率已改为 320×240，接入串口时需同步调整 `VISION_CENTER_X`，或由 K230 侧把坐标换算到 640 坐标系。

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
- OLED 上电执行一次「全白 1 秒 → 全黑」自检，之后仅在空闲模式刷新陀螺仪读数，整屏刷新约 26 ms
- `bmi088` 已接入主循环（初始化 + 零偏标定 + z 轴角速度显示）
- `ui` 仍是预留代码，内部调用的 `oled_draw_string()` / `oled_update()` 与现有 `oled.h` 接口不一致，未加入编译
- 蓝牙指令为单字节裸协议，无帧头与校验
- K230 模型 `yolov8n_320.kmodel` 已被 `.gitignore` 排除，需从 Kendryte 模型库下载后放到板子 `/sdcard/` 下
