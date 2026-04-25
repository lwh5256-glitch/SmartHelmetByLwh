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
uint8_t beep_test_mode = 0;  /* 保留报警屏蔽标志，当前不再由按键切换 */
uint8_t wifi_config_mode = 0;  /* WiFi閰嶇綉妯″紡: 0-鍏抽棴, 1-SmartConfig */
uint8_t wifi_connected = 0;    /* WiFi杩炴帴鐘舵€?*/
uint8_t atkcld_connected = 0;  /* 鍘熷瓙浜戣繛鎺ョ姸鎬?*/
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WIFI_ALARM_MUTE_MS             15000U
#define LIGHT_SENSOR_DARK_STATE        GPIO_PIN_SET
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
  MX_USART3_UART_Init();  /* ESP8266浣跨敤USART3 */
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
    uint8_t light_is_dark = 0;
    uint8_t fill_light_auto = 1;
    uint8_t fill_light_on = 0;
    uint32_t atkcld_publish_timer = 0;
    uint32_t wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

    delay_init(168);
    lcd_init();
    beep_init();
    key_init();

    /* PMS7003鍒濆鍖?*/
    PMS7003_Init();

    /* 鏍囬 - 浣跨敤24鍙峰瓧浣?*/
    lcd_show_string(30, 5, 200, 24, 24, "Smart Helmet", RED);

    /* MPU6050鍒濆鍖?*/
    lcd_show_string(10, 35, 110, 16, 16, "MPU:Init...", BLUE);
    mpu_status = MPU_Init();
    if(mpu_status == 0)
    {
        lcd_show_string(10, 35, 110, 16, 16, "MPU:OK    ", BLUE);
    }
    else
    {
        lcd_show_string(10, 35, 110, 16, 16, "MPU:Error ", RED);
        delay_ms(2000);
    }

    /* 鍒濆鍖栨憯鍊掓娴嬪櫒 */
    fall_detect_init(&fall_detector);

    /* 鍒嗙粍1: 绯荤粺鐘舵€?*/
    lcd_show_string(10, 60, 220, 16, 16, "== Status ==", BLUE);
    lcd_show_string(10, 80, 100, 16, 16, "System:", BLACK);
    lcd_show_string(70, 80, 150, 16, 16, "Normal    ", BLUE);
    lcd_show_string(10, 100, 100, 16, 16, "State:", BLACK);

    /* 鍒嗙粍2: 绌烘皵璐ㄩ噺 */
    lcd_show_string(10, 125, 220, 16, 16, "== Air Quality ==", BLUE);
    lcd_show_string(10, 145, 220, 16, 16, "PM2.5:            ", BLACK);
    lcd_show_string(10, 165, 220, 16, 16, "Dust:             ", BLACK);

    /* 鍒嗙粍3: 鍔犻€熷害 */
    lcd_show_string(10, 190, 220, 16, 16, "== Acceleration ==", BLUE);
    lcd_show_string(10, 210, 100, 16, 16, "Accel X:", BLACK);
    lcd_show_string(10, 230, 100, 16, 16, "Accel Y:", BLACK);
    lcd_show_string(10, 250, 100, 16, 16, "Accel Z:", BLACK);

    /* 鍒嗙粍4: 缃戠粶 */
    lcd_show_string(10, 275, 220, 16, 16, "== Network ==", BLUE);
    lcd_show_string(10, 295, 110, 16, 16, "WiFi:      ", BLACK);
    lcd_show_string(10, 315, 220, 16, 16, "MODE:AUTO     ", BLUE);
    lcd_show_string(130, 315, 110, 16, 16, "LAMP:OFF", BLACK);

    /* ESP8266鍒濆鍖?*/
    lcd_show_string(130, 35, 110, 16, 16, "WiFi:Init...", BLUE);
    esp8266_init();

    if(esp8266_check() == 0)
    {
        lcd_show_string(130, 35, 110, 16, 16, "WiFi:OK    ", BLUE);
        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

        /* 涓婄數鑷姩灏濊瘯鑱旂綉 */
        lcd_show_string(10, 295, 220, 16, 16, "WiFi:Connect..    ", BLUE);
        if(esp8266_auto_connect() == 0)
        {
            wifi_connected = 1;
            lcd_show_string(10, 295, 220, 16, 16, "WiFi:Connected    ", GREEN);
            wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

            /* 杩炴帴鍘熷瓙浜?*/
            lcd_show_string(130, 295, 110, 16, 16, "Cld:Conn..", BLUE);
            if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
            {
                atkcld_connected = 1;
                lcd_show_string(130, 295, 110, 16, 16, "Cld:OK    ", GREEN);
                wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
            }
            else
            {
                lcd_show_string(130, 295, 110, 16, 16, "Cld:Error ", RED);
                wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
            }
        }
        else
        {
            lcd_show_string(10, 295, 220, 16, 16, "WiFi:Need Config  ", BLUE);
            wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
        }
    }
    else
    {
        lcd_show_string(130, 35, 110, 16, 16, "WiFi:Error ", RED);
        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
    }

    delay_ms(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
        /* 鎸夐敭鎵弿 */
        key_val = key_scan(0);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0));

        if(key_val == KEY_UP_PRESS)
        {
            /* PA0鎸変笅: 鐘舵€佸垏鎹?- 鍚姩SmartConfig / 鍋滄閰嶇綉鎴栨柇寮€WiFi */
            if(!wifi_config_mode && !wifi_connected)
            {
                /* 褰撳墠鏈厤缃戜笖鏈繛鎺?-> 鍚姩SmartConfig */
                wifi_config_mode = 1;
                lcd_show_string(10, 295, 220, 16, 16, "WiFi:Config...    ", BLUE);
                if(esp8266_start_smartconfig() == 0)
                {
                    lcd_show_string(10, 295, 220, 16, 16, "WiFi:Waiting...   ", BLUE);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
                else
                {
                    lcd_show_string(10, 295, 220, 16, 16, "WiFi:Cfg Error    ", RED);
                    wifi_config_mode = 0;
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                }
            }
            else
            {
                /* 褰撳墠姝ｅ湪閰嶇綉鎴栧凡杩炴帴 -> 鍋滄/鏂紑 */
                if(wifi_config_mode)
                {
                    esp8266_stop_smartconfig();
                    wifi_config_mode = 0;
                    lcd_show_string(10, 295, 220, 16, 16, "WiFi:Stopped      ", BLACK);
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
                    lcd_show_string(10, 295, 220, 16, 16, "WiFi:Disconnected ", BLACK);
                    lcd_show_string(130, 295, 110, 16, 16, "Cld:      ", BLACK);
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
                lcd_show_string(10, 315, 220, 16, 16, "MODE:MAN      ", BLACK);
                lcd_show_string(130, 315, 110, 16, 16, fill_light_on ? "LAMP:ON " : "LAMP:OFF", fill_light_on ? BLUE : BLACK);
            }
            else
            {
                /* 切换回自动模式 */
                lcd_show_string(10, 315, 220, 16, 16, "MODE:AUTO     ", BLUE);
            }
        }

        if (t % 5 == 0)    /* 姣?0ms璇诲彇涓€娆?*/
        {
            light_is_dark = (HAL_GPIO_ReadPin(LIGHT_SENSOR_GPIO_Port, LIGHT_SENSOR_Pin) == LIGHT_SENSOR_DARK_STATE);
            if(fill_light_auto)
            {
                fill_light_on = light_is_dark ? 1 : 0;
                HAL_GPIO_WritePin(FILL_LIGHT_GPIO_Port, FILL_LIGHT_Pin, fill_light_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
                lcd_show_string(10, 315, 220, 16, 16, "MODE:AUTO     ", BLUE);
                lcd_show_string(130, 315, 110, 16, 16, fill_light_on ? "LAMP:ON " : "LAMP:OFF", fill_light_on ? BLUE : BLACK);
            }

            /* 妫€鏌martConfig閰嶇綉鐘舵€?*/
            if(wifi_config_mode)
            {
                if(strstr((char *)esp8266_rx_buf, "WIFI GOT IP") != NULL)
                {
                    wifi_config_mode = 0;
                    wifi_connected = 1;
                    esp8266_stop_smartconfig();
                    lcd_show_string(10, 295, 220, 16, 16, "WiFi:Connected    ", GREEN);
                    wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;

                    /* 杩炴帴鍘熷瓙浜?*/
                    lcd_show_string(130, 295, 110, 16, 16, "Cld:Conn..", BLUE);
                    if(esp8266_connect_atkcld(ATKCLD_DEVICE_ID, ATKCLD_DEVICE_PWD) == 0)
                    {
                        atkcld_connected = 1;
                        lcd_show_string(130, 295, 110, 16, 16, "Cld:OK    ", GREEN);
                        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                    }
                    else
                    {
                        lcd_show_string(130, 295, 110, 16, 16, "Cld:Error ", RED);
                        wifi_mute_until_ms = HAL_GetTick() + WIFI_ALARM_MUTE_MS;
                    }
                }
            }

            /* 璇诲彇PMS7003鏁版嵁 */
            pms_data = PMS7003_GetData();
            if(pms_data->valid)
            {
                /* 鏄剧ずPM2.5 - 甯︽姤璀︽彁绀?*/
                if(pms_data->pm2_5_cf1 > 1000)
                {
                    sprintf((char *)display_buf,"PM2.5:%4d ALARM!", pms_data->pm2_5_cf1);
                    lcd_show_string(10, 145, 220, 16, 16, (char *)display_buf, RED);
                    pm25_alarm = 1;
                }
                else
                {
                    sprintf((char *)display_buf,"PM2.5:%4d ug/m3 ", pms_data->pm2_5_cf1);
                    lcd_show_string(10, 145, 220, 16, 16, (char *)display_buf, BLACK);
                    pm25_alarm = 0;
                }

                /* 杩戜技绮夊皹娴撳害鎹㈢畻: dust(mg/m3) = pm2.5(ug/m3) / 1000.0 */
                float dust_mg = pms_data->pm2_5_cf1 / 1000.0f;
                sprintf((char *)display_buf,"Dust:%.3f mg/m3  ", dust_mg);
                lcd_show_string(10, 165, 220, 16, 16, (char *)display_buf, BLACK);

                /* 鍘熷瓙浜戝畾鏈熶笂鎶M2.5鏁版嵁 (姣?绉? */
                if(atkcld_connected && (system_time_ms - atkcld_publish_timer >= 5000))
                {
                    atkcld_publish_timer = system_time_ms;
                    char data[32];
                    sprintf(data, "PM2.5:%d\r\n", pms_data->pm2_5_cf1);
                    esp8266_send_atkcld_data(data);
                }
            }

            /* 璇诲彇MPU6050鍔犻€熷害鏁版嵁 */
            if(mpu_status == 0)
            {
                if(MPU_Get_Accelerometer(&accel_x, &accel_y, &accel_z) == 0)
                {
                    fall_detected = fall_detect_update(&fall_detector, accel_x, accel_y, accel_z, system_time_ms);

                    sprintf((char *)display_buf,"Accel X:%6d", accel_x);
                    lcd_show_string(10, 210, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Y:%6d", accel_y);
                    lcd_show_string(10, 230, 200, 16, 16, (char *)display_buf, BLACK);
                    sprintf((char *)display_buf,"Accel Z:%6d", accel_z);
                    lcd_show_string(10, 250, 200, 16, 16, (char *)display_buf, BLACK);

                    sprintf((char *)display_buf,"%-10s", fall_detect_get_state_string(fall_detector.state));
                    lcd_show_string(70, 100, 150, 16, 16, (char *)display_buf, BLUE);

                    if(fall_detected)
                    {
                        lcd_show_string(70, 80, 150, 16, 16, "FALL!!    ", RED);
                        if(!beep_test_mode)
                        {
                            beep_on();
                            alarm_count = 0;
                        }
                    }
                }
            }
        }

        /* 鎽斿€掓姤璀︽帶鍒?- 鎽斿€掓娴嬩紭鍏堢骇鏈€楂?*/
        if(!beep_test_mode && fall_detector.state == FALL_STATE_DETECTED)
        {
            lcd_show_string(70, 80, 150, 16, 16, "FALL!!    ", RED);
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
            lcd_show_string(70, 80, 150, 16, 16, "PM2.5!!   ", RED);
            beep_on();
        }
        else if(!beep_test_mode)
        {
            lcd_show_string(70, 80, 150, 16, 16, "Normal    ", BLUE);
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
