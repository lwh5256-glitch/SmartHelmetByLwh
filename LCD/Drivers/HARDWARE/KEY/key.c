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
 * @brief       初始化按键
 * @param       无
 * @retval      无
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能时钟 */
    KEY_UP_GPIO_CLK_ENABLE();
    KEY0_GPIO_CLK_ENABLE();
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();

    /* 配置KEY_UP (PA0) - 按下为高电平 */
    GPIO_InitStruct.Pin = KEY_UP_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;  /* 下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 配置KEY0/KEY1/KEY2 (PE4/PE3/PE2) - 按下为低电平 */
    GPIO_InitStruct.Pin = KEY0_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  /* 上拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY0_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY1_GPIO_PIN;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY2_GPIO_PIN;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief       按键扫描
 * @param       mode: 0-不支持连续按, 1-支持连续按
 * @retval      按键值: KEY_NONE, KEY_UP_PRESS, KEY0_PRESS, KEY1_PRESS, KEY2_PRESS
 */
uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_state = 0;  /* 按键状态: 0-未按下, 1-已按下 */
    uint8_t key_val = KEY_NONE;

    if(mode == 1)
    {
        key_state = 0;  /* 支持连续按,每次都检测 */
    }

    /* 检测按键按下 */
    if(key_state == 0)
    {
        if(KEY_UP_READ() == GPIO_PIN_SET)  /* KEY_UP按下(高电平) */
        {
            delay_ms(10);  /* 消抖 */
            if(KEY_UP_READ() == GPIO_PIN_SET)
            {
                key_state = 1;
                key_val = KEY_UP_PRESS;
            }
        }
        else if(KEY0_READ() == GPIO_PIN_RESET)  /* KEY0按下(低电平) */
        {
            delay_ms(10);
            if(KEY0_READ() == GPIO_PIN_RESET)
            {
                key_state = 1;
                key_val = KEY0_PRESS;
            }
        }
        else if(KEY1_READ() == GPIO_PIN_RESET)  /* KEY1按下(低电平) */
        {
            delay_ms(10);
            if(KEY1_READ() == GPIO_PIN_RESET)
            {
                key_state = 1;
                key_val = KEY1_PRESS;
            }
        }
        else if(KEY2_READ() == GPIO_PIN_RESET)  /* KEY2按下(低电平) */
        {
            delay_ms(10);
            if(KEY2_READ() == GPIO_PIN_RESET)
            {
                key_state = 1;
                key_val = KEY2_PRESS;
            }
        }
    }

    /* 检测按键释放 */
    if(KEY_UP_READ() == GPIO_PIN_RESET &&
       KEY0_READ() == GPIO_PIN_SET &&
       KEY1_READ() == GPIO_PIN_SET &&
       KEY2_READ() == GPIO_PIN_SET)
    {
        key_state = 0;  /* 所有按键都释放 */
    }

    return key_val;
}
