/**
 ******************************************************************************
 * @file    current.c
 * @brief   三相电流采集 (OPAMP→ADC→DMA)
 * @note    硬件: Rshunt=3mΩ, 运放Gain=10, Vbias=1.65V, 12bit ADC@3.3V
 *          量程 ±48A, 换算: I(A) = (Raw*3.3/4095 - 1.65) / 0.03
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "current.h"
#include "adc.h"
#include "opamp.h"

/* 硬件参数 -------------------------------------------------*/
#define ADC_VREF      3.3f
#define ADC_RES       4095.0f
#define V_BIAS        1.65f
#define OPAMP_GAIN    10.0f
#define R_SHUNT       0.003f
#define DENOMINATOR   (OPAMP_GAIN * R_SHUNT)  /* 0.03 */

#define CALIB_SAMPLES 100    /* 校准采样次数 */
#define CALIB_DELAY   1      /* 每次采样间隔 ms */

/* DMA 循环缓存 ----------------------------------------------*/
uint16_t adc1_buf[2];  /* [0]=U相(ADC1_CH3), [1]=W相(ADC1_CH12) */
uint16_t adc2_buf[1];  /* [0]=V相(ADC2_CH3) */

/* 零偏值: 各相静止时的 ADC Raw 实测值 (理想为2048) */
static float adc_zero[3] = {2048.0f, 2048.0f, 2048.0f};

/**
 * @brief  ADC原始值 → 相电流(A)
 * @param  raw  ADC原始值
 * @param  zero 该相零偏校准值
 */
static float RawToCurrent(uint16_t raw, float zero)
{
    float raw_cal = (float)raw - zero + 2048.0f;
    float v_op    = raw_cal * ADC_VREF / ADC_RES;
    float delta   = v_op - V_BIAS;
    return delta / DENOMINATOR;
}

/**
 * @brief  启动 ADC1/ADC2 的 DMA 循环采集
 * @note   在 OPAMP + ADC 初始化之后调用
 */
void Current_Init(void)
{
    /* 启动三个OPAMP，使能放大器输出 */
    HAL_OPAMP_Start(&hopamp1);
    HAL_OPAMP_Start(&hopamp2);
    HAL_OPAMP_Start(&hopamp3);

    /* 启动ADC-DMA循环采集，等待TIM1_TRGO触发 */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc1_buf, 2);
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc2_buf, 1);
}

/**
 * @brief  零偏校准: 电机静止时采100次取均值
 * @note   TIM1 PWM必须已启动(TRGO触发ADC)，但电机不能转动(无电流)
 */
void Current_Calibrate(void)
{
    float sum[3] = {0};

    HAL_Delay(20);  /* 等待DMA缓冲填满首批数据 */

    for (int i = 0; i < CALIB_SAMPLES; i++)
    {
        sum[0] += adc1_buf[0];  /* U相 */
        sum[1] += adc2_buf[0];  /* V相 */
        sum[2] += adc1_buf[1];  /* W相 */
        HAL_Delay(CALIB_DELAY);
    }

    adc_zero[0] = sum[0] / CALIB_SAMPLES;
    adc_zero[1] = sum[1] / CALIB_SAMPLES;
    adc_zero[2] = sum[2] / CALIB_SAMPLES;
}

/**
 * @brief  读取三相电流 (从 DMA buffer)
 * @param  Iu/Iv/Iw: 输出, 单位 A
 */
void Current_ReadAll(float *Iu, float *Iv, float *Iw)
{
    *Iu = RawToCurrent(adc1_buf[0], adc_zero[0]);
    *Iv = RawToCurrent(adc2_buf[0], adc_zero[1]);
    *Iw = RawToCurrent(adc1_buf[1], adc_zero[2]);
}
