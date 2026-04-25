/**
 ****************************************************************************************************
 * @file        key.c
 * @author      闄嗘枃璞?
 * @version     V1.0
 * @date        2026-03-23
 * @brief       鎸夐敭椹卞姩浠ｇ爜
 ****************************************************************************************************
 */

#include "key.h"
#include "delay.h"

/**
 * @brief       鍒濆鍖栨寜閿?
 * @param       鏃?
 * @retval      鏃?
 */
void key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 浣胯兘鏃堕挓 */
    KEY_UP_GPIO_CLK_ENABLE();
    KEY0_GPIO_CLK_ENABLE();
    KEY1_GPIO_CLK_ENABLE();
    KEY2_GPIO_CLK_ENABLE();

    /* 閰嶇疆KEY_UP (PA0) - 鎸変笅涓洪珮鐢靛钩 */
    GPIO_InitStruct.Pin = KEY_UP_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;  /* 涓嬫媺 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_UP_GPIO_PORT, &GPIO_InitStruct);

    /* 閰嶇疆KEY0/KEY1/KEY2 (PE4/PE3/PE2) - 鎸変笅涓轰綆鐢靛钩 */
    GPIO_InitStruct.Pin = KEY0_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;  /* 涓婃媺 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY0_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY1_GPIO_PIN;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = KEY2_GPIO_PIN;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief       鎸夐敭鎵弿
 * @param       mode: 0-涓嶆敮鎸佽繛缁寜, 1-鏀寔杩炵画鎸?
 * @retval      鎸夐敭鍊? KEY_NONE, KEY_UP_PRESS, KEY0_PRESS, KEY1_PRESS, KEY2_PRESS
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
