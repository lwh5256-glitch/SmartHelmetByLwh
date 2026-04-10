/**
 ****************************************************************************************************
 * @file        beep.h
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       蜂鸣器驱动代码
 ****************************************************************************************************
 */

#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f4xx_hal.h"

/* 蜂鸣器引脚定义 */
#define BEEP_GPIO_PORT              GPIOF
#define BEEP_GPIO_PIN               GPIO_PIN_8
#define BEEP_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOF_CLK_ENABLE()

/* 蜂鸣器控制宏 */
#define BEEP_ON()                   HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET)
#define BEEP_OFF()                  HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET)
#define BEEP_TOGGLE()               HAL_GPIO_TogglePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN)

/* 函数声明 */
void beep_init(void);
void beep_on(void);
void beep_off(void);
void beep_toggle(void);
void beep_alarm(uint16_t times, uint16_t duration_ms);

#endif
