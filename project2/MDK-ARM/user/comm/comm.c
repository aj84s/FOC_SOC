/**
 ******************************************************************************
 * @file    comm.c
 * @brief   串口通信模块实现
 * @note    USART2 DMA发送 + 中断接收 + 简易命令解析
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "comm.h"
#include "stm32g4xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Private variables ---------------------------------------------------------*/

static uint8_t tx_buf[COMM_TX_BUF_SIZE];
static uint8_t rx_buf[COMM_RX_BUF_SIZE];

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  USART2 GPIO + DMA 初始化
 */
static void Comm_USART2_Init(void)
{
    /* TODO: PA2(TX)/PA3(RX) AF7, 115200-8-N-1, DMA发送 */
}

/**
 * @brief  解析接收到的命令
 *         格式: "CMD:ARGS" 或 "CMD?"
 */
static void Comm_ParseCmd(const char *rx_line)
{
    /* TODO: 分割命令名和参数, 查找已注册的 handler 并调用 */
}

/**
 * @brief  格式化遥测帧
 *         格式: "SOC:xx,RPM:xxxx,I:xxxx,V:xxxx,P:xxxx,T:xxx\r\n"
 */
static uint16_t Comm_FormatFrame(Comm_Telemetry_t *data, uint8_t *buf)
{
    /* TODO: snprintf 格式化数据帧 */
    return 0;
}

/* Public functions ----------------------------------------------------------*/

void Comm_Init(void)
{
    /* TODO: USART2 初始化 + DMA 配置 */
}

void Comm_Task(void)
{
    /* TODO: 检查新数据 → 打包遥测 → DMA 发送 */
}

void Comm_SendTelemetry(Comm_Telemetry_t *data)
{
    /* TODO: 格式化 + DMA 发送 */
}

void Comm_SendString(const char *str)
{
    /* TODO: 阻塞发送字符串 (调试用) */
}

int Comm_Printf(const char *fmt, ...)
{
    /* TODO: vsnprintf → UART 发送 */
    return 0;
}

void Comm_RegisterHandler(const char *name, Comm_CmdHandler_t handler)
{
    /* TODO: 注册命令处理回调 (MOTOR:START, MOTOR:STOP, STATUS?, RATE:xxx) */
}
