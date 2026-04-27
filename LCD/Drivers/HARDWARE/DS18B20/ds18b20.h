#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

typedef enum
{
    DS18B20_CH_BODY = 0,
    DS18B20_CH_ENV = 1,
    DS18B20_CH_COUNT
} DS18B20_Channel_t;

void DS18B20_Init(void);
uint8_t DS18B20_StartConvert(DS18B20_Channel_t channel);
uint8_t DS18B20_StartConvertAll(void);
uint8_t DS18B20_ReadTempX10(DS18B20_Channel_t channel, int16_t *temp_x10);

#endif /* __DS18B20_H */
