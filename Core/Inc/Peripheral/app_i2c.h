/*
 * app_i2c.h
 *
 *  Created on: Dec 14, 2019
 *      Author: VAIO
 */

#ifndef INC_APP_I2C_H_
#define INC_APP_I2C_H_

#include "stdbool.h"

HAL_StatusTypeDef I2C_Init(void);
bool I2C_write(uint8_t address, uint8_t * data_w, size_t w_len);
bool I2C_write_and_read(uint8_t address, uint8_t * data_w, size_t w_len, uint8_t * data_r, size_t r_len);
bool I2C_read(uint8_t address, uint8_t * data_r, size_t r_len);
bool I2C_mem_write(uint8_t address, uint16_t mem_address, uint16_t mem_size, uint8_t * data_w, size_t w_len);

#endif /* INC_APP_I2C_H_ */
