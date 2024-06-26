/*
 * fota_fsm.c
 *
 *  Created on: Nov 17, 2021
 *      Author: DO XUAN THO
 */
#include <ota.h>
#include "config.h"
#include "fota_fsm.h"
#include "ServerCommunication/app_http.h"
#include "ServerCommunication/app_mqtt.h"
#include "Component/app_sim7600.h"

#define FLASH_BUFFER_SIZE 	2048
#define MAX_RETRY		3

FSM_State_Typedef fsm_curr_state = CHECK_OTA_REQUESTED;
FSM_State_Typedef fsm_prev_state = CHECK_OTA_REQUESTED;
Firmware_Data_State firmware_state;
static uint8_t flash_buffer[2048];
static uint8_t UPDATE_PERCENT_COMPLETE_TOPIC[MAX_LENGTH_TOPIC];
static uint8_t PAYLOAD[MAX_LENGTH_PAYLOAD];
static uint8_t ret;

void Check_Ota_Requested();
void Start_Simcom();
void Mqtt_Configuration();
void HTTP_Firmware_Processing();
void Update_Complete_Percent();


FSM_Machine_TypeDef fsm_state_machine[]={
		{CHECK_OTA_REQUESTED			, 		Check_Ota_Requested				},
		{START_SIMCOM					, 		Start_Simcom				},
		{MQTT_CONFIGURATION				, 		Mqtt_Configuration},
		{HTTP_FIRMWARE_PROCESSING		, 		HTTP_Firmware_Processing		},
		{UPDATE_COMPLETE_PERCENT		, 		Update_Complete_Percent		}
};

/**
 * HTTP_Display_State()
 * @brief This is function for display state of State Machine. It only show State when having a state changation.
 */
void FSM_Display_State(void){
	if(fsm_curr_state!=fsm_prev_state){
		fsm_prev_state = fsm_curr_state;
		switch (fsm_curr_state) {
			case CHECK_OTA_REQUESTED:
				LOG("\r\nCHECK OTA REQUESTED\r\n");
				break;
			case START_SIMCOM:
				LOG("\r\nSTART SIMCOM\r\n");
				break;
			case MQTT_CONFIGURATION:
				LOG("\r\nMQTT CONFIGURATION\r\n");
				break;
			case HTTP_FIRMWARE_PROCESSING:
				LOG("\r\nHTTP FIRMWARE PROCESSING\r\n");
				break;
			case UPDATE_COMPLETE_PERCENT:
				LOG("\r\nUPDATE PERCENT COMPLETE\r\n");
				break;
			default:
				break;
		}
	}

}

/**
 * HTTP_Run()
 * @brief This is function can be called from external file. It run follow state machine method. Not have param.
 */
uint8_t FSM_Run(void){
	FSM_Display_State();
	if(fsm_curr_state < FSM_MAX_STATE){
		(*fsm_state_machine[fsm_curr_state].func)();
		return 0;
	}
	else{
		return 1;
	}
}

void Check_Ota_Requested(){
	if(Ota_Is_Requested()){
		Clear_Ota_Requested();
		fsm_curr_state = START_SIMCOM;
	}else {
		LOG("\r\Jump_To_Last_Firmware\r\n");
		Jump_To_Last_Firmware();
	}
}

void Start_Simcom(){
	if(Sim7600_Run()){
		fsm_curr_state = MQTT_CONFIGURATION;
	}
}

void Mqtt_Configuration(){
	static uint8_t mqtt_retry_count = 0;
	MQTT_Run();
	if(MQTT_Get_State() == MQTT_WAIT_NEXT_COMMAND){
		fsm_curr_state = HTTP_FIRMWARE_PROCESSING;
	}
	else if (MQTT_Get_State() == MQTT_MAX_STATE){
		SIM7600_Set_State(SIM7600_RESET);
		MQTT_Set_State(MQTT_INIT);
		mqtt_retry_count ++;
		if(mqtt_retry_count > MAX_RETRY){
			// Update Firmware Fail -> Jump to Current Firmware
			Jump_To_Current_Firmware();
		}else{
			// Restart SIMCOM again
			fsm_curr_state = START_SIMCOM;
		}
	}
}

uint8_t complete_percent;
uint8_t pre_complete_percent;
void HTTP_Firmware_Processing(){
	HTTP_Run();
	switch (HTTP_Get_State()) {
		case HTTP_DONE:
			if(Get_Update_Firmware_Status() == UPDATE_SUCCESS){
				sprintf(UPDATE_PERCENT_COMPLETE_TOPIC,"%s/%s/ota",MODEL,CONFIG_get()->device_id);
				sprintf(PAYLOAD,"SUCCESS");
			}
			else{
				sprintf(UPDATE_PERCENT_COMPLETE_TOPIC,"%s/%s/ota",MODEL,CONFIG_get()->device_id);
				if(firmware_state == ERR_CHECKSUM){
					sprintf(PAYLOAD,"ERR_CHECKSUM");
				}
				else if(firmware_state == ERR_CURRENT_FIRMWARE_ADDRESS_WRONG){
					sprintf(PAYLOAD,"ERR_CURRENT_FIRMWARE_ADDRESS_WRONG");
				}
			}
			MQTT_Add_Message(UPDATE_PERCENT_COMPLETE_TOPIC, PAYLOAD);
			fsm_curr_state = UPDATE_COMPLETE_PERCENT;
			break;
		case HTTP_READ:
			// If have new complete percent, It will publish to Server
			if(complete_percent != pre_complete_percent){
				pre_complete_percent = complete_percent;
				sprintf(UPDATE_PERCENT_COMPLETE_TOPIC,"%s/%s/ota",MODEL, CONFIG_get()->device_id);
				sprintf(PAYLOAD,"%d",complete_percent);
				MQTT_Add_Message(UPDATE_PERCENT_COMPLETE_TOPIC, PAYLOAD);
				fsm_curr_state = UPDATE_COMPLETE_PERCENT;
			}
			break;
		case HTTP_MAX_STATE:
			Reset_SimConfiguration_State();
			HTTP_Set_State(HTTP_INIT);
			fsm_curr_state = START_SIMCOM;
			break;
		default:
			break;
	}
}


void Update_Complete_Percent(){
	if(is_Has_Message_Available()){
		MQTT_Dispatch_Message();
	}
	MQTT_Run();
	switch (MQTT_Get_State()) {
		case MQTT_WAIT_NEXT_COMMAND:
			if(HTTP_Get_State() == HTTP_DONE){
				if(Get_Update_Firmware_Status() == UPDATE_SUCCESS){
					LOG("\r\nJump_To_Current_Firmware\r\n");
					Jump_To_Current_Firmware();
				}
				else{
					LOG("\r\nJump_To_Factory_Firmware\r\n");
					Jump_To_Factory_Firmware();
				}
			}
			else if (HTTP_Get_State()==HTTP_READ){
				fsm_curr_state = HTTP_FIRMWARE_PROCESSING;
			}
			break;
		case MQTT_MAX_STATE:
			SIM7600_Set_State(SIM7600_RESET);
			HTTP_Set_State(HTTP_INIT);
			MQTT_Set_State(MQTT_INIT);
			fsm_curr_state = START_SIMCOM;
			break;
		default:
			break;
	}
}

FSM_State_Typedef FSM_Get_State(){
	return fsm_curr_state;
}
void FSM_Set_State(FSM_State_Typedef fsm_state){
	fsm_curr_state = fsm_state;
}
