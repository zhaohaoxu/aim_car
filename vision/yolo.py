import os, sys
from media.sensor import *
from media.display import *
from media.media import *
from libs.Utils import *
import nncase_runtime as nn #ai2d 预处理+ kpu 推理
import ulab.numpy as np
import image
import time


DISPLAY_WIDTH, DISPLAY_HEIGHT = 320, 240
AI_WIDTH, AI_HEIGHT = 320, 240
# 模型尺寸
model_size = [320, 320]

# 解码框框
def decode_boxes(out, ratio, top, left, conf_thres=0.35):
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


sensor = None
try:
    sensor = Sensor()
    sensor.reset()
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
    kpu.load_kmodel("/sdcard/yolov8n_320.kmodel")

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

        # 首帧诊断：看看输出形状和数值范围
        if count == 0:
            a = kpu_input_tensor.to_numpy()
            print("input min=%d max=%d" % (int(np.min(a)), int(np.max(a))))
            print("out shape = %s" % (out.shape,))
            t = out[0].transpose()
            rows = t.shape[0]
            for k in (0, 1, rows - 1):
                print("row%d:" % k, t[k][0:6])
            bx = t[:, 0:4]
            print("box min=%.2f max=%.2f" % (float(np.min(bx)), float(np.max(bx))))
            cls = t[:, 4:]
            print("cls min=%.4f max=%.4f" % (float(np.min(cls)), float(np.max(cls))))

        # 解码出所有候选框，再做 NMS 去重
        boxes = decode_boxes(out, ratio, top, left)
        boxes = nms(boxes)

        # 清空画布
        osd_img.clear()
        for b in boxes:
            # 坐标限制在屏幕范围内，防止越界报错,x1,y1,x2,y2 分别是框的左上角和右下角坐标
            x1 = int(max(0, min(DISPLAY_WIDTH - 1, b[0])))
            y1 = int(max(0, min(DISPLAY_HEIGHT - 1, b[1])))
            x2 = int(max(0, min(DISPLAY_WIDTH - 1, b[2])))
            y2 = int(max(0, min(DISPLAY_HEIGHT - 1, b[3])))
            # 画红色矩形框
            osd_img.draw_rectangle(x1, y1, x2 - x1, y2 - y1, color=(255, 0, 0, 255), thickness=2)
            # 在框左上角显示置信度
            osd_img.draw_string_advanced(x1, max(0, y1 - 20), 20, "%.2f" % b[4], color=(255, 0, 0, 255))
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
