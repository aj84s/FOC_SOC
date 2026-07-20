#include "key.h"

/**
  * @brief  按键扫描函数（阻塞式，带20ms消抖）
  * @note   检测到按键按下→消抖→等待释放→返回按键值
  *         每按一次返回一次，不会重复触发
  * @retval KEY_NONE / KEY1_PRESS / KEY2_PRESS / KEY3_PRESS
  */
uint8_t KEY_Scan(void)
{
    /* 检测KEY1（低电平有效） */
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET)
    {
        HAL_Delay(20);  /* 消抖 */
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET)
        {
            while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_SET);  /* 等待释放 */
            return KEY1_PRESS;
        }
    }

    /* 检测KEY2 */
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET)
    {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET)
        {
            while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_SET);
            return KEY2_PRESS;
        }
    }

    /* 检测KEY3 */
    if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET)
    {
        HAL_Delay(20);
        if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET)
        {
            while (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_SET);
            return KEY3_PRESS;
        }
    }

    return KEY_NONE;
}
