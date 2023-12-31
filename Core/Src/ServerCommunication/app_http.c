/**
 *@file  app_http.c
 *@author thodo
 */


#include <Component/app_sim7600.h>
#include <Peripheral/app_uart.h>
#include <ServerCommunication/app_http.h>
#include <stdio.h>
#include "app_scheduler.h"
#include "app_ATProcessing.h"


#define ACTION_TIMEOUT					200		/*!< Timeout for action command : Get, post ,put ,...*/
#define TEMP_VERSION_BUFFER_LENGTH 		100
#define LINE_BUFFER_LENGTH				300

FlagStatus http_timeout_flag = SET;
FlagStatus para_timeout = RESET;
FlagStatus default_atcommand = SET;
FlagStatus fota_check_version = SET;
FlagStatus checksum_correct = SET;
uint8_t version[VERSION_LENGTH];
uint8_t version_index= 0;

// Get Content Length
uint8_t http_num_ignore = 0;
uint32_t content_length = 0;
uint16_t num_byte_FF_add_to_end_buffer = 0;


uint16_t result = 0;
uint8_t data_index_2 = 0;

FlagStatus started_record_firmware = RESET;
FlagStatus got_total_cabinet = RESET;
FlagStatus ready_to_warmup_cloudfunction = RESET;

HTTP_State prev_http_state = HTTP_INIT;
HTTP_State http_state = HTTP_INIT;
char http_at_command[150];

//Sync duration and Total Cabinet

uint16_t total_cabinet;
uint8_t cabinet_index = 0;

uint16_t firmware_version_check;
uint16_t firmware_checksum = 0;
uint8_t checksum = 0;
uint32_t http_response_remain = 0;
extern uint8_t pre_complete_percent;
extern uint8_t complete_percent;
uint8_t firmware_data[PAGESIZE*2];
uint32_t firmware_address = CURRENT_FIRMWARE_ADDR;
uint16_t firmware_address_prev_offet = (uint16_t)CURRENT_FIRMWARE_ADDR;
uint16_t firmware_address_curr_offet = (uint16_t)CURRENT_FIRMWARE_ADDR;
uint8_t prev_num_byte = 0;
uint16_t firmware_index = 0;
uint32_t firmware_size = 0;
uint16_t one_kb_index = 0;
uint8_t update_status;
/*
 * cabinet_attribute = 0 -> boardID
 * cabinet_attribute = 1 -> portID
 */
uint8_t	cabinet_attribute = 0;
char logMsg[50];

void HTTP_Done();
FlagStatus is_Firmware_Line_Data_Correct(uint8_t *buffer, uint16_t buffer_len);
FlagStatus HTTP_Firmware_Version();
Firmware_Data_State HTTP_Firmware_Data();


HTTP_Machine_TypeDef http_state_machine[]={
		{HTTP_INIT					, 		HTTP_Init				},
		{HTTP_WAIT_FOR_INIT			, 		HTTP_Wait_For_Init		},

		{HTTP_PARA					, 		HTTP_Para				},
		{HTTP_WAIT_FOR_PARA			, 		HTTP_Wait_For_Para		},

		{HTTP_ACTION				, 		HTTP_Action				},
		{HTTP_WAIT_FOR_ACTION		, 		HTTP_Wait_For_Action	},

		{HTTP_READ					, 		HTTP_Read				},
		{HTTP_WAIT_FOR_READ			, 		HTTP_Wait_For_Read		},

		{HTTP_TERM					, 		HTTP_Term				},
		{HTTP_WAIT_FOR_TERM			, 		HTTP_Wait_For_Term		},
		{HTTP_DONE					, 		HTTP_Done				}

};

/**
 * HTTP_Display_State()
 * @brief This is function for display state of State Machine. It only show State when having a state changation.
 */
void HTTP_Display_State(void){
	if(prev_http_state!=http_state){
		prev_http_state = http_state;
		switch (http_state) {
			case HTTP_INIT:
				LOG("\r\nHTTP INIT\r\n");
				break;
			case HTTP_WAIT_FOR_INIT:
				LOG("\r\nHTTP WAIT FOR INIT\r\n");
				break;
			case HTTP_PARA:
				LOG("\r\nHTTP PARA\r\n");
				break;
			case HTTP_WAIT_FOR_PARA:
				LOG("\r\nHTTP WAIT FOR PARA\r\n");
				break;
			case HTTP_ACTION:
				LOG("\r\nHTTP ACTION\r\n");
				break;
			case HTTP_WAIT_FOR_ACTION:
				LOG("\r\nHTTP WAIT FOR ACTION\r\n");
				//TODO recheck, break or not ?
				break;
			case HTTP_READ:
				LOG("\r\nHTTP READ\r\n");
				break;
			case HTTP_WAIT_FOR_READ:
				LOG("\r\nHTTP WAIT FOR READ\r\n");
				break;
			case HTTP_TERM:
				LOG("\r\nHTTP TERM\r\n");
				break;
			case HTTP_WAIT_FOR_TERM:
				LOG("\r\nHTTP WAIT FOR TERM\r\n");
				break;
			case HTTP_DONE:
				LOG("\r\nHTTP DONE\r\n");
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
uint8_t HTTP_Run(){
	HTTP_Display_State();
	if(http_state < HTTP_DONE){
		(*http_state_machine[http_state].func)();
		return 0;
	}
	else if(http_state == HTTP_DONE){
		LOG("Get into HTTP DONE");
		return 1;
	}
	else{
		//HTTP Error
		return 2;
	}
}

/**
 * HTTP_Init()
 * @brief This is function for initiating Http service
 */
void HTTP_Init(){
	content_length = 0;
	Clear_Reiceive_Buffer();
	if(default_atcommand){
		sprintf(http_at_command,"AT+HTTPINIT\r\n");
	}
	UART_SIM7600_Transmit((uint8_t *)http_at_command);
	Clear_Http_Command();
	http_state = HTTP_WAIT_FOR_INIT;
}


/**
 * HTTP_Wait_For_Init()
 * @brief This is function for waiting respond from initiating Http service
 */
void HTTP_Wait_For_Init(){
//	Wait_For_Respone(AT_OK);
	switch (Get_AT_Result()){
		case AT_OK:
			Clear_AT_Result();
			http_state=HTTP_PARA;
			break;
		case AT_ERROR:
			Clear_AT_Result();
			http_state=HTTP_MAX_STATE;
			break;
		default:
			break;
	}
}


/**
 * HTTP_Para()
 * @brief This is function for passing parameter to HTTP Request
 */
void HTTP_Para(){
	Clear_Reiceive_Buffer();
	if (default_atcommand) {
		if(fota_check_version){
			sprintf(http_at_command,"AT+HTTPPARA=\"URL\",\"http://ota.chipfc.com/card_vendor_firmware/version.txt\"\r\n");
		}
		else{
			sprintf(http_at_command,"AT+HTTPPARA=\"URL\",\"http://ota.chipfc.com/card_vendor_firmware/%s/firmware.hex\"\r\n",version);
		}
	}
	UART_SIM7600_Transmit((uint8_t*)http_at_command);
	Clear_Http_Command();
	http_state = HTTP_WAIT_FOR_PARA;
}

/**
 * HTTP_Wait_For_Para()
 * @brief This is function for waiting respone from  HTTP_PARA state
 * If AT_Result is AT_OK so switch to HTTP_ACTION else it's AT_ERROR so switch to HTTP_MAX_STATE
 */
void HTTP_Wait_For_Para(){
//	Wait_For_Respone(AT_OK);
	switch (Get_AT_Result()){
		case AT_OK:
			Clear_AT_Result();
			http_state=HTTP_ACTION;
			break;
		case AT_ERROR:
			Clear_AT_Result();
			http_state=HTTP_MAX_STATE;
			break;
		default:
			break;
	}
}


/**
 * HTTP_Action()
 * @brief This is function for create request with method : GET,POST,PUT
 * Passing 0 :GET ,1:POST ,...
 */
void HTTP_Action(){
	Clear_Reiceive_Buffer();
	if (default_atcommand) {
		sprintf(http_at_command,"AT+HTTPACTION=0\r\n");
	}
	UART_SIM7600_Transmit((uint8_t*)http_at_command);
	Clear_Http_Command();
	Clear_Http_Timeout_Flag();
	SCH_Add_Task(Set_Http_Timeout_Flag, ACTION_TIMEOUT, 0);
	http_state = HTTP_WAIT_FOR_ACTION;
}


/**
 * HTTP_Wait_For_Action()
 * @brief This is function for waiting HTTP_ACTION respone
 * If AT_Result is AT_OK so switch to HTTP_READ to read HTTP Respone body
 * else AT_ERROR so switch to HTTP_MAX_STATE to reset Simcom7600
 */
void HTTP_Wait_For_Action(){
	if(is_Http_TimeOutFlag()){
//		Wait_For_Respone(AT_OK);
		switch (Get_AT_Result()){
			case AT_OK:
				Clear_AT_Result();
				break;
			case AT_HTTP_RESPONSE:
				// Get Content-Length:
				if(HTTP_Get_Content_Length()){
					http_response_remain = HTTP_Return_Content_Length();
					firmware_size = http_response_remain;
					content_length = 0;
					sprintf(logMsg,"\r\n%d\r\n",http_response_remain);
					LOG(logMsg);
					Clear_AT_Result();
					http_state = HTTP_READ;
				}
				break;
			case AT_ERROR:
				Clear_AT_Result();
				http_state = HTTP_MAX_STATE;
				break;
			default:
				break;
		}
	}
}


/**
 * HTTP_Read()
 * @brief This is function for read HTTP respone body, passing for AT number of read data. Default is MAX_HTTP_BODY = 400
 */
void HTTP_Read(){
	uint32_t read_size;
	if(fota_check_version){
		read_size = http_response_remain;
	}
	else{
		static uint8_t num_show_lcd = 0;
		if(http_response_remain == 0){
			sprintf(logMsg,"Complete 100%");
			LOG(logMsg);
		}
		else if((firmware_size - http_response_remain) > 10240 * num_show_lcd){
			complete_percent = (firmware_size- http_response_remain) * 100 / firmware_size;
			sprintf(logMsg,"Complete %ld%", complete_percent);
			LOG(logMsg);
			num_show_lcd ++;
		}
		if(http_response_remain > (FIRMWARE_READ_SIZE_PER_TIME)){
			read_size = FIRMWARE_READ_SIZE_PER_TIME;
			http_response_remain = http_response_remain -  (FIRMWARE_READ_SIZE_PER_TIME);
		}
		else if (http_response_remain > 0 && http_response_remain < (FIRMWARE_READ_SIZE_PER_TIME)){
			read_size = http_response_remain;
			http_response_remain = 0;
		}
	}
	if (default_atcommand) {
		sprintf(http_at_command,"AT+HTTPREAD=0,%d\r\n",read_size);
	}
	Clear_Reiceive_Buffer();
	sprintf(logMsg,"http_response_remain: %ld\r\n",http_response_remain);
	LOG(logMsg);
	sprintf(logMsg,"firmware_index: %ld\r\n",firmware_index);
	LOG(logMsg);
	UART_SIM7600_Transmit((uint8_t *)http_at_command);
	Clear_Http_Timeout_Flag();
	SCH_Add_Task(Set_Http_Timeout_Flag, 100, 0);
	Clear_Http_Command();
	HAL_Delay(100);
	http_state = HTTP_WAIT_FOR_READ;
}


/**
 * HTTP_Wait_For_Read()
 * @brief This is function for waiting HTTP_Read respone
 * If AT_Result is AT_OK so clear AT result
 * else if AT_Result = AT_HTTP_RESPONE so Get BoardID and PortID , If It's done so switch to HTTP_PARA state.
 * else if AT_Result = AT_NOT_FOUND switch to HTTP_PARA every timeout
 * else if AT_Result = AT_ERROR switch to HTTP_MAX_STATE to reset Simcom7600
 */
uint32_t firmware_index_end;
extern Firmware_Data_State firmware_state;
void HTTP_Wait_For_Read(){
	if(fota_check_version){
		FlagStatus flag_ret;
		switch (Get_AT_Result()) {
			case AT_OK:
				flag_ret = HTTP_Firmware_Version();
				if(flag_ret){
					fota_check_version = RESET;
					Clear_AT_Result();
					http_state = HTTP_PARA;
					return;
				}
				break;
			default:
				break;
		}
	}
	else{
		switch (Get_AT_Result()) {
			case AT_OK:
				firmware_state = HTTP_Firmware_Data();
				switch (firmware_state) {
					case DONE:
						if(http_response_remain == 0){
							LOG("\r\nJump To Current Firmware\r\n");
							Update_Firmware_Success();
							http_state = HTTP_DONE;
//							Jump_To_Current_Firmware();
						}
						else{
							Clear_AT_Result();
							http_state = HTTP_READ;
						}
						break;
					case PROCESSING:
						/*Continuous Firmware Process*/
						break;
					case ERR_CHECKSUM:
						LOG("\r\nChecksum Error\r\n");
						LOG("\r\nJump To Factory Firmware\r\n");
//						Jump_To_Factory_Firmware();
						Update_Firmware_Failed();
						http_state = HTTP_DONE;
						break;

					case ERR_CURRENT_FIRMWARE_ADDRESS_WRONG:
						LOG("\r\nCURRENT FIRMWARE ADDRESS WRONG\r\n");
						LOG("\r\nJump To Current Firmware\r\n");
//						Jump_To_Current_Firmware();
						Update_Firmware_Failed();
						http_state = HTTP_DONE;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	return;
}



/**
 * HTTP_Term()
 * @brief This is function for terminaring HTTP Service
 */
void HTTP_Term(){
//	Clear_Reiceive_Buffer();
	if (default_atcommand) {
		sprintf(http_at_command,"AT+HTTPTERM\r\n");
	}
	UART_SIM7600_Transmit((uint8_t*)http_at_command);
	Clear_Http_Command();
	http_state = HTTP_WAIT_FOR_TERM;
}


/**
 * HTTP_Wait_For_Term()
 * @brief This is function for waiting HTTP_TERM state respone
 * If AT_Result is AT_OK so switch to HTTP_DONE
 * else switch to HTTP_MAX_STATE and reset Simcom7600
 */
void HTTP_Wait_For_Term(){
//	Wait_For_Respone(AT_OK);
	switch (Get_AT_Result()){
		case AT_OK:
			Clear_AT_Result();
			http_state=HTTP_INIT;
			break;
		case AT_ERROR:
			Clear_AT_Result();
			http_state=HTTP_MAX_STATE;
			break;
		default:
			break;
	}
}


/**
 * HTTP_Done()
 * @brief This is function for waiting FSM get out of HTTP Operation
 */
void HTTP_Done(){
	return;
}


/**
 * is_Http_TimeOutFlag()
 * @brief Get http_timeout_flag
 * @return http_timeout_flag
 */
FlagStatus is_Http_TimeOutFlag(void){
	return http_timeout_flag;
}



/**
 * Set_Http_Timeout_Flag()
 * @brief Set http_timeout_flag to SET
 */
void Set_Http_Timeout_Flag(void){
	http_timeout_flag = SET;
}



/**
 * Clear_Http_Timeout_Flag()
 * @brief Clear http_timeout_flag to RESET
 */
void Clear_Http_Timeout_Flag(void){
	http_timeout_flag = RESET;
}



/**
 * Get_IntegerValue_From_HTTP_Respone()
 * @brief This is function for Get Interger value from HTTP Respone body. It will read 4 character in UART buffer to ignore unnessesary character and then read Interger value until meet '"' charracter.
 * @return uint8_t value
 */
uint16_t Get_IntegerValue_From_HTTP_Respone(){
//	"integerValue$(pointer here)": "0"
//	-> need to ignore 4 character
	if(UART_SIM7600_Received_Buffer_Available()){
		uint8_t val_of_character = UART_SIM7600_Read_Received_Buffer();
		if(data_index_2 ++< 2){
			UART_DEBUG_Transmit("1");
			return 0xFFFF;
		}
		if(val_of_character!=','){
			UART_DEBUG_Transmit("2");
			result = result *10 + (uint8_t)val_of_character-48;
			return 0xFFFF;
		}
		else{
			sprintf(logMsg,"\r\n%d",result);
			UART_DEBUG_Transmit("3\r\n");
			UART_DEBUG_Transmit(logMsg);
			data_index_2 = 0;
			return result;
		}
	}
	else{
		return 0xFFFF;
	}
}



FlagStatus HTTP_Firmware_Version(){
	static uint8_t temp_version_name_buffer[TEMP_VERSION_BUFFER_LENGTH];
	static uint8_t temp_version_name_index = 0;
	static FlagStatus prepare_record_version_name = RESET;
	static FlagStatus start_record_version_name = RESET;
	if(UART_SIM7600_Received_Buffer_Available()){
		temp_version_name_buffer[temp_version_name_index] = UART_SIM7600_Read_Received_Buffer();
//		UART_DEBUG_Transmit_Size(temp_version_name_buffer + temp_version_name_index, 1);
		if(isReceiveData_New(temp_version_name_buffer, temp_version_name_index + 1, TEMP_VERSION_BUFFER_LENGTH, "+HTTPREAD: ")){
			prepare_record_version_name = SET;
		}
		if(prepare_record_version_name){
			if(start_record_version_name){
				//Check whether stop record version name
				if(temp_version_name_buffer[temp_version_name_index]=='\r'){
					start_record_version_name = RESET;
					prepare_record_version_name = RESET;
					return SET;
				}
				else{
					version[version_index++] = temp_version_name_buffer[temp_version_name_index];
				}
			}
			if(isReceiveData_New(temp_version_name_buffer, temp_version_name_index + 1, TEMP_VERSION_BUFFER_LENGTH, "\r\n")){
				start_record_version_name = SET;
			}

		}
		temp_version_name_index = (temp_version_name_index + 1)% TEMP_VERSION_BUFFER_LENGTH;
	}
	return RESET;
}

/*
 * Line is: ":0101010101010 and checksum :01, last is "\r\n""
 * We ignore ':' character and calculate checksum from 01....010 and ignore checksum value
 */
FlagStatus is_Firmware_Line_Data_Correct(uint8_t *buffer, uint16_t buffer_len){
	static FlagStatus ret;
	checksum = 0;

	for (uint16_t var = 0; var < buffer_len - 3 - 2; var=var+2) {
//		sprintf(log,"%c-%c\r\n",buffer[var],buffer[var+1]);
//		LOG(log);
		checksum = checksum + (Char2Hex(buffer[var]) << 4) + Char2Hex(buffer[var+1]);
	}
	checksum =~checksum;
	checksum += 1;
	uint8_t checksum_inline = (Char2Hex(buffer[buffer_len - 3 -2 ]) << 4) + Char2Hex(buffer[buffer_len -3-2 +1 ]);
//	sprintf(log,"Check sum %x\r\n",checksum);
//	LOG(log);
//	sprintf(log,"Calculated Check sum %x\r\n",checksum_inline);
//	LOG(log);
	ret = (checksum == checksum_inline);
	return ret;
}
/*
 * Return Size of Firmware Data which Received from UART
 */

Firmware_Data_State HTTP_Firmware_Data(){
	static FlagStatus prepare_record_firmware_data = RESET;
	static FlagStatus start_record_firmware_data = RESET;
	static uint8_t line_buffer[LINE_BUFFER_LENGTH];
	static uint16_t line_buffer_index = 0;
	static uint8_t temp_at_response_buffer[LINE_BUFFER_LENGTH];
	static uint16_t temp_at_response_index = 0;
	static FlagStatus first_http_read = SET;
	static uint32_t firmware_address_of_hexfile;
	static FlagStatus get_2bytes_firmware_address = SET;

	/*
	 * The first time get data pattern is
	 * {
	 *  	"new_version": 2,
	 *  	"checksum": 115,
	 *  	"data": "0ABCDGEGGASD...
	 *  			ASDB12312512412...
	 *  			01231291512925192"
	 *  So We need seperate "new_version" and "checksum" field out of "data"
	 */
	if(UART_SIM7600_Received_Buffer_Available()){
		temp_at_response_buffer[temp_at_response_index] = UART_SIM7600_Read_Received_Buffer();
//		UART_DEBUG_Transmit_Size(temp_at_response_buffer + temp_at_response_index, 1);
		//Check if end of SIM respond
		if(isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "\r\n+HTTPREAD: 0")||
				isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "\r\n+HTTPREAD:0")){
//			LOG("1\r\n");
			if(firmware_index >= PAGESIZE){
				Flash_Erase(firmware_address, 1);
				Flash_Write_Char(firmware_address, firmware_data, PAGESIZE);
//				LOG("1.1\r\n");
				firmware_address+= PAGESIZE;
//				memcpy(firmware_data,firmware_data+PAGESIZE,firmware_index-PAGESIZE);
				for (int var = PAGESIZE; var < firmware_index; ++var) {
					firmware_data[var-PAGESIZE] = firmware_data[var];
				}
				firmware_index = firmware_index - PAGESIZE;
			}
			else if(http_response_remain == 0){
				Flash_Erase(firmware_address, firmware_index);
				Flash_Write_Char(firmware_address, firmware_data, firmware_index);
				firmware_address+= firmware_index;
				firmware_index = 0;
			}
			if(isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "\r\n+HTTPREAD: 0")){
				line_buffer_index = line_buffer_index - strlen("\r\n+HTTPREAD: 0") + 1;
//				LOG("DONE 1");
			}
			else{
				line_buffer_index = line_buffer_index - strlen("\r\n+HTTPREAD:0") + 1;
//				LOG("DONE 2");
			}
			start_record_firmware_data = RESET;
			return DONE;
		}
		// Check whether start of SIM Respond
#if defined(SIM7600) && SIM7600 == 1
		else if(isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "HTTPREAD: DATA")){
#elif defined(SIM7670A) && SIM7670A == 1
		else if(!start_record_firmware_data && isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "+HTTPREAD: ")){
#endif
			//LOG("2\r\n");
			prepare_record_firmware_data = SET;
			return PROCESSING;
		}
		else if(prepare_record_firmware_data){
			//LOG("3\r\n");
			if(isReceiveData_New(temp_at_response_buffer, temp_at_response_index+1, LINE_BUFFER_LENGTH, "\r\n")){
				start_record_firmware_data = SET;
				prepare_record_firmware_data = RESET;
				// reset line_index
				return PROCESSING;
			}
		}
		else if(start_record_firmware_data){
			//LOG("4");
			// Check whether that data is not end of HTTP READ
			line_buffer[line_buffer_index] = temp_at_response_buffer[temp_at_response_index];
			line_buffer_index = (line_buffer_index +1)%LINE_BUFFER_LENGTH;
			if(http_response_remain == 0){
				if(isReceiveData_New(line_buffer, line_buffer_index, LINE_BUFFER_LENGTH, "\r\n+")|| isReceiveData_New(line_buffer, line_buffer_index, LINE_BUFFER_LENGTH, "\r\n:")){
					if(isReceiveData_New(line_buffer, line_buffer_index, LINE_BUFFER_LENGTH, "\r\n+")){
						line_buffer_index-=2;
					}
					//LOG("5\r\n");
//					UART_DEBUG_Transmit_Size(line_buffer, line_buffer_index);
//					sprintf(log,"\r\ntemp_at_response_index: %ld\r\n",temp_at_response_index);
//					LOG(log);
//					sprintf(log,"\r\nline_buffer_index: %ld\r\n",line_buffer_index);
//					LOG(log);
					// Calculator checksum
					if(first_http_read){
						for (uint16_t var = 0; var < line_buffer_index; ++var) {
							line_buffer[var] = line_buffer[var+1];
						}
						line_buffer_index --;
						first_http_read = RESET;
					}
					if(is_Firmware_Line_Data_Correct(line_buffer, line_buffer_index)){
						// Check whether that line is the firmware data or not
						if(Char2Hex(line_buffer[7])==0){
							firmware_address_curr_offet = ((uint16_t)(Char2Hex(line_buffer[2]))<<12) +((uint16_t)(Char2Hex(line_buffer[3]))<<8) +((uint16_t)(Char2Hex(line_buffer[4]))<<4)+(uint16_t)(Char2Hex(line_buffer[5]));
							if(get_2bytes_firmware_address){
								get_2bytes_firmware_address = RESET;
								firmware_address_of_hexfile = (firmware_address_of_hexfile<<16) | (uint32_t)firmware_address_curr_offet;
								if(firmware_address_of_hexfile != CURRENT_FIRMWARE_ADDR){
									return ERR_CURRENT_FIRMWARE_ADDRESS_WRONG;
								}
							}
							if(firmware_address_prev_offet >= 0xFFF0){
								num_byte_FF_add_to_end_buffer = 0xFFFF - firmware_address_prev_offet + 1 + firmware_address_curr_offet -  (uint16_t)prev_num_byte;
							}
							else{
								num_byte_FF_add_to_end_buffer = firmware_address_curr_offet - firmware_address_prev_offet - prev_num_byte;
							}
							for (uint16_t var = 0; var < num_byte_FF_add_to_end_buffer; var++) {
								firmware_data[firmware_index++] = 0xFF;
							}
							prev_num_byte = (Char2Hex(line_buffer[0])<<4) + Char2Hex(line_buffer[1]);
							firmware_address_prev_offet = firmware_address_curr_offet;


							for (uint16_t var = 8; var < line_buffer_index - 3 -2; var=var+2) {
								//Save line to firmware data
								firmware_data[firmware_index++] = (Char2Hex(line_buffer[var])<<4)+ Char2Hex(line_buffer[var+1]);
							}
						}
						else if(Char2Hex(line_buffer[7])==4){
							firmware_address_of_hexfile = ((uint16_t)(Char2Hex(line_buffer[8]))<<12) +((uint16_t)(Char2Hex(line_buffer[9]))<<8) + ((uint16_t)(Char2Hex(line_buffer[10]))<<4)+(uint16_t)(Char2Hex(line_buffer[11]));
						}
						line_buffer_index = 0;
					}
					else{
						checksum_correct = RESET;
						return ERR_CHECKSUM;
					}
				}
			}
			else{
				if(isReceiveData_New(line_buffer, line_buffer_index, LINE_BUFFER_LENGTH, "\r\n:")){
//					LOG("6");
//					UART_DEBUG_Transmit_Size(line_buffer, line_buffer_index);
//					sprintf(log,"\r\n temp_at_response_index: %ld\r\n",temp_at_response_index);
//					LOG(log);
//					sprintf(log,"\r\n line_buffer_index: %ld\r\n",line_buffer_index);
//					LOG(log);
					// Calculator checksum
					//LOG("7\r\n");
					if(first_http_read){
						for (uint16_t var = 0; var < line_buffer_index; ++var) {
							line_buffer[var] = line_buffer[var+1];
						}
						line_buffer_index --;
						first_http_read = RESET;
					}
					if(is_Firmware_Line_Data_Correct(line_buffer, line_buffer_index)){
						// Check whether that line is the firmware data or not
//							LOG("8\r\n");
						if(Char2Hex(line_buffer[7])==0){
//								LOG("9\r\n");
							firmware_address_curr_offet = ((uint16_t)(Char2Hex(line_buffer[2]))<<12) +((uint16_t)(Char2Hex(line_buffer[3]))<<8) + ((uint16_t)(Char2Hex(line_buffer[4]))<<4)+(uint16_t)(Char2Hex(line_buffer[5]));
							if(get_2bytes_firmware_address){
								get_2bytes_firmware_address = RESET;
								firmware_address_of_hexfile = (firmware_address_of_hexfile<<16) | (uint32_t)firmware_address_curr_offet;
								sprintf(logMsg,"\r\n2 byte total :%x\r\n",firmware_address_of_hexfile);
								LOG(logMsg);
								if(firmware_address_of_hexfile != CURRENT_FIRMWARE_ADDR){
									return ERR_CURRENT_FIRMWARE_ADDRESS_WRONG;
								}
							}
							if(firmware_address_prev_offet >= 0xFFF0){
								num_byte_FF_add_to_end_buffer = 0xFFFF - firmware_address_prev_offet + 1 + firmware_address_curr_offet - prev_num_byte;
							}
							else{
								num_byte_FF_add_to_end_buffer = firmware_address_curr_offet - firmware_address_prev_offet - prev_num_byte;
							}
							//LOG("10\r\n");
							for (uint16_t var = 0; var < num_byte_FF_add_to_end_buffer; var++) {
								firmware_data[firmware_index++] = 0xFF;
							}
							//LOG("11\r\n");
							prev_num_byte = (Char2Hex(line_buffer[0])<<4) + Char2Hex(line_buffer[1]);
							firmware_address_prev_offet = firmware_address_curr_offet;
							//LOG("12\r\n");
							for (uint16_t var = 8; var < line_buffer_index - 3 -2; var=var+2) {
								//Save line to firmware data
								firmware_data[firmware_index++] = (Char2Hex(line_buffer[var])<<4)+ Char2Hex(line_buffer[var+1]);
							}
							//LOG("13\r\n");
						}
						else if(Char2Hex(line_buffer[7])==4){
							firmware_address_of_hexfile = ((uint16_t)(Char2Hex(line_buffer[8]))<<12) +((uint16_t)(Char2Hex(line_buffer[9]))<<8) + ((uint16_t)(Char2Hex(line_buffer[10]))<<4)+(uint16_t)(Char2Hex(line_buffer[11]));
							sprintf(logMsg,"\r\n2 byte low :%x\r\n",firmware_address_of_hexfile);
							LOG(logMsg);
						}
						line_buffer_index = 0;
						//LOG("14");
					}
					else{
						checksum_correct = RESET;
						return ERR_CHECKSUM;
					}
				}
			}
		}
		temp_at_response_index = (temp_at_response_index +1)%LINE_BUFFER_LENGTH;
	}
	return PROCESSING;
}



FlagStatus HTTP_Get_Content_Length(){
	uint8_t temp;
	if(UART_SIM7600_Received_Buffer_Available()){
		temp = UART_SIM7600_Read_Received_Buffer();
		if(temp == '\r'){
			http_num_ignore = 0;
			return SET;
		}
		if(http_num_ignore==2){
			content_length = content_length*10 + (uint32_t)temp - (uint32_t)48;
		}
		if(temp == ','){
			http_num_ignore ++;
		}
	}
	return RESET;
}

/**
 * HTTP_Get_State()
 * @brief Get state of HTTP State Machine
 * @return http_state
 */
HTTP_State HTTP_Get_State(){
	return http_state;
}



/**
 * HTTP_Set_State()
 * @brief This is function for setting state to HTTP State Machine
 */
void HTTP_Set_State(HTTP_State _http_state){
	http_state = _http_state;
}

void Reset_Result(){
	result = 0;
}

void Reset_HttpConfiguration_State(){
	Reset_No_Board();
	HTTP_Set_State(HTTP_INIT);
}

void Clear_Http_Command(){
	default_atcommand = SET;
}

void Set_Http_Command(char * atcommand){
	sprintf(http_at_command,"%s",atcommand);
	default_atcommand = RESET;
}

uint32_t HTTP_Return_Content_Length(){
	return content_length;
}

uint32_t Get_Firmware_Size(){
	return firmware_size;
}

