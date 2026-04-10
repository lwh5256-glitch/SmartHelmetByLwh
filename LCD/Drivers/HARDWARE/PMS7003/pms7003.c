#include "pms7003.h"
#include "string.h"

/* UART接收缓冲区 */
#define PMS7003_BUFFER_SIZE 64
static uint8_t rx_buffer[PMS7003_BUFFER_SIZE];
static uint8_t rx_index = 0;

/* PMS7003数据 */
static PMS7003_Data_t pms_data = {0};

/* UART句柄 (外部定义) */
extern UART_HandleTypeDef huart1;

/**
 * @brief PMS7003初始化
 */
void PMS7003_Init(void)
{
    memset(&pms_data, 0, sizeof(PMS7003_Data_t));
    rx_index = 0;

    /* 启动UART接收中断 */
    HAL_UART_Receive_IT(&huart1, &rx_buffer[rx_index], 1);
}

/**
 * @brief 校验和验证
 * @param data: 数据缓冲区
 * @return 1-校验通过, 0-校验失败
 */
static uint8_t PMS7003_CheckSum(uint8_t *data)
{
    uint16_t sum = 0;
    uint16_t checksum;

    /* 计算校验和: 从帧头到校验和前的所有字节 */
    for(int i = 0; i < 30; i++)
    {
        sum += data[i];
    }

    /* 获取数据帧中的校验和 */
    checksum = (data[30] << 8) | data[31];

    return (sum == checksum) ? 1 : 0;
}

/**
 * @brief 解析PMS7003数据帧
 * @param data: 数据缓冲区
 */
static void PMS7003_ParseData(uint8_t *data)
{
    uint16_t frame_len;

    /* 验证帧头 */
    if(data[0] != 0x42 || data[1] != 0x4D)
    {
        return;
    }

    /* 验证帧长度 (应该是0x001C = 28) */
    frame_len = (data[2] << 8) | data[3];
    if(frame_len != 28)
    {
        return;
    }

    /* 校验和验证 */
    if(!PMS7003_CheckSum(data))
    {
        return;
    }

    /* 解析数据 - PMS7003传输格式：High byte first (大端序) */
    pms_data.pm1_0_cf1 = (data[4] << 8) | data[5];
    pms_data.pm2_5_cf1 = (data[6] << 8) | data[7];
    pms_data.pm10_cf1 = (data[8] << 8) | data[9];
    pms_data.pm1_0_atm = (data[10] << 8) | data[11];
    pms_data.pm2_5_atm = (data[12] << 8) | data[13];
    pms_data.pm10_atm = (data[14] << 8) | data[15];
    pms_data.particles_0_3 = (data[16] << 8) | data[17];
    pms_data.particles_0_5 = (data[18] << 8) | data[19];
    pms_data.particles_1_0 = (data[20] << 8) | data[21];
    pms_data.particles_2_5 = (data[22] << 8) | data[23];
    pms_data.particles_5_0 = (data[24] << 8) | data[25];
    pms_data.particles_10 = (data[26] << 8) | data[27];

    pms_data.valid = 1;
}

/* 帧接收缓冲区 */
static uint8_t frame_buf[32];
static uint8_t frame_index = 0;
static uint8_t rx_byte;

/**
 * @brief UART接收回调 - 在stm32f4xx_it.c中调用
 */
void PMS7003_UART_RxCallback(void)
{
    uint8_t byte = rx_byte;

    if(frame_index == 0)
    {
        /* 等待帧头第一字节 */
        if(byte == 0x42)
        {
            frame_buf[frame_index++] = byte;
        }
    }
    else if(frame_index == 1)
    {
        /* 等待帧头第二字节 */
        if(byte == 0x4D)
        {
            frame_buf[frame_index++] = byte;
        }
        else
        {
            frame_index = 0;
        }
    }
    else
    {
        frame_buf[frame_index++] = byte;
        if(frame_index >= 32)
        {
            /* 收到完整帧，解析数据 */
            PMS7003_ParseData(frame_buf);
            frame_index = 0;
        }
    }

    /* 继续接收下一字节 */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

/**
 * @brief 获取PMS7003数据
 */
PMS7003_Data_t* PMS7003_GetData(void)
{
    return &pms_data;
}

/**
 * @brief PMS7003处理函数 (主循环调用)
 */
void PMS7003_Process(void)
{
    /* 数据处理在中断回调中完成，此函数保留供扩展 */
}
