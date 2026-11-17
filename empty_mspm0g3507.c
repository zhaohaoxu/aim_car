#include "ti_msp_dl_config.h"
#include "motor.h"
#include "uart.h"
#include "key.h"
#include "trace.h"
#include "oled.h"
#include "k230.h"
#include "bmi088.h"

#define DELAY_1MS       (32000)

#define LOOP_MS         (10)       /*丢线时间*/
#define LOST_STOP_MS    (500)      /*丢线时间超过0.5s就停车*/

#define RUN_SPEED       (100)      /* 直行速度 */
#define TURN_SPEED      (350)      /* 原地转向速度 */

#define TURN_GAIN       (90)       /* 偏差 1 格 -> 左右差 45（8 路偏差最大 ±7） */

#define VISION_CENTER_X   (320)    /* 图像中心 x，640 宽的一半 */
#define VISION_DEAD_X     (40)     /* 目标在中心 ±40 像素内视为已对准，停车 */
#define VISION_MIN_AREA   (900)    /* 色块像素数小于此值视为未找到 */
#define VISION_MIN_TURN   (180)    /* 最小旋转速度，太低电机带不动 */
#define VISION_MAX_TURN   (350)    /* 最大旋转速度 */
#define VISION_GAIN       (3)      /* 偏差 1 像素 -> 3 单位速度 */

/* 把角速度格式化成固定 7 个字符：符号 + 3 位整数 + '.' + 1 位小数 + 一个空格。
 * 固定宽度是为了每次覆盖写时不留下上一帧的残影 */
static void gyro_format(char *s, float v)
{
    int32_t  x10 = (int32_t)(v * 10.0f);
    uint32_t a, ip;
    uint8_t  i = 0;

    if (x10 < 0) {
        s[i++] = '-';
        x10 = -x10;
    } else {
        s[i++] = ' ';
    }

    a  = (uint32_t)x10;
    ip = a / 10;

    s[i++] = (ip >= 100) ? (char)('0' + (ip / 100) % 10) : ' ';
    s[i++] = (ip >= 10)  ? (char)('0' + (ip / 10) % 10)  : ' ';
    s[i++] = (char)('0' + (ip % 10));
    s[i++] = '.';
    s[i++] = (char)('0' + (a % 10));
    s[i++] = ' ';
    s[i]   = '\0';
}

int main(void)
{
    uint8_t running = 0;            /* 0=停车  1=循迹中 */
    uint8_t bt_mode = 0;            /* 0=停车  1=蓝牙遥控 */
    uint8_t cv_mode = 0;            /* 0=停车  1=视觉追踪 */
    int     cmd     = 'X';          /* 蓝牙当前指令，松手后保持 */
    int     lost_ms = 0;            /* 连续丢线累计时长 */
    uint8_t gyro_div = 0;           /* 陀螺仪显示分频计数 */

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

    /* 标定期间（约 0.4s）车必须完全静止，否则零偏会被写歪 */
    bmi088_init_gyro();
    bmi088_calib_gyro(200);

    while (1) {
        key_id_t k = key_scanf();
        if (k == KEY_1) {           /* KEY1 启动循迹 */
            running     = 1;
            bt_mode     = 0;
            cv_mode     = 0;
            lost_ms     = 0;
        }
        if (k == KEY_2) {           /* KEY2 进入蓝牙遥控 */
            running     = 0;
            bt_mode     = 1;
            cv_mode     = 0;
            cmd         = 'X';
        }
        if (k == KEY_3) {           /* KEY3 进入视觉追踪 */
            running     = 0;
            bt_mode     = 0;
            cv_mode     = 1;
            lost_ms     = 0;
        }
        if (k == KEY_4) {           /* KEY4 停车 */
            running     = 0;
            bt_mode     = 0;
            cv_mode     = 0;
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
        } else if (cv_mode) {
            uint8_t  payload[K230_PAYLOAD_MAX];
            uint8_t  len;
            uint8_t  found = 0;
            uint8_t  got   = 0;
            uint16_t cx    = VISION_CENTER_X;
            uint16_t area  = 0;

            /* 把队列里的帧全部取完，只保留最新一帧 */
            while (k230_read_frame(payload, &len)) {
                if (len >= 7) {
                    found = payload[0];
                    cx    = (uint16_t)(payload[1] | ((uint16_t)payload[2] << 8));
                    area  = (uint16_t)(payload[5] | ((uint16_t)payload[6] << 8));
                    got   = 1;
                }
            }

            /* 收到有效帧就翻转 LED4，用来判断串口链路是否正常 */
            if (got) {
                DL_GPIO_togglePins(GPIO_LED_STATUS_PORT, GPIO_LED_STATUS_LED4_PIN);
            }

            if (found && area >= VISION_MIN_AREA) {
                /* 比例控制：偏得越多转得越快，接近中心时降速，避免全速冲过死区 */
                int16_t err = (int16_t)((int16_t)cx - VISION_CENTER_X);
                int16_t mag = (err < 0) ? (int16_t)(-err) : err;
                int32_t spd;

                if (mag <= VISION_DEAD_X) {
                    motor_stop_all();                                     /* 已对准中心 */
                } else {
                    spd = (int32_t)mag * VISION_GAIN;
                    if (spd < VISION_MIN_TURN) { spd = VISION_MIN_TURN; }
                    if (spd > VISION_MAX_TURN) { spd = VISION_MAX_TURN; }

                    if (err < 0) {
                        motor_drive((int16_t)(-spd), (int16_t)spd);       /* 左转 */
                    } else {
                        motor_drive((int16_t)spd, (int16_t)(-spd));       /* 右转 */
                    }
                }
            } else {
                motor_stop_all();
            }
        } else {
            char buf[10];

            motor_stop_all();

            /* 空闲时每 200ms 刷一次屏：整屏刷新约 26ms，太频繁会拖慢按键响应 */
            if (++gyro_div >= 20) {
                gyro_div = 0;
                gyro_format(buf, bmi088_gyro_z_dps());
                OLED_show_string(0, 0, "GYRO Z");
                OLED_show_string(0, 2, buf);
                OLED_refresh();
            }
        }

        delay_cycles(DELAY_1MS * LOOP_MS);
    }
}



