#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"

#define ENCODER_POLE_PAIRS  2           // 电机极对数
#define ENCODER_PPR         12          // 每机械转霍尔跳变数 = 6 * pole_pairs

typedef struct
{
    float    speed_rpm;        // 当前转速 (RPM)
    uint8_t  hall_state;       // 当前霍尔状态 (1~6)
    uint8_t  last_hall_state;  // 上一次霍尔状态
    int8_t   direction;        // 方向: 1=正转, -1=反转, 0=未确定
    uint32_t pulse_count;      // 累计霍尔跳变次数
    float    electrical_angle; // 估算电角度 (rad)
} Encoder_t;

extern Encoder_t encoder;

void    Encoder_Init(void);
void    Encoder_IC_Callback(TIM_HandleTypeDef *htim);
float   Encoder_GetSpeed(void);
uint8_t Encoder_GetHallState(void);
float   Encoder_GetElectricalAngle(void);

#endif
