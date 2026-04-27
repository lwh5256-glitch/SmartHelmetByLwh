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
 * @brief       发送 DHT11 起始信号
 * @param       无
 * @retval      无
 */
static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);    /* 主机拉低数据线 */
    delay_ms(20);       /* 保持至少 18 ms */
    DHT11_DQ_OUT(1);    /* DQ=1 */
    delay_us(30);       /* 主机释放总线 20~40 us */
}

/**
 * @brief       检查 DHT11 响应信号
 * @param       无
 * @retval      0: DHT11 正常
 *              1: DHT11 无响应或超时
 */
uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (DHT11_DQ_IN && retry < 100)  /* 等待 DHT11 拉低，典型 40~80 us */
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

        while (!DHT11_DQ_IN && retry < 100) /* 等待 DHT11 拉高，典型 40~80 us */
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;
    }
    
    return rval;
}

/**
 * @brief       读取 DHT11 的 1 bit 数据
 * @param       无
 * @retval      读取到的位值: 0 / 1
 */
uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)  /* 等待数据线进入低电平 */
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    while (!DHT11_DQ_IN && retry < 100) /* 等待数据线进入高电平 */
    {
        retry++;
        delay_us(1);
    }

    delay_us(40);       /* 延时 40 us 后判定当前位值 */

    if (DHT11_DQ_IN)    /* 高电平持续更久表示当前 bit 为 1 */
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

/**
 * @brief       读取 DHT11 的 1 字节数据
 * @param       无
 * @retval      读取到的字节
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 逐位读取 8 bit */
    {
        data <<= 1;                 /* 先左移，为新位腾出位置 */
        data |= dht11_read_bit();   /* 读取并拼入当前 bit */
    }

    return data;
}

/**
 * @brief       读取一帧温湿度数据
 * @param       temp: 温度输出，范围约 0~50 摄氏度
 * @param       humi: 湿度输出，范围约 20%~90%
 * @retval      0: 读取成功
 *              1: 读取失败
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    dht11_reset();

    if (dht11_check() == 0)
    {
        for (i = 0; i < 5; i++)     /* 读取 5 字节，共 40 bit */
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
 * @brief       初始化 DHT11 数据引脚并检测设备
 * @param       无
 * @retval      0: 初始化成功
 *              1: 设备不存在或通信异常
 */
uint8_t dht11_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    DHT11_DQ_GPIO_CLK_ENABLE();     /* 使能 DHT11 数据引脚时钟 */

    gpio_init_struct.Pin = DHT11_DQ_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;            /* 开漏输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(DHT11_DQ_GPIO_PORT, &gpio_init_struct);   /* 初始化 DHT11 数据引脚 */
    /* DHT11 使用单总线，初始化为开漏上拉输出，空闲时保持高电平。 */

    dht11_reset();
    return dht11_check();
}
























