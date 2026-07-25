#include "encoder.h"
#include "tim.h"

/* ==================== 常量定义 ==================== */
#define TIM3_FREQ         1000000.0f   // 定时器频率 1MHz (170MHz / 170)
#define HALL_EDGES        12           // 每机械转霍尔边沿数 = 6 * 2极对

/* RPM = 60 * TIM3_FREQ / (Δcnt * HALL_EDGES) = 5,000,000 / Δcnt */
#define RPM_CONSTANT      5000000.0f

/* 超过此时间无hall边沿则认为电机已停止 */
#define SPEED_TIMEOUT_MS  200

/* ==================== 全局变量 ==================== */
volatile float    encoder_speed_rpm  = 0.0f;
volatile uint32_t encoder_last_tick = 0;

/* ==================== 函数实现 ==================== */

/**
 * @brief  初始化编码器模块
 */
void Encoder_Init(void)
{
    encoder_speed_rpm  = 0.0f;
    encoder_last_tick  = HAL_GetTick();

    HAL_TIMEx_HallSensor_Start_IT(&htim3);
}

/**
 * @brief  TIM3霍尔边沿中断回调 — 仅测速
 */
void Encoder_IC_Callback(TIM_HandleTypeDef *htim)
{
    (void)htim;

    uint32_t delta_cnt = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);

    if (delta_cnt == 0)
        return;

    encoder_speed_rpm  = RPM_CONSTANT / (float)delta_cnt;
    encoder_last_tick  = HAL_GetTick();
}

/**
 * @brief  获取当前转速，超时自动归零
 */
float Encoder_GetSpeed(void)
{
    if (HAL_GetTick() - encoder_last_tick > SPEED_TIMEOUT_MS)
    {
        encoder_speed_rpm = 0.0f;
    }
    return encoder_speed_rpm;
}
