#include "foc.h"
#include "tim.h"
#include <math.h>

// FOC参数结构体
FOC_Param_t foc =
{
    .pole_pairs = 7,
    .voltage_power = 12.0f,
    .Uq = 3.0f
};

// 电机的机械角度和电角度
float shaft_angle = 0;
float electrical_angle = 0;

//
void FOC_Init(void)
{
    // 高边
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

	// 低边互补
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
}

// 角度归到0~2π范围内
static float normalizeAngle(float angle)
{
    angle = fmodf(angle, TWO_PI);
    if (angle < 0.0f)
        angle += TWO_PI;
    return angle;
}

// 1. 逆帕克变换
// 输入: Uq, Ud - 旋转坐标系下的电压分量
//       angle_el - 电角度
// 输出: Ualpha, Ubeta - 静止坐标系下的电压分量
static void inversePark(
    float Uq, float Ud, float angle_el,
    float *Ualpha, float *Ubeta)
{
    // 逆帕克变换公式
    *Ualpha = Ud * cos(angle_el) - Uq * sin(angle_el);
    *Ubeta  = Ud * sin(angle_el) + Uq * cos(angle_el);
}

// 2. 逆克拉克变换
// 输入: Ualpha, Ubeta - 静止坐标系下的电压分量
// 输出: Ua, Ub, Uc - 三相电压
static void inverseClarke(
    float Ualpha, float Ubeta,
    float *Ua, float *Ub, float *Uc)
{
    // 逆克拉克变换公式
    *Ua = Ualpha;
    *Ub = -0.5f * Ualpha + (sqrt(3)/2) * Ubeta;
    *Uc = -0.5f * Ualpha - (sqrt(3)/2) * Ubeta;
}

// 3. 设置三相PWM占空比
// 输入: Ua, Ub, Uc - 三相电压
// 内置操作: 将电压转换为占空比，并设置到定时器的比较寄存器
// 输出: 无
void FOC_SetPWM(float Ua, float Ub, float Uc)
{
    float duty_a;
    float duty_b;
    float duty_c;

    // 将电压转换为归一化占空比
    duty_a = Ua / foc.voltage_power;
    duty_b = Ub / foc.voltage_power;
    duty_c = Uc / foc.voltage_power;

    // 限制占空比在0~1范围内
    duty_a = duty_a > 1 ? 1 : (duty_a < 0 ? 0 : duty_a);
    duty_b = duty_b > 1 ? 1 : (duty_b < 0 ? 0 : duty_b);
    duty_c = duty_c > 1 ? 1 : (duty_c < 0 ? 0 : duty_c);

    // 获取定时器的自动重装载值
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

    // 设置PWM占空比
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty_a*arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty_b*arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, duty_c*arr);
}

/**
 * @brief  设置d/q轴目标电压，生成三相PWM输出
 * @param  Uq    q轴电压
 * @param  Ud    d轴电压
 * @param  angle_el 电角度(rad)
 */
void FOC_SetPhaseVoltage(float Uq, float Ud, float angle_el)
{
    float Ualpha, Ubeta;
    float Ua, Ub, Uc;

    // 1. 电角度归一化至 [0, 2π)
    angle_el = normalizeAngle(angle_el);

    // 2. 反Park变换 dq -> αβ
    inversePark(Ud, Uq, angle_el, &Ualpha, &Ubeta);
    // 3. 反Clarke变换 αβ -> abc三相电压
    inverseClarke(Ualpha, Ubeta, &Ua, &Ub, &Uc);

    // 4. 反Clarke输出电压范围：[-Vbus/2 , +Vbus/2]
    //PWM只能输出0~Vbus，叠加直流偏置抬升到正区间
    float v_half = foc.voltage_power / 2.0f;
    Ua += v_half;
    Ub += v_half;
    Uc += v_half;

    // 5. 更新三相PWM占空比
    FOC_SetPWM(Ua, Ub, Uc);
}

/**
 * @brief  开环速度运行函数
 * @param  target_velocity 目标机械角速度 (rad/s)
 * @retval 当前输出Uq电压
 */
void FOC_velocityOpenLoop(float target_velocity)
{
    static uint32_t last_time = 0;
    uint32_t now = HAL_GetTick();

    // 计算周期时间 s
    float Ts = (now - last_time) * 0.001f;
    // 超时保护：防止第一次运行、长时间卡死造成角度跳跃
    if (Ts <= 0.0f || Ts > 0.5f)
    {
        Ts = 0.001f;
    }
    last_time = now;

    // 积分更新机械角度
    shaft_angle += target_velocity * Ts;
    shaft_angle = normalizeAngle(shaft_angle);

    // 机械角度 -> 电角度
    electrical_angle = shaft_angle * foc.pole_pairs;
    electrical_angle = normalizeAngle(electrical_angle);

    // 开环给定 Uq，Ud=0
    //float Uq = foc.voltage_power / 6.0f;
	float Uq = 2.0f;
    FOC_SetPhaseVoltage(Uq, 0.0f, electrical_angle);
}
