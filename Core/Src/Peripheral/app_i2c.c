/*
 * app_i2c.c
 *
 *  Created on: Dec 14, 2019
 *      Author: VAIO
 */

#include "main.h"
#include "Peripheral/app_i2c.h"


#define I2C_ADDRESS        0x30F

/* I2C SPEEDCLOCK define to max value: 400 KHz on STM32F1xx*/
#define I2C_TIMEOUT			1000
#define I2C_SPEEDCLOCK   400000
#define I2C_DUTYCYCLE    I2C_DUTYCYCLE_2

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* I2C handler declaration */
I2C_HandleTypeDef I2cHandle;

/**
  * @brief 	I2C peripheral initialization
  * @param 	None
  * @retval None
  */
HAL_StatusTypeDef I2C_Init(void){
	/*##-1- Configure the I2C peripheral ######################################*/
	  I2cHandle.Instance             = I2C1;
	  I2cHandle.Init.ClockSpeed      = I2C_SPEEDCLOCK;
	  I2cHandle.Init.DutyCycle       = I2C_DUTYCYCLE;
	  I2cHandle.Init.OwnAddress1     = 0;//I2C_ADDRESS;
	  I2cHandle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	  I2cHandle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	  I2cHandle.Init.OwnAddress2     = 0;//0xFF;
	  I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	  I2cHandle.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;

	  if(HAL_I2C_Init(&I2cHandle) != HAL_OK)
	  {
	    /* Initialization Error */
		  return HAL_ERROR;
	    Error_Handler();
	  }
	  return HAL_OK;
}


bool I2C_write(uint8_t address, uint8_t * data_w, size_t w_len){
	// Write
	bool success = HAL_I2C_Master_Transmit(&I2cHandle, address, data_w, w_len, I2C_TIMEOUT) == HAL_OK;
	return success;
}

bool I2C_read(uint8_t address, uint8_t * data_r, size_t r_len){
	// Read
	bool success = HAL_I2C_Master_Receive(&I2cHandle, address, data_r, r_len, I2C_TIMEOUT) == HAL_OK;
	return success;
}

bool I2C_write_and_read(uint8_t address, uint8_t * data_w, size_t w_len, uint8_t * data_r, size_t r_len){
	// Write
	bool success = I2C_write(address, data_w, w_len);
	// Read
	success = I2C_read(address, data_r, r_len) && success;
	return success;
}


bool I2C_mem_write(uint8_t address, uint16_t mem_address, uint16_t mem_size, uint8_t * data_w, size_t w_len){
	// Write
	bool success = HAL_I2C_Mem_Write(&I2cHandle, address, mem_address, mem_size, data_w, w_len, I2C_TIMEOUT);
	return success;
}

bool I2C_mem_read(uint8_t address, uint16_t mem_address, uint16_t mem_size, uint8_t * data_w, size_t w_len){
	// Write
	bool success = HAL_I2C_Mem_Read(&I2cHandle, address, mem_address, mem_size, data_w, w_len, I2C_TIMEOUT);
	return success;
}



