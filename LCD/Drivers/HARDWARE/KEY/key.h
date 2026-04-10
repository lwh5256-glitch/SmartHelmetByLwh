/**
 ****************************************************************************************************
 * @file        key.h
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       按键驱动代码
 ****************************************************************************************************
 */

#ifndef __KEY_H
#define __KEY_H

#include "stm32f4xx_hal.h"

/* 按键引脚定义 */
#define KEY_UP_GPIO_PORT            GPIOA
#define KEY_UP_GPIO_PIN             GPIO_PIN_0
#define KEY_UP_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()

#define KEY0_GPIO_PORT              GPIOE
#define KEY0_GPIO_PIN               GPIO_PIN_4
#define KEY0_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()

#define KEY1_GPIO_PORT              GPIOE
#define KEY1_GPIO_PIN               GPIO_PIN_3
#define KEY1_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()

#define KEY2_GPIO_PORT              GPIOE
#define KEY2_GPIO_PIN               GPIO_PIN_2
#define KEY2_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOE_CLK_ENABLE()

/* 按键读取宏 */
#define KEY_UP_READ()               HAL_GPIO_ReadPin(KEY_UP_GPIO_PORT, KEY_UP_GPIO_PIN)
#define KEY0_READ()                 HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)
#define KEY1_READ()                 HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2_READ()                 HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)

/* 按键值定义 */
#define KEY_NONE                    0
#define KEY_UP_PRESS                1
#define KEY0_PRESS                  2
#define KEY1_PRESS                  3
#define KEY2_PRESS                  4

/* 函数声明 */
void key_init(void);
uint8_t key_scan(uint8_t mode);

#endif
