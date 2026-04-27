#include "ds18b20.h"
#include "delay.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} DS18B20_Bus_t;

static const DS18B20_Bus_t ds18b20_bus[DS18B20_CH_COUNT] =
{
    {GPIOG, GPIO_PIN_9},
    {GPIOG, GPIO_PIN_10}
};

static void ds18b20_set_output(DS18B20_Channel_t channel)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = ds18b20_bus[channel].pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ds18b20_bus[channel].port, &gpio_init);
}

static void ds18b20_set_input(DS18B20_Channel_t channel)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Pin = ds18b20_bus[channel].pin;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ds18b20_bus[channel].port, &gpio_init);
}

static void ds18b20_write_pin(DS18B20_Channel_t channel, GPIO_PinState state)
{
    HAL_GPIO_WritePin(ds18b20_bus[channel].port, ds18b20_bus[channel].pin, state);
}

static GPIO_PinState ds18b20_read_pin(DS18B20_Channel_t channel)
{
    return HAL_GPIO_ReadPin(ds18b20_bus[channel].port, ds18b20_bus[channel].pin);
}

static uint8_t ds18b20_reset(DS18B20_Channel_t channel)
{
    uint16_t retry = 0;

    ds18b20_set_output(channel);
    ds18b20_write_pin(channel, GPIO_PIN_RESET);
    delay_us(750);
    ds18b20_write_pin(channel, GPIO_PIN_SET);
    ds18b20_set_input(channel);
    delay_us(15);

    while ((ds18b20_read_pin(channel) == GPIO_PIN_SET) && (retry < 200))
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 200)
    {
        return 1;
    }

    retry = 0;
    while ((ds18b20_read_pin(channel) == GPIO_PIN_RESET) && (retry < 240))
    {
        retry++;
        delay_us(1);
    }

    return (retry >= 240) ? 1 : 0;
}

static void ds18b20_write_byte(DS18B20_Channel_t channel, uint8_t data)
{
    uint8_t i;

    ds18b20_set_output(channel);
    for (i = 0; i < 8; i++)
    {
        ds18b20_write_pin(channel, GPIO_PIN_RESET);
        if (data & 0x01)
        {
            delay_us(2);
            ds18b20_write_pin(channel, GPIO_PIN_SET);
            delay_us(60);
        }
        else
        {
            delay_us(60);
            ds18b20_write_pin(channel, GPIO_PIN_SET);
            delay_us(2);
        }
        data >>= 1;
    }
}

static uint8_t ds18b20_read_bit(DS18B20_Channel_t channel)
{
    uint8_t bit;

    ds18b20_set_output(channel);
    ds18b20_write_pin(channel, GPIO_PIN_RESET);
    delay_us(2);
    ds18b20_write_pin(channel, GPIO_PIN_SET);
    ds18b20_set_input(channel);
    delay_us(12);

    bit = (ds18b20_read_pin(channel) == GPIO_PIN_SET) ? 1 : 0;
    delay_us(50);

    return bit;
}

static uint8_t ds18b20_read_byte(DS18B20_Channel_t channel)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8; i++)
    {
        data >>= 1;
        if (ds18b20_read_bit(channel))
        {
            data |= 0x80;
        }
    }

    return data;
}

void DS18B20_Init(void)
{
    uint8_t i;

    __HAL_RCC_GPIOG_CLK_ENABLE();

    for (i = 0; i < DS18B20_CH_COUNT; i++)
    {
        ds18b20_set_output((DS18B20_Channel_t)i);
        ds18b20_write_pin((DS18B20_Channel_t)i, GPIO_PIN_SET);
    }
}

uint8_t DS18B20_StartConvert(DS18B20_Channel_t channel)
{
    if (channel >= DS18B20_CH_COUNT)
    {
        return 1;
    }

    if (ds18b20_reset(channel) != 0)
    {
        return 1;
    }

    ds18b20_write_byte(channel, 0xCC);
    ds18b20_write_byte(channel, 0x44);

    return 0;
}

uint8_t DS18B20_StartConvertAll(void)
{
    uint8_t i;
    uint8_t error = 0;

    for (i = 0; i < DS18B20_CH_COUNT; i++)
    {
        error |= DS18B20_StartConvert((DS18B20_Channel_t)i);
    }

    return error;
}

uint8_t DS18B20_ReadTempX10(DS18B20_Channel_t channel, int16_t *temp_x10)
{
    uint8_t temp_l;
    uint8_t temp_h;
    int16_t raw;

    if ((channel >= DS18B20_CH_COUNT) || (temp_x10 == 0))
    {
        return 1;
    }

    if (ds18b20_reset(channel) != 0)
    {
        return 1;
    }

    ds18b20_write_byte(channel, 0xCC);
    ds18b20_write_byte(channel, 0xBE);

    temp_l = ds18b20_read_byte(channel);
    temp_h = ds18b20_read_byte(channel);
    raw = (int16_t)(((uint16_t)temp_h << 8) | temp_l);

    *temp_x10 = (int16_t)((raw * 10) / 16);

    return 0;
}
