/*
 * app_gpio.c
 *
 *  Created on: Jul 26, 2021
 *      Author: peter
 */

#include "main.h"
#include "app_scheduler.h"
#include "Peripheral/app_gpio.h"

#define FLASH_BLINK_TIME			31		/*!< 31 ticks in 10ms timer ~ 300ms */
#define BEAT_BLINK_TIME				97		/*!< 97 ticks in 10ms timer ~ 1000ms */
#define SLOW_TIME					149		/*!< 149 ticks in 10ms timer ~ 1500ms */
#define QUICK_TIME					47		/*!< 47 ticks in 10ms timer ~ 500ms */


void Switch_Init(void);
void Sim7600_GPIO_Init(void);

/**
  * @brief 	GPIO Initialization Function: 74HC245, LED, Buzzer, SPI_CS, SwitchID, SIM7600_GPIO
  * @param 	None
  * @retval None
  */
void GPIO_Init(void)
{
//	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
}







