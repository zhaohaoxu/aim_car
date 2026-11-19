# 04 蓝牙遥控

> **本篇目标**：手机通过蓝牙串口助手发 `W/A/S/D/X`，小车实时响应。
> **前置**：完成 [03 循迹](03-trace.md)。
> **验收标准**：手机发 `W` 前进、`S` 后退、`A` 左转、`D` 右转、`X` 停止。

## 硬件

| 信号 | 引脚 | 说明 |
| --- | --- | --- |
| TX | PB6 | MSPM0 → 蓝牙模块 RX |
| RX | PB7 | 蓝牙模块 TX → MSPM0 |
| 波特率 | 9600 | HC-05 默认值 |

> 接线注意：**TX 接对方的 RX**，交叉接线，不要 TX 对 TX。

## 原理

蓝牙模块对单片机来说就是一根「无线的串口线」。手机连上模块后发一个字符，单片机从 UART 收到同一个字符。

```mermaid
flowchart LR
    APP[手机蓝牙串口助手] -->|无线| HC05[HC-05]
    HC05 -->|UART1 9600| MCU[MSPM0]
    MCU -->|W/A/S/D/X 解析| MOTOR[电机]
```

**指令是单字节裸协议**，没有帧头也没有校验。数据量小、人工操作，丢一两个字节不影响体验。

## 指令表

| 字符 | 动作 | 实现 |
| --- | --- | --- |
| `W` | 前进 | `motor_drive(RUN, RUN)` |
| `S` | 后退 | `motor_drive(-RUN, -RUN)` |
| `A` | 左转 | `motor_drive(-TURN, TURN)` |
| `D` | 右转 | `motor_drive(TURN, -TURN)` |
| `X` | 停止 | `motor_stop_all()` |

**指令保持**：发一次 `W` 后，车会一直前进，直到收到新指令。这样手机松手也不会突然停车。

## 代码

```c
#define RUN_SPEED   (100)
#define TURN_SPEED  (350)

int main(void)
{
    int cmd = 'X';

    SYSCFG_DL_init();
    motor_init();

    while (1) {
        int c = uart_bt_get_char();           /* 非阻塞，没数据返回 -1 */
        if (c >= 0) {
            if (c == 'W' || c == 'S' || c == 'A' || c == 'D' || c == 'X') {
                cmd = c;                      /* 只接受这 5 个合法指令 */
            }
        }

        switch (cmd) {
        case 'W': motor_drive(RUN_SPEED, RUN_SPEED);    break;
        case 'S': motor_drive(-RUN_SPEED, -RUN_SPEED);  break;
        case 'A': motor_drive(-TURN_SPEED, TURN_SPEED); break;
        case 'D': motor_drive(TURN_SPEED, -TURN_SPEED); break;
        default:  motor_stop_all();                     break;
        }

        delay_cycles(32000 * 10);             /* 10 ms 一轮 */
    }
}
```

## 验收

| 检查 | 通过标准 |
| --- | --- |
| 配对 | 手机能搜到 HC-05 并连接（默认密码 1234 或 0000） |
| 前进 | 发 `W`，四轮前进并保持 |
| 转向 | 发 `A` 左转、`D` 右转 |
| 停止 | 发 `X` 立即停 |
| 容错 | 发 `Z` 等非法字符，车状态不变 |

## 常见问题

| 现象 | 原因 |
| --- | --- |
| 发指令没反应 | TX/RX 没交叉接；或波特率不是 9600 |
| 收到乱码 | 手机 App 的发送编码不是 ASCII；换成「文本模式」 |
| 连不上蓝牙 | 模块已被其他设备占用，先断开；或模块进入 AT 模式 |
| 车一直转不前进 | 只发了 `A` / `D` 没发 `W`，指令会保持 |

## 下一步

[05 K230 摄像头](05-camera.md) —— 让视觉模块出画面。
