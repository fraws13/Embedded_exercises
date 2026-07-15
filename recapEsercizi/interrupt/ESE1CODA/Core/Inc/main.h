/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTNR_Pin GPIO_PIN_4
#define BTNR_GPIO_Port GPIOC
#define BTNR_EXTI_IRQn EXTI4_IRQn
#define BTNV_Pin GPIO_PIN_13
#define BTNV_GPIO_Port GPIOB
#define BTNV_EXTI_IRQn EXTI15_10_IRQn
#define BTNG_Pin GPIO_PIN_8
#define BTNG_GPIO_Port GPIOC
#define BTNG_EXTI_IRQn EXTI9_5_IRQn
#define LEDR_Pin GPIO_PIN_10
#define LEDR_GPIO_Port GPIOC
#define LEDG_Pin GPIO_PIN_11
#define LEDG_GPIO_Port GPIOC
#define LEDV_Pin GPIO_PIN_12
#define LEDV_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
