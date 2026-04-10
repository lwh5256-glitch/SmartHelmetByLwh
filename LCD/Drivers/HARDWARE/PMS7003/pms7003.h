#ifndef __PMS7003_H
#define __PMS7003_H

#include "main.h"

/* PMS7003数据帧结构 */
typedef struct {
    uint16_t pm1_0_cf1;      /* PM1.0浓度 (CF=1, μg/m³) */
    uint16_t pm2_5_cf1;      /* PM2.5浓度 (CF=1, μg/m³) */
    uint16_t pm10_cf1;       /* PM10浓度 (CF=1, μg/m³) */
    uint16_t pm1_0_atm;      /* PM1.0浓度 (大气环境, μg/m³) */
    uint16_t pm2_5_atm;      /* PM2.5浓度 (大气环境, μg/m³) */
    uint16_t pm10_atm;       /* PM10浓度 (大气环境, μg/m³) */
    uint16_t particles_0_3;  /* 0.3um颗粒物数量 */
    uint16_t particles_0_5;  /* 0.5um颗粒物数量 */
    uint16_t particles_1_0;  /* 1.0um颗粒物数量 */
    uint16_t particles_2_5;  /* 2.5um颗粒物数量 */
    uint16_t particles_5_0;  /* 5.0um颗粒物数量 */
    uint16_t particles_10;   /* 10um颗粒物数量 */
    uint8_t  valid;          /* 数据有效标志 */
} PMS7003_Data_t;

/* 函数声明 */
void PMS7003_Init(void);
void PMS7003_Process(void);
PMS7003_Data_t* PMS7003_GetData(void);

#endif /* __PMS7003_H */
