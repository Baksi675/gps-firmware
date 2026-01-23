/**
 * @file cmd.h
 * @author github.com/Baksi675
 * @brief CMD subsystem header.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef CMD_H__
#define CMD_H__

#include "log.h"

/**
 * @brief Function pointer prototype to the command handler function.
 * 
 */
typedef void (*CMD_HANDLER_tf)(uint32_t argc, char** argv);

/**
 * @brief Command info struct.
 * 
 */
typedef struct {
	const char *const name;
	const char *const help;
	const CMD_HANDLER_tf handler;
}CMD_INFO_ts;

/**
 * @brief Client info struct.
 * 
 */
typedef struct {
	const char* const name;
	const uint8_t num_cmds;
	const CMD_INFO_ts *const cmds_ptr;
	LOG_LEVEL_te *const log_level_ptr;
}CMD_CLIENT_INFO_ts;

ERR_te cmd_register(CMD_CLIENT_INFO_ts *cmd_client_info);
ERR_te cmd_execute(char *console_text);

#endif