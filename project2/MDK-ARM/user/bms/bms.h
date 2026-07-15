/**
 ******************************************************************************
 * @file    bms.h
 * @brief   BMS (Battery Management System) 电池管理系统模块
 * @note    INA226数据采集 + 库仑计数 + SOC估算 + 电机参数补偿
 ******************************************************************************
 */

#ifndef __BMS_H
#define __BMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/** @brief 电池状态 */
typedef enum {
    BMS_STATE_NORMAL   = 0x00,   /* 正常 */
    BMS_STATE_CHARGING = 0x01,   /* 充电中 */
    BMS_STATE_LOW      = 0x02,   /* 低电量警告 */
    BMS_STATE_CRITICAL = 0x03,   /* 临界欠压 */
} BMS_State_t;

/** @brief 电池数据包 */
typedef struct {
    uint16_t bus_voltage;         /* 总线电压 (mV) */
    int16_t  current;             /* 电流 (mA, 正=放电) */
    int32_t  power;               /* 功率 (mW) */
    uint8_t  soc;                 /* SOC 百分比 (0-100) */
    uint32_t coulomb_counter;     /* 库仑计数 (mAs) */
    uint16_t temperature;         /* 温度 (0.1°C, 可选NTC) */
    BMS_State_t state;            /* 电池状态 */
} BMS_Data_t;

/* Exported constants --------------------------------------------------------*/

/* 电池参数 (3S LiPo) */
#define BMS_CELLS            3
#define BMS_VOLTAGE_FULL     12600   /* 满电电压 12.6V */
#define BMS_VOLTAGE_NOMINAL  11100   /* 标称电压 11.1V */
#define BMS_VOLTAGE_CUTOFF   9000    /* 放电截止 9.0V */
#define BMS_CAPACITY_MAH     2200    /* 标称容量 */

/* INA226 I2C 地址 */
#define INA226_ADDR          0x40    /* A0=A1=GND */

/* Exported functions --------------------------------------------------------*/

/** @brief BMS 模块初始化 (INA226 + 校准) */
void BMS_Init(void);

/** @brief BMS 主任务 (100ms 周期) */
void BMS_Task(void);

/** @brief 获取当前电池数据 */
BMS_Data_t BMS_GetData(void);

/** @brief 读取 INA226 总线电压 (mV) */
uint16_t BMS_ReadBusVoltage(void);

/** @brief 读取 INA226 电流 (mA) */
int16_t BMS_ReadCurrent(void);

/** @brief 估算 SOC (库仑计数 + 开路电压校正) */
uint8_t BMS_EstimateSOC(void);

/** @brief 电机参数补偿 (SOC/RPM → 补偿系数) */
float BMS_GetMotorCompensation(uint8_t soc, int16_t rpm);

/* Private defines -----------------------------------------------------------*/

#endif /* __BMS_H */
