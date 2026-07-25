/**
 ******************************************************************************
 * @file    bms.c
 * @brief   BMS 电池管理系统模块实现
 * @note    INA226驱动 + 库仑计数 + SOC估算 + 电压补偿
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "bms.h"
#include "i2c.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define INA226_CONFIG    0x4527   /* 16avg, 1.1ms, shunt&bus 连续 */
#define INA226_MANUF_ID  0x5449   /* 制造商ID，用于I2C通信验证 */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  INA226 写16位寄存器 (HAL I2C1)
 */
static HAL_StatusTypeDef INA226_WriteReg(uint8_t reg, uint16_t data)
{
    uint8_t buf[3] = {reg, data >> 8, data & 0xFF};
    return HAL_I2C_Master_Transmit(&hi2c1, INA226_ADDR, buf, 3, 10);
}

/**
 * @brief  INA226 读16位寄存器 (HAL I2C1)
 */
static uint16_t INA226_ReadReg(uint8_t reg)
{
    uint8_t buf[2] = {0};
    HAL_I2C_Master_Transmit(&hi2c1, INA226_ADDR, &reg, 1, 10);
    HAL_I2C_Master_Receive(&hi2c1, INA226_ADDR, buf, 2, 10);
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/**
 * @brief  INA226 初始化配置
 *         先读 Manuf ID 验证I2C通信 → 再写Config寄存器
 * @retval 1=成功, 0=失败
 */
static uint8_t BMS_INA226_Init(void)
{
    /* 1. 读 Manufacturer ID (0xFE) 验证 I2C 通信 */
    uint16_t id = INA226_ReadReg(0xFE);
    if (id != INA226_MANUF_ID)
    {
        printf("INA226 err:0x%04X\r\n", id);
        return 0;
    }

    /* 2. 写配置寄存器: 16avg, 1.1ms转换, shunt&bus连续模式 */
    INA226_WriteReg(0x00, INA226_CONFIG);

    return 1;
}

/**
 * @brief  库仑计数更新
 *         积分电流 × 时间增量
 */
static void BMS_CoulombCount(int16_t current_ma)
{
    /* TODO: 库仑计数累积 */
    (void)current_ma;
}

/**
 * @brief  开路电压法校正 SOC
 *         条件: 电流 < 50mA 持续 5s
 */
static uint8_t BMS_OCV_Correct(void)
{
    /* TODO: OCV → SOC 查表校正 */
    return 0;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  BMS 模块初始化
 * @retval 1=成功, 0=INA226通信异常
 */
uint8_t BMS_Init(void)
{
    return BMS_INA226_Init();
}

/**
 * @brief  BMS 主任务 (100ms 周期: 读电压/电流并打印)
 */
void BMS_Task(void)
{
    uint16_t v = BMS_ReadBusVoltage();
    int16_t  i = BMS_ReadBusCurrent();
    printf("V:%dmV I:%dmA\r\n", v, i);
}

/**
 * @brief  获取当前电池数据快照
 */
BMS_Data_t BMS_GetData(void)
{
    BMS_Data_t data;
    data.bus_voltage    = BMS_ReadBusVoltage();
    data.current        = BMS_ReadBusCurrent();
    data.power          = (int32_t)data.bus_voltage * data.current / 1000;
    data.soc            = 0;
    data.coulomb_counter = 0;
    data.temperature    = 0;
    data.state          = BMS_STATE_NORMAL;
    return data;
}

/**
 * @brief  读 INA226 总线电压
 * @retval 总线电压 (mV), LSB=1.25mV
 */
uint16_t BMS_ReadBusVoltage(void)
{
    uint16_t raw = INA226_ReadReg(0x02);
    return (uint16_t)(raw * 1.25f);
}

/**
 * @brief  读 INA226 分流电压 → 换算电流
 * @retval 电流 (mA), 正=放电
 * @note   Rshunt=0.1Ω, Shunt LSB=2.5μV
 *         I(mA) = raw × 2.5μV / 0.1Ω = raw × 0.025
 */
int16_t BMS_ReadBusCurrent(void)
{
    int16_t raw = (int16_t)INA226_ReadReg(0x01);
    return (int16_t)(raw * 0.025f);
}

/**
 * @brief  SOC 估算 (库仑计数 + OCV 校正融合)
 */
uint8_t BMS_EstimateSOC(void)
{
    /* TODO: SOC 估算 */
    return 0;
}

/**
 * @brief  电机参数补偿 (SOC/RPM → 补偿系数)
 *         电池电压随 SOC 下降 → 相同PWM占空比下转矩下降
 *         补偿策略: 低 SOC 时适当增大 Iq 参考值
 */
float BMS_GetMotorCompensation(uint8_t soc, int16_t rpm)
{
    /* TODO: 补偿算法 */
    (void)soc;
    (void)rpm;
    return 1.0f;
}
