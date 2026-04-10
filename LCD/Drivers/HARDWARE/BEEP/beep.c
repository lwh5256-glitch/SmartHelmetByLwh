/**
 ****************************************************************************************************
 * @file        beep.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       蜂鸣器驱动代码
 ****************************************************************************************************
 */

#include "beep.h"
#include "delay.h"

/**
 * @brief       初始化蜂鸣器
 * @param       无
 * @retval      无
 */
void beep_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能GPIOF时钟 */
    BEEP_GPIO_CLK_ENABLE();

    /* 配置PF8为推挽输出 */
    GPIO_InitStruct.Pin = BEEP_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BEEP_GPIO_PORT, &GPIO_InitStruct);

    /* 初始化为关闭状态 */
    BEEP_OFF();
}

/**
 * @brief       打开蜂鸣器
 * @param       无
 * @retval      无
 */
void beep_on(void)
{
    BEEP_ON();
}

/**
 * @brief       关闭蜂鸣器
 * @param       无
 * @retval      无
 */
void beep_off(void)
{
    BEEP_OFF();
}

/**
 * @brief       翻转蜂鸣器状态
 * @param       无
 * @retval      无
 */
void beep_toggle(void)
{
    BEEP_TOGGLE();
}

/**
 * @brief       蜂鸣器报警
 * @param       times: 鸣叫次数
 * @param       duration_ms: 每次鸣叫持续时间(ms)
 * @retval      无
 */
void beep_alarm(uint16_t times, uint16_t duration_ms)
{
    for(uint16_t i = 0; i < times; i++)
    {
        BEEP_ON();
        delay_ms(duration_ms);
        BEEP_OFF();
        delay_ms(duration_ms);
    }
}
