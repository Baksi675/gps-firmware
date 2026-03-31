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
#include "init.h"
#include "log.h"
#include "modules.h"
#include "stm32f401re_gpio.h"
#include "cmd.h"
#include "common.h"

/**
 * @brief Internal structure representing a single IO instance.
 *
 * @details
 * Holds the GPIO configuration copied from @ref IO_CFG_ts, the
 * human-readable name, and a flag indicating whether this pool
 * slot is occupied.
 */
struct io_handle_s {
    /** GPIO configuration for this IO pin. */
    GPIO_CFG_ts gpio_cfg;

    /** Human-readable name of this IO instance (null-terminated). */
    char name[CONFIG_IO_MAX_NAME_LEN];

    /** True when this slot is occupied by an active IO instance. */
    bool in_use;
};

/**
 * @brief Internal state of the IO subsystem.
 *
 * @details
 * Holds the pool of IO handles, the count of active handles,
 * and the subsystem lifecycle flags.
 */
struct internal_state_s {
    /** Pool of IO handle instances. */
    IO_HANDLE_ts ios[CONFIG_IO_MAX_OBJECTS];

    /** Number of currently active (in-use) IO handles. */
    uint32_t io_num;

    /** Active log level for this subsystem. */
    LOG_LEVEL_te log_level;

    /** Module identifier used for log messages. */
    MODULES_te subsys;

    /** True after @ref io_init_subsys has completed successfully. */
    bool initialized;

    /** True after @ref io_start_subsys has been called. */
    bool started;
};

/** @brief Singleton instance of the IO subsystem internal state. */
static struct internal_state_s internal_state;

/* ---- Forward declarations for command handlers ---- */
static ERR_te io_cmd_w_handler(uint32_t argc, char **argv);
static ERR_te io_cmd_r_handler(uint32_t argc, char **argv);
static ERR_te io_cmd_info_handler(uint32_t argc, char **argv);

/**
 * @brief Table of CLI commands registered by the IO subsystem.
 *
 * @details
 * Registered with the command subsystem via @ref io_cmd_client_info
 * during @ref io_init_subsys.
 */
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

/**
 * @brief Registration descriptor passed to the command subsystem.
 *
 * @details
 * Bundles the command table, its size, the subsystem name prefix used
 * on the CLI, and a pointer to the runtime log-level variable.
 */
static CMD_CLIENT_INFO_ts io_cmd_client_info = {
    .cmds_ptr = io_cmds,
    .num_cmds = sizeof(io_cmds) / sizeof(io_cmds[0]),
    .name = "io",
    .log_level_ptr = &internal_state.log_level
};

/**
 * @defgroup io_public_apis IO Public APIs
 * @{
 */

/** @brief Initializes the IO subsystem. @see io_init_subsys */
ERR_te io_init_subsys(void) {
    ERR_te err = 0;

    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    internal_state = (struct internal_state_s){ 0 };

    internal_state.log_level = LOG_LEVEL_INFO;
    internal_state.subsys = MODULES_IO;
    internal_state.initialized = true;
    internal_state.started = false;

    init_log();

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

/** @brief Deinitializes the IO subsystem. @see io_deinit_subsys */
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

/** @brief Starts the IO subsystem. @see io_start_subsys */
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

/** @brief Stops the IO subsystem. @see io_stop_subsys */
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

/** @brief Initializes and registers an IO handle. @see io_init_handle */
ERR_te io_init_handle(IO_CFG_ts *io_cfg, IO_HANDLE_ts **io_handle_o) {
    if(internal_state.io_num == CONFIG_IO_MAX_OBJECTS) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "io_init_handle: subsystem out of memory space"
        );

        return ERR_NOT_ENOUGH_SPACE;
    }

    gpio_init(io_cfg->gpio_handle);

    for(uint32_t i = 0; i < CONFIG_IO_MAX_OBJECTS; i++) {
        if(internal_state.ios[i].in_use == false) {
            internal_state.ios[i].gpio_cfg = *io_cfg->gpio_handle;
            txt_cpy(internal_state.ios[i].name,
                io_cfg->name,
                get_str_len(io_cfg->name) + 1);

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

/** @brief Deinitializes an IO handle. @see io_deinit_handle */
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
            internal_state.ios[i].gpio_cfg = (GPIO_CFG_ts){ 0 };

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

/** @brief Writes a logic level to an IO pin. @see io_write */
ERR_te io_write(IO_HANDLE_ts *io_handle, PIN_STATUS_te pin_status) {
    if(internal_state.initialized && internal_state.started) {
        gpio_write(
            io_handle->gpio_cfg.port,
            io_handle->gpio_cfg.pin,
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

/** @brief Reads the current logic level of an IO pin. @see io_read */
ERR_te io_read(IO_HANDLE_ts const *io_handle, PIN_STATUS_te *pin_status_o) {
    if(internal_state.initialized && internal_state.started) {
        *pin_status_o = gpio_read(
            io_handle->gpio_cfg.port,
            io_handle->gpio_cfg.pin
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
 * @defgroup io_command_handlers IO Command Handlers
 * @{
 */

/**
 * @brief CLI handler for the "w" command. Writes a logic level to a named IO pin.
 *
 * @details
 * Expected invocation: `io w <name> <1|on|0|off>`
 *
 * Searches the registered IO pool for a handle whose name matches
 * @c argv[2], then calls @ref io_write with @ref HIGH or @ref LOW
 * depending on @c argv[3].
 *
 * @param[in] argc Argument count. Must be exactly 4.
 * @param[in] argv Argument list: argv[0] = "io", argv[1] = "w",
 *                 argv[2] = IO name, argv[3] = "1", "on", "0", or "off".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 4, the name is not found,
 *   or the value string is not recognized
 * - Propagated error from @ref io_write on failure
 */
static ERR_te io_cmd_w_handler(uint32_t argc, char **argv) {
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
 * @brief CLI handler for the "r" command. Reads and logs the logic level of a named IO pin.
 *
 * @details
 * Expected invocation: `io r <name>`
 *
 * Searches the registered IO pool for a handle whose name matches
 * @c argv[2], then calls @ref io_read and logs the result.
 *
 * @param[in] argc Argument count. Must be exactly 3.
 * @param[in] argv Argument list: argv[0] = "io", argv[1] = "r",
 *                 argv[2] = IO name.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 3 or the name is not found
 * - Propagated error from @ref io_read on failure
 */
static ERR_te io_cmd_r_handler(uint32_t argc, char **argv) {
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
 * @brief CLI handler for the "info" command. Logs the name and GPIO mode of all active IO handles.
 *
 * @details
 * Expected invocation: `io info`
 *
 * Iterates over the internal IO pool and logs the name and GPIO mode
 * of every slot that is currently in use.
 *
 * @param[in] argc Argument count. Must be exactly 2.
 * @param[in] argv Argument list: argv[0] = "io", argv[1] = "info".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 2
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
                internal_state.ios[i].gpio_cfg.mode
            );
        }
    }

    return ERR_OK;
}

/** @} */