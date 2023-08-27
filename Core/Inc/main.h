/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#define VERSION_LENGTH 			6
/*
 * Firmware Choosen
 */
#define FACTORY_FIRMWARE_CHOOSEN	1
#define CURRENT_FIRMWARE_CHOOSEN	2
/*
 * Update Firmware Status
 */
#define UPDATE_SUCCESS				0
#define UPDATE_FAILED				1

/**
 * FOTA Requested
 */
#define FOTA_IS_NOT_REQUESTED		0
#define FOTA_REQUESTED				1
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */


#define BOOTLOADER_FIRMWARE_ADDR	0x08000000			// Address of Bootloader Firmware 32KBytes
#define FACTORY_FIRMWARE_ADDR		0x08008000			// Address of Factory Firmware 96KBytes
#define CURRENT_FIRMWARE_ADDR		0x08020000			// Address of Current Firmware 96KBytes
#define FIRMWARE_CHOOSEN_ADDR		0x0803F000			// Address of Firmware choosen
#define FOTA_REQUESTED_ADDR			0x0803F800			// Address of Fota requested

#define FIRMWARE_READ_SIZE_PER_TIME	1024
#define TEMP_BUFFER_SIZE			2048
#define FIRMWARE_PAGE_LENGTH		50					// 50Pages



#define MQTT_SERVER					"\"tcp://35.240.158.2:8883\""
//note that ClientID be differenced between lockers
#define USER_NAME					"\"eboost-k2\""
#define PASS_WORD					"\"ZbHzPb5W\""

#define MAX_LENGTH_TOPIC				50
#define MAX_LENGTH_PAYLOAD				50
#define TIMER_CYCLE				 		10 /*!< 10ms*/

//AT Command Timeout
#define COMMAND_TIMEOUT 				200 //200 ms

//IMEI
#define IMEI_LENGTH						15
#define IMEI_INDEX						20

//DEPOSIT
#define DEPOSIT_LENGTH					10

/*
 * Peripheral Definition
 */

#define SIM7600			0
#define SIM7670A		1

//SIMCOM GPIO
#define SIM7600_4G_PWRON_PORT		GPIOC
#define SIM7600_4G_PWRON			GPIO_PIN_8
#define SIM7600_4G_PERST_PORT		GPIOC
#define SIM7600_4G_PERST			GPIO_PIN_9

/*UART Constant*/

//Max buffer for RX
#define UART_RX_BUFFERSIZE 			4096
#define CAN_RX_BUFFER				20
#define RX3_BUFFERSIZE				5


//UART Simcom7600 Constant
#define UART_SIMCOM_INSTANCE		USART1
#define UART_DEBUG_INSTANCE			USART3

/*Timer Constant*/
#define TIMER_INSTANCE				TIM3
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define LOG	UART_DEBUG_Transmit
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
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
