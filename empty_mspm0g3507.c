#include "ti_msp_dl_config.h"
#include "motor.h"
#include "uart.h"
#include "key.h"
#include "trace.h"
#include "oled.h"
#include "k230.h"

#define DELAY_1MS       (32000)

#define LOOP_MS         (10)       /*丢线时间*/
#define LOST_STOP_MS    (500)      /*丢线时间超过0.5s就停车*/

#define RUN_SPEED       (100)      /* 直行速度 */
#define TURN_SPEED      (350)      /* 原地转向速度 */

#define TURN_GAIN       (90)       /* 偏差 1 格 -> 左右差 45（8 路偏差最大 ±7） */

#define VISION_CENTER_X (320)      /* 图像中心 x，640 宽的一半 */
#define VISION_SPEED    (200)      /* 视觉追踪基础速度 */
#define VISION_DIFF_MAX (150)      /* 转向差速限幅 */
#define VISION_MIN_AREA (900)      /* 色块面积小于此值视为未找到 */



int main(void)
{
    uint8_t running = 0;            /* 0=停车  1=循迹中 */
    uint8_t bt_mode = 0;            /* 0=停车  1=蓝牙遥控 */
    uint8_t vision_mode = 0;        /* 0=停车  1=视觉追踪 */
    int     cmd     = 'X';          /* 蓝牙当前指令，松手后保持 */
    int     lost_ms = 0;            /* 连续丢线累计时长 */

    SYSCFG_DL_init();
    k230_init();
    motor_init();
    motor_stop_all();

    OLED_init();
    OLED_fill(0xFF);
    OLED_refresh();
    delay_cycles(DELAY_1MS * 1000);
    OLED_clear();
    OLED_refresh();

    while (1) {
        key_id_t k = key_scanf();
        if (k == KEY_1) {           /* KEY1 启动循迹 */
            running     = 1;
            bt_mode     = 0;
            vision_mode = 0;
            lost_ms     = 0;
        }
        if (k == KEY_2) {           /* KEY2 进入蓝牙遥控 */
            running     = 0;
            bt_mode     = 1;
            vision_mode = 0;
            cmd         = 'X';
        }
        if (k == KEY_3) {           /* KEY3 停车 */
            running     = 0;
            bt_mode     = 0;
            vision_mode = 0;
        }
        if (k == KEY_4) {           /* KEY4 进入视觉追踪 */
            running     = 0;
            bt_mode     = 0;
            vision_mode = 1;
            lost_ms     = 0;
        }

        if (bt_mode) {
            int c = uart_bt_get_char();

            /* 收到合法指令才更新，松手不发数据时保持上一条 */
            if (c >= 0) {
                if (c == 'W' || c == 'S' || c == 'A' || c == 'D' || c == 'X') {
                    cmd = c;
                }
            }

            switch (cmd) {
            case 'W': motor_drive(RUN_SPEED, RUN_SPEED);    break;
            case 'S': motor_drive(-RUN_SPEED, -RUN_SPEED);  break;
            case 'A': motor_drive(-TURN_SPEED, TURN_SPEED); break;
            case 'D': motor_drive(TURN_SPEED, -TURN_SPEED); break;
            default:  motor_stop_all();                     break;
            }
        } else if (running) {
            int16_t error = trace_error();   /* -7~+7 是偏差，TRACE_LOST 是丢线 */

            if (error == TRACE_LOST) {
                /* 短暂丢线：保持直行去找线；超过 0.5s 就停车 */
                lost_ms += LOOP_MS;
                if (lost_ms >= LOST_STOP_MS) {
                    running = 0;
                    lost_ms = 0;
                    motor_stop_all();
                } else {
                    motor_drive(RUN_SPEED, RUN_SPEED);
                }
            } else {
                /* 偏差 -7(线在左) ~ +7(线在右) */
                int16_t diff = (int16_t)(error * TURN_GAIN);
                lost_ms = 0;
                /* 修正 */
                motor_drive((int16_t)(RUN_SPEED + diff),
                            (int16_t)(RUN_SPEED - diff));
            }
        } else if (vision_mode) {
            uint8_t  payload[K230_PAYLOAD_MAX];
            uint8_t  len;
            uint8_t  found = 0;
            uint16_t cx    = VISION_CENTER_X;
            uint16_t area  = 0;

            /* 把队列里的帧全部取完，只保留最新一帧 */
            while (k230_read_frame(payload, &len)) {
                if (len >= 7) {
                    found = payload[0];
                    cx    = (uint16_t)(payload[1] | ((uint16_t)payload[2] << 8));
                    area  = (uint16_t)(payload[5] | ((uint16_t)payload[6] << 8));
                }
            }

            if (found && area >= VISION_MIN_AREA) {
                /* 目标偏右(cx>320) -> diff>0 -> 左轮加速，车向右转 */
                int16_t err  = (int16_t)((int16_t)cx - VISION_CENTER_X);
                int16_t diff = (int16_t)((int32_t)err * VISION_SPEED / VISION_CENTER_X);

                if (diff >  VISION_DIFF_MAX) { diff =  VISION_DIFF_MAX; }
                if (diff < -VISION_DIFF_MAX) { diff = -VISION_DIFF_MAX; }

                motor_drive((int16_t)(VISION_SPEED + diff),
                            (int16_t)(VISION_SPEED - diff));
            } else {
                motor_stop_all();
            }
        } else {
            motor_stop_all();
        }

        delay_cycles(DELAY_1MS * LOOP_MS);
    }
}



