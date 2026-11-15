#include "ti_msp_dl_config.h"
#include "motor.h"
#include "uart.h"
#include "key.h"
#include "trace.h"
#include "oled.h"

#define DELAY_1MS       (32000)

#define LOOP_MS         (10)       /*丢线时间*/
#define LOST_STOP_MS    (500)      /*丢线时间超过0.5s就停车*/

#define RUN_SPEED       (100)      /* 直行速度 */
#define TURN_SPEED      (350)      /* 原地转向速度 */

#define TURN_GAIN       (90)       /* 偏差 1 格 -> 左右差 45（8 路偏差最大 ±7） */



int main(void)
{
    uint8_t running = 0;            /* 0=停车  1=循迹中 */
    uint8_t bt_mode = 0;            /* 0=停车  1=蓝牙遥控 */
    int     cmd     = 'X';          /* 蓝牙当前指令，松手后保持 */
    int     lost_ms = 0;            /* 连续丢线累计时长 */

    SYSCFG_DL_init();
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
            running = 1;
            bt_mode = 0;
            lost_ms = 0;
        }
        if (k == KEY_2) {           /* KEY2 进入蓝牙遥控 */
            running = 0;
            bt_mode = 1;
            cmd     = 'X';
        }
        if (k == KEY_3) {           /* KEY3 停车 */
            running = 0;
            bt_mode = 0;
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
        } else {
            motor_stop_all();
        }

        delay_cycles(DELAY_1MS * LOOP_MS);
    }
}



