/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (MASTER - Ottimizzazione Numerica)
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
#define UART_MSG_SIZE 100
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
uint8_t master_rx_val = 0xFF;            // Singolo byte di atterraggio dallo Slave
uint8_t master_tx_val = 0xFF;            // Byte dummy per generare il clock SPI

char log_buffer_master[UART_MSG_SIZE];   // Buffer per le stampe su PuTTY
uint8_t uart_master_busy = 0;            // Semaforo software UART
volatile uint8_t spi_master_busy = 0;     // Semaforo software SPI

// Il codice corretto memorizzato direttamente in valore numerico binario
// "011" in binario corrisponde al numero decimale 3
const uint8_t SECRET_CODE_NUM = 3;
uint32_t last_poll_time = 0;             // Per il polling non bloccante
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
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_SPI2_Init();

  /* USER CODE BEGIN 2 */
  snprintf(log_buffer_master, sizeof(log_buffer_master), "Master Numerico Avviato! SECRET_CODE = %d\r\n", SECRET_CODE_NUM);
  HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)log_buffer_master, strlen(log_buffer_master));
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	uint32_t current_time = HAL_GetTick();

	// Polling non bloccante cadenzato ogni 2 secondi
	if (current_time - last_poll_time >= 2000) {
		last_poll_time = current_time;

		if (!spi_master_busy) {
			spi_master_busy = 1;

			// Protezione hardware: forzo il reset del target prima dello scambio
			master_rx_val = 0xFF;

			// 1. ATTIVAZIONE SLAVE (NSS Software): Abbasso manualmente il Chip Select
			HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_RESET);

			// 2. CHIAMATA ASINCRONA: Invio 1 byte dummy per ricevere 1 byte dallo Slave
			HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_IT(&hspi2, &master_tx_val, &master_rx_val, 1);

			// Se la chiamata fallisce immediatamente a livello software, svuoto i semafori
			if (status != HAL_OK) {
				HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET);
				spi_master_busy = 0;

				if (!uart_master_busy) {
					uart_master_busy = 1;
					snprintf(log_buffer_master, sizeof(log_buffer_master), "[POLLING FALLITO] Errore HAL: %d\r\n", status);
					HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)log_buffer_master, strlen(log_buffer_master));
				}
			}
		}
	}
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

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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
// ==============================================================================
// 1. CALLBACK FINE TRASMISSIONE/RICEZIONE SPI (SUCCESSO)
// ==============================================================================
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == hspi2.Instance) {
        // Disattivazione immediata linea CS (NSS Software)
        HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET);

        // Validazione stringente: accettiamo solo valori reali da 0 a 7 (3 bit)
        // Scartiamo 0xFF (Slave a riposo) o rumore elettrico ad alta impedenza
        if (master_rx_val <= 7) {

            if (master_rx_val == SECRET_CODE_NUM) {
                uart_master_busy = 1;
                snprintf(log_buffer_master, sizeof(log_buffer_master),
                         "[MASTER] Ricevuto Valore: %d -> ACCESSO GARANTITO!\r\n", master_rx_val);
            } else {
                uart_master_busy = 1;
                snprintf(log_buffer_master, sizeof(log_buffer_master),
                         "[MASTER] Ricevuto Valore: %d -> ACCESSO NEGATO!\r\n", master_rx_val);
            }
            HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)log_buffer_master, strlen(log_buffer_master));
        }

        // Rilascio il semaforo software SPI per permettere il prossimo polling
        spi_master_busy = 0;
    }
}

// ==============================================================================
// 2. CALLBACK INTERCETTAZIONE ERRORI HARDWARE SPI
// ==============================================================================
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == hspi2.Instance) {
		// Sgancio forzatamente lo Slave rialzando la linea di selezione
		HAL_GPIO_WritePin(NSS_GPIO_Port, NSS_Pin, GPIO_PIN_SET);

		if (!uart_master_busy) {
			uart_master_busy = 1;
			snprintf(log_buffer_master, sizeof(log_buffer_master), "[HARDWARE ERROR] Eccezione sul bus SPI. Errore: 0x%08X\r\n", hspi->ErrorCode);
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)log_buffer_master, strlen(log_buffer_master));
		}

		// Reset forzato del semaforo per non congelare i polling futuri
		spi_master_busy = 0;
	}
}

// ==============================================================================
// 3. CALLBACK FINE TRASMISSIONE UART
// ==============================================================================
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == hlpuart1.Instance) {
        // Rilascio il semaforo software del Logger seriale
        uart_master_busy = 0;
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
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
