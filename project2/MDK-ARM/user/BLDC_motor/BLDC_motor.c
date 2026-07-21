#include "BLDC_motor.h"

// 换相表：定义二维数组，每一行表示一个换相步骤，每一列表示对应的六路PWM是否输出
static const uint8_t comm_table[6][6] = {
    {1, 0, 0, 1, 0, 0},  // Step0: U+ V-
    {1, 0, 0, 0, 0, 1},  // Step1: U+ W-
    {0, 0, 1, 0, 0, 1},  // Step2: V+ W-
    {0, 1, 1, 0, 0, 0},  // Step3: V+ U-
    {0, 1, 0, 0, 1, 0},  // Step4: W+ U-
    {0, 0, 0, 1, 1, 0}   // Step5: W+ V-
};

// 定义生成电机节拍的时钟句柄
TIM_HandleTypeDef *htim_BLDC;

// 定义电机转动方向枚举变量
static MOTOR_DIRTypeDef BLDC_dir = MOTOR_DIR_FORWARD;  // 默认正转

// 定义电机转动状态枚举变量
static MOTOR_STATETypeDef BLDC_state = MOTOR_STATE_STOP;  // 默认停止

// 定义当前位置变量
static uint8_t BLDC_current_step = 0;  // 当前步数

// 定义单步运行时间变量：tick，定时器周期/节拍
static uint32_t BLDC_step_time = 50;  // 默认单步运行时间为50个时钟周期

// 定义ticks变量
static uint32_t BLDC_ticks = 0;  // 用于计数节拍的变量


// 关闭所有的PWM通道
static void BLDC_ALLPWMChanel_OFF(void)
{
    if(htim_BLDC == NULL)
    {
        // 错误处理：传入的定时器句柄为空
        return;
    }
    // 关闭三相桥的上半桥臂
    HAL_TIM_PWM_Stop(htim_BLDC, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(htim_BLDC, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(htim_BLDC, TIM_CHANNEL_3);
    // 关闭三相桥的下半桥臂
    HAL_TIMEx_PWMN_Stop(htim_BLDC, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(htim_BLDC, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(htim_BLDC, TIM_CHANNEL_3);
}

// 1. 电机驱动初始化函数
void BLDC_Motor_Init(TIM_HandleTypeDef *htim)
{
    if (htim == NULL)
    {
        // 错误处理：传入的定时器句柄为空
        return;
    }
    BLDC_dir = MOTOR_DIR_FORWARD;
    BLDC_state = MOTOR_STATE_STOP;
    htim_BLDC = htim;
    BLDC_current_step = 0;
    BLDC_step_time = 50;
    BLDC_ticks = 0;

    // 关闭所有的PWM通道
    BLDC_ALLPWMChanel_OFF();
}

// 2. 设置电机转动方向函数
void BLDC_Motor_SetDir(MOTOR_DIRTypeDef direction)
{
    BLDC_dir = direction;
}

// 3. 设置电机转动状态函数
void BLDC_Motor_SetState(MOTOR_STATETypeDef state)
{
    BLDC_state = state;
    if (state == MOTOR_STATE_STOP)
    {
        // 停止电机时，关闭所有的PWM通道
        BLDC_ALLPWMChanel_OFF();
    }
}

// 4. 读取电机转动方向函数
MOTOR_DIRTypeDef BLDC_Motor_GetDir(void)
{
    return BLDC_dir;
}

// 5. 读取电机转动状态函数
MOTOR_STATETypeDef BLDC_Motor_GetState(void)
{
    return BLDC_state;
}

// 6. 单步换相函数
void BLDC_Motor_OnceStep(uint8_t step)
{
    if (htim_BLDC == NULL)
    {
        // 错误处理：定时器句柄未初始化
        return;
    }

    // 关闭所有的PWM通道
    BLDC_ALLPWMChanel_OFF();

    // 暂时定义占空比为50%
    uint16_t duty = 50;

    // 设置统一占空比
    __HAL_TIM_SET_COMPARE(htim_BLDC, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(htim_BLDC, TIM_CHANNEL_2, duty);
    __HAL_TIM_SET_COMPARE(htim_BLDC, TIM_CHANNEL_3, duty);

    // 第二步：开启当前步需要的通道
    for (uint8_t i = 0; i < 6; i++)
    {
        if (comm_table[BLDC_current_step][i])
        {
            switch (i)
            {
                // 上半桥臂
                case 0: HAL_TIM_PWM_Start(htim_BLDC, TIM_CHANNEL_1); break;  // U+
                case 2: HAL_TIM_PWM_Start(htim_BLDC, TIM_CHANNEL_2); break;  // V+
                case 4: HAL_TIM_PWM_Start(htim_BLDC, TIM_CHANNEL_3); break;  // W+
                // 下半桥臂
                case 1: HAL_TIMEx_PWMN_Start(htim_BLDC, TIM_CHANNEL_1); break;// U-
                case 3: HAL_TIMEx_PWMN_Start(htim_BLDC, TIM_CHANNEL_2); break;// V-
                case 5: HAL_TIMEx_PWMN_Start(htim_BLDC, TIM_CHANNEL_3); break;// W-
                default: break;
            }
        }
    }
}

// 7. 执行单步换相函数
void BLDC_Motor_ExcuteStep(void)
{
    if (BLDC_dir == MOTOR_DIR_FORWARD)
    {
        BLDC_current_step = (BLDC_current_step + 1) % 6;  // 正转
    }
    else
    {
        BLDC_current_step = (BLDC_current_step + 5) % 6;  // 反转
    }

    // 执行单步换相
    BLDC_Motor_OnceStep(BLDC_current_step);
}

// 8. 设置单步运行时间函数
void BLDC_Motor_SetStepTime(uint32_t time)
{
    BLDC_step_time = time;
}

// 9. 电机节拍执行函数
void BLDC_Motor_Tick(void)
{
    switch(BLDC_state)
    {
        // 停止状态，关闭所有PWM输出
        case MOTOR_STATE_STOP:
            BLDC_ALLPWMChanel_OFF();
            break;
        // 单步运行状态，按单步执行时间持续输出PWM信号
        case MOTOR_STATE_STEP_MOVE: 
            if (BLDC_ticks == 0)
            {
                //执行一步
                BLDC_Motor_ExcuteStep();
            }
            BLDC_ticks++;
            if (BLDC_ticks >= BLDC_step_time)
            {
                // ticks清零
                BLDC_ticks = 0;
                // 达到单步运行时间，停止输出PWM信号
                BLDC_ALLPWMChanel_OFF();
                BLDC_state = MOTOR_STATE_STOP;
            }
            break;
        // 连续运行状态，持续输出PWM信号
        case MOTOR_STATE_CONT_MOVE:
            BLDC_ticks++;
            if (BLDC_ticks >= BLDC_step_time)
            {
                // ticks清零
                BLDC_ticks = 0;
                // 执行下一步换相
                BLDC_Motor_ExcuteStep();
            }
            break;
        default:
            // 其他状态，不执行任何操作
            BLDC_ALLPWMChanel_OFF();
            break;
    }
}
