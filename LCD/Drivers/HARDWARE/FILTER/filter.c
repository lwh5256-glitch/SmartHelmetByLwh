/**
 ****************************************************************************************************
 * @file        filter.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       数字滤波算法实现
 ****************************************************************************************************
 */

#include "filter.h"
#include <string.h>

/**
 * @brief       初始化滑动平均滤波器
 * @param       filter: 滤波器指针
 * @retval      无
 */
void moving_average_init(moving_average_filter_t *filter)
{
    memset(filter->buffer, 0, sizeof(filter->buffer));
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0.0f;
}

/**
 * @brief       滑动平均滤波器更新
 * @param       filter: 滤波器指针
 * @param       input: 输入值
 * @retval      滤波后的输出值
 */
float moving_average_update(moving_average_filter_t *filter, float input)
{
    /* 减去最旧的值 */
    filter->sum -= filter->buffer[filter->index];

    /* 添加新值 */
    filter->buffer[filter->index] = input;
    filter->sum += input;

    /* 更新索引 */
    filter->index = (filter->index + 1) % FILTER_WINDOW_SIZE;

    /* 更新计数 */
    if(filter->count < FILTER_WINDOW_SIZE)
    {
        filter->count++;
    }

    /* 返回平均值 */
    return filter->sum / filter->count;
}

/**
 * @brief       初始化一阶低通滤波器
 * @param       filter: 滤波器指针
 * @param       alpha: 滤波系数 (0-1), 越小滤波越强
 * @retval      无
 */
void lowpass_filter_init(lowpass_filter_t *filter, float alpha)
{
    filter->alpha = alpha;
    filter->output = 0.0f;
    filter->initialized = 0;
}

/**
 * @brief       一阶低通滤波器更新
 * @param       filter: 滤波器指针
 * @param       input: 输入值
 * @retval      滤波后的输出值
 */
float lowpass_filter_update(lowpass_filter_t *filter, float input)
{
    if(!filter->initialized)
    {
        filter->output = input;
        filter->initialized = 1;
    }
    else
    {
        /* 一阶低通滤波: y[n] = alpha * x[n] + (1-alpha) * y[n-1] */
        filter->output = filter->alpha * input + (1.0f - filter->alpha) * filter->output;
    }

    return filter->output;
}
