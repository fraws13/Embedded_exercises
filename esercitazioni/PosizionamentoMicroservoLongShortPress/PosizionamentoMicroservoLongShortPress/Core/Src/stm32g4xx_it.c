/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32g4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
bool pushed=false; //indica se il tasto è premuto
bool ready=true; //indica se il servo è pronto per una rotazione
uint16_t posizionamento=0; //0 sta fermo, 1 in movimento 90 o 180, 2 è arrivato e aspetta 10 sec
volatile uint64_t countms=0; // consta i ms al premere del pulsante
volatile uint64_t count10sec=0; //conta fino a 10 sec
volatile uint64_t pulse=0;
volatile uint64_t targetPulse=0; //setta il pulse per l angolo in cui si deve spostare il servo
volatile uint64_t count=0; //conta ogni quanti tick si deve applicare il moviemnto del servo
volatile uint64_t countLed=0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim6;
/* USER CODE BEGIN EV */
extern TIM_HandleTypeDef htim1;


/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(BTN_DOWN_Pin);
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1 and DAC3 channel underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		count++;
		if (pushed) countms++;
		switch (posizionamento) {
		case (0):
		//il timer si avvia solo dopo il primo tocco del bottone
		//appena il bottone si preme si avrà l alternanaza dei led
		HAL_GPIO_WritePin(GPIOA, LED_R_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, LED_G_Pin, GPIO_PIN_RESET);
		break;
		case (1):
			countLed++;
			//dico di tornare a 0°, faccio una modifica a pulse ogni 85 tick del timer
			if (count >= 85) {
				//aumento il pulse+1
				if (pulse < targetPulse) {
					pulse += 1;
					__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
				}else if (pulse > targetPulse) {
					pulse -= 1;
					__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
				}else{
					posizionamento = 2; //sono arrivato
					ready=true;
					countLed=0;


				}
				count=0;
				count10sec=0;
			}
			if (countLed % 250 == 0) {
				HAL_GPIO_TogglePin(GPIOA, LED_R_Pin);
				HAL_GPIO_TogglePin(GPIOA, LED_G_Pin);
			}
		break;
		case(2): //aspetto 10 sec
			if(pulse==50){
				posizionamento=0;

			}
			count10sec++;
			if(count10sec>=10000){
				targetPulse=50;
				posizionamento=1;
				ready=false;
			}
		break;
		default:
			//caso 0
		break;
		}
	}

}




void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (!ready) return; //se il motore sta ruotando non accetto input da pulsante
	static uint32_t last_interrupt = 0;
	uint32_t current_time = HAL_GetTick();

	// Debouncing: se sono passati meno di 50ms dall'ultimo interrupt, ignora
	if (current_time - last_interrupt < 50)
		return;
	last_interrupt = current_time;
	if(GPIO_Pin == BTN_DOWN_Pin){
			//stiamo in pull down, 1 (set) se il tasto è premuto
			if(HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin)== GPIO_PIN_SET){
				countms=0; //reset con un nuovo conteggio
				pushed=true;
				HAL_TIM_Base_Start_IT(&htim6);

			}
			//quando il bottone è rilasciato, torna a stato 0 (reset)
			else if (HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin) == GPIO_PIN_RESET) {
				pushed = false;
				if (countms > 100 && countms <= 1000) {
					//short press
					count = 0;
					posizionamento = 1; //indico che si deve applicare una rotazione
					targetPulse = 100; //il pulse deve andare fino a 90
					ready = false; //non accetto altre rotazioni

				} else if (countms > 1000) {
					count = 0;
					posizionamento = 1; //indico che si deve applicare una rotazione
					targetPulse = 120; //il pulse deve andare fino a 180
					ready = false; //non accetto altre rotazioni
				}

			}

	}
}

/* USER CODE END 1 */
