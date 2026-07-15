/**
 ******************************************************************************
 * @file    comm.h
 * @brief   串口通信模块
 * @note    USART2 数据上报 + 命令解析
 ******************************************************************************
 */

#ifndef __COMM_H
#define __COMM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/** @brief 通信状态 */
typedef enum {
    COMM_IDLE        = 0x00,   /* 空闲 */
    COMM_TX_BUSY     = 0x01,   /* 发送中 */
    COMM_RX_PENDING  = 0x02,   /* 有待处理命令 */
} Comm_State_t;

/** @brief 上行数据帧 */
typedef struct {
    uint8_t  soc;        /* SOC (%) */
    int16_t  rpm;        /* 转速 (RPM) */
    int16_t  current;    /* 电流 (mA) */
    uint16_t voltage;    /* 电压 (mV) */
    int32_t  power;      /* 功率 (mW) */
    int16_t  temp;       /* 温度 (0.1°C) */
} Comm_Telemetry_t;

/* Exported constants --------------------------------------------------------*/

#define COMM_UART_HANDLE    huart2         /* USART2 句柄 */
#define COMM_TX_BUF_SIZE    64             /* 发送缓冲区 */
#define COMM_RX_BUF_SIZE    32             /* 接收缓冲区 */
#define COMM_REPORT_PERIOD  100            /* 上报周期 (ms) */

/* USART2 引脚 */
#define COMM_TX_PIN         GPIO_PIN_2     /* PA2 */
#define COMM_RX_PIN         GPIO_PIN_3     /* PA3 */
#define COMM_TX_PORT        GPIOA
#define COMM_RX_PORT        GPIOA

/* Exported functions --------------------------------------------------------*/

/** @brief 串口通信初始化 (USART2 + DMA) */
void Comm_Init(void);

/** @brief 通信任务 (100ms 周期) */
void Comm_Task(void);

/** @brief 发送遥测数据帧 */
void Comm_SendTelemetry(Comm_Telemetry_t *data);

/** @brief 发送字符串 (调试用) */
void Comm_SendString(const char *str);

/** @brief printf 重定向 (可选) */
int Comm_Printf(const char *fmt, ...);

/** @brief 注册命令回调 */
typedef void (*Comm_CmdHandler_t)(const char *cmd, const char *args);
void Comm_RegisterHandler(const char *name, Comm_CmdHandler_t handler);

/* Private defines -----------------------------------------------------------*/

#endif /* __COMM_H */
