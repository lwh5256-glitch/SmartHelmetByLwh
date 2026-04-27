/**
 ****************************************************************************************************
 * @file        key.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       按键驱动代码
 ****************************************************************************************************
 */

#include "key.h"
#include "delay.h"

/**
 * @brief       初始化按键 GPIO
 * @param       无
 * @retval      无
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能按键所在 GPIO 端口时钟 */
    KEY_UP_GPIO_CLK_ENABLE();
    KEY0_GPIO_CLK_ENABLE();
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();

    /* 配置 KEY_UP (PA0)，按下时为高电平 */
    GPIO_InitStruct.Pin = KEY_UP_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;  /* 下拉输入 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置 KEY0/KEY1/KEY2 (PE4/PE3/PE2)，按下时为低电平 */
    GPIO_InitStruct.Pin = KEY0_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  /* 上拉输入 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY0_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY1_GPIO_PIN;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY2_GPIO_PIN;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief       按键扫描
 * @param       mode: 0-不支持连按, 1-允许重新同步当前按键状态
 * @retval      按键值: KEY_NONE, KEY_UP_PRESS, KEY0_PRESS, KEY1_PRESS, KEY2_PRESS
 */
uint8_t key_scan(uint8_t mode)
{
    static GPIO_PinState prev_key_up = GPIO_PIN_RESET;
    static GPIO_PinState prev_key0 = GPIO_PIN_SET;
    uint8_t key_val = KEY_NONE;
    GPIO_PinState key_up;
    GPIO_PinState key0;

    if(mode == 1)
    {
        prev_key_up = KEY_UP_READ();
        prev_key0 = KEY0_READ();
    }

    key_up = KEY_UP_READ();
    key0 = KEY0_READ();

    if(prev_key_up == GPIO_PIN_RESET && key_up == GPIO_PIN_SET)
    {
        delay_ms(10);
        if(KEY_UP_READ() == GPIO_PIN_SET)
        {
            key_val = KEY_UP_PRESS;
            key_up = KEY_UP_READ();
        }
    }
    else if(prev_key0 == GPIO_PIN_SET && key0 == GPIO_PIN_RESET)
    {
        delay_ms(10);
        if(KEY0_READ() == GPIO_PIN_RESET)
        {
            key_val = KEY0_PRESS;
            key0 = KEY0_READ();
        }
    }

    prev_key_up = key_up;
    prev_key0 = key0;

    return key_val;
}
