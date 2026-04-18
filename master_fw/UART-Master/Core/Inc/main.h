// Copyright 2026 TOK3T
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
#ifndef MASTER_FW_UART_MASTER_CORE_INC_MAIN_H_
#define MASTER_FW_UART_MASTER_CORE_INC_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

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
#define Led_1_Pin GPIO_PIN_0
#define Led_1_GPIO_Port GPIOA
#define Led_2_Pin GPIO_PIN_1
#define Led_2_GPIO_Port GPIOA
#define Led_Signal_Pin GPIO_PIN_3
#define Led_Signal_GPIO_Port GPIOA
#define Button_Signal_Pin GPIO_PIN_7
#define Button_Signal_GPIO_Port GPIOA
#define Button_Signal_EXTI_IRQn EXTI9_5_IRQn
#define Led_8_Pin GPIO_PIN_0
#define Led_8_GPIO_Port GPIOB
#define Led_5_Pin GPIO_PIN_1
#define Led_5_GPIO_Port GPIOB
#define Led_4_Pin GPIO_PIN_8
#define Led_4_GPIO_Port GPIOA
#define Led_3_Pin GPIO_PIN_11
#define Led_3_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define LD3_Pin GPIO_PIN_3
#define LD3_GPIO_Port GPIOB
#define Button_1_Pin GPIO_PIN_4
#define Button_1_GPIO_Port GPIOB
#define Button_1_EXTI_IRQn EXTI4_IRQn
#define Button_2_Pin GPIO_PIN_5
#define Button_2_GPIO_Port GPIOB
#define Button_2_EXTI_IRQn EXTI9_5_IRQn
#define Led_6_Pin GPIO_PIN_6
#define Led_6_GPIO_Port GPIOB
#define Led_7_Pin GPIO_PIN_7
#define Led_7_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif  // MASTER_FW_UART_MASTER_CORE_INC_MAIN_H_
