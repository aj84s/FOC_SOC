#ifndef __KEY_H__
#define __KEY_H__

#include "main.h"

/* 按键返回值定义 */
#define KEY_NONE    0   /* 无按键按下 */
#define KEY1_PRESS  1   /* KEY1 按下 */
#define KEY2_PRESS  2   /* KEY2 按下 */
#define KEY3_PRESS  3   /* KEY3 按下 */

uint8_t KEY_Scan(void);

#endif /* __KEY_H__ */
