/**
 * @file cmd.c
 * @author github.com/Baksi675
 * @brief CMD subsystem implementation
 * @version 0.1
 * @date 2025-11-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>

#include "cmd.h"
#include "common.h"
#include "err.h"
#include "log.h"
#include "configuration.h"
#include "modules.h"

struct internal_state_s {
	MODULES_te subsys;
	LOG_LEVEL_te log_level;
	CMD_CLIENT_INFO_ts *cmd_client_info_arr[CONFIG_CMD_MAX_CLIENTS];
};
static struct internal_state_s internal_state = { 
	.subsys = MODULES_CMD,
	.log_level = LOG_LEVEL_INFO
 };

 /** 
 * @defgroup CMD_Public_APIs CMD Public APIs
 * @{
 */

/**
 * @brief Registers a command in the cmd module from the clients side.
 * 
 * @param cmd_client_info Struct containing information about client.
 */
ERR_te cmd_register(CMD_CLIENT_INFO_ts *cmd_client_info) {
	for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
		if(internal_state.cmd_client_info_arr[i] == (void*)0) {
			internal_state.cmd_client_info_arr[i] = cmd_client_info;
			break;
		}
		else if(internal_state.cmd_client_info_arr[i] != (void*)0 && i == CONFIG_CMD_MAX_CLIENTS - 1) {
			return ERR_NOT_ENOUGH_SPACE;
		}
	}

	return ERR_OK;
 }


 /**
  * @brief Deregisters a client from the cmd module.
  * 
  * @param cmd_client_info Client info pointer.
  * @return ERR_te Error generated during execution.
  */
ERR_te cmd_deregister(CMD_CLIENT_INFO_ts const *cmd_client_info) {
	for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
		if(internal_state.cmd_client_info_arr[i] == cmd_client_info) {
			internal_state.cmd_client_info_arr[i] = (void*)0;
			break;
		}
		else if(i == CONFIG_CMD_MAX_CLIENTS - 1) {
			return ERR_DEINITIALIZATION_FAILURE;
		}
	}

	return ERR_OK;
}
	

 /**
  * @brief Interprets and executes a command or calls the command handler of a client.
  * 
  * @param console_text The command text to process.
  * @return int8_t Returns 0 if everything went successfully, -1 if errors occured.
  */
ERR_te cmd_execute(char *console_text) {
	ERR_te err;

	uint16_t num_tokens = 0;
	char *tokens[CONFIG_CMD_MAX_TOKENS];
	CMD_CLIENT_INFO_ts *client_info;

	str_tokenize(console_text, " ", CONFIG_CMD_MAX_TOKENS, tokens, &num_tokens);

	if(num_tokens == 0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level, 
			"cmd_execute: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	// 1. Handle wildcard commands
	if(str_cmp("*", tokens[0]) == true) {
		if(num_tokens < 2) {
			// No wildcard command given after wildcard character
			LOG_ERROR(
				internal_state.subsys, 
				internal_state.log_level, 
				"cmd_execute: invalid arguments"
			);

			return ERR_INVALID_ARGUMENT;
		}
		// Handle wildcard log 
		if(str_cmp("log", tokens[1]) == true) {
			if(num_tokens == 2) {
				for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
					client_info = internal_state.cmd_client_info_arr[i];

					if(client_info == (void*)0) {
						continue;
					}

					LOG_LEVEL_te log_level = *client_info->log_level_ptr;

					char *log_level_name[16];
					
					err = log_get_level_name(log_level, (char*)log_level_name);
					if(err != ERR_OK) {
						return err;
					}

					LOG_INFO(
						internal_state.subsys, 
						internal_state.log_level, 
						"%s log level: %s", 
						client_info->name, 
						log_level_name
					);

					return ERR_OK;
				}
			}
			else if(num_tokens == 3) {
				const char *log_level_str = tokens[2];
				
				LOG_LEVEL_te log_level = 0;
				err = log_level_to_int((char*)log_level_str, &log_level);
				if(err != ERR_OK) {
					return err;
				}

				for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
					client_info = internal_state.cmd_client_info_arr[i];

					if(client_info == (void*)0) {
						continue;
					}

					*client_info->log_level_ptr = log_level;
				}
			}
			else {
				LOG_ERROR(
					internal_state.subsys, 
					internal_state.log_level, 
					"cmd_execute: invalid arguments"
				);

				return ERR_INVALID_ARGUMENT;
			}

			return ERR_OK;
		}
	}

	// 2. Handle version command
	if(str_cmp("version", tokens[0]) == true) {
		if(num_tokens > 1) {
			LOG_ERROR(
				internal_state.subsys, 
				internal_state.log_level, 
				"cmd_execute: invalid arguments"
			);
			return ERR_INVALID_ARGUMENT;
		}
		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level, 
			"Version=%s", CONFIG_VERSION
		);

		return ERR_OK;
	}

	// 3. Handle help command
	if(str_cmp("help", tokens[0]) == true) {
		if(num_tokens == 1) {
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level, 
				"Listing general commands:"
			);
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level, 
				"1. version: Prints firmware version, usage: version"
			);
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level, 
				"2. help: Shows help information for modules, usage: help <module>"
			);
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level, 
				"3. log: Shows or sets the log level of modules, usage: log <module|*> <level>"
			);

			for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
				if(internal_state.cmd_client_info_arr[i] == (void*)0) {
					continue;
				}

				client_info = internal_state.cmd_client_info_arr[i];

				LOG_INFO(
					internal_state.subsys, 
					internal_state.log_level, 
					"Listing %s commands:",
					client_info->name
				);

				for(uint8_t j = 0; j < client_info->num_cmds; j++) {
					LOG_INFO(
						internal_state.subsys, 
						internal_state.log_level,
						"%d. %s: %s", j + 1, 
						client_info->cmds_ptr[j].name, 
						client_info->cmds_ptr[j].help
					);
				}
			}
			return ERR_OK;
		}
		else if(num_tokens == 2) {
			for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
				if(internal_state.cmd_client_info_arr[i] == (void*)0) {
					continue;
				}

				client_info = internal_state.cmd_client_info_arr[i];	

				if(str_cmp(client_info->name, tokens[1]) == true) {
					LOG_INFO(
						internal_state.subsys, 
						internal_state.log_level, 
						"Listing %s commands:",
						client_info->name
					);

					for(uint8_t j = 0; j < client_info->num_cmds; j++) {
						LOG_INFO(
							internal_state.subsys, 
							internal_state.log_level,
							"%d. %s: %s", j + 1, 
							client_info->cmds_ptr[j].name, 
							client_info->cmds_ptr[j].help
						);
					}

					return ERR_OK;
				}
			}
		}
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level, 
			"cmd_execute: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;
	}

	// 4. Handle client commands
	for(uint8_t i = 0; i < CONFIG_CMD_MAX_CLIENTS; i++) {
		if(internal_state.cmd_client_info_arr[i] == (void*)0) {
			continue;
		}

		client_info = internal_state.cmd_client_info_arr[i];

		if(str_cmp(tokens[0], client_info->name) == true) {
			if(str_cmp(tokens[1], "log") == true) {
				if(num_tokens == 2) {
					LOG_LEVEL_te log_level = *client_info->log_level_ptr;

					char *log_level_name[16];
					err = log_get_level_name(log_level, (char*)log_level_name);
					if(err != ERR_OK) {
						return err;
					}

					LOG_INFO(
						internal_state.subsys,
						internal_state.log_level,  
						"%s log level: %s",
						client_info->name, 
						log_level_name
					);

					return ERR_OK;
				}
				else if(num_tokens == 3) {
					const char *log_level_str = tokens[2];
					LOG_LEVEL_te log_level = 0;
					
					err = log_level_to_int((char*)log_level_str, &log_level);
					if(err != ERR_OK) {
						return err;
					}

					*client_info->log_level_ptr = log_level;
					
					return ERR_OK;
				}
				else if(num_tokens > 3) {
					LOG_ERROR(
						internal_state.subsys, 
						internal_state.log_level,  
						"cmd_execute: invalid arguments"
					);

					return ERR_INVALID_ARGUMENT;
				}
			}
			for(uint8_t j = 0; j < client_info->num_cmds; j++) {
				if(str_cmp(tokens[1], client_info->cmds_ptr[j].name) == true) {
					client_info->cmds_ptr[j].handler(num_tokens, tokens);
					return ERR_OK;
				}

				if(j == client_info->num_cmds - 1) {
					LOG_ERROR(
						internal_state.subsys, 
						internal_state.log_level,  
						"cmd_execute: invalid arguments"
					);

					return ERR_INVALID_ARGUMENT;
				}
			}
		}
	}
	LOG_ERROR(
		internal_state.subsys,
		internal_state.log_level, 
		"cmd_execute: invalid arguments"
	);

	return ERR_INVALID_ARGUMENT;
}

/** @} */