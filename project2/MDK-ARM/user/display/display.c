/**
 ******************************************************************************
 * @file    display.c
 * @brief   OLED 显示模块实现
 * @note    SSD1306 128x64 I2C 驱动 + 多页面切换
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "display.h"
#include "stm32g4xx_hal.h"
#include <string.h>
#include <stdio.h>

/* Private variables ---------------------------------------------------------*/

/* 显示缓冲区 */
static uint8_t disp_buf[SSD1306_WIDTH * SSD1306_PAGES];

/* 字体 (5x7 基本ASCII) */
/* TODO: 引入字体表 */

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  SSD1306 写命令
 */
static void Display_WriteCmd(uint8_t cmd)
{
    /* TODO: I2C 写命令 (0x00 + cmd) */
}

/**
 * @brief  SSD1306 写数据
 */
static void Display_WriteData(uint8_t *data, uint16_t len)
{
    /* TODO: I2C 写数据 (0x40 + data[]) */
}

/**
 * @brief  刷新全屏 (缓冲区 → OLED)
 */
static void Display_Refresh(void)
{
    /* TODO: 将 disp_buf 通过 I2C 写入 SSD1306 GDDRAM */
}

/**
 * @brief  缓冲区写像素
 */
static void Display_SetPixel(uint8_t x, uint8_t y, uint8_t on)
{
    /* TODO: 单像素写入缓冲区 */
}

/* Public functions ----------------------------------------------------------*/

void Display_Init(void)
{
    /* TODO: SSD1306 初始化序列 + 清屏 */
}

void Display_Task(void)
{
    /* TODO: 333ms 周期刷新:  读取最新数据 → 更新缓冲区 → 刷新 */
}

void Display_SwitchPage(Display_Page_t page)
{
    /* TODO: 切换页面 (配合按键中断) */
}

void Display_Clear(void)
{
    /* TODO: 清空缓冲区 + 刷新 */
}

void Display_String(uint8_t x, uint8_t y, const char *str)
{
    /* TODO: 逐字符写入缓冲区 (5x7字体) */
}

void Display_ShowBattery(uint8_t soc, uint16_t voltage, int16_t current)
{
    /* TODO: 电池状态页面布局 */
}

void Display_ShowMotor(int16_t rpm, uint16_t throttle)
{
    /* TODO: 电机状态页面布局 */
}
