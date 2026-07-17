/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fsm_logic.h"
#include "ds3231_for_stm32_hal.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

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
  //dichiaro le variabili per il semaforo
  led_t green;
  led_t yellow;
  led_t red;
  timer_t timer;
  button_t button;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  //inizializzo il DS3231 al bus I2C1
  DS3231_Init(&hi2c1);
  //disabilito le interruzioni, le attivo solo dopo aver configurato il DS3231
  __disable_irq();

  //setto l'ora di partenza sul DS3231
  DS3231_SetFullTime(10, 44, 30);
  // giorno=5, mese=6, venerdì=5, anno=2025
  DS3231_SetFullDate(5, 6, 5, 2025);

  //Dice al DS3231 come usare il pin /SQW:
  DS3231_SetInterruptMode(DS3231_ALARM_INTERRUPT); // in questo caso per l allarme
  DS3231_ClearAlarm1Flag(); //pulisco il flag di allarme 1, per far si che non venga generato un altro interrupt

  // Alarm1: venerdì 10:45:00 -> ATTIVO
  DS3231_EnableAlarm1(DS3231_ENABLED);
  DS3231_SetAlarm1Mode(DS3231_A1_MATCH_S_M_H_DAY);// l allarme scatta quando la data e l ora sono uguali a quelle settate
  DS3231_SetAlarm1Second(0);
  DS3231_SetAlarm1Minute(45);
  DS3231_SetAlarm1Hour(10);
  DS3231_SetAlarm1Day(5);

  // Alarm2: venerdì 10:48 -> INATTIVO
  DS3231_ClearAlarm2Flag();
  DS3231_EnableAlarm2(DS3231_ENABLED);
  DS3231_SetAlarm2Mode(DS3231_A2_MATCH_M_H_DAY);
  DS3231_SetAlarm2Minute(48);
  DS3231_SetAlarm2Hour(10);
  DS3231_SetAlarm2Day(5);

  //riabilito le interruzioni
  __enable_irq();

  if (led_init(&green,  LEDV_GPIO_Port, LEDV_Pin, LED_INIT_STATE_OFF) != LED_OK) return -1;
  if (led_init(&yellow, LEDG_GPIO_Port, LEDG_Pin, LED_INIT_STATE_OFF) != LED_OK) return -1;
  if (led_init(&red,    LEDR_GPIO_Port, LEDR_Pin, LED_INIT_STATE_OFF) != LED_OK) return -1;

  if (button_init(&button, BUTTON_GPIO_Port, BUTTON_Pin, BUTTON_INIT_STATE_OFF) != BUTTON_OK)
      return -1;

  if (timer_init(&timer, &htim6, 16000000) != TIMER_OK)
      return -1;

  if (FSM_init(&button, &timer, &green, &yellow, &red) != FSM_OK)
      return -1;

  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (FSM_step() != FSM_OK)
        return -1;
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
