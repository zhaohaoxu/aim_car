# 07 串口协议打通

> **本篇目标**：K230 每帧检测结果通过串口发给 MSPM0，MSPM0 解析成功后 LED4 闪烁。
> **前置**：完成 [06 YOLO](06-yolo.md) 和 [01 点灯](01-led.md)。
> **验收标准**：K230 运行时，MSPM0 的 LED4 持续闪烁；IDE 停止脚本后，LED4 停止闪烁。

## 接线

| MSPM0 | 方向 | K230 |
| --- | --- | --- |
| PA0（TX） | → | IO_4（UART1_RXD） |
| PA1（RX） | ← | IO_3（UART1_TXD） |
| GND | — | GND |

> **共地是必须的**。两块板只接信号线不接 GND，电平没有共同参考点，收到的就是乱码。

## 协议设计

### 为什么不是直接发字符串

字符串要解析、要分隔符、长度不定，一旦丢一个字节就整帧错位，很难恢复。**定长二进制帧**没有这个问题。

### 帧结构（11 字节）

| 字节 | 内容 | 说明 |
| --- | --- | --- |
| 0 | `0xAA` | 帧头 1 |
| 1 | `0x55` | 帧头 2 |
| 2 | `LEN = 7` | 负载长度 |
| 3 | `found` | 1 = 找到球，0 = 没找到 |
| 4~5 | `cx` | 中心 x，小端 |
| 6~7 | `cy` | 中心 y，小端 |
| 8~9 | `area` | 像素面积，小端 |
| 10 | `SUM` | 从 LEN 到最后一个负载字节的累加和，取低 8 位 |

**三个设计点**：

| 设计 | 作用 |
| --- | --- |
| 双帧头 `AA 55` | 从任意乱流中重新同步；单帧头容易和数据的 `AA` 混淆 |
| LEN 字段 | 将来加字段（比如距离）不用改接收端解析骨架 |
| SUM 校验 | 串口偶尔误码，校验不过就丢帧重找，不会把错误坐标传给电机 |

### 校验和怎么算

```python
s = len(payload)              # 从 LEN 开始
for b in payload:
    s = (s + b) & 0xFF        # 只留低 8 位
```

C 侧对应：

```c
sum = c;                      /* 收到 LEN 时初始化 */
sum += c;                     /* 每收一个负载字节累加 */
```

两边必须用**同一个起点**（都是 LEN），否则永远校验失败。

## K230 侧发送

```python
import struct
from machine import UART, FPIOA

fpioa = FPIOA()
fpioa.set_function(3, FPIOA.UART1_TXD)
fpioa.set_function(4, FPIOA.UART1_RXD)
uart = UART(UART.UART1, 115200)

def send_ball(found, cx, cy, area):
    if area > 65535:                    # uint16 上限保护
        area = 65535
    payload = struct.pack("<BHHH", found, cx & 0xFFFF, cy & 0xFFFF, area)
    s = len(payload)
    for b in payload:
        s = (s + b) & 0xFF
    uart.write(bytes([0xAA, 0x55, len(payload)]) + payload + bytes([s]))
```

`"<BHHH"`：`<` 小端，`B` 1 字节，`H` 2 字节。

## MSPM0 侧接收

接收分三层，每层解决一个问题：

```mermaid
flowchart LR
    IRQ[UART RX 中断] -->|存字节| RB[环形缓冲 256 字节]
    RB -->|ring_pop| SM[状态机 4 状态]
    SM -->|完整帧| APP[控制逻辑]
```

### 第 1 层：中断 + 环形缓冲

```c
void UART_VISION_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_VISION_INST)) {
    case DL_UART_MAIN_IIDX_RX:
        while (!DL_UART_Main_isRXFIFOEmpty(UART_VISION_INST)) {
            uint8_t  c    = DL_UART_Main_receiveData(UART_VISION_INST);
            uint16_t next = (uint16_t)((rx_head + 1) % K230_RX_SIZE);
            if (next != rx_tail) {      /* 满则丢新数据 */
                rx_buf[rx_head] = c;
                rx_head = next;
            }
        }
        break;
    default: break;
    }
}
```

要点：

- **`while` 不是 `if`**：115200 下 1 字节约 87 µs，中断响应的几十 µs 里 FIFO 可能已进多个字节，必须排空。
- **环形缓冲「牺牲一格」**：`head == tail` 判空，`(head+1)%N == tail` 判满，256 格实际用 255 格。
- **`volatile`**：`rx_buf/rx_head/rx_tail` 被中断和主循环同时访问，必须每次从内存读。

### 第 2 层：状态机解帧

```c
case 0:  if (c == 0xAA) state = 1;                       /* 找帧头 1 */
case 1:  if (c == 0x55) state = 2;                       /* 找帧头 2 */
         else state = (c == 0xAA) ? 1 : 0;               /* AA AA 55 也能接上 */
case 2:  if (c > K230_PAYLOAD_MAX) state = 0;            /* 长度非法重来 */
         else { plen = c; idx = 0; sum = c; state = 3; }
case 3:  if (idx < plen) { payload[idx++] = c; sum += c; }
         else { state = 0; if (c == sum) { *len = plen; return 1; } }
```

四个状态的本质：**逐字节确认「这是帧头 → 这是长度 → 这是负载 → 这是校验」**，任何一步不符就回到状态 0 重新找头。

### 第 3 层：主循环取帧

```c
while (k230_read_frame(payload, &len)) {
    if (len >= 7) {
        found = payload[0];
        cx    = payload[1] | (payload[2] << 8);
        area  = payload[5] | (payload[6] << 8);
    }
}
```

**用 `while` 把积压帧全部取完**，只保留最新一帧——旧坐标没有控制价值。

## 验收

| 检查 | 通过标准 |
| --- | --- |
| 接线 | K230 IO3 → MSPM0 PA1，GND 相连 |
| 发送 | K230 运行时串口有数据 |
| 接收 | MSPM0 LED4 持续闪烁 |
| 心跳 | K230 停止脚本后，LED4 在 300 ms 内停止闪烁 |
| 校验 | 拔掉 GND 再插回，LED4 恢复闪烁（不会卡死） |

**LED4 是本篇最重要的调试工具**：闪 = 整条链路通，不闪 = 链路断。

## 常见问题

| 现象 | 原因 |
| --- | --- |
| LED4 不闪 | 没共地；TX/RX 接反；K230 侧 FPIOA 没设成 UART1 |
| LED4 闪但数据乱 | 波特率不一致（两边必须都是 115200） |
| 偶发不闪 | 中断里用了 `if` 没排空 FIFO，丢字节导致帧错位 |
| 一插电就闪 | 正常，K230 未运行时也可能收到残留电平，判断以「K230 运行/停止」为准 |
| 校验总是不通过 | 两边 SUM 起点不一致（必须都从 LEN 开始） |

## 下一步

[08 视觉追踪联调](08-tracking.md) —— 让小车真正追着球跑。
