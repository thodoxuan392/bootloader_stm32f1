/*
 * config.h
 *
 *  Created on: Jun 2, 2023
 *      Author: xuanthodo
 */

#ifndef INC_APP_CONFIG_H_
#define INC_APP_CONFIG_H_

#include "stdio.h"
#include "stdbool.h"

#define VERSION_MAX_LEN		8
#define VERSION	 			"1.0.0"

#define MODEL				"card-vendor"

#define DEVICE_ID_MAX_LEN				10
#ifndef DEVICE_ID_DEFAULT
	#define DEVICE_ID_DEFAULT		"123456"
#endif

typedef struct {
	char version[VERSION_MAX_LEN];
	char device_id[DEVICE_ID_MAX_LEN];
}CONFIG_t;

bool CONFIG_init();
CONFIG_t * CONFIG_get();
void CONFIG_set(CONFIG_t *);
void CONFIG_clear();
void CONFIG_test();

#endif /* INC_APP_CONFIG_H_ */
