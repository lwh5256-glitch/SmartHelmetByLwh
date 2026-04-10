/**
 ****************************************************************************************************
 * @file        filter.h
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       数字滤波算法
 ****************************************************************************************************
 */

#ifndef __FILTER_H
#define __FILTER_H

#include <stdint.h>

/* 滑动平均滤波器窗口大小 */
#define FILTER_WINDOW_SIZE      5

/* 滑动平均滤波器结构体 */
typedef struct {
    float buffer[FILTER_WINDOW_SIZE];
    uint8_t index;
    uint8_t count;
    float sum;
} moving_average_filter_t;

/* 一阶低通滤波器结构体 */
typedef struct {
    float alpha;        /* 滤波系数 0-1, 越小滤波越强 */
    float output;       /* 滤波输出 */
    uint8_t initialized;
} lowpass_filter_t;

/* 函数声明 */
void moving_average_init(moving_average_filter_t *filter);
float moving_average_update(moving_average_filter_t *filter, float input);

void lowpass_filter_init(lowpass_filter_t *filter, float alpha);
float lowpass_filter_update(lowpass_filter_t *filter, float input);

#endif
