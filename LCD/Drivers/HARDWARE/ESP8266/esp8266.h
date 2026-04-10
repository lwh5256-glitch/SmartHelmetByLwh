/**
 * @file        esp8266.h
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-04-01
 * @brief       ESP8266 WiFi模块驱动代码 (USART3: PB10-TX, PB11-RX)
 */

#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include <string.h>
#include <stdio.h>

/* ESP8266使用USART3 (PB10-TX, PB11-RX) */
extern UART_HandleTypeDef huart3;

/* 接收缓冲区大小 */
#define ESP8266_RX_BUF_SIZE     1024

/* 超时时间(ms) */
#define ESP8266_TIMEOUT_SHORT   500
#define ESP8266_TIMEOUT_MEDIUM  2000
#define ESP8266_TIMEOUT_LONG    10000

/* ESP8266工作模式 */
typedef enum {
    ESP8266_MODE_STA = 1,       /* Station模式 */
    ESP8266_MODE_AP = 2,        /* AP模式 */
    ESP8266_MODE_AP_STA = 3     /* AP+Station模式 */
} esp8266_mode_t;

/* WiFi配置结构体 */
typedef struct {
    char ssid[32];
    char password[64];
    uint8_t configured;  /* 0-未配置, 1-已配置 */
} wifi_config_t;

/* 原子云配置 */
#define ATKCLD_DEVICE_ID        "04454061963646886703"
#define ATKCLD_DEVICE_PWD       "12345678"

/* 函数声明 */
void esp8266_init(void);
uint8_t esp8266_send_cmd(const char *cmd, const char *ack, uint16_t timeout);
uint8_t esp8266_check(void);
uint8_t esp8266_set_mode(esp8266_mode_t mode);
uint8_t esp8266_connect_ap(const char *ssid, const char *pwd);
uint8_t esp8266_disconnect_ap(void);
uint8_t esp8266_send_data(const char *data, uint16_t len);
void esp8266_clear_buf(void);
uint8_t esp8266_start_smartconfig(void);
uint8_t esp8266_stop_smartconfig(void);
void esp8266_save_config(const char *ssid, const char *pwd);
uint8_t esp8266_load_config(wifi_config_t *config);
uint8_t esp8266_auto_connect(void);
uint8_t esp8266_get_connected_ap(char *ssid, uint16_t ssid_len);
uint8_t esp8266_connect_atkcld(const char *id, const char *pwd);
uint8_t esp8266_disconnect_atkcld(void);
uint8_t esp8266_send_atkcld_data(const char *data);

/* 接收缓冲区 */
extern uint8_t esp8266_rx_buf[ESP8266_RX_BUF_SIZE];
extern uint16_t esp8266_rx_cnt;

#endif /* __ESP8266_H */
