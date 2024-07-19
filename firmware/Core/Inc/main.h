/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32g4xx_ll_ucpd.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_dma.h"

#include "stm32g4xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c3;
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim4;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VDDA_APPLI 3300
#define ADC_TERMO_SENSOR_Pin GPIO_PIN_0
#define ADC_TERMO_SENSOR_GPIO_Port GPIOA
#define BTN_UP_Pin GPIO_PIN_4
#define BTN_UP_GPIO_Port GPIOA
#define BTN_DOWN_Pin GPIO_PIN_5
#define BTN_DOWN_GPIO_Port GPIOA
#define BTN_START_STOP_Pin GPIO_PIN_6
#define BTN_START_STOP_GPIO_Port GPIOA
#define LED_START_STOP_Pin GPIO_PIN_7
#define LED_START_STOP_GPIO_Port GPIOA
#define LED_OUT_Pin GPIO_PIN_0
#define LED_OUT_GPIO_Port GPIOB
#define DISP_I2C_SCL_Pin GPIO_PIN_8
#define DISP_I2C_SCL_GPIO_Port GPIOA
#define DISP_I2C_SDA_Pin GPIO_PIN_5
#define DISP_I2C_SDA_GPIO_Port GPIOB
#define MCH_CONTROL_Pin GPIO_PIN_9
#define MCH_CONTROL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
