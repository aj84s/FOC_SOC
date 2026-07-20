#ifndef __BLDC_MOTOR_H__
#define __BLDC_MOTOR_H__

#include "main.h"

// 定义电机转动方向枚举
typedef enum
{
    MOTOR_DIR_FORWARD = 0,    // 正转
    MOTOR_DIR_BACKWARD        // 反转
} MOTOR_DIRTypeDef;

// 定义电机转动状态枚举
typedef enum
{
    MOTOR_STATE_STOP = 0,       // 停止
    MOTOR_STATE_STEP_MOVE,      // 单步运行
    MOTOR_STATE_CONT_MOVE       // 连续运行
} MOTOR_STATETypeDef;

// 1. 电机初始化函数
void BLDC_Motor_Init(TIM_HandleTypeDef *htim);

// 2. 设置电机转动方向函数
void BLDC_Motor_SetDir(MOTOR_DIRTypeDef direction);

// 3. 设置电机转动状态函数
void BLDC_Motor_SetState(MOTOR_STATETypeDef state);

// 4. 读取电机转动方向函数
MOTOR_DIRTypeDef BLDC_Motor_GetDir(void);

// 5. 读取电机转动状态函数
MOTOR_STATETypeDef BLDC_Motor_GetState(void);

// 6. 单步换相函数
void BLDC_Motor_OnceStep(uint8_t step);

// 7. 执行单步换相函数
void BLDC_Motor_ExcuteStep(void);

// 8. 设置单步运行时间函数
void BLDC_Motor_SetStepTime(uint32_t time);

// 9. 电机节拍执行函数
void BLDC_Motor_Tick(void);

#endif /* __BLDC_MOTOR_H__ */
