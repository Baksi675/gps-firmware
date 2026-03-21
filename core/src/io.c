/**
 * @file io.c
 * @author github.com/Baksi675
 * @brief This module implements a digital IO module.
 * @date 2026-01-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "io.h"
#include "configuration.h"
#include "err.h"
#include "log.h"
#include "modules.h"
#include "stm32f401re_gpio.h"
#include "cmd.h"
#include "common.h"

struct io_handle_s {
	GPIO_HANDLE_ts gpio_handle;
	char name[CONFIG_IO_MAX_NAME_LEN];
	bool in_use;
};

struct internal_state_s {
	IO_HANDLE_ts ios[CONFIG_IO_MAX_OBJECTS];
	uint32_t io_num;
	LOG_LEVEL_te log_level;
	MODULES_te subsys;
	bool initialized;
	bool started;
};
static struct internal_state_s internal_state;

static ERR_te io_cmd_w_handler(uint32_t argc, char **argv);
static ERR_te io_cmd_r_handler(uint32_t argc, char **argv);
static ERR_te io_cmd_info_handler(uint32_t argc, char **argv);

static CMD_INFO_ts io_cmds[] = {
	{
		.name = "w",
		.help = "Writes to an IO pin, usage: io w <name> <1|0>",
		.handler = io_cmd_w_handler
	},
	{
		.name = "r",
		.help = "Reads from an IO pin, usage: io r <name>",
		.handler = io_cmd_r_handler
	},
	{
		.name = "info",
		.help = "Shows IO information, usage: io info",
		.handler = io_cmd_info_handler
	}
};

static CMD_CLIENT_INFO_ts io_cmd_client_info = {
	.cmds_ptr = io_cmds,
	.num_cmds = sizeof(io_cmds) / sizeof(io_cmds[0]),
	.name = "io",
	.log_level_ptr = &internal_state.log_level
};

 /** 
 * @defgroup IO_Public_APIs IO Public APIs
 * @{
 */

/**
 * @brief Initializes the IO subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te io_init_subsys(void) {
	ERR_te err = 0;

	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };
	
	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_IO;
	internal_state.initialized = true;
	internal_state.started = false;
	
	err = cmd_register(&io_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_init_subsys: cmd_register error"
		);

		return err;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"io_init_subsys: subsys initialized"
	);

	return ERR_OK;
}

/**
 * @brief Deinitializes the IO subsystem internal state to zeroes.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te io_deinit_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state = (struct internal_state_s){ 0 };

		cmd_deregister(&io_cmd_client_info);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_deinit_subsys: subsys is not initialized or not stopped"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"io_deinit_subsys: subsys deinitialized"
	);

	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te io_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"io_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_start_subsys: subsys not initialized or already started"
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
ERR_te io_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"io_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;	
}

// Not needed for this module
/*ERR_te io_get_def_conf(IO_CONFIG_ts *io_config) {

}*/

/**
 * @brief Initializes and creates an IO handle based on the given configuration.
 * 
 * @param[in] io_config The IO configuration structure.
 * @param[out] io_handle_o The initialized IO handle.
 * @return ERR_te Error code generated during execution.
 */
ERR_te io_init_handle(IO_CONFIG_ts *io_config, IO_HANDLE_ts **io_handle_o) {
	if(internal_state.io_num == CONFIG_IO_MAX_OBJECTS) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_init_handle: subsystem out of memory space"
		);

		return ERR_NOT_ENOUGH_SPACE;
	}

	gpio_init(io_config->gpio_handle);

	for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
		if(internal_state.ios[i].in_use == false) {
			internal_state.ios[i].gpio_handle = *io_config->gpio_handle;
			txt_cpy(internal_state.ios[i].name,
				 io_config->name, 
				 get_str_len(io_config->name) + 1);

			internal_state.ios[i].in_use = true;

			internal_state.io_num++;

			*io_handle_o = &internal_state.ios[i];

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"io_init_handle: io handle %s initialized",
				internal_state.ios[i].name
			);

			break;
		}
	}

	return ERR_OK;
}

/**
 * @brief Deinitializes an io object.
 * 
 * @param[in] io_handle The io handle.
 * @return ERR_te The error code generated during execution.
 */
ERR_te io_deinit_handle(IO_HANDLE_ts const *io_handle) {
	if(internal_state.io_num == 0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_deinit_handle: no such handle to deinitialize"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}
	
	for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
		if(&internal_state.ios[i] == io_handle) {
			internal_state.ios[i].gpio_handle = (GPIO_HANDLE_ts){ 0 };

			uint8_t name_len = get_str_len(internal_state.ios[i].name) + 1;
			char name[name_len];

			str_cpy(name, internal_state.ios[i].name, name_len);
			
			for(uint32_t j = 0; j < name_len; j++) {
				internal_state.ios[i].name[j] = '\0';
			}

			internal_state.ios[i].in_use = false;

			internal_state.io_num--;

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"io_deinit_handle: io handle %s deinitialized",
				name
			);

			break;
		}

		if(i == CONFIG_IO_MAX_OBJECTS - 1) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_deinit_handle: no such handle to deinitialize"
		);
			
			return ERR_UNINITIALZIED_OBJECT;
		}
	}

	return ERR_OK;
}

// Not needed for this module
/*ERR_te io_run_handle(IO_HANDLE_ts *io_handle) {

}*/		

// Not needed for this module
/*ERR_te io_run_handle_all(void) {

}*/

/**
 * @brief Writes to an io object.
 * 
 * @param[in] io_handle The IO handle.
 * @param[in] pin_status The pin status.
 * @return ERR_te The error code generated during execution.
 */
ERR_te io_write(IO_HANDLE_ts *io_handle, PIN_STATUS_te pin_status) {
	if(internal_state.initialized && internal_state.started) {
		gpio_write(
			io_handle->gpio_handle.port, 
			io_handle->gpio_handle.pin, 
			pin_status
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_write: subsystem not initialized or started"
		);

		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

/**
 * @brief Reads an io object.
 * 
 * @param[in] io_handle The handle of the io object.
 * @param[out] pin_status_o The pin status of the io object.
 * @return ERR_te Error generated during execution.
 */
ERR_te io_read(IO_HANDLE_ts const *io_handle, PIN_STATUS_te *pin_status_o) {
	if(internal_state.initialized && internal_state.started) {
		*pin_status_o = gpio_read(
			io_handle->gpio_handle.port,
			io_handle->gpio_handle.pin
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"io_read: subsystem not initialized or started"
		);

		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

 /** @} */

  /** 
 * @defgroup IO_COMMAND_HANDLERS IO COMMAND HANDLERS
 * @{
 */

/**
 * @brief Handler routine for the w command. Writes an io object.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te io_cmd_w_handler(uint32_t argc, char** argv) {
	ERR_te err;
	
	if(argc != 4) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"io_cmd_w_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;
	} 

	for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
		if(str_cmp(argv[2], internal_state.ios[i].name) == true) {
			if(str_cmp(argv[3], "0") == true ||
			str_cmp(argv[3], "off") == true) {
				err = io_write(&internal_state.ios[i], LOW);
				if(err != ERR_OK)
					return err;

				return ERR_OK;
			}
			else if(str_cmp(argv[3], "1") == true ||
			str_cmp(argv[3], "on") == true) {
				err = io_write(&internal_state.ios[i], HIGH);
				if(err != ERR_OK)
					return err;

				return ERR_OK;
			}
			else {
				LOG_ERROR(
					internal_state.subsys,
					internal_state.log_level,
					"io_cmd_w_handler: invalid arguments"
				);
				return ERR_INVALID_ARGUMENT;
			}
		}

		if(i == internal_state.io_num - 1) {
			LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level, 
			"io_cmd_w_handler: invalid arguments"
			);
			return ERR_INVALID_ARGUMENT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Handler routine for the r command. Reads an io object.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te io_cmd_r_handler(uint32_t argc, char** argv) {
	ERR_te err;
	
	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"io_cmd_r_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;
	} 

	for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
		if(str_cmp(argv[2], internal_state.ios[i].name) == true) {
			PIN_STATUS_te pin_status = 0;

			err = io_read(&internal_state.ios[i], &pin_status);
			if(err != ERR_OK) {
				return err;
			}

			LOG_INFO(
				internal_state.subsys,
				internal_state.log_level,
				"io_cmd_r_handler: %s pin status: %d",
				internal_state.ios[i].name,
				pin_status
			);

			return ERR_OK;
		}

		if(i == internal_state.io_num - 1) {
			LOG_ERROR(
				internal_state.subsys,
				internal_state.log_level, 
				"io_cmd_r_handler: invalid arguments"
			);
			return ERR_INVALID_ARGUMENT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Handler routine for the info command. Shows information about available commands.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te io_cmd_info_handler(uint32_t argc, char **argv) {
	if(argc != 2) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"io_cmd_info_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	LOG_INFO(internal_state.subsys, internal_state.log_level, "Printing IO information:");

	for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
		if(internal_state.ios[i].in_use == true) {
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"name: %s, mode: %d", 
				internal_state.ios[i].name, 
				internal_state.ios[i].gpio_handle.mode
			);
		}
	}

	return ERR_OK;
}

/** @} */

