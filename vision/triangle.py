"""
蓝色纸张上的三角形识别 —— 纯 OpenMV / CanMV API 最简版

原理：纸张和三角形内部是同一个颜色，被深色描边隔开，
      find_blobs 会返回两块互不相连的色块：
        最大的一块        -> 蓝色纸张
        里面较小的一块    -> 三角形内部

      三角形像素约占外接矩形的 50%（矩形 ~100%，圆形 ~78%），
      所以用“填充率”就能判断是不是三角形，不需要轮廓算法。
"""

import time, os, gc, struct

from media.sensor import *
from media.display import *
from media.media import *
from machine import UART, FPIOA

W, H = 640, 480
USE_IDE_DISPLAY = True

# 只识别画面中间这一块，这个 ROI 同时决定预览窗口的大小
ROI_X, ROI_Y = 160, 80
ROI_W, ROI_H = 320, 240
ROI = (ROI_X, ROI_Y, ROI_W, ROI_H)

TH_BLUE    = (20, 90, -30, 25, -60, 0)   # 蓝色纸张在 LAB 下的范围
MIN_PIXELS = 1500                        # 色块最小像素数，滤掉噪点

TRI_BOX_MIN = 2500                       # 三角形外接矩形面积下限
TRI_BOX_MAX = 60000                      # 上限（纸张那块的面积远大于它）
TRI_FILL_MIN = 0.40                      # 填充率下限（三角形约 0.50）
TRI_FILL_MAX = 0.62                      # 上限（矩形 1.0 / 圆形 0.78 被排除）

# 与 MSPM0 的串口：K230 UART1，TX = IO_3，RX = IO_4，115200
uart = None
try:
    fpioa = FPIOA()
    fpioa.set_function(3, FPIOA.UART1_TXD)
    fpioa.set_function(4, FPIOA.UART1_RXD)
    uart = UART(UART.UART1, 115200, timeout=1000)
except Exception:
    pass


def send_frame(found, cx, cy, area):
    """0xAA 0x55 LEN PAYLOAD SUM，与 MSPM0 的 k230.c 对应"""
    if uart is None:
        return
    body = struct.pack('<BHHH', found, cx, cy, area)
    n    = len(body)
    chk  = (n + sum(body)) & 0xFF
    uart.write(b'\xAA\x55' + bytes([n]) + body + bytes([chk]))


sensor = None
display = False

try:
    sensor = Sensor(width=W, height=H)
    sensor.reset()
    sensor.set_framesize(width=W, height=H)
    sensor.set_pixformat(Sensor.RGB565)

    if USE_IDE_DISPLAY:
        try:
            Display.init(Display.VIRT, width=ROI_W, height=ROI_H, fps=60)
            display = True
        except Exception:
            pass

    MediaManager.init()
    sensor.run()

    fps = time.clock()

    while True:
        fps.tick()
        os.exitpoint()
        img = sensor.snapshot()

        blobs = img.find_blobs([TH_BLUE], pixels_threshold=MIN_PIXELS,
                               area_threshold=0, merge=False, roi=ROI)

        # 面积最大的那块就是纸张
        paper = None
        paper_box = 0
        for b in blobs:
            box = b.w() * b.h()
            if box > paper_box:
                paper_box = box
                paper = b

        # 填充率符合三角形的那块
        tri = None
        tri_pixels = 0
        for b in blobs:
            box = b.w() * b.h()
            if box < TRI_BOX_MIN or box > TRI_BOX_MAX:
                continue
            fill = b.pixels() / box
            if TRI_FILL_MIN <= fill <= TRI_FILL_MAX and b.pixels() > tri_pixels:
                tri_pixels = b.pixels()
                tri = b

        if paper:
            img.draw_rectangle(paper.x(), paper.y(), paper.w(), paper.h(),
                               color=(255, 255, 0), thickness=2)
        if tri:
            x, y, w, h = tri.rect()
            img.draw_rectangle(x, y, w, h, color=(255, 0, 0), thickness=3)
            # 把三角形中心发给 MSPM0，它在 cv_mode 下会原地旋转对准
            send_frame(1, x + w // 2, y + h // 2, tri.pixels())
        else:
            send_frame(0, 0, 0, 0)

        if display:
            out = img.copy(roi=ROI)
            out.compressed_for_ide()
            Display.show_image(out)
            if tri:
                print('TRIANGLE', tri.rect())

        gc.collect()

except KeyboardInterrupt:
    print('user stop')
except BaseException as e:
    print("Exception '%s'" % e)
finally:
    if isinstance(sensor, Sensor):
        sensor.stop()
    if display:
        Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    MediaManager.deinit()
