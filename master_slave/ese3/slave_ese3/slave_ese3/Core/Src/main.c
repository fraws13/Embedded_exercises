/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (SLAVE - Ottimizzazione Numerica)
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
#define UART_MSG_SIZE  80
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
uint8_t spi_tx_val = 0xFF;               // Singolo byte numerico da inviare (Default: 0xFF)
uint8_t spi_rx_val = 0xFF;               // Byte di atterraggio dummy dal Master
char log_buffer_uart[UART_MSG_SIZE];     // Buffer per i log PuTTY

volatile char rx_char;                   // Singolo byte ricevuto da UART ad ogni interrupt

int idx = 0;                             // Indice di riempimento bit (0 a 3)
uint8_t uart_busy = 0;                   // Semaforo software UART
uint8_t spi_busy = 0;                    // Semaforo software SPI

static uint32_t last_button1_time = 0;   // Tempo ultimo clic Pulsante 1
static uint32_t last_button2_time = 0;   // Tempo ultimo clic Pulsante 2
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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_SPI2_Init();

  /* USER CODE BEGIN 2 */
  snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Slave Numerico Pronto. Inserisci 3 cifre dai pulsanti...\r\n");
  HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)log_buffer_uart, strlen(log_buffer_uart));

  // Abilito la ricezione asincrona in interrupt del tasto Invio da tastiera
  HAL_UART_Receive_IT(&hlpuart1, (uint8_t*)&rx_char, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
// 1. CALLBACK PRESSIONE TASTO INVIO DA TASTIERA UART
// ==============================================================================
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == hlpuart1.Instance) {

		if (rx_char == '\n' || rx_char == '\r') {

			// Antiduplicato seriale: consuma il \n parassita se siamo già a riposo
			if (idx == 0 && spi_tx_val == 0xFF) {
				HAL_UART_Receive_IT(&hlpuart1, (uint8_t*)&rx_char, 1);
				return;
			}

			// CASO ERRORE: Mancano cifre nel buffer
			if (idx < 3) {
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET); // Accendo il LED di errore

				if (!uart_busy) {
					uart_busy = 1;
					snprintf(log_buffer_uart, sizeof(log_buffer_uart), "ERRORE: Inserite solo %d cifre di 3! Reset.\r\n", idx);
					HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
				}
				idx = 0;
				spi_tx_val = 0xFF; // Ripristino stato di vuoto
			}
			// CASO SUCCESSO: Codice a 3 bit pronto ed equivalente a un numero decimale (0-7)
			else {
				if (!spi_busy) {
					spi_busy = 1;
					HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // Spegno eventuale errore precedente

					if (!uart_busy) {
						uart_busy = 1;
						snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Codice intero elaborato: %d. Armamento SPI...\r\n", spi_tx_val);
						HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
					}

					// Armo l'SPI Slave per trasmettere un SINGOLO byte (Dimensione = 1)
					HAL_SPI_TransmitReceive_IT(&hspi2, &spi_tx_val, &spi_rx_val, 1);
				}
				idx = 0;
			}
		}
		HAL_UART_Receive_IT(&hlpuart1, (uint8_t*)&rx_char, 1); // Riarmo l'ascolto UART
	}
}

// ==============================================================================
// 2. CALLBACK FINE TRASMISSIONE SPI (IL MASTER HA COMPLETATO IL POLLING)
// ==============================================================================
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == hspi2.Instance) {
		if (!uart_busy) {
			uart_busy = 1;
			snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Master ha prelevato il dato. Bus Libero!\r\n");
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
		}

		// Forzo la rimessa in stato di riposo della linea (0xFF) per i polling successivi
		spi_tx_val = 0xFF;
		spi_busy = 0;
	}
}

// ==============================================================================
// 3. GESTIONE PULSANTI FISICI INTERRUPT EXTI (BIT SHIFTING REAL-TIME)
// ==============================================================================
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	uint32_t current_time = HAL_GetTick();

	// Sblocco e reset manuale da stato di Errore tramite pressione di un tasto qualsiasi
	if (GPIO_Pin == BUTTON1_Pin || GPIO_Pin == BUTTON2_Pin) {
		if (HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin) == GPIO_PIN_SET) {
			if (GPIO_Pin == BUTTON1_Pin && (current_time - last_button1_time < 200)) return;
			if (GPIO_Pin == BUTTON2_Pin && (current_time - last_button2_time < 200)) return;

			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
			idx = 0;
			spi_tx_val = 0xFF;

			if (GPIO_Pin == BUTTON1_Pin) last_button1_time = current_time;
			if (GPIO_Pin == BUTTON2_Pin) last_button2_time = current_time;

			if (!uart_busy) {
				uart_busy = 1;
				snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Reset completato. Inserisci nuovo codice...\r\n");
				HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
			}
			return; // Consumo il clic solo per resettare l'errore
		}
	}

	// Al primo inserimento utile, pulisco lo stato di riposo portandolo a 0
	if (idx == 0 && spi_tx_val == 0xFF) {
		spi_tx_val = 0;
	}

	// --------------------------------------------------------------------------
	// INSERIMENTO PULSANTE 1 (Equivale a inserire un bit a '1')
	// --------------------------------------------------------------------------
	if (GPIO_Pin == BUTTON1_Pin) {
		if (current_time - last_button1_time < 200) return; // Debounce
		last_button1_time = current_time;

		if (idx >= 3) { // Overflow preventivo
			idx = 0; spi_tx_val = 0xFF;
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			return;
		}

		// Shifto a sinistra di 1 ed eseguo un OR logico con 0x01 per piazzare il bit 1 in coda
		spi_tx_val = (spi_tx_val << 1) | 0x01;
		idx++;

		if (!uart_busy) {
			uart_busy = 1;
			snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Inserito '1' (Cifre complete: %d/3)\r\n", idx);
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
		}
	}

	// --------------------------------------------------------------------------
	// INSERIMENTO PULSANTE 2 (Equivale a inserire un bit a '0')
	// --------------------------------------------------------------------------
	if (GPIO_Pin == BUTTON2_Pin) {
		if (current_time - last_button2_time < 200) return; // Debounce
		last_button2_time = current_time;

		if (idx >= 3) { // Overflow preventivo
			idx = 0; spi_tx_val = 0xFF;
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			return;
		}

		// Shifto a sinistra di 1 ed eseguo un OR con 0x00 (piazza un bit a 0 in coda)
		spi_tx_val = (spi_tx_val << 1) | 0x00;
		idx++;

		if (!uart_busy) {
			uart_busy = 1;
			snprintf(log_buffer_uart, sizeof(log_buffer_uart), "Inserito '0' (Cifre complete: %d/3)\r\n", idx);
			HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) log_buffer_uart, strlen(log_buffer_uart));
		}
	}
}

// ==============================================================================
// 4. CALLBACK FINE TRASMISSIONE UART
// ==============================================================================
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == hlpuart1.Instance) {
		uart_busy = 0; // Sblocco del logger seriale
	}
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
