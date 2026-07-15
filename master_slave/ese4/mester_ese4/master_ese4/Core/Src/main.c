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
#include "dma.h"
#include "usart.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_BUFFER_SIZE 10

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t master_rx_buffer[MAX_BUFFER_SIZE];
uint8_t master_tx_buffer[MAX_BUFFER_SIZE];
uint32_t current_time;
uint32_t last_timer_time = 0;
uint32_t somma=0;
float media=0;
char log_buffer[100];                      // Buffer per il logger UART
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
  MX_DMA_Init();
  MX_SPI2_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
	memset(master_tx_buffer, 0x00, MAX_BUFFER_SIZE);

	// DEBUG UART: Invio un messaggio di test sincrono (bloccante) per verificare PuTTY
	// Se non vedi questa stringa all'avvio, il problema è il Baud Rate o il cablaggio TX/RX.
	char msg_test[] = "--- Master Inizializzato e Pronto ---\r\n";
	HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg_test, strlen(msg_test), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  current_time=HAL_GetTick(); //inizio a contare il tempo
	  if(current_time-last_timer_time>=2000){
		  last_timer_time=current_time;
		  HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_RESET);
		  HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t *)master_tx_buffer,(uint8_t *)master_rx_buffer,  MAX_BUFFER_SIZE);
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
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
	if (hspi->Instance == hspi2.Instance) {
		HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET);
		uint8_t n=master_rx_buffer[0];
		if(n>0 && n< MAX_BUFFER_SIZE){
			somma = 0;
			for(uint8_t i=1;i<=n;i++){
				somma+=master_rx_buffer[i];
			}
			media=(float)somma/n;
			int parte_intera = (int)media;
			int parte_decimale = (int)((media - parte_intera) * 10); // Estrae la prima cifra decimale
			if (hlpuart1.gState == HAL_UART_STATE_READY) {
				snprintf(log_buffer, sizeof(log_buffer),"Campioni validi: %d, Temp. media: %d.%d\r\n", n, parte_intera,parte_decimale);
				HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer,strlen(log_buffer));
			}
		}
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	// Callback vuota ma necessaria per confermare il reset dello stato interno dell'HAL UART
	if (huart->Instance == hlpuart1.Instance) {
		// Lo stato torna automaticamente a READY grazie alla libreria HAL
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == hspi2.Instance) {
		HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET); // Rilascia il bus

		if (hlpuart1.gState == HAL_UART_STATE_READY) {
			snprintf(log_buffer, sizeof(log_buffer), "[ERR hardware] Errore SPI sollevato: 0x%08X\r\n", hspi->ErrorCode);
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer, strlen(log_buffer));
		}
	}
}

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
