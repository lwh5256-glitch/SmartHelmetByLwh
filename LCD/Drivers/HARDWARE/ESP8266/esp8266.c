/**
 * @file        esp8266.c
 * @author      陆文豪
 * @version     V1.0
 * @date        2026-04-01
 * @brief       ESP8266 WiFi模块驱动代码 (USART3: PB10-TX, PB11-RX)
 */

#include "esp8266.h"
#include "usart.h"

/* 接收缓冲区 */
uint8_t esp8266_rx_buf[ESP8266_RX_BUF_SIZE];
uint16_t esp8266_rx_cnt = 0;

/* WiFi配置 */
static wifi_config_t wifi_config = {0};

/* USART3句柄 - 外部定义 */
extern UART_HandleTypeDef huart3;

/**
 * @brief ESP8266初始化
 */
void esp8266_init(void)
{
    /* 初始化USART3 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 115200;  /* 先尝试115200 */
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }

    /* 使能USART3接收中断 */
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    esp8266_clear_buf();

    /* 等待模块启动 */
    HAL_Delay(1000);
}

/**
 * @brief 清空接收缓冲区
 */
void esp8266_clear_buf(void)
{
    memset(esp8266_rx_buf, 0, ESP8266_RX_BUF_SIZE);
    esp8266_rx_cnt = 0;
}

/**
 * @brief 发送AT命令
 * @param cmd: 命令字符串
 * @param ack: 期望的应答字符串
 * @param timeout: 超时时间(ms)
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_send_cmd(const char *cmd, const char *ack, uint16_t timeout)
{
    esp8266_clear_buf();

    /* 发送命令 */
    HAL_UART_Transmit(&huart3, (uint8_t *)cmd, strlen(cmd), 100);

    /* 等待应答 */
    uint32_t start_time = HAL_GetTick();
    while ((HAL_GetTick() - start_time) < timeout)
    {
        if (strstr((char *)esp8266_rx_buf, ack) != NULL)
        {
            return 0;
        }
        HAL_Delay(10);
    }

    return 1;
}

/**
 * @brief 检查ESP8266是否正常
 * @return 0-正常, 1-异常
 */
uint8_t esp8266_check(void)
{
    uint8_t retry = 3;
    uint32_t baud_rates[] = {115200, 9600, 57600};  /* 尝试常见波特率 */
    uint8_t i;

    for(i = 0; i < 3; i++)
    {
        /* 设置波特率 */
        huart3.Init.BaudRate = baud_rates[i];
        HAL_UART_Init(&huart3);
        HAL_Delay(100);

        retry = 3;
        while (retry--)
        {
            if (esp8266_send_cmd("AT\r\n", "OK", ESP8266_TIMEOUT_SHORT) == 0)
            {
                return 0;  /* 找到正确波特率 */
            }
            HAL_Delay(500);
        }
    }

    return 1;
}

/**
 * @brief 设置ESP8266工作模式
 * @param mode: 工作模式
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_set_mode(esp8266_mode_t mode)
{
    char cmd[32];
    sprintf(cmd, "AT+CWMODE=%d\r\n", mode);
    return esp8266_send_cmd(cmd, "OK", ESP8266_TIMEOUT_MEDIUM);
}

/**
 * @brief 连接WiFi AP
 * @param ssid: WiFi名称
 * @param pwd: WiFi密码
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_connect_ap(const char *ssid, const char *pwd)
{
    char cmd[128];

    /* 设置为Station模式 */
    if (esp8266_set_mode(ESP8266_MODE_STA) != 0)
    {
        return 1;
    }

    /* 如果密码为空，尝试使用ESP8266保存的配置自动连接 */
    if (pwd == NULL || pwd[0] == '\0')
    {
        /* 使能自动连接 */
        esp8266_send_cmd("AT+CWAUTOCONN=1\r\n", "OK", ESP8266_TIMEOUT_MEDIUM);
        HAL_Delay(3000);  /* 等待自动连接 */

        /* 检查是否已连接 */
        if (esp8266_send_cmd("AT+CWJAP?\r\n", "+CWJAP:", ESP8266_TIMEOUT_MEDIUM) == 0)
        {
            return 0;  /* 已连接 */
        }
        return 1;
    }

    /* 连接AP */
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
    return esp8266_send_cmd(cmd, "OK", ESP8266_TIMEOUT_LONG);
}

/**
 * @brief 断开WiFi连接
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_disconnect_ap(void)
{
    return esp8266_send_cmd("AT+CWQAP\r\n", "OK", ESP8266_TIMEOUT_MEDIUM);
}

/**
 * @brief 发送数据
 * @param data: 数据指针
 * @param len: 数据长度
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_send_data(const char *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, len, 1000);
    return 0;
}

/**
 * @brief 启动SmartConfig配网
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_start_smartconfig(void)
{
    /* 设置为Station模式 */
    if (esp8266_set_mode(ESP8266_MODE_STA) != 0)
    {
        return 1;
    }

    /* 启动SmartConfig */
    return esp8266_send_cmd("AT+CWSTARTSMART\r\n", "OK", ESP8266_TIMEOUT_MEDIUM);
}

/**
 * @brief 停止SmartConfig配网
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_stop_smartconfig(void)
{
    return esp8266_send_cmd("AT+CWSTOPSMART\r\n", "OK", ESP8266_TIMEOUT_MEDIUM);
}

/**
 * @brief 保存WiFi配置到内存
 * @param ssid: WiFi名称
 * @param pwd: WiFi密码
 */
void esp8266_save_config(const char *ssid, const char *pwd)
{
    strncpy(wifi_config.ssid, ssid, sizeof(wifi_config.ssid) - 1);
    strncpy(wifi_config.password, pwd, sizeof(wifi_config.password) - 1);
    wifi_config.configured = 1;
}

/**
 * @brief 读取WiFi配置
 * @param config: 配置结构体指针
 * @return 0-有配置, 1-无配置
 */
uint8_t esp8266_load_config(wifi_config_t *config)
{
    if (wifi_config.configured)
    {
        memcpy(config, &wifi_config, sizeof(wifi_config_t));
        return 0;
    }
    return 1;
}

/**
 * @brief 自动连接WiFi（使用已保存的配置）
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_auto_connect(void)
{
    /* 使用ESP8266内部保存的配置自动连接 */
    return esp8266_connect_ap("", "");
}


/**
 * @brief 获取当前连接的AP的SSID
 * @param ssid: 存储SSID的缓冲区
 * @param ssid_len: 缓冲区长度
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_get_connected_ap(char *ssid, uint16_t ssid_len)
{
    char *p1, *p2;

    if (esp8266_send_cmd("AT+CWJAP?\r\n", "OK", ESP8266_TIMEOUT_MEDIUM) != 0)
    {
        return 1;
    }

    /* 解析返回的SSID: +CWJAP:"ssid","mac",channel,rssi */
    p1 = strstr((char *)esp8266_rx_buf, "+CWJAP:\"");
    if (p1 != NULL)
    {
        p1 += 8;  /* 跳过 +CWJAP:" */
        p2 = strstr(p1, "\"");
        if (p2 != NULL && (p2 - p1) < ssid_len)
        {
            strncpy(ssid, p1, p2 - p1);
            ssid[p2 - p1] = '\0';
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 连接原子云服务器
 * @param id: 设备ID
 * @param pwd: 设备密码
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_connect_atkcld(const char *id, const char *pwd)
{
    char cmd[128];
    uint8_t ret;

    sprintf(cmd, "AT+ATKCLDSTA=\"%s\",\"%s\"\r\n", id, pwd);
    ret = esp8266_send_cmd(cmd, "CLOUD CONNECTED", 10000);

    if (ret == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief 断开原子云连接
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_disconnect_atkcld(void)
{
    uint8_t ret;

    ret = esp8266_send_cmd("AT+ATKCLDCLS\r\n", "CLOUD DISCONNECT", 500);

    if (ret == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief 发送数据到原子云（透传模式）
 * @param data: 要发送的数据字符串
 * @return 0-成功, 1-失败
 */
uint8_t esp8266_send_atkcld_data(const char *data)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, strlen(data), 1000);
    return 0;
}
