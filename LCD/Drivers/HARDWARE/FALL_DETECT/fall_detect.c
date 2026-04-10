/**
 ****************************************************************************************************
 * @file        fall_detect.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-03-23
 * @brief       摔倒检测算法实现
 ****************************************************************************************************
 */

#include "fall_detect.h"
#include <math.h>
#include <string.h>

/**
 * @brief       初始化摔倒检测器
 * @param       detector: 检测器指针
 * @retval      无
 */
void fall_detect_init(fall_detector_t *detector)
{
    memset(detector, 0, sizeof(fall_detector_t));

    detector->state = FALL_STATE_NORMAL;
    detector->state_start_time = 0;
    detector->last_detect_time = 0;
    detector->fall_detected = 0;

    /* 初始化低通滤波器, alpha=0.3 */
    lowpass_filter_init(&detector->accel_x_filter, 0.3f);
    lowpass_filter_init(&detector->accel_y_filter, 0.3f);
    lowpass_filter_init(&detector->accel_z_filter, 0.3f);

    detector->accel_magnitude = 16384.0f;  /* 1g */
    detector->accel_magnitude_prev = 16384.0f;
}

/**
 * @brief       计算加速度向量模长
 * @param       x, y, z: 三轴加速度
 * @retval      加速度模长
 */
static float calculate_magnitude(float x, float y, float z)
{
    return sqrtf(x*x + y*y + z*z);
}

/**
 * @brief       更新摔倒检测
 * @param       detector: 检测器指针
 * @param       accel_x, accel_y, accel_z: 三轴加速度原始值
 * @param       current_time_ms: 当前时间(ms)
 * @retval      1-检测到摔倒, 0-未检测到
 */
uint8_t fall_detect_update(fall_detector_t *detector,
                           short accel_x, short accel_y, short accel_z,
                           uint32_t current_time_ms)
{
    /* 滤波处理 */
    float filtered_x = lowpass_filter_update(&detector->accel_x_filter, (float)accel_x);
    float filtered_y = lowpass_filter_update(&detector->accel_y_filter, (float)accel_y);
    float filtered_z = lowpass_filter_update(&detector->accel_z_filter, (float)accel_z);

    /* 计算加速度模长 */
    detector->accel_magnitude_prev = detector->accel_magnitude;
    detector->accel_magnitude = calculate_magnitude(filtered_x, filtered_y, filtered_z);

    /* 冷却时间检查 */
    if(detector->fall_detected &&
       (current_time_ms - detector->last_detect_time) < FALL_DETECT_COOLDOWN_MS)
    {
        return 0;  /* 在冷却期内,不进行检测 */
    }

    /* 状态机 */
    switch(detector->state)
    {
        case FALL_STATE_NORMAL:
            /* 检测自由落体: 加速度突然减小 */
            if(detector->accel_magnitude < FALL_ACCEL_THRESHOLD_LOW)
            {
                detector->state = FALL_STATE_FREEFALL;
                detector->state_start_time = current_time_ms;
            }
            break;

        case FALL_STATE_FREEFALL:
            /* 检测冲击: 加速度突然增大 */
            if(detector->accel_magnitude > FALL_ACCEL_THRESHOLD_HIGH)
            {
                detector->state = FALL_STATE_IMPACT;
                detector->state_start_time = current_time_ms;
            }
            /* 超时返回正常状态 - 延长到1000ms */
            else if((current_time_ms - detector->state_start_time) > 1000)
            {
                detector->state = FALL_STATE_NORMAL;
            }
            break;

        case FALL_STATE_IMPACT:
            /* 检测静止: 加速度变化小 */
            if(fabs(detector->accel_magnitude - detector->accel_magnitude_prev) < FALL_STABLE_THRESHOLD)
            {
                detector->state = FALL_STATE_STABLE;
                detector->state_start_time = current_time_ms;
            }
            /* 超时返回正常状态 - 延长到1000ms */
            else if((current_time_ms - detector->state_start_time) > 1000)
            {
                detector->state = FALL_STATE_NORMAL;
            }
            break;

        case FALL_STATE_STABLE:
            /* 检查是否持续静止 */
            if(fabs(detector->accel_magnitude - detector->accel_magnitude_prev) < FALL_STABLE_THRESHOLD)
            {
                /* 静止时间足够长,判定为摔倒 */
                if((current_time_ms - detector->state_start_time) >= FALL_STABLE_TIME_MS)
                {
                    detector->state = FALL_STATE_DETECTED;
                    detector->fall_detected = 1;
                    detector->last_detect_time = current_time_ms;
                    return 1;  /* 检测到摔倒 */
                }
            }
            else
            {
                /* 不再静止,返回正常状态 */
                detector->state = FALL_STATE_NORMAL;
            }
            break;

        case FALL_STATE_DETECTED:
            /* 等待冷却时间后返回正常状态 */
            if((current_time_ms - detector->last_detect_time) >= FALL_DETECT_COOLDOWN_MS)
            {
                detector->state = FALL_STATE_NORMAL;
                detector->fall_detected = 0;
            }
            break;
    }

    return 0;
}

/**
 * @brief       重置摔倒检测器
 * @param       detector: 检测器指针
 * @retval      无
 */
void fall_detect_reset(fall_detector_t *detector)
{
    detector->state = FALL_STATE_NORMAL;
    detector->fall_detected = 0;
}

/**
 * @brief       获取状态字符串
 * @param       state: 状态
 * @retval      状态字符串
 */
const char* fall_detect_get_state_string(fall_state_t state)
{
    switch(state)
    {
        case FALL_STATE_NORMAL:     return "Normal";
        case FALL_STATE_FREEFALL:   return "FreeFall";
        case FALL_STATE_IMPACT:     return "Impact";
        case FALL_STATE_STABLE:     return "Stable";
        case FALL_STATE_DETECTED:   return "FALL!";
        default:                    return "Unknown";
    }
}
