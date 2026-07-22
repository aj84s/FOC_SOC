#ifndef __FOC_H
#define __FOC_H

#include "main.h"
#include "math.h"

#define PI 3.1415926f
#define TWO_PI 6.2831853f

typedef struct
{
    float pole_pairs;      // 极对数
    float voltage_power;   // 母线电压
    float Uq;              // 输出电压
}FOC_Param_t;

void FOC_Init(void);
void FOC_velocityOpenLoop(float target_velocity);

#endif
