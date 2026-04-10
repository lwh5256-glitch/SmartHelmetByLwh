/**
 ****************************************************************************************************
 * @file        fall_detect.h
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       摔倒检测算法
 ****************************************************************************************************
 */

#ifndef __FALL_DETECT_H
#define __FALL_DETECT_H

#include <stdint.h>
#include "filter.h"

/* 摔倒检测参数 */
#define FALL_ACCEL_THRESHOLD_HIGH   17000   /* 加速度突变上限阈值 (约1.04g) - 冲击阈值 */
#define FALL_ACCEL_THRESHOLD_LOW    10500   /* 加速度突变下限阈值 (约0.64g) - 自由落体阈值 */
#define FALL_STABLE_THRESHOLD       4000    /* 静止判断阈值 - 平衡的静止判断 */
#define FALL_STABLE_TIME_MS         350     /* 静止持续时间(ms) - 平衡响应时间 */
#define FALL_DETECT_COOLDOWN_MS     3000    /* 检测冷却时间(ms) */

/* 摔倒检测状态 */
typedef enum {
    FALL_STATE_NORMAL = 0,      /* 正常状态 */
    FALL_STATE_FREEFALL,        /* 自由落体状态 */
    FALL_STATE_IMPACT,          /* 冲击状态 */
    FALL_STATE_STABLE,          /* 静止状态 */
    FALL_STATE_DETECTED         /* 检测到摔倒 */
} fall_state_t;

/* 摔倒检测结构体 */
typedef struct {
    fall_state_t state;
    uint32_t state_start_time;
    uint32_t last_detect_time;

    lowpass_filter_t accel_x_filter;
    lowpass_filter_t accel_y_filter;
    lowpass_filter_t accel_z_filter;

    float accel_magnitude;
    float accel_magnitude_prev;

    uint8_t fall_detected;
} fall_detector_t;

/* 函数声明 */
void fall_detect_init(fall_detector_t *detector);
uint8_t fall_detect_update(fall_detector_t *detector, short accel_x, short accel_y, short accel_z, uint32_t current_time_ms);
void fall_detect_reset(fall_detector_t *detector);
const char* fall_detect_get_state_string(fall_state_t state);

#endif
