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
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "string.h"
#include "stdlib.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MSG_SIZE 100
#define QUEUE_SIZE 8 //massimo numero di messaggi che posso inserire
#define SIZE 10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static volatile uint8_t tx_busy=0; //1 se la trasmissione è impegnata
static char buffer[QUEUE_SIZE][MSG_SIZE];
static queue_t my_queue;
static char temp_msg[MSG_SIZE];
static uint32_t periodo_totale;
static uint32_t half_period;
static volatile uint8_t set_period=0; //
static char c;
static char buffer_number[SIZE];
static uint8_t idx;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void printMessage(const char* msg){

	char temp[MSG_SIZE];
	memset(temp, 0, MSG_SIZE);
	strcpy(temp,msg);
	queue_enqueue(&my_queue, temp);
	//abbiamo accodato il messaggio


	/*con la bloccante si opera facilemente senza coda e blocca la cpu nel while prima che tutti e 3 messaggi non sono stati inviati ricevuti e stampati
	una vota trasmessi tutti si alza il falg TC Transmission Complete la cpu si sblocca
	// Usiamo la trasmissione bloccante o IT. Per stampare in sequenza 3 messaggi,
    // la bloccante (Polling) in questa funzione di supporto evita sovrapposizioni.
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)msg, strlen(msg), 100);
    HAL_UART_Transmit(&hlpuart1, (uint8_t*)"\r\n", 2, 10); // A capo automatico
    */
}


void printMenu(){
	const char *msg1="Benvenuti, Questa applicazione consente di impostare il periodo del led integrato";
	const char *msg2="Per impostare il periodo cliccare sul pulsante \r\n";
	const char *msg3="Buon divertimento!!!";
	printMessage(msg1);
	printMessage(msg2);
	printMessage(msg3);
}

//verifica se busy_tx è alto, se si lo estre dalla e lo trasmette
void uart_tx_task(void){

	if(tx_busy) return; //se è alto allora si sta già estraendo, non faccio nulla
	//dove salva il valore estratto non puo essere una varialbe locale della funxione
	//estrarre elemento dalla coda
	if (queue_extract(&my_queue, temp_msg) == QUEUE_OK) {
		//siccome usciamo dalla funzione, la variabile di dove salviamo l elemento estratto non può essere una varaibile locale
		HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) temp_msg, strlen(temp_msg));
		tx_busy = 1;

	}

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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  //queue_init(&my_queue,(uint8_t*) buffer, MSG_SIZE, QUEUE_SIZE);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  queue_init(&my_queue,(uint8_t *) buffer, MSG_SIZE, QUEUE_SIZE);
  printMenu();
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	//ad ogni ciclo non sto bloccando l esecuzione della cpu
	uart_tx_task();
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV8;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == BUTTON_Pin) {
		if (tx_busy) return;
		const char *msg = "digita il periodo:\r\n";

		//Dice alla UART di smettere immediatamente di ascoltare il canale in background.
		//Cancella tutto quello che stavi aspettando di ricevere,
		//azzera i buffer, torna nello stato Libero perché adesso devo trasmettere un messaggio prioritario
		HAL_UART_AbortReceive(&hlpuart1);
		idx = 0;
		memset(buffer_number, 0, SIZE); // Pulisce il buffer da vecchi residui

		//funzione non bloccante, delega il lavoro alla periferica hardaware(UART) e al NVIC, liberando la CPU
		HAL_UART_Transmit_IT(&hlpuart1, (uint8_t*) msg, strlen(msg));

	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart==&hlpuart1){
		tx_busy=0;
		HAL_UART_Receive_IT(&hlpuart1, (uint8_t*) &c, 1);

	}
}



void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart==&hlpuart1){
		//il buffer deve essere chiuso
		if(c=='\n' || c=='\r'){
			buffer_number[idx]='\0';
			periodo_totale = atoi(buffer_number);
			// Controllo di sicurezza: evita divisioni per zero o valori negativi
			if (periodo_totale >= 2) {
				// Applichi la formula richiesta
				half_period = (periodo_totale / 2) - 1;

				// Aggiorni direttamente il registro hardware ARR del Timer
				__HAL_TIM_SET_AUTORELOAD(&htim6, half_period);
				//resetto il contatore
				__HAL_TIM_SET_COUNTER(&htim6, 0);
			}
			idx=0;
		}
		if(c>='0' && c<='9'){
			buffer_number[idx++]=c;
			if (idx == SIZE - 1) {
				//stiamo nel ultimo spazio disponibile del buffer, dobbiamo inserire il terminatore
				buffer_number[idx] = '\0';
				uint32_t periodo_totale = atoi(buffer_number);
				// Controllo di sicurezza: evita divisioni per zero o valori negativi
				if (periodo_totale >= 2) {
					// Applichi la formula richiesta
					uint32_t half_period = (periodo_totale / 2) - 1;

					// Aggiorni direttamente il registro hardware ARR del Timer
					__HAL_TIM_SET_AUTORELOAD(&htim6, half_period);
					//resetto il contatore
					__HAL_TIM_SET_COUNTER(&htim6, 0);
					idx = 0;

				}
			}
		}

		//devo riarmare la ricezione,stiamo dicendo che abbiamo ottenuto il byte e dobbiamo richiederne un altro
		//nel caso in cui si inserisca un numero a due cifre, prima ne richieno uno, riarmo e ne richiedo un altro
		HAL_UART_Receive_IT(&hlpuart1, (uint8_t*) &c, 1);

	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); // Cambia stato ogni 500ms = 1Hz totale
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
