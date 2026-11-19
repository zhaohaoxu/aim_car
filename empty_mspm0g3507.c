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

#define TURN_GAIN       (90)       /* 偏差 1 格 -> 左右差 90（8 路偏差最大 ±7） */

#define K230_DATA_MAX    (32)   /* K230数据最大长度 */

//视觉追踪参数
#define VISION_CENTER_X   (160)    /* 图像中心 x，320 宽的一半 */
#define VISION_DEAD_X     (40)     /* 偏差小于此值只直行，不转向 */
#define VISION_MIN_AREA   (200)    /* 色块像素数小于此值视为未找到 */
#define VISION_STOP_AREA  (6000)   /* 面积大于此值认为已贴近目标，停车 */
#define VISION_MAX_TURN   (350)    /* 最大差速修正量 */
#define VISION_GAIN       (3)      /* 偏差 1 像素 -> 3 单位差速 */
#define VISION_BASE_SPEED (150)    /* 追踪时基础前进速度 */

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

    /* 视觉状态跨循环保持：K230 约 70ms 一帧，主循环 10ms，不能每轮清零 */
    uint8_t  cv_found   = 0;        /* 上一帧是否找到目标 */
    uint16_t cv_cx      = VISION_CENTER_X;  /* 上一帧目标中心 x */
    uint16_t cv_area    = 0;        /* 上一帧目标面积 */
    int      cv_lost_ms = 0;        /* 连续收不到帧的时长 */

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
            cv_found    = 0;
            cv_cx       = VISION_CENTER_X;
            cv_area     = 0;
            cv_lost_ms  = 0;
        }
        if (k == KEY_4) {           /* KEY4 停车 */
            running     = 0;
            bt_mode     = 0;
            cv_mode     = 0;
        }

        if (running) {
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
                motor_drive((int16_t)(RUN_SPEED + diff),(int16_t)(RUN_SPEED - diff));
            }
        } else if (bt_mode) {
            int c = uart_bt_get_char();
            //按键指令
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
        } else if (cv_mode) {
            uint8_t payload[K230_DATA_MAX];//k230数据数据
            uint8_t len;//数据长度
            uint8_t got = 0;//本轮是否收到帧

            /* 把队列里的帧全部取完，只保留最新一帧 */
            while (k230_read_frame(payload, &len)) {
                if (len >= 7) {
                    cv_found = payload[0];
                    cv_cx    = (uint16_t)(payload[1] | ((uint16_t)payload[2] << 8));
                    cv_area  = (uint16_t)(payload[5] | ((uint16_t)payload[6] << 8));
                    got      = 1;
                }
            }

            /* 收到数据翻转LED4；超过 300ms 收不到就认为丢了目标 */
            if (got) {
                DL_GPIO_togglePins(GPIO_LED_STATUS_PORT, GPIO_LED_STATUS_LED4_PIN);
                cv_lost_ms = 0;
            } else {
                cv_lost_ms += LOOP_MS;
                if (cv_lost_ms >= 300) {
                    cv_found = 0;
                }
            }

            /* 处理数据：边走边追 */
            if (cv_found && cv_area >= VISION_MIN_AREA) {
                int16_t err = (int16_t)((int16_t)cv_cx - VISION_CENTER_X);
                int16_t mag = (err < 0) ? (int16_t)(-err) : err;
                int16_t turn;
                int16_t left;
                int16_t right;

                if (cv_area >= VISION_STOP_AREA) {
                    motor_stop_all();                    /* 已贴近目标 */
                } else {
                    /* 偏差越大修正越强，死区内只直行 */
                    turn = 0;
                    if (mag > VISION_DEAD_X) {
                        turn = (int16_t)((mag - VISION_DEAD_X) * VISION_GAIN);
                        if (turn > VISION_MAX_TURN) { turn = VISION_MAX_TURN; }
                    }

                    left  = VISION_BASE_SPEED;
                    right = VISION_BASE_SPEED;
                    if (err < 0) {                       /* 球在左：左轮慢、右轮快 */
                        left  -= turn;
                        right += turn;
                    } else {                             /* 球在右：左轮快、右轮慢 */
                        left  += turn;
                        right -= turn;
                    }
                    motor_drive(left, right);
                }
            } else {
                motor_stop_all();
            }//OLED部分  
        } else {
            char buf[10];

            motor_stop_all();

            /*刷新OLED */
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



