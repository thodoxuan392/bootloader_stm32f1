/*
 * app_version.c
 *
 *  Created on: Nov 17, 2021
 *      Author: DO XUAN THO
 */

#include <ota.h>
#include "main.h"
#include "flash.h"
#include "Peripheral/app_uart.h"


static uint8_t update_status;

void Jump_To_Factory_Firmware(){
	uint8_t firmware_choosen = FACTORY_FIRMWARE_CHOOSEN;
	Flash_Erase(FIRMWARE_CHOOSEN_ADDR, sizeof(firmware_choosen));
	Flash_Write_Char(FIRMWARE_CHOOSEN_ADDR, &firmware_choosen, sizeof(firmware_choosen));
	void (*app_reset_handler)(void) = (void*)(*(volatile uint32_t*) (FACTORY_FIRMWARE_ADDR + 4));
	app_reset_handler();
}

void Jump_To_Current_Firmware(){
	uint8_t firmware_choosen = CURRENT_FIRMWARE_CHOOSEN;
	Flash_Erase(FIRMWARE_CHOOSEN_ADDR, sizeof(firmware_choosen));
	Flash_Write_Char(FIRMWARE_CHOOSEN_ADDR, &firmware_choosen, sizeof(firmware_choosen));
	void (*app_reset_handler)(void) = (void*)(*(volatile uint32_t*) (CURRENT_FIRMWARE_ADDR + 4));
	app_reset_handler();
}

void Jump_To_Last_Firmware(){
	uint8_t firmware_choosen = Flash_Read_Int(FIRMWARE_CHOOSEN_ADDR);
	switch (firmware_choosen) {
		case 0xFF:
		case FACTORY_FIRMWARE_CHOOSEN:
			Jump_To_Factory_Firmware();
			break;
		case CURRENT_FIRMWARE_CHOOSEN:
			Jump_To_Current_Firmware();
		default:
			break;
	}
}

void Update_Firmware_Failed(){
	update_status = UPDATE_FAILED;
}
void Update_Firmware_Success(){
	update_status = UPDATE_SUCCESS;
}

bool Ota_Is_Requested(){
	uint8_t ota_is_requested = Flash_Read_Int(FOTA_REQUESTED_ADDR);
	return (ota_is_requested == FOTA_REQUESTED);
}

void Clear_Ota_Requested(){
	uint16_t ota_is_requested = FOTA_IS_NOT_REQUESTED;
	Flash_Erase(FOTA_REQUESTED_ADDR, sizeof(ota_is_requested));
	Flash_Write_Char(FOTA_REQUESTED_ADDR, &ota_is_requested, sizeof(ota_is_requested));
}

uint8_t Get_Update_Firmware_Status(){
	return update_status;
}





