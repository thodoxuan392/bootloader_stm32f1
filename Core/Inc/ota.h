/*
 * app_version.h
 *
 *  Created on: Nov 17, 2021
 *      Author: DO XUAN THO
 */

#ifndef INC_OTA_H_
#define INC_OTA_H_

#include "main.h"
#include "stdbool.h"

void Jump_To_Factory_Firmware();
void Jump_To_Current_Firmware();
void Update_Firmware_Failed();
void Update_Firmware_Success();
bool Ota_Is_Requested();
void Clear_Ota_Requested();
uint8_t Get_Update_Firmware_Status();

#endif /* INC_OTA_H_ */
