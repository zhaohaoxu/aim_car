import os
from media.sensor import *
from media.display import *
from media.media import *
from libs.Utils import *
import nncase_runtime as nn #ai2d 预处理+ kpu 推理
import ulab.numpy as np
import image
import time
from machine import UART, FPIOA
import struct


DISPLAY_WIDTH, DISPLAY_HEIGHT = 320, 240
AI_WIDTH, AI_HEIGHT = 320, 240
# 模型尺寸
model_size = [320, 320]

# 解码框框
def decode_boxes(out, ratio, top, left, conf_thres=0.6):
    if len(out.shape) == 3:
        out = out[0]
    if out.shape[0] < out.shape[1]:
        out = out.transpose()
    n, c = out.shape
    boxes = []
    for i in range(n):
        score = float(np.max(out[i][4:]))
        if score < conf_thres:
            continue
        cx, cy, w, h = out[i, 0], out[i, 1], out[i, 2], out[i, 3]
        x1 = (cx - w / 2 - left) / ratio
        y1 = (cy - h / 2 - top) / ratio
        x2 = (cx + w / 2 - left) / ratio
        y2 = (cy + h / 2 - top) / ratio
        boxes.append((x1, y1, x2, y2, score))
    return boxes

# 非极大值抑制
def nms(boxes, iou_thres=0.45):
    boxes = sorted(boxes, key=lambda b: b[4], reverse=True)
    keep = []
    while len(boxes) > 0:
        best = boxes[0]
        keep.append(best)
        rest = []
        for b in boxes[1:]:
            iw = min(best[2], b[2]) - max(best[0], b[0])
            ih = min(best[3], b[3]) - max(best[1], b[1])
            if iw > 0 and ih > 0:
                inter = iw * ih
                a1 = (best[2] - best[0]) * (best[3] - best[1])
                a2 = (b[2] - b[0]) * (b[3] - b[1])
                if inter / (a1 + a2 - inter) >= iou_thres:
                    continue
            rest.append(b)
        boxes = rest
    return keep

# 发送数据到MSPM03507
def send_ball(found, cx, cy, area):
    #溢出保护
    if area > 65535:
        area = 65535
    #"<BHHH"小端，found，cx，cy，area
    payload = struct.pack("<BHHH", found, cx & 0xFFFF, cy & 0xFFFF, area)
    s = len(payload)#校验
    for b in payload:
        s = (s + b) & 0xFF
    # 11 字节协议：AA 55 LEN | found cx(2) cy(2) area(2) | SUM
    uart.write(bytes([0xAA, 0x55, len(payload)]) + payload + bytes([s]))


sensor = None
cx_smooth = -1.0
try:
    #串口通信：IO3=TX, IO4=RX，115200
    fpioa = FPIOA()
    fpioa.set_function(3, FPIOA.UART1_TXD)
    fpioa.set_function(4, FPIOA.UART1_RXD)
    uart = UART(UART.UART1, 115200)

    sensor = Sensor()
    sensor.reset()
    # 摄像头倒装 180°：水平镜像 + 垂直翻转，把画面转正
    sensor.set_hmirror(True)
    sensor.set_vflip(True)
    # 设置摄像头分辨率
    sensor.set_framesize(width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, chn=CAM_CHN_ID_0)
    # 设置摄像头像素格式
    sensor.set_pixformat(Sensor.YUV420SP, chn=CAM_CHN_ID_0)

    # 设置AI分辨率
    sensor.set_framesize(width=AI_WIDTH, height=AI_HEIGHT, chn=CAM_CHN_ID_1)
    # 设置AI像素格式
    sensor.set_pixformat(Sensor.RGBP888, chn=CAM_CHN_ID_1)

    # 绑定摄像头到屏幕
    sensor_bind_info = sensor.bind_info(x=0, y=0, chn=CAM_CHN_ID_0)
    # 绑定摄像头到屏幕显示层，to_ide=True IDE预览
    Display.bind_layer(**sensor_bind_info, layer=Display.LAYER_VIDEO1)
    # 初始化显示层
    Display.init(Display.VIRT, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, osd_num=1, to_ide=True)

    # OSD透明画布（画框）
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    osd_img.clear()
    # 限制显示通道帧率
    sensor._set_chn_fps(chn=CAM_CHN_ID_0, fps=15)
    sensor._set_chn_fps(chn=CAM_CHN_ID_1, fps=15)

    # ai2d 预处理，letterbox_pad_param 补充图片
    top, bottom, left, right, ratio = letterbox_pad_param([AI_WIDTH, AI_HEIGHT], model_size)

    ai2d = nn.ai2d()
    #输入输出数据类型
    ai2d.set_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)
    #补边参数
    ai2d.set_pad_param(True, [0, 0, 0, 0, top, bottom, left, right], 0, [128, 128, 128])
    #缩放参数
    ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
    #构建模型
    ai2d_builder = ai2d.build([1, 3, AI_HEIGHT, AI_WIDTH], [1, 3, model_size[1], model_size[0]])
    kpu_input_tensor = nn.from_numpy(np.ones((1, 3, model_size[1], model_size[0]), dtype=np.uint8))

    # 加载 kmodel
    kpu = nn.kpu()
    kpu.load_kmodel("/sdcard/yolo11n_det_320.kmodel")

    # 初始化媒体管理器
    MediaManager.init()
    sensor.run()

    fps = time.clock()
    count = 0
    while True:
        os.exitpoint()
        fps.tick()#统计帧率

        # 从AI通道取一帧
        img = sensor.snapshot(chn=CAM_CHN_ID_1)
        # 交给 ai2d 做 resize + pad，结果写进 kpu_input_tensor
        ai2d_builder.run(nn.from_numpy(img.to_numpy_ref()), kpu_input_tensor)
        # kpu推理
        kpu.set_input_tensor(0, kpu_input_tensor)
        kpu.run()

        # 取出推理结果
        out = kpu.get_output_tensor(0).to_numpy()

        # 解码出所有候选框，再做 NMS 去重
        boxes = decode_boxes(out, ratio, top, left)
        boxes = nms(boxes)

        # 清空画布
        osd_img.clear()
        best = None
        best_area = 0
        # 遍历所有候选框
        for b in boxes:
            # 坐标限制在屏幕范围内，防止越界报错,x1,y1,x2,y2 分别是框的左上角和右下角坐标
            x1 = int(max(0, min(DISPLAY_WIDTH - 1, b[0])))
            y1 = int(max(0, min(DISPLAY_HEIGHT - 1, b[1])))
            x2 = int(max(0, min(DISPLAY_WIDTH - 1, b[2])))
            y2 = int(max(0, min(DISPLAY_HEIGHT - 1, b[3])))
            # 画红色矩形框
            osd_img.draw_rectangle(x1, y1, x2 - x1, y2 - y1, color=(255, 0, 0, 255), thickness=2)
            # 显示置信度和框面积，面积用来调 MSPM0 的远近阈值
            area = (x2 - x1) * (y2 - y1)
            osd_img.draw_string_advanced(x1, max(0, y1 - 20), 20, "%.2f %d" % (b[4], area), color=(255, 0, 0, 255))
            # 记录面积最大的框作为跟踪目标
            if area > best_area:
                best_area = area
                best = (x1, y1, x2, y2)
        # 目标框
        if best is not None:
            # 最佳框用绿色加粗
            osd_img.draw_rectangle(best[0], best[1], best[2] - best[0], best[3] - best[1],
                                   color=(0, 255, 0, 255), thickness=3)
            cx = (best[0] + best[2]) // 2
            cy = (best[1] + best[3]) // 2
            # 一阶低通滤波：抑制框在几像素范围内抖动引起的左右摆
            if cx_smooth < 0:
                cx_smooth = cx
            else:
                cx_smooth = cx_smooth * 0.7 + cx * 0.3
            send_ball(1, int(cx_smooth), cy, best_area)
        else:
            cx_smooth = -1.0
            # 没找到目标也要发帧，MSPM0 靠这个判断链路存活
            send_ball(0, 0, 0, 0)

        # 把画布显示
        Display.show_image(osd_img, 0, 0, Display.LAYER_OSD0)

        #帧率统计
        count += 1
        if count % 30 == 0:
            print("fps=%.1f" % fps.fps())

except KeyboardInterrupt:#用户手动停止
    print("user stop")
except BaseException as e:#其他异常停止
    print("Exception '%s'" % e)
finally:
    # 停止摄像头
    if isinstance(sensor, Sensor):
        sensor.stop()
    # 退出屏幕
    Display.deinit()
    # 进入睡眠状态
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # 退出媒体管理器
    MediaManager.deinit()
