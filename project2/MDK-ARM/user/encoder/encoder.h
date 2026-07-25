#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"

extern volatile float    encoder_speed_rpm;   // 当前转速 (RPM)
extern volatile uint32_t encoder_last_tick;   // 最后一次hall边沿的系统tick

void Encoder_Init(void);
void Encoder_IC_Callback(TIM_HandleTypeDef *htim);
float Encoder_GetSpeed(void);

#endif
