/**
 * @file cmd.h
 * @author github.com/Baksi675
 * @brief Command subsystem public API.
 *
 * @details
 * This module provides a lightweight command dispatch system that allows
 * other subsystems (clients) to register named commands accessible via
 * a text-based console interface.
 *
 * Features include:
 * - Client registration and deregistration
 * - Tokenized command parsing and dispatch
 * - Built-in wildcard, help, version, and per-client log-level commands
 *
 * Typical usage:
 * - Populate a @ref CMD_INFO_ts array with command names, help strings, and handlers
 * - Populate a @ref CMD_CLIENT_INFO_ts with the array and a subsystem name
 * - Register the client using @ref cmd_register
 * - Pass console input to @ref cmd_execute for parsing and dispatch
 * - Deregister using @ref cmd_deregister when the client shuts down
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup cmd_module Command Module
 * @brief Text-based command dispatch system for registered subsystem clients.
 * @{
 */

#ifndef CMD_H__
#define CMD_H__

#include "log.h"

/**
 * @defgroup cmd_types Command Types
 * @ingroup cmd_module
 * @brief Data types used by the command subsystem.
 * @{
 */

/**
 * @brief Function pointer type for a command handler.
 *
 * @details
 * Each command registered with the subsystem must provide a handler
 * matching this signature. The handler receives the tokenized argument
 * list produced by @ref cmd_execute.
 *
 * @param[in] argc Number of tokens in @p argv.
 * @param[in] argv Array of token strings. argv[0] is the client name,
 *                 argv[1] is the command name, and subsequent entries
 *                 are command-specific arguments.
 *
 * @return An @ref ERR_te error code indicating success or failure.
 */
typedef ERR_te (*CMD_HANDLER_tf)(uint32_t argc, char **argv);

/**
 * @brief Describes a single command exposed by a client.
 *
 * @details
 * An array of these structures is provided to @ref CMD_CLIENT_INFO_ts
 * to register all commands belonging to a subsystem client.
 */
typedef struct {
    /** Null-terminated command name as typed on the console (e.g. "getpushed"). */
    const char *const name;

    /** Null-terminated help string displayed by the built-in help command. */
    const char *const help;

    /** Pointer to the function that handles this command. */
    const CMD_HANDLER_tf handler;
} CMD_INFO_ts;

/**
 * @brief Describes a subsystem client registering with the command module.
 *
 * @details
 * Passed to @ref cmd_register to associate a subsystem name with its
 * command table and log-level variable. The command module stores a
 * pointer to this structure, so it must remain valid for the lifetime
 * of the registration.
 */
typedef struct {
    /** Null-terminated client name used as the first token on the console (e.g. "button"). */
    const char *const name;

    /** Number of entries in @ref cmds_ptr. */
    const uint8_t num_cmds;

    /** Pointer to the array of commands exposed by this client. */
    const CMD_INFO_ts *const cmds_ptr;

    /**
     * Pointer to the client's runtime log-level variable.
     * The command module uses this to implement the built-in
     * per-client and wildcard log-level commands.
     */
    LOG_LEVEL_te *const log_level_ptr;
} CMD_CLIENT_INFO_ts;

/** @} */

/**
 * @defgroup cmd_api Command Public API
 * @ingroup cmd_module
 * @brief Public functions to interact with the command subsystem.
 * @{
 */

/**
 * @brief Registers a client with the command subsystem.
 *
 * @details
 * Adds the client's @ref CMD_CLIENT_INFO_ts to the internal client table,
 * making its commands available for dispatch via @ref cmd_execute.
 *
 * The pointer @p cmd_client_info must remain valid until the client
 * calls @ref cmd_deregister.
 *
 * @param[in] cmd_client_info Pointer to the client descriptor to register.
 *
 * @return
 * - ERR_OK on success
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of clients is already registered
 */
ERR_te cmd_register(CMD_CLIENT_INFO_ts *cmd_client_info);

/**
 * @brief Deregisters a client from the command subsystem.
 *
 * @details
 * Removes the client identified by @p cmd_client_info from the internal
 * client table, making its commands unavailable for future dispatch.
 *
 * @param[in] cmd_client_info Pointer to the client descriptor to deregister.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the client was not found in the table
 */
ERR_te cmd_deregister(CMD_CLIENT_INFO_ts const *cmd_client_info);

/**
 * @brief Parses and executes a command from a console text string.
 *
 * @details
 * Tokenizes @p console_text by spaces and dispatches to the appropriate
 * handler based on the following built-in command hierarchy:
 *
 * - `version` — prints the firmware version
 * - `help [<client>]` — lists all commands or those of a specific client
 * - `* log [<level>]` — gets or sets the log level of all registered clients
 * - `<client> log [<level>]` — gets or sets the log level of a specific client
 * - `<client> <command> [args...]` — dispatches to the matching client handler
 *
 * @param[in] console_text Null-terminated string containing the command to execute.
 *                         The string is modified in place during tokenization.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if the input is empty, malformed, or no matching command is found
 * - Other error codes propagated from the invoked command handler
 */
ERR_te cmd_execute(char *console_text);

/** @} */

#endif

/** @} */