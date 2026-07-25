/**
 ******************************************************************************
 * @file    current.h
 * @brief   三相电流采集模块 (OPAMP + ADC + DMA)
 * @note    Rshunt=3mΩ, 运放增益=10×, 偏置=1.65V
 *          ADC1: U相(CH3) + W相(CH12), ADC2: V相(CH3)
 *          TIM1_TRGO 触发, DMA循环
 ******************************************************************************
 */

#ifndef __CURRENT_H
#define __CURRENT_H

#include "main.h"

/* DMA 循环缓存 */
extern uint16_t adc1_buf[2];  /* [0]=U相, [1]=W相 */
extern uint16_t adc2_buf[1];  /* [0]=V相 */

/* 函数声明 */
void Current_Init(void);
void Current_Calibrate(void);
void Current_ReadAll(float *Iu, float *Iv, float *Iw);

#endif /* __CURRENT_H */
