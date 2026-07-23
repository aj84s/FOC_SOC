#include "encoder.h"
#include "tim.h"

/* ==================== 常量定义 ==================== */
#define PI              3.1415926f
#define TIM3_FREQ       1000000.0f   // 定时器频率 1MHz (170MHz / 170)
#define HALL_EDGES      12           // 每机械转霍尔边沿数 = 6 * 2极对

/* 转速常数: RPM = 60 * TIM3_FREQ / (Δcnt * HALL_EDGES)
 *            = 60,000,000 / (Δcnt * 12)
 *            = 5,000,000 / Δcnt                              */
#define RPM_CONSTANT    5000000.0f

/* ==================== 霍尔状态映射表 ==================== */
/* GPIO IDR bit[8:6] (H3,H2,H1) → 霍尔状态 1~6
 * raw = (GPIOC->IDR >> 6) & 0x07
 * ┌──────┬───────────────────┬───────┐
 * │ raw  │  H3  H2  H1      │ state │
 * ├──────┼───────────────────┼───────┤
 * │  2   │   0   1   0       │   1   │
 * │  3   │   0   1   1       │   5   │
 * │  1   │   0   0   1       │   4   │
 * │  5   │   1   0   1       │   6   │
 * │  4   │   1   0   0       │   2   │
 * │  6   │   1   1   0       │   3   │
 * └──────┴───────────────────┴───────┘
 * 注: 实际序列需在硬件上验证校准                              */
static const uint8_t hall_map[8] = {
    0, 4, 1, 5, 2, 6, 3, 0
};

/* ==================== 正转序列表 ==================== */
/* forward_next[curr_state] = 下一个正转状态
 * 正转序列: 1→5→4→6→2→3→1→...                             */
static const uint8_t forward_next[7] = {
    0,    // 0 - 无效
    5,    // 1 → 5
    3,    // 2 → 3
    1,    // 3 → 1
    6,    // 4 → 6
    4,    // 5 → 4
    2,    // 6 → 2
};

/* ==================== 电角度映射表 ==================== */
/* 每个霍尔状态对应的电角度 (弧度)
 * state 1=0°, 5=60°, 4=120°, 6=180°, 2=240°, 3=300°     */
static const float hall_angle_rad[7] = {
    0.0f,
    0.0f,                              // state 1: 0°
    PI * 4.0f / 3.0f,                  // state 2: 240°
    PI * 5.0f / 3.0f,                  // state 3: 300°
    PI * 2.0f / 3.0f,                  // state 4: 120°
    PI / 3.0f,                         // state 5: 60°
    PI,                                // state 6: 180°
};

/* ==================== 全局实例 ==================== */
Encoder_t encoder = {0};

/* ==================== 函数实现 ==================== */

/**
 * @brief  初始化编码器模块
 * @retval 无
 */
void Encoder_Init(void)
{
    /* 清零结构体 */
    encoder.speed_rpm        = 0.0f;
    encoder.hall_state       = 0;
    encoder.last_hall_state  = 0;
    encoder.direction        = 0;
    encoder.pulse_count      = 0;
    encoder.electrical_angle = 0.0f;

    /* 读取初始霍尔状态 */
    uint8_t raw = (GPIOC->IDR >> 6) & 0x07;
    encoder.hall_state      = hall_map[raw];
    encoder.last_hall_state = encoder.hall_state;

    /* 启动TIM3霍尔传感器接口 + CC1中断 */
    HAL_TIMEx_HallSensor_Start_IT(&htim3);
}

/**
 * @brief  TIM3输入捕获回调 (霍尔边沿中断)
 * @param  htim TIM句柄
 * @retval 无
 */
void Encoder_IC_Callback(TIM_HandleTypeDef *htim)
{
    (void)htim;

    /* 1. 读取捕获值: 两次霍尔跳变间的定时器计数值 */
    uint32_t delta_cnt = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);

    if (delta_cnt == 0)
    {
        return;  // 异常保护
    }

    /* 2. 读取当前霍尔状态 */
    uint8_t raw = (GPIOC->IDR >> 6) & 0x07;
    uint8_t hall = hall_map[raw];

    if (hall == 0)
    {
        return;  // 无效霍尔状态
    }

    /* 3. 更新脉冲计数 */
    encoder.pulse_count++;

    /* 4. 判断方向 */
    if (encoder.last_hall_state != 0 && encoder.last_hall_state != hall)
    {
        if (forward_next[encoder.last_hall_state] == hall)
        {
            encoder.direction = 1;   // 正转
        }
        else if (forward_next[hall] == encoder.last_hall_state)
        {
            encoder.direction = -1;  // 反转
        }
    }

    /* 5. 计算转速 RPM */
    encoder.speed_rpm = RPM_CONSTANT / (float)delta_cnt;

    /* 6. 更新电角度 */
    encoder.electrical_angle = hall_angle_rad[hall];

    /* 7. 更新霍尔状态 */
    encoder.last_hall_state = encoder.hall_state;
    encoder.hall_state      = hall;
}

/**
 * @brief  获取当前转速
 * @retval 转速 (RPM)
 */
float Encoder_GetSpeed(void)
{
    return encoder.speed_rpm;
}

/**
 * @brief  获取当前霍尔状态
 * @retval 霍尔状态 (1~6)
 */
uint8_t Encoder_GetHallState(void)
{
    return encoder.hall_state;
}

/**
 * @brief  获取估算的电角度
 * @retval 电角度 (rad)
 */
float Encoder_GetElectricalAngle(void)
{
    return encoder.electrical_angle;
}
