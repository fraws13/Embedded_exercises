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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdlib.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SIZE 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t half_period=500;
static volatile uint8_t set_period=0; //
static char c;
static char buffer[SIZE];
static uint8_t idx;
static uint8_t uart_rx_completed=1;

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
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	HAL_Delay(half_period);
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

//HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
//HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size)
void print_message(const char* msg){
	if(uart_rx_completed){
		uart_rx_completed=0;
		HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*)msg, strlen(msg));

	}
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == BUTTON_Pin){

		//Dice alla UART di smettere immediatamente di ascoltare il canale in background.
		//Cancella tutto quello che stavi aspettando di ricevere,
		//azzera i buffer, torna nello stato Libero perché adesso devo trasmettere un messaggio prioritario
		HAL_UART_AbortReceive(&hlpuart1);
		idx = 0;
		memset(buffer, 0, SIZE); // Pulisce il buffer da vecchi residui

		print_message("\r\ninserire il perdiodo\r\n");
		//funzione non bloccante, delega il lavoro alla periferica hardaware(UART) e al NVIC, liberando la CPU
	}
}


//viene chiamata esattamente nel microsecondo
//in cui l'ultimo bit dell'ultimo byte è uscito fisicamente dal pin del microcontrollore
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart==&hlpuart1){
		uart_rx_completed=1;
		//deve essere della stessa interfaccia
		//RICHIESTA DI RICEZIONE non BLOCCANTE
		//quando il dato dall uart viene depositato nel RDR
		//l'hardware della UART solleva automaticamente un flag (un interruttore)
		//chiamato RXNE (Receive Data Register Not Empty - Registro di ricezione non vuoto).
		//questo dato viene preso in RDR e depositato in nella varaibile c
		HAL_UART_Receive_IT(&hlpuart1, (uint8_t*) &c, 1);



	}
}

//Viene chiamata automaticamente dall'hardware non appena il microcontrollore
//ha finito di ricevere il numero di byte che gli avevi chiesto di aspettare con la HAL_UART_Receive_IT.

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart==&hlpuart1){
		//il buffer deve essere chiuso
		if(c=='\n' || c=='\r'){
			buffer[idx]='\0';
			half_period=atoi(buffer)/2;
			idx=0;
			return;
		}
		if(c>='0' && c<='9'){
			buffer[idx++]=c;
			if (idx == SIZE - 1) {
				//stiamo nel ultimo spazio disponibile del buffer, dobbiamo inserire il terminatore
				buffer[idx] = '\0';
				half_period = (atoi(buffer)) / 2;
				idx = 0;

			}
		}

		//devo riarmare la ricezione,stiamo dicendo che abbiamo ottenuto il byte e dobbiamo richiederne un altro
		//nel caso in cui si inserisca un numero a due cifre, prima ne richieno uno, riarmo e ne richiedo un altro
		HAL_UART_Receive_IT(&hlpuart1, (uint8_t*) &c, 1);
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
