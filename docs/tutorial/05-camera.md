# 05 K230 摄像头出图

> **本篇目标**：K230 摄像头画面显示在 CanMV IDE 预览窗口里。
> **前置**：完成 [00 环境搭建](00-environment.md) 的 K230 部分。
> **验收标准**：IDE 里能看到实时画面，帧率 15 fps 左右。

## 硬件

| 项目 | 说明 |
| --- | --- |
| 视觉模块 | CanMV-K230-LP4 V3.0 |
| 摄像头 | OV5647（MIPI CSI2），本工程实测 1920×1080@30 输入 |
| 连接 | USB 连电脑，CanMV IDE 通过串口/USB 与板子通信 |

## 核心概念：双通道

K230 的 Sensor 可以同时输出两路图像：

| 通道 | 格式 | 用途 |
| --- | --- | --- |
| `CAM_CHN_ID_0` | YUV420SP | 给显示屏做硬件直通 |
| `CAM_CHN_ID_1` | RGBP888 | 给 AI 做输入（`(3,H,W)` CHW 布局，无 batch 维） |

**为什么要分两路**：显示屏要的是 YUV，AI 要的是 RGB 且尺寸不同。分两路各取所需，互不干扰，也省去 CPU 转格式的开销。

## ⚠️ 三个顺序问题（黑屏根因）

这三步顺序错了，就是黑屏，而且不报错：

```mermaid
flowchart LR
    A[sensor.bind_info] --> B[Display.bind_layer]
    B --> C[Display.init]
    C --> D[Display.show_image]
```

| 顺序 | 说明 |
| --- | --- |
| 1. `bind_info()` | 先拿到通道的宽高格式等描述信息 |
| 2. `bind_layer()` | 在 **init 之前**把视频层绑到显示层 |
| 3. `Display.init()` | 申请画布（`osd_num=1` 表示要一层 OSD） |

另外两个坑：

- `Display.init(..., osd_num=1)` 的 OSD 数量**运行时不能再改**，要几层就在 init 时定好。
- `to_ide=True` 会把每帧压成 JPEG 走 USB，很吃性能，是 `runScript timed out` 的常见元凶。只用板载屏幕时关掉它。

## 最小可用代码

```python
from media.sensor import *
from media.display import *
from media.media import *
import os, time

WIDTH, HEIGHT = 320, 240

sensor = Sensor()
sensor.reset()
# 显示通道
sensor.set_framesize(width=WIDTH, height=HEIGHT, chn=CAM_CHN_ID_0)
sensor.set_pixformat(Sensor.YUV420SP, chn=CAM_CHN_ID_0)

# 必须：先 bind_info，再 bind_layer，最后 Display.init
info = sensor.bind_info(x=0, y=0, chn=CAM_CHN_ID_0)
Display.bind_layer(**info, layer=Display.LAYER_VIDEO1)
Display.init(Display.VIRT, width=WIDTH, height=HEIGHT, to_ide=True)

MediaManager.init()
sensor.run()

try:
    while True:
        os.exitpoint()
        time.sleep_ms(10)
except KeyboardInterrupt:
    pass
finally:
    sensor.stop()
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()
```

> 摄像头的 **180° 倒装**用两行解决：
> ```python
> sensor.set_hmirror(True)
> sensor.set_vflip(True)
> ```
> 两个都开就是画面转 180°；只开一个是左右镜像或上下翻转。

## 验收

| 检查 | 通过标准 |
| --- | --- |
| 出图 | IDE 预览窗口有实时画面 |
| 分辨率 | 画面 320×240 |
| 帧率 | 约 15 fps（用 `sensor._set_chn_fps(fps=15)` 限制） |
| 方向 | 画面正向，不颠倒不镜像 |

## 常见问题

| 现象 | 原因 |
| --- | --- |
| 黑屏 | `bind_layer` 在 `Display.init` 之后调用了 |
| 画面卡住 | `to_ide=True` 开销太大，关掉 |
| 报 `OSError` | 没调 `MediaManager.init()` 就 `sensor.run()` |
| 画面颠倒 | 摄像头倒装，加 `set_hmirror(True)` + `set_vflip(True)` |
| 退出后再次运行报错 | `finally` 里没做 `sensor.stop()` / `Display.deinit()` |

## 下一步

[06 YOLO 识别钢球](06-yolo.md) —— 让 K230 看懂画面里有没有球。
