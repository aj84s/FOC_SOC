/**
 ******************************************************************************
 * @file    display.h
 * @brief   OLED 显示模块
 * @note    SSD1306 128x64 I2C OLED
 ******************************************************************************
 */

#ifndef __DISPLAY_H
#define __DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/** @brief OLED 显示页面 */
typedef enum {
    DISP_PAGE_MAIN    = 0x00,   /* 主页面: SOC + RPM + 电压 */
    DISP_PAGE_CURRENT = 0x01,   /* 电流页面: 实时电流 + 功率 */
    DISP_PAGE_DEBUG   = 0x02,   /* 调试页面: 电机参数 */
} Display_Page_t;

/* Exported constants --------------------------------------------------------*/

#define SSD1306_ADDR        0x3C    /* OLED I2C 地址 */
#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64
#define SSD1306_PAGES       8       /* 64/8 = 8 pages */

/* Exported functions --------------------------------------------------------*/

/** @brief OLED 初始化 (I2C + SSD1306 配置) */
void Display_Init(void);

/** @brief 显示任务 (333ms 周期, 3Hz) */
void Display_Task(void);

/** @brief 切换显示页面 */
void Display_SwitchPage(Display_Page_t page);

/** @brief 清屏 */
void Display_Clear(void);

/** @brief 显示字符串 (指定位置) */
void Display_String(uint8_t x, uint8_t y, const char *str);

/** @brief 显示电池状态页面 */
void Display_ShowBattery(uint8_t soc, uint16_t voltage, int16_t current);

/** @brief 显示电机状态页面 */
void Display_ShowMotor(int16_t rpm, uint16_t throttle);

/* Private defines -----------------------------------------------------------*/

#endif /* __DISPLAY_H */
