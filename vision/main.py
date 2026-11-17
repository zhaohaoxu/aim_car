import time, os, gc, struct

from media.sensor import *
from media.display import *
from media.media import *
from machine import UART, FPIOA

DETECT_WIDTH = 640
DETECT_HEIGHT = 480

#预览窗口
USE_IDE_DISPLAY = True

#K230 UART1，TX = IO_3(GPIO3)，RX = IO_4(GPIO4)，115200
uart = None
try:
    fpioa = FPIOA()
    fpioa.set_function(3, FPIOA.UART1_TXD)
    fpioa.set_function(4, FPIOA.UART1_RXD)
    uart = UART(UART.UART1, 115200, timeout=1000)
except Exception:
    pass

# 发送一帧数据
def send_frame(found, cx, cy, area):
    if uart is None:
        return
    body = struct.pack('<BHHH', found, cx, cy, area)#
    n    = len(body)#长度
    chk  = (n + sum(body)) & 0xFF#校验
    uart.write(b'\xAA\x55' + bytes([n]) + body + bytes([chk]))#发送

# 阈值
thresholds_red = [((0, 70, 30, 127, -128, 127))] # 红色

sensor = None
display = False

try:
    # 使用默认配置构造一个Sensor对象
    sensor = Sensor(width = DETECT_WIDTH, height = DETECT_HEIGHT)
    # sensor复位
    sensor.reset()
    # 设置水平镜像
    # sensor.set_hmirror(False)
    # 设置垂直翻转
    #sensor.set_vflip(True)
    # 设置通道 0 输出大小
    sensor.set_framesize(width = DETECT_WIDTH, height = DETECT_HEIGHT)
    # 设置通道 0 输出格式
    sensor.set_pixformat(Sensor.RGB565)

    # 使用 IDE 作为输出
    if USE_IDE_DISPLAY:
        try:
            # 初始化显示
            Display.init(Display.VIRT, width = 384, height = 320, fps = 60)
            display = True
        except Exception:
            pass

    # 初始化媒体管理器
    MediaManager.init()
    # sensor开始运行
    sensor.run()

    fps = time.clock()

    while True:
        fps.tick()

        # 检查是否应该退出
        os.exitpoint()
        img = sensor.snapshot()

        # 找出最大红色色块，画框并发送给 MSPM0
        best = None
        for blob_red in img.find_blobs(thresholds_red, pixels_threshold=900, area_threshold=0, merge=True):
            img.draw_rectangle([v for v in blob_red.rect()], color=(255, 0, 0), thickness=20)
            if best is None or blob_red.pixels() > best.pixels():
                best = blob_red
        if best is None:
            send_frame(0, 0, 0, 0)
        else:
            send_frame(1, best.cx(), best.cy(), min(best.pixels(), 65535))
        # 将结果绘制到屏幕上
        if display:
            img = img.copy(roi=(196,52,384,320))
            img.compressed_for_ide()
            Display.show_image(img)
        gc.collect()

        # 离线运行时没有终端，打印会拖慢主循环，仅在 IDE 调试时输出
        if display:
            print(fps.fps())
except KeyboardInterrupt as e:
    print(f"user stop")
except BaseException as e:
    print(f"Exception '{e}'")
finally:
    # sensor停止运行
    if isinstance(sensor, Sensor):
        sensor.stop()
    # 销毁display
    if display:
        Display.deinit()
    # 休眠
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # 释放媒体缓冲区
    MediaManager.deinit()