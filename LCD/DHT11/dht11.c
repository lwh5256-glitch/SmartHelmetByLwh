/**
 * @file        dht11.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-04-01
 * @brief       DHT11数字温湿度传感器 驱动代码
 */

#include "dht11.h"
#include "delay.h"


/**
 * @brief       澶浣DHT11
 * @param       data: 瑕ョ版
 * @retval      
 */
static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);    /* 浣DQ */
    delay_ms(20);       /* 浣冲18ms */
    DHT11_DQ_OUT(1);    /* DQ=1 */
    delay_us(30);       /* 涓绘烘楂20~40us */
}

/**
 * @brief       绛寰DHT11搴
 * @param       
 * @retval      0, DHT11姝ｅ父
 *              1, DHT11寮甯/涓瀛
 */
uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (DHT11_DQ_IN && retry < 100)  /* DHT11浼浣40~80us */
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 100)
    {
        rval = 1;
    }
    else
    {
        retry = 0;

        while (!DHT11_DQ_IN && retry < 100) /* DHT11浣浼娆℃楂40~80us */
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;
    }
    
    return rval;
}

/**
 * @brief       浠DHT11璇诲涓涓浣
 * @param       
 * @retval      璇诲扮浣: 0 / 1
 */
uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)  /* 绛寰涓轰靛钩 */
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    while (!DHT11_DQ_IN && retry < 100) /* 绛寰楂靛钩 */
    {
        retry++;
        delay_us(1);
    }

    delay_us(40);       /* 绛寰40us */

    if (DHT11_DQ_IN)    /* 规寮舵杩 bit */
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

/**
 * @brief       浠DHT11璇诲涓涓瀛
 * @param       
 * @retval      璇诲扮版
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 寰璇诲8浣版 */
    {
        data <<= 1;                 /* 楂浣版杈, 宸绉讳浣 */
        data |= dht11_read_bit();   /* 璇诲1bit版 */
    }

    return data;
}

/**
 * @brief       浠DHT11璇诲涓娆℃版
 * @param       temp: 娓╁害(:0~50掳)
 * @param       humi: 婀垮害(:20%~90%)
 * @retval      0, 姝ｅ父.
 *              1, 澶辫触
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    dht11_reset();

    if (dht11_check() == 0)
    {
        for (i = 0; i < 5; i++)     /* 璇诲40浣版 */
        {
            buf[i] = dht11_read_byte();
        }

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief       濮DHT11IO DQ 舵娴DHT11瀛
 * @param       
 * @retval      0, 姝ｅ父
 *              1, 涓瀛/涓姝ｅ父
 */
uint8_t dht11_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    DHT11_DQ_GPIO_CLK_ENABLE();     /* 寮DQ寮堕 */

    gpio_init_struct.Pin = DHT11_DQ_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;            /* 寮婕杈 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 涓 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 楂 */
    HAL_GPIO_Init(DHT11_DQ_GPIO_PORT, &gpio_init_struct);   /* 濮DHT11_DQ寮 */
    /* DHT11_DQ寮妯″璁剧疆,寮婕杈,涓, 杩峰氨涓ㄥ璁剧疆IO瑰浜, 寮婕杈虹跺(=1), 涔浠ヨ诲澶ㄤ俊风楂浣靛钩 */

    dht11_reset();
    return dht11_check();
}
























