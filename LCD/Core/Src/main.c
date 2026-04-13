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
#include "stdio.h"                                      
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
float pitch,roll,yaw;
int int_part;
uint8_t display_buf[20];
fall_detector_t fall_detector;
uint32_t system_time_ms = 0;
uint8_t beep_test_mode = 0;  /* 蜂鸣器测试模式: 0-关闭, 1-开启 */
uint8_t wifi_config_mode = 0;  /* WiFi配网模式: 0-关闭, 1-SmartConfig */
uint8_t wifi_connected = 0;    /* WiFi连接状态 */
uint8_t atkcld_connected = 0;  /* 原子云连接状态 */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
  MX_USART3_UART_Init();  /* ESP8266使用USART3 */
  /* USER CODE BEGIN 2 */
    uint8_t t = 0;
    uint8_t temperature;
    uint8_t humidity;
    uint8_t mpu_status = 0;
    short accel_x, accel_y, accel_z;
    short gyro_x, gyro_y, gyro_z;
    uint8_t fall_detected = 0;
    uint8_t alarm_count = 0;
    uint8_t key_val = 0;
    PMS7003_Data_t *pms_data;
    uint8_t pm25_alarm = 0;
    uint32_t atkcld_publish_timer = 0;

    delay_init(168);
    lcd_init();
    beep_init();
    key_init();

    /* PMS7003初始化 */
    PMS7003_Init();

    lcd_show_string(30, 50, 200, 16, 16, "Fall Detection", RED);
    lcd_show_string(30, 70, 200, 16, 16, "System", RED);

    /* MPU6050初始化 */
    lcd_show_string(30, 90, 200, 16, 16, "MPU6050 Init...", BLUE);
    mpu_status = MPU_Init();
    if(mpu_status == 0)
    {
        lcd_show_string(30, 90, 200, 16, 16, "MPU6050 OK     ", BLUE);
    }
    else
    {
        lcd_show_string(30, 90, 200, 16, 16, "MPU6050 Error  ", RED);
        delay_ms(2000);
    }

    /* 初始化摔倒检测器 */
    fall_detect_init(&fall_detector);

    lcd_show_string(30, 110, 200, 16, 16, "Status: Normal ", BLUE);
    lcd_show_string(30, 130, 200, 16, 16, "PM2.5:         ", BLACK);
    lcd_show_string(30, 150, 200, 16, 16, "Dust:          ", BLACK);
    lcd_show_string(30, 170, 200, 16, 16, "Accel X:       ", BLACK);
    lcd_show_string(30, 190, 200, 16, 16, "Accel Y:       ", BLACK);
    lcd_show_string(30, 210, 200, 16, 16, "Accel Z:       ", BLACK);
    lcd_show_string(30, 230, 200, 16, 16, "State:         ", BLACK);
    lcd_show_string(30, 250, 200, 16, 16, "WiFi:          ", BLACK);
    lcd_show_string(30, 270, 200, 16, 16, "Cloud:         ", BLACK);
    lcd_show_string(30, 290, 200, 16, 16, "BEEP: OFF     ", BLACK);

    /* ESP8266初始化 */
    lcd_show_string(30, 310, 200, 16, 16, "ESP8266 Init...", BLUE);
    esp8266_init();

    if(esp8266_check() == 0)
    {
        lcd_show_string(30, 310, 200, 16, 16, "ESP8266 OK     ", BLUE);
    }
    else
    {
        lcd_show_string(30, 310, 200, 16, 16, "ESP8266 Error  ", RED);
    }

    delay_ms(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
        /* 按键扫描 */
        key_val = key_scan(0);
        if(key_val == KEY1_PRESS)
        {
            /* KEY1按下: 蜂鸣器开关切换 */
            beep_test_mode = !beep_test_mode;
            if(beep_test_mode)
            {
                beep_on();
                lcd_show_string(30, 290, 200, 16, 16, "BEEP: ON      ", BLUE);
            }
            else
            {
                beep_off();
                lcd_show_string(30, 290, 200, 16, 16, "BEEP: OFF     ", BLACK);
            }
        }
        else if(key_val == KEY_UP_PRESS)
        {
            /* KEY_UP按下: 启动SmartConfig配网 */
            wifi_config_mode = 1;
            lcd_show_string(30, 250, 200, 16, 16, "WiFi: Config...", BLUE);
            if(esp8266_start_smartconfig() == 0)
            {
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Waiting  ", BLUE);
            }
            else
            {
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Cfg Err  ", RED);
                wifi_config_mode = 0;
            }
        }
        else if(key_val == KEY0_PRESS)
        {
            /* KEY0按下: 停止配网/断开WiFi */
            if(wifi_config_mode)
            {
                esp8266_stop_smartconfig();
                wifi_config_mode = 0;
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Stopped  ", BLACK);
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
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Disconn  ", BLACK);
                lcd_show_string(30, 270, 200, 16, 16, "Cloud:         ", BLACK);
            }
        }
        else if(key_val == KEY2_PRESS)
        {
            /* KEY2按下: 尝试连接WiFi */
            lcd_show_string(30, 250, 200, 16, 16, "WiFi: Connect..", BLUE);
            if(esp8266_auto_connect() == 0)
            {
                wifi_connected = 1;
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Connected", GREEN);

                /* 连接原子云 */
                lcd_show_string(30, 270, 200, 16, 16, "Cloud: Connect.", BLUE);
                if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
                {
                    atkcld_connected = 1;
                    lcd_show_string(30, 270, 200, 16, 16, "Cloud: Connected", GREEN);
                }
                else
                {
                    lcd_show_string(30, 270, 200, 16, 16, "Cloud: Conn Err", RED);
                }
            }
            else
            {
                lcd_show_string(30, 250, 200, 16, 16, "WiFi: Conn Err ", RED);
            }
        }

        if (t % 5 == 0)    /* 每50ms读取一次 */
        {
            /* 检查SmartConfig配网状态 */
            if(wifi_config_mode)
            {
                if(strstr((char *)esp8266_rx_buf, "WIFI CONNECTED") != NULL)
                {
                    wifi_config_mode = 0;
                    wifi_connected = 1;
                    esp8266_stop_smartconfig();
                    lcd_show_string(30, 250, 200, 16, 16, "WiFi: Connected", GREEN);

                    /* 连接原子云 */
                    lcd_show_string(30, 270, 200, 16, 16, "Cloud: Connect.", BLUE);
                    if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
                    {
                        atkcld_connected = 1;
                        lcd_show_string(30, 270, 200, 16, 16, "Cloud: Connected", GREEN);
                    }
                    else
                    {
                        lcd_show_string(30, 270, 200, 16, 16, "Cloud: Conn Err", RED);
                    }
                }
            }

            /* 读取PMS7003数据 */
            pms_data = PMS7003_GetData();
            if(pms_data->valid)
            {
                /* 显示PM2.5 */
                sprintf((char *)display_buf,"PM2.5:%4d ug/m3", pms_data->pm2_5_cf1);
                lcd_show_string(30, 130, 200, 16, 16, (char *)display_buf, BLACK);

                /* 近似粉尘浓度换算: dust(mg/m3) = pm2.5(ug/m3) / 1000.0 */
                float dust_mg = pms_data->pm2_5_cf1 / 1000.0f;
                sprintf((char *)display_buf,"Dust:%.3f mg/m3", dust_mg);
                lcd_show_string(30, 150, 200, 16, 16, (char *)display_buf, BLACK);

                if(pms_data->pm2_5_cf1 > 1000)
                {
                    pm25_alarm = 1;
                }
                else
                {
                    pm25_alarm = 0;
                }

                /* 原子云定期上报PM2.5数据 (每5秒) */
                if(atkcld_connected && (system_time_ms - atkcld_publish_timer >= 5000))
                {
                    atkcld_publish_timer = system_time_ms;
                    char data[32];
                    sprintf(data, "PM2.5:%d\r\n", pms_data->pm2_5_cf1);
                    esp8266_send_atkcld_data(data);
                }
            }

            /* 读取MPU6050加速度数据 */
            if(mpu_status == 0)
            {
                if(MPU_Get_Accelerometer(&accel_x, &accel_y, &accel_z) == 0)
                {
                    fall_detected = fall_detect_update(&fall_detector, accel_x, accel_y, accel_z, system_time_ms);

                    sprintf((char *)display_buf,"Accel X:%6d", accel_x);
                    lcd_show_string(30, 170, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Y:%6d", accel_y);
                    lcd_show_string(30, 190, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Z:%6d", accel_z);
                    lcd_show_string(30, 210, 200, 16, 16, (char *)display_buf, BLACK);

                    sprintf((char *)display_buf,"State: %-10s", fall_detect_get_state_string(fall_detector.state));
                    lcd_show_string(30, 230, 200, 16, 16, (char *)display_buf, BLUE);

                    if(fall_detected)
                    {
                        lcd_show_string(30, 110, 200, 16, 16, "Status: FALL!! ", RED);
                        if(!beep_test_mode)
                        {
                            beep_on();
                            alarm_count = 0;
                        }
                    }
                }
            }
        }

        /* 摔倒报警控制 */
        if(!beep_test_mode && fall_detector.state == FALL_STATE_DETECTED)
        {
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
            lcd_show_string(30, 110, 200, 16, 16, "Status: PM2.5!!", RED);
            beep_on();
        }
        else if(!beep_test_mode)
        {
            lcd_show_string(30, 110, 200, 16, 16, "Status: Normal ", BLUE);
            beep_off();
        }

        delay_ms(10);
        t++;
        system_time_ms += 10;

        if (t == 20)
        {
            t = 0;
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
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
