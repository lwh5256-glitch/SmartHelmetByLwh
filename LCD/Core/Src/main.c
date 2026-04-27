/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd.h"
#include "delay.h"
#include "IIC.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "mpu6050.h"
#include "fall_detect.h"
#include "beep.h"
#include "key.h"
#include "esp8266.h"
#include "pms7003.h"
#include "ds18b20.h"
#include "stdio.h"                                      
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
float pitch,roll,yaw;
int int_part;
uint8_t display_buf[32];
fall_detector_t fall_detector;
uint32_t system_time_ms = 0;
uint8_t beep_test_mode = 0;  /* 保留报警屏蔽标志，当前不再由按键切换 */
uint8_t wifi_config_mode = 0;  /* WiFi 配网模式: 0-关闭, 1-SmartConfig */
uint8_t wifi_connected = 0;    /* WiFi 连接状态 */
uint8_t atkcld_connected = 0;  /* 原子云连接状态 */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WIFI_ALARM_MUTE_MS             15000U
#define LIGHT_SENSOR_DARK_STATE        GPIO_PIN_SET
#define DS18B20_CONVERT_MS             750U
#define DS18B20_REFRESH_MS             2000U

#define LCD_Y_TITLE                    5
#define LCD_Y_INIT                     35
#define LCD_Y_STATUS_TITLE             60
#define LCD_Y_STATUS_LINE1             80
#define LCD_Y_STATUS_LINE2             100
#define LCD_Y_AIR_TITLE                125
#define LCD_Y_AIR_PM                   145
#define LCD_Y_TEMP_BODY                165
#define LCD_Y_TEMP_ENV                 185
#define LCD_Y_MOTION_TITLE             205
#define LCD_Y_MOTION_X                 225
#define LCD_Y_MOTION_Y                 245
#define LCD_Y_MOTION_Z                 265
#define LCD_Y_NET                      295
#define LCD_Y_LIGHT                    315
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void format_temp_x10(char *buf, const char *label, int16_t temp_x10, uint8_t valid)
{
    uint16_t abs_temp;

    if (!valid)
    {
        sprintf(buf, "%s:--.-C       ", label);
        return;
    }

    abs_temp = (temp_x10 < 0) ? (uint16_t)(-temp_x10) : (uint16_t)temp_x10;
    sprintf(buf, "%s:%c%3d.%1dC    ",
            label,
            (temp_x10 < 0) ? '-' : ' ',
            abs_temp / 10,
            abs_temp % 10);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();  /* ESP8266 使用 USART3 */
  /* USER CODE BEGIN 2 */
    uint8_t t = 0;
    uint8_t mpu_status = 0;
    short accel_x, accel_y, accel_z;
    short gyro_x, gyro_y, gyro_z;
    uint8_t fall_detected = 0;
    uint8_t alarm_count = 0;
    uint8_t key_val = 0;
    PMS7003_Data_t *pms_data;
    uint8_t pm25_alarm = 0;
    uint8_t light_is_dark = 0;
    uint8_t fill_light_auto = 1;
    uint8_t fill_light_on = 0;
    int16_t temp_body_x10 = 0;
    int16_t temp_env_x10 = 0;
    uint8_t temp_body_valid = 0;
    uint8_t temp_env_valid = 0;
    uint8_t temp_convert_pending = 0;
    uint32_t temp_convert_start_ms = 0;
    uint32_t temp_refresh_timer = 0;
    uint32_t atkcld_publish_timer = 0;
    uint32_t wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

    delay_init(168);
    lcd_init();
    beep_init();
    key_init();
    DS18B20_Init();
    DS18B20_StartConvertAll();
    temp_convert_pending = 1;
    temp_convert_start_ms = HAL_GetTick();

    /* 初始化 PMS7003 */
    PMS7003_Init();

    /* 标题，使用 24 号字体 */
    lcd_show_string(30, LCD_Y_TITLE, 200, 24, 24, "Smart Helmet", RED);

    /* 初始化 MPU6050 */
    lcd_show_string(10, LCD_Y_INIT, 110, 16, 16, "MPU:Init...", BLUE);
    mpu_status = MPU_Init();
    if(mpu_status == 0)
    {
        lcd_show_string(10, LCD_Y_INIT, 110, 16, 16, "MPU:OK    ", BLUE);
    }
    else
    {
        lcd_show_string(10, LCD_Y_INIT, 110, 16, 16, "MPU:Error ", RED);
        delay_ms(2000);
    }

    /* 初始化摔倒检测器 */
    fall_detect_init(&fall_detector);

    /* 分组 1: 系统状态 */
    lcd_show_string(10, LCD_Y_STATUS_TITLE, 220, 16, 16, "== Status ==", BLUE);
    lcd_show_string(10, LCD_Y_STATUS_LINE1, 100, 16, 16, "System:", BLACK);
    lcd_show_string(70, LCD_Y_STATUS_LINE1, 150, 16, 16, "Normal    ", BLUE);
    lcd_show_string(10, LCD_Y_STATUS_LINE2, 100, 16, 16, "State:", BLACK);

    /* 分组 2: 空气质量和温度 */
    lcd_show_string(10, LCD_Y_AIR_TITLE, 220, 16, 16, "== Air Quality ==", BLUE);
    lcd_show_string(10, LCD_Y_AIR_PM, 220, 16, 16, "PM2.5:            ", BLACK);
    lcd_show_string(10, LCD_Y_TEMP_BODY, 220, 16, 16, "BodyT:--.-C       ", BLACK);
    lcd_show_string(10, LCD_Y_TEMP_ENV, 220, 16, 16, "EnvT:--.-C        ", BLACK);

    /* 分组 3: 加速度 */
    lcd_show_string(10, LCD_Y_MOTION_TITLE, 220, 16, 16, "== Acceleration ==", BLUE);
    lcd_show_string(10, LCD_Y_MOTION_X, 100, 16, 16, "Accel X:", BLACK);
    lcd_show_string(10, LCD_Y_MOTION_Y, 100, 16, 16, "Accel Y:", BLACK);
    lcd_show_string(10, LCD_Y_MOTION_Z, 100, 16, 16, "Accel Z:", BLACK);

    /* 分组 4: 网络与补光 */
    lcd_show_string(10, LCD_Y_NET, 110, 16, 16, "WiFi:      ", BLACK);
    lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:      ", BLACK);
    lcd_show_string(10, LCD_Y_LIGHT, 120, 16, 16, "MODE:AUTO     ", BLUE);
    lcd_show_string(130, LCD_Y_LIGHT, 110, 16, 16, "LAMP:OFF", BLACK);

    /* 初始化 ESP8266 */
    lcd_show_string(130, LCD_Y_INIT, 110, 16, 16, "WiFi:Init...", BLUE);
    esp8266_init();

    if(esp8266_check() == 0)
    {
        lcd_show_string(130, LCD_Y_INIT, 110, 16, 16, "WiFi:OK    ", BLUE);
        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

        /* 上电后自动尝试连接 WiFi */
        lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Connect ", BLUE);
        if(esp8266_auto_connect() == 0)
        {
            wifi_connected = 1;
            lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:OK      ", GREEN);
            wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

            /* 连接原子云 */
            lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:Conn..", BLUE);
            if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
            {
                atkcld_connected = 1;
                lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:OK    ", GREEN);
                wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
            }
            else
            {
                lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:Error ", RED);
                wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
            }
        }
        else
        {
            lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Config  ", BLUE);
            wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
        }
    }
    else
    {
        lcd_show_string(130, LCD_Y_INIT, 110, 16, 16, "WiFi:Error ", RED);
        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
    }

    delay_ms(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
        /* 按键扫描 */
        key_val = key_scan(0);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0));

        if(key_val == KEY_UP_PRESS)
        {
            /* PA0 按下: 启动 SmartConfig，或停止配网 / 断开 WiFi */
            if(!wifi_config_mode && !wifi_connected)
            {
                /* 当前未配网且未连接，启动 SmartConfig */
                wifi_config_mode = 1;
                lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Config  ", BLUE);
                if(esp8266_start_smartconfig() == 0)
                {
                    lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Waiting ", BLUE);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
                else
                {
                    lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:CfgErr  ", RED);
                    wifi_config_mode = 0;
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
            }
            else
            {
                /* 当前正在配网或已连接，执行停止 / 断开 */
                if(wifi_config_mode)
                {
                    esp8266_stop_smartconfig();
                    wifi_config_mode = 0;
                    lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Stopped ", BLACK);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
                else if(wifi_connected)
                {
                    if(atkcld_connected)
                    {
                        esp8266_disconnect_atkcld();
                        atkcld_connected = 0;
                    }
                    esp8266_disconnect_ap();
                    wifi_connected = 0;
                    lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:Off     ", BLACK);
                    lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:      ", BLACK);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
            }
        }
        else if(key_val == KEY0_PRESS)
        {
            /* PE4按下: 切换自动/手动补光模式 */
            fill_light_auto = !fill_light_auto;
            if(!fill_light_auto)
            {
                /* 切换到手动模式时，翻转当前灯状态 */
                fill_light_on = !fill_light_on;
                HAL_GPIO_WritePin(FILL_LIGHT_GPIO_Port, FILL_LIGHT_Pin, fill_light_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
                lcd_show_string(10, LCD_Y_LIGHT, 120, 16, 16, "MODE:MAN      ", BLACK);
                lcd_show_string(130, LCD_Y_LIGHT, 110, 16, 16, fill_light_on ? "LAMP:ON " : "LAMP:OFF", fill_light_on ? BLUE : BLACK);
            }
            else
            {
                /* 切换回自动模式 */
                lcd_show_string(10, LCD_Y_LIGHT, 120, 16, 16, "MODE:AUTO     ", BLUE);
            }
        }

        if (t % 5 == 0)    /* 每 50 ms 处理一次周期任务 */
        {
            light_is_dark = (HAL_GPIO_ReadPin(LIGHT_SENSOR_GPIO_Port, LIGHT_SENSOR_Pin) == LIGHT_SENSOR_DARK_STATE);
            if(fill_light_auto)
            {
                fill_light_on = light_is_dark ? 1 : 0;
                HAL_GPIO_WritePin(FILL_LIGHT_GPIO_Port, FILL_LIGHT_Pin, fill_light_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
                lcd_show_string(10, LCD_Y_LIGHT, 120, 16, 16, "MODE:AUTO     ", BLUE);
                lcd_show_string(130, LCD_Y_LIGHT, 110, 16, 16, fill_light_on ? "LAMP:ON " : "LAMP:OFF", fill_light_on ? BLUE : BLACK);
            }

            if(temp_convert_pending && (HAL_GetTick() - temp_convert_start_ms >= DS18B20_CONVERT_MS))
            {
                temp_body_valid = (DS18B20_ReadTempX10(DS18B20_CH_BODY, &temp_body_x10) == 0);
                temp_env_valid = (DS18B20_ReadTempX10(DS18B20_CH_ENV, &temp_env_x10) == 0);

                format_temp_x10((char *)display_buf, "BodyT", temp_body_x10, temp_body_valid);
                lcd_show_string(10, LCD_Y_TEMP_BODY, 220, 16, 16, (char *)display_buf, temp_body_valid ? BLACK : RED);
                format_temp_x10((char *)display_buf, "EnvT", temp_env_x10, temp_env_valid);
                lcd_show_string(10, LCD_Y_TEMP_ENV, 220, 16, 16, (char *)display_buf, temp_env_valid ? BLACK : RED);

                temp_convert_pending = 0;
                temp_refresh_timer = HAL_GetTick();
            }

            if(!temp_convert_pending && (HAL_GetTick() - temp_refresh_timer >= DS18B20_REFRESH_MS))
            {
                DS18B20_StartConvertAll();
                temp_convert_pending = 1;
                temp_convert_start_ms = HAL_GetTick();
            }

            /* 检查 SmartConfig 配网状态 */
            if(wifi_config_mode)
            {
                if(strstr((char *)esp8266_rx_buf, "WIFI GOT IP") != NULL)
                {
                    wifi_config_mode = 0;
                    wifi_connected = 1;
                    esp8266_stop_smartconfig();
                    lcd_show_string(10, LCD_Y_NET, 120, 16, 16, "WiFi:OK      ", GREEN);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

                    /* 连接原子云 */
                    lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:Conn..", BLUE);
                    if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
                    {
                        atkcld_connected = 1;
                        lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:OK    ", GREEN);
                        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                    }
                    else
                    {
                        lcd_show_string(130, LCD_Y_NET, 110, 16, 16, "Cld:Error ", RED);
                        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                    }
                }
            }

            /* 读取 PMS7003 数据 */
            pms_data = PMS7003_GetData();
            if(pms_data->valid)
            {
                /* 显示 PM2.5，并在超阈值时给出告警提示 */
                if(pms_data->pm2_5_cf1 > 1000)
                {
                    sprintf((char *)display_buf,"PM2.5:%4d ALARM!", pms_data->pm2_5_cf1);
                    lcd_show_string(10, LCD_Y_AIR_PM, 220, 16, 16, (char *)display_buf, RED);
                    pm25_alarm = 1;
                }
                else
                {
                    sprintf((char *)display_buf,"PM2.5:%4d ug/m3 ", pms_data->pm2_5_cf1);
                    lcd_show_string(10, LCD_Y_AIR_PM, 220, 16, 16, (char *)display_buf, BLACK);
                    pm25_alarm = 0;
                }

                /* 定时向原子云上报 PM2.5 数据，每 5 秒一次 */
                if(atkcld_connected && (system_time_ms - atkcld_publish_timer >= 5000))
                {
                    atkcld_publish_timer = system_time_ms;
                    char data[32];
                    sprintf(data, "PM2.5:%d\r\n", pms_data->pm2_5_cf1);
                    esp8266_send_atkcld_data(data);
                }
            }

            /* 读取 MPU6050 加速度数据 */
            if(mpu_status == 0)
            {
                if(MPU_Get_Accelerometer(&accel_x, &accel_y, &accel_z) == 0)
                {
                    fall_detected = fall_detect_update(&fall_detector, accel_x, accel_y, accel_z, system_time_ms);

                    sprintf((char *)display_buf,"Accel X:%6d", accel_x);
                    lcd_show_string(10, LCD_Y_MOTION_X, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Y:%6d", accel_y);
                    lcd_show_string(10, LCD_Y_MOTION_Y, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Z:%6d", accel_z);
                    lcd_show_string(10, LCD_Y_MOTION_Z, 200, 16, 16, (char *)display_buf, BLACK);

                    sprintf((char *)display_buf,"%-10s", fall_detect_get_state_string(fall_detector.state));
                    lcd_show_string(70, LCD_Y_STATUS_LINE2, 150, 16, 16, (char *)display_buf, BLUE);

                    if(fall_detected)
                    {
                        lcd_show_string(70, LCD_Y_STATUS_LINE1, 150, 16, 16, "FALL!!    ", RED);
                        if(!beep_test_mode)
                        {
                            beep_on();
                            alarm_count = 0;
                        }
                    }
                }
            }
        }

        /* 告警控制，摔倒告警优先级最高 */
        if(!beep_test_mode && fall_detector.state == FALL_STATE_DETECTED)
        {
            lcd_show_string(70, LCD_Y_STATUS_LINE1, 150, 16, 16, "FALL!!    ", RED);
            if(alarm_count < 60)
            {
                if(alarm_count % 10 == 0)
                {
                    beep_toggle();
                }
                alarm_count++;
            }
            else
            {
                beep_off();
            }
        }
        else if(!beep_test_mode && pm25_alarm)
        {
            lcd_show_string(70, LCD_Y_STATUS_LINE1, 150, 16, 16, "PM2.5!!   ", RED);
            beep_on();
        }
        else if(!beep_test_mode)
        {
            lcd_show_string(70, LCD_Y_STATUS_LINE1, 150, 16, 16, "Normal    ", BLUE);
            beep_off();
        }

        delay_ms(10);
        t++;
        system_time_ms += 10;
        if (t == 20)
        {
            t = 0;
        }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
