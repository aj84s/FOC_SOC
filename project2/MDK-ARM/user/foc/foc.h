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
void FOC_velocityOpenLoop(float target_velocity,float Uq);

extern uint8_t motor_enable;
void FOC_Motor_Toggle(void);

/* 电角度(rad)，供外部做正变换 */
extern float electrical_angle;

/* 正变换：三相电流 + 电角度 → Id, Iq */
void FOC_Current2DQ(float Ia, float Ib, float Ic, float angle_el,
                    float *Id, float *Iq);

/* PI控制器 */
typedef struct {
    float Kp;
    float Ki;
    float integral;
    float output_max;
    float output_min;
} PI_Controller_t;

void PI_Init(PI_Controller_t *pi, float Kp, float Ki, float out_max);
float PI_Update(PI_Controller_t *pi, float error, float Ts);

/* 电流环闭环 */
void FOC_CurrentLoop(float Iq_ref);
extern float target_iq;

#endif
