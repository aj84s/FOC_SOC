/**
 ******************************************************************************
 * @file    foc.h
 * @brief   FOC (Field-Oriented Control) 电机控制模块
 * @note    TIM1 PWM输出 + ADC电流采样 + Clark/Park变换 + PID电流环
 ******************************************************************************
 */

#ifndef __FOC_H
#define __FOC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/** @brief FOC 控制状态 */
typedef enum {
    FOC_STATE_IDLE  = 0x00,   /* 未启动 */
    FOC_STATE_ALIGN = 0x01,   /* 预对齐 */
    FOC_STATE_RUN   = 0x02,   /* 闭环运行 */
    FOC_STATE_FAULT = 0x03,   /* 故障停机 */
} FOC_State_t;

/** @brief 电机运行参数 */
typedef struct {
    int16_t  target_speed;      /* 目标转速 (RPM) */
    int16_t  actual_speed;      /* 实际转速 (RPM) */
    int16_t  iq_ref;            /* q轴参考电流 */
    int16_t  iq_actual;         /* q轴实际电流 */
    int16_t  id_ref;            /* d轴参考电流 */
    int16_t  id_actual;         /* d轴实际电流 */
    uint16_t bus_voltage;       /* 母线电压 (mV) */
    uint16_t throttle;          /* 油门输入 (0-4095 ADC) */
    FOC_State_t state;          /* 当前状态 */
} MotorParam_t;

/* Exported constants --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/** @brief FOC 模块初始化 (TIM1 PWM + ADC + OPAMP) */
void FOC_Init(void);

/** @brief FOC 电流环主循环 (TIM1 中断触发, 20kHz) */
void FOC_CurrentLoop(void);

/** @brief 设置目标转速 */
void FOC_SetTargetSpeed(int16_t rpm);

/** @brief 启动电机 */
void FOC_Start(void);

/** @brief 停止电机 (惯性停车) */
void FOC_Stop(void);

/** @brief 紧急停机 (刹车) */
void FOC_EmergencyStop(void);

/** @brief 获取当前电机参数 */
MotorParam_t FOC_GetMotorParam(void);

/* Private defines -----------------------------------------------------------*/

#endif /* __FOC_H */
