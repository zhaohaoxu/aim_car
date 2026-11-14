import time, os, gc, sys, math

from media.sensor import *
from media.display import *
from media.media import *

DETECT_WIDTH = 640
DETECT_HEIGHT = 480


thresholds_red = [(0, 0, 0, 255, 255, 255)] # 红色

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
    Display.init(Display.VIRT, width = DETECT_WIDTH, height = DETECT_HEIGHT, fps = 100)
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

        for blob_red in img.find_blobs(thresholds_red, pixels_threshold=0, area_threshold=900, merge=True):
                img.draw_rectangle([v for v in blob_red.rect()], color=(255, 0, 0), thickness=20)
        # 将结果绘制到屏幕上
        img = img.copy(roi=(196,52,384,320))
        img.compressed_for_ide()
        Display.show_image(img)
        gc.collect()

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
    Display.deinit()
    # 休眠
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # 释放媒体缓冲区
    MediaManager.deinit()