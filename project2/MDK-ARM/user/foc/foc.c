#include "foc.h"
#include "tim.h"
#include "current.h"
#include <math.h>

// FOC参数结构体
FOC_Param_t foc =
{
    .pole_pairs = 2,
    .voltage_power = 11.5f,
    .Uq = 3.0f
};

// 电机的机械角度和电角度
float shaft_angle = 0;
float electrical_angle = 0;

// 目标转速和目标电压
volatile float target_rpm = 500.0f;
volatile float target_Uq  = 6.0f;

// PI控制器实例
PI_Controller_t pi_id;
PI_Controller_t pi_iq;

// Q轴P值
float PID_Pvalue_Q = 0.05f;

// 电流环目标值
float target_iq = 1.0f;  /* 启动默认0.3A */

// 电机启停状态
uint8_t motor_enable = 0;

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

    /* 初始化PI控制器 */
    PI_Init(&pi_id, 0.5f, 10.0f, 5.7f);  /* Id环: Kp=0.5, Ki=10, ±5.7V */
    PI_Init(&pi_iq, PID_Pvalue_Q, 0.0f, 5.7f);  /* Iq环: Kp=1.0, Ki=50, ±5.7V */
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
    inversePark(Uq, Ud, angle_el, &Ualpha, &Ubeta);
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
 * @param  target_velocity 目标机械角速度 (rpm)
 * @retval 当前输出Uq电压
 */
void FOC_velocityOpenLoop(float target_velocity,float Uq)
{
//		//不使用TIM中断时
//    static uint32_t last_time = 0;
//    uint32_t now = HAL_GetTick();

//    // 计算周期时间 s
//    float Ts = (now - last_time) * 0.001f;
//    // 超时保护：防止第一次运行、长时间卡死造成角度跳跃
//    if (Ts <= 0.0f || Ts > 0.5f)
//    {
//        Ts = 0.001f;
//    }
//    last_time = now;
	
		//使用TIM中断时,Ts固定为时钟周期
		float Ts = 0.001f;   // TIM2 = 1kHz
		
    // rpm —— rad/s
		target_velocity = target_velocity/60.0f*6.28f;
	
		// 积分更新机械角度
    shaft_angle += target_velocity * Ts;
    shaft_angle = normalizeAngle(shaft_angle);

    // 机械角度 -> 电角度
    electrical_angle = shaft_angle * foc.pole_pairs;
    electrical_angle = normalizeAngle(electrical_angle);

    // 开环给定 Uq，Ud=0
		//float Uq = 2.3f;
    FOC_SetPhaseVoltage(Uq, 0.0f, electrical_angle);
}

/**
 * @brief  正Clarke变换: abc → αβ (等幅值)
 *         Iα = Ia
 *         Iβ = (Ia + 2*Ib) / √3
 * @note   Ic 冗余 (平衡时 Ia+Ib+Ic=0)
 */
static void forwardClarke(float Ia, float Ib, float Ic,
                          float *Ialpha, float *Ibeta)
{
    *Ialpha = Ia;
    *Ibeta  = (Ia + 2.0f * Ib) * 0.577350269f;  /* 1/√3 */
    (void)Ic;
}

/**
 * @brief  正Park变换: αβ → dq
 *         Id =  Iα·cosθ + Iβ·sinθ
 *         Iq = -Iα·sinθ + Iβ·cosθ
 */
static void forwardPark(float Ialpha, float Ibeta, float angle_el,
                        float *Id, float *Iq)
{
    float s = sinf(angle_el);
    float c = cosf(angle_el);
    *Id =  Ialpha * c + Ibeta * s;
    *Iq = -Ialpha * s + Ibeta * c;
}

/**
 * @brief  三相电流 → dq轴电流 (正Clarke + 正Park一步完成)
 * @param  angle_el  电角度(rad)
 */
void FOC_Current2DQ(float Ia, float Ib, float Ic, float angle_el,
                    float *Id, float *Iq)
{
    float Ialpha, Ibeta;
    forwardClarke(Ia, Ib, Ic, &Ialpha, &Ibeta);
    forwardPark(Ialpha, Ibeta, angle_el, Id, Iq);
}

/**
 * @brief  PI控制器初始化
 * @param  out_max  输出限幅绝对值 (out_min = -out_max)
 */
void PI_Init(PI_Controller_t *pi, float Kp, float Ki, float out_max)
{
    pi->Kp = Kp;
    pi->Ki = Ki;
    pi->integral = 0.0f;
    pi->output_max =  out_max;
    pi->output_min = -out_max;
}

/**
 * @brief  PI控制器更新 (条件积分抗饱和)
 * @param  error  设定值 - 实际值
 * @param  Ts     采样周期 (s)
 * @retval 控制输出
 */
float PI_Update(PI_Controller_t *pi, float error, float Ts)
{
    float P_out = pi->Kp * error;

    /* 条件积分: 输出未饱和 或 误差反向(退饱和)时累加 */
    float out = P_out + pi->integral;
    if ((out < pi->output_max && out > pi->output_min) ||
        (error * pi->integral < 0))
    {
        pi->integral += pi->Ki * error * Ts;
    }

    /* 积分限幅 */
    if (pi->integral > pi->output_max)  pi->integral = pi->output_max;
    if (pi->integral < pi->output_min)  pi->integral = pi->output_min;

    /* 总输出限幅 */
    out = P_out + pi->integral;
    if (out > pi->output_max)  out = pi->output_max;
    if (out < pi->output_min)  out = pi->output_min;

    return out;
}

/**
 * @brief  电流闭环控制 (读电流→Clarke/Park→PI→PWM)
 * @param  Iq_ref  目标q轴电流 (A)
 * @note   替代 FOC_velocityOpenLoop，电角度仍由开环积分产生
 */
void FOC_CurrentLoop(float Iq_ref)
{
    float Ia, Ib, Ic, Id, Iq;
    float Ud, Uq;
    float Ts = 0.001f;  /* TIM2 = 1kHz */

    /* 1. 读取三相电流 → Id, Iq */
    Current_ReadAll(&Ia, &Ib, &Ic);
    FOC_Current2DQ(Ia, Ib, Ic, electrical_angle, &Id, &Iq);

    /* 2. 更新电角度 (开环积分，维持旋转磁场) */
    shaft_angle += target_rpm / 60.0f * TWO_PI * Ts;
    shaft_angle = normalizeAngle(shaft_angle);
    electrical_angle = shaft_angle * foc.pole_pairs;
    electrical_angle = normalizeAngle(electrical_angle);

    /* 3. PI调节: Id→0, Iq→Iq_ref */
    Ud = PI_Update(&pi_id, 0.0f - Id, Ts);
    Uq = PI_Update(&pi_iq, Iq_ref - Iq, Ts);

    /* 4. 输出PWM */
    FOC_SetPhaseVoltage(Uq, Ud, electrical_angle);
}

/**
 * @brief  电机启停切换
 * @note   KEY3触发，停止时关断三相PWM
 */
void FOC_Motor_Toggle(void)
{
    motor_enable = !motor_enable;
    if (!motor_enable)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);

        /* 清零PI积分，防止下次启动瞬态冲击 */
        pi_id.integral = 0.0f;
        pi_iq.integral = 0.0f;
    }
}
