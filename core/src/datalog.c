/**
 * @file datalog.c
 * @author github.com/Baksi675
 * @brief Data log module implementation file. Implements the GPS data logging feature of the GPS device.
 * @version 0.1
 * @date 2026-02-15
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdbool.h>

#include "datalog.h"
#include "common.h"
#include "configuration.h"
#include "err.h"
#include "log.h"
#include "modules.h"
#include "cmd.h"
#include "ff.h"
#include "arm_cortex_m4_systick.h"
#include "neo6.h"


struct datalog_handle_s {
	char name[CONFIG_DATALOG_MAX_NAME_LEN];
	DATALOG_TIME_te datalog_time;
	uint32_t last_upd_time;	
	FATFS fs;    
	FIL fil;      
	FRESULT fr;
	UINT bw, br;
	bool in_use;
	bool initialized;
};

struct internal_state_s {
	DATALOG_HANDLE_ts datalogs[CONFIG_DATALOG_MAX_OBJECTS];
	uint32_t datalog_num;
	MODULES_te subsys;
	LOG_LEVEL_te log_level;
	bool initialized;
	bool started;
};
static struct internal_state_s internal_state;

static ERR_te datalog_cmd_list_handler(uint32_t argc, char **argv);

CMD_INFO_ts datalog_cmds[] = {
	{
		.name = "list",
		.help = "Lists active datalog objects, usage: datalog list",
		.handler = datalog_cmd_list_handler
	}
};	

CMD_CLIENT_INFO_ts datalog_cmd_client_info = {
	.cmds_ptr = datalog_cmds,
	.num_cmds = sizeof(datalog_cmds) / sizeof(datalog_cmds[0]),
	.log_level_ptr = &internal_state.log_level,
	.name = "datalog"
};

/**
 * @brief Initializes the subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te datalog_init_subsys(void) {
	ERR_te err;

	if(internal_state.initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_init_subsys: subsys is already initialized"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };

	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_DATALOG;
	internal_state.initialized = true;
	internal_state.started = false;

	err = cmd_register(&datalog_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_init_subsys: cmd_register error"
		);
		return err;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"datalog_init_subsys: subsys initialized"
	);

	return ERR_OK;
}

/**
 * @brief Deinitializes the subsystem internal state to zeroes.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te datalog_deinit_subsys(void) {
	if(internal_state.initialized) {
		internal_state = (struct internal_state_s){ 0 };

		cmd_deregister(&datalog_cmd_client_info);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_deinit_subsys: subsys is not initialized"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"datalog_deinit_subsys: subsys deinitialized"
	);

	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te datalog_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_start_subsys: subsys not initialized or already started"
		);

		return ERR_UNKNOWN;
	}

	return ERR_OK;	
}

/**
 * @brief Stops the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te datalog_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

/**
 * @brief Initializes a handle.
 * 
 * @param[in] datalog_config Configuration structure.
 * @param[out] datalog_handle_o Initialized handle.
 * @return ERR_te Error generated during execution.
 */
ERR_te datalog_init_handle(DATALOG_CONFIG_ts *datalog_config, DATALOG_HANDLE_ts **datalog_handle_o) {
	uint8_t free_index;

	if(!internal_state.initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_init_handle: subsys not initialized"
		);

		return ERR_INITIALIZATION_FAILURE;
	}
	
	if(internal_state.datalog_num == CONFIG_DATALOG_MAX_OBJECTS) {
		LOG_CRITICAL(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_init_handle: subsystem out of memory space"
		);

		return ERR_NOT_ENOUGH_SPACE;
	}	

	if(!datalog_config) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_init_handle: invalid argument"
		);
		return ERR_INVALID_ARGUMENT;
	}

	bool found = false;

	for(uint32_t i = 0; i < CONFIG_DATALOG_MAX_OBJECTS; i++) {
		if(!internal_state.datalogs[i].in_use) {
			free_index = i;
			found = true;
			break;
		}
	}

	if(!found)
		return ERR_NOT_ENOUGH_SPACE;

    // Mount filesystem
    internal_state.datalogs[free_index].fr = f_mount(&internal_state.datalogs[free_index].fs, "0:", 1);   
    if (internal_state.datalogs[free_index].fr != FR_OK) {
        return ERR_INITIALIZATION_FAILURE;
    }

    // Create / open file
    internal_state.datalogs[free_index].fr = f_open(&internal_state.datalogs[free_index].fil, "0:datalog.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (internal_state.datalogs[free_index].fr != FR_OK) {
        return ERR_INITIALIZATION_FAILURE;;
    }

	f_close(&internal_state.datalogs[free_index].fil);

	str_cpy(
		internal_state.datalogs[free_index].name, 
		datalog_config->name, 
		get_str_len(datalog_config->name) + 1
	);
	internal_state.datalogs[free_index].datalog_time = datalog_config->datalog_time;
	internal_state.datalogs[free_index].in_use = true;

	internal_state.datalog_num++;

	*datalog_handle_o = &internal_state.datalogs[free_index];

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"datalog_init_handle: handle %s initialized",
		internal_state.datalogs[free_index].name
	);


	return ERR_OK;
}

/**
 * @brief Deinitializes the handle from the internal state.
 * 
 * @param[in] datalog_handle The handle to deinitialize. 
 * @return ERR_te Error generated during execution.
 */
ERR_te datalog_deinit_handle(DATALOG_HANDLE_ts *datalog_handle) {
	if(!datalog_handle->initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_deinit_handle: handle not initialized"
		);
		return ERR_ILLEGAL_ACTION;
	}

	if(internal_state.datalog_num == 0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"datalog_deinit_handle: no such handle to deinitialize"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}
	
	for(uint32_t i = 0; i < CONFIG_DATALOG_MAX_OBJECTS; i++) {
		if(&internal_state.datalogs[i] == datalog_handle) {
			uint8_t name_len = get_str_len(internal_state.datalogs[i].name) + 1;
			char name[name_len];
			str_cpy(name, internal_state.datalogs[i].name, name_len);

			internal_state.datalogs[i] = (DATALOG_HANDLE_ts){ 0 };

			internal_state.datalog_num--;

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"datalog_deinit_handle: handle %s deinitialized",
				name
			);

			break;
		}

		if(i == CONFIG_DATALOG_MAX_OBJECTS - 1) {
			LOG_ERROR(
				internal_state.subsys, 
				internal_state.log_level,
				"datalog_deinit_handle: no such handle to deinitialize"
			);
			
			return ERR_UNINITIALZIED_OBJECT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Runs the handle.
 * 
 * @param[in] datalog_handle The handle to run. 
 * @return ERR_te Error generated during execution.
 */
ERR_te datalog_run_handle(DATALOG_HANDLE_ts *datalog_handle) {
	uint32_t time = systick_get_ms();

	// Time interval has not been reached yet, return
	if(time - datalog_handle->last_upd_time < datalog_handle->datalog_time * 1000) {
		return ERR_OK;
	}

    // Open file
    datalog_handle->fr = f_open(&datalog_handle->fil, "0:datalog.txt", FA_OPEN_APPEND | FA_WRITE);
    if (datalog_handle->fr != FR_OK) {

        return ERR_UNKNOWN;
    }

	NEO6_INFO_ts *neo6_info = (void*)0;
	neo6_get_info(&neo6_info);

    // Set up time string
	uint32_t time_len = get_str_len(neo6_info->time) + 2;
	char time_str[time_len];
	str_cpy(time_str, neo6_info->time, time_len - 2);
	time_str[time_len - 2] = '\r';
	time_str[time_len - 1] = '\n';

	// Write time string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, time_str, time_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != time_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

	// Set up latitude string
	uint32_t lat_len = get_str_len(neo6_info->lat) + 2;
	char lat_str[lat_len];
	str_cpy(lat_str, neo6_info->lat, lat_len - 2);
	lat_str[lat_len - 2] = '\r';
	lat_str[lat_len - 1] = '\n';

	// Write latitude string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, lat_str, lat_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != lat_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

	// Set up longitude string
	uint32_t lon_len = get_str_len(neo6_info->lon) + 2;
	char lon_str[lon_len];
	str_cpy(lon_str, neo6_info->lon,lon_len - 2);
	lon_str[lon_len - 2] = '\r';
	lon_str[lon_len - 1] = '\n';

	// Write longitude string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, lon_str, lon_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != lon_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

	// Set up orthometric height string
	uint32_t ort_height_len = get_str_len(neo6_info->ort_height) + 2;
	char ort_height_str[ort_height_len];
	str_cpy(ort_height_str, neo6_info->ort_height,ort_height_len - 2);
	ort_height_str[ort_height_len - 2] = '\r';
	ort_height_str[ort_height_len - 1] = '\n';

	// Write orthometric height string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, ort_height_str, ort_height_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != ort_height_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

	// Set up movement speed string
	uint32_t mov_speed_len = get_str_len(neo6_info->mov_speed) + 2;
	char mov_speed_str[mov_speed_len];
	str_cpy(mov_speed_str, neo6_info->mov_speed,mov_speed_len - 2);
	mov_speed_str[mov_speed_len - 2] = '\r';
	mov_speed_str[mov_speed_len - 1] = '\n';

	// Write movement speed string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, mov_speed_str, mov_speed_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != mov_speed_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

	// Set up movement direction string
	uint32_t mov_dir_len = get_str_len(neo6_info->mov_dir) + 2;
	char mov_dir_str[mov_dir_len];
	str_cpy(mov_dir_str, neo6_info->mov_dir,mov_dir_len - 2);
	mov_dir_str[mov_dir_len - 2] = '\r';
	mov_dir_str[mov_dir_len - 1] = '\n';

	// Write movement direction string to file
    datalog_handle->fr = f_write(&datalog_handle->fil, mov_dir_str, mov_dir_len, &datalog_handle->bw);
    if (datalog_handle->fr != FR_OK || datalog_handle->bw != mov_dir_len) {
		f_close(&datalog_handle->fil);

		return ERR_UNKNOWN;
    }

    datalog_handle->fr = f_write(&datalog_handle->fil, "\r\n", 2, &datalog_handle->bw);

    f_close(&datalog_handle->fil);

	datalog_handle->last_upd_time = systick_get_ms();

	return ERR_OK;
}

/**
 * @brief Handler routine for the list command. Shows information about objects commands.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te datalog_cmd_list_handler(uint32_t argc, char **argv) {
	if(argc != 2) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"datalog_cmd_list_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;		
	}

	for(uint32_t i = 0; i < CONFIG_DATALOG_MAX_OBJECTS; i++) {
		if(internal_state.datalogs[i].in_use == true) {
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"%s", 
				internal_state.datalogs[i].name
			);
		}
	}

	return ERR_OK;
}