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
#include "init.h"
#include "log.h"
#include "modules.h"
#include "cmd.h"
#include "ff.h"
#include "arm_cortex_m4_systick.h"
#include "neo6.h"

/**
 * @brief Internal structure representing a single data log instance.
 *
 * @details
 * Holds all runtime state for one log, including the FatFs objects,
 * the configured logging interval, the timestamp of the last write,
 * and lifecycle flags.
 *
 * This structure is opaque to the caller; it is accessed only through
 * the @ref DATALOG_HANDLE_ts pointer returned by @ref datalog_init_handle.
 */
struct datalog_handle_s {
    /** Human-readable name of this log instance (null-terminated). */
    char name[CONFIG_DATALOG_MAX_NAME_LEN];

    /** Configured logging interval. */
    DATALOG_TIME_te datalog_time;

    /** Systick timestamp (ms) of the most recent log write. */
    uint32_t last_upd_time;

    /** FatFs filesystem object for this log instance. */
    FATFS fs;

    /** FatFs file object for the log file. */
    FIL fil;

    /** Most recent FatFs result code. */
    FRESULT fr;

    /** Bytes written in the most recent f_write call. */
    UINT bw;

    /** Bytes read in the most recent f_read call. */
    UINT br;

    /** True when this slot is occupied by an active log instance. */
    bool in_use;

    /** True when this handle has been fully initialized. */
    bool initialized;
};

/**
 * @brief Internal state of the data log subsystem.
 *
 * @details
 * Holds the pool of log handles, the count of active handles,
 * and the subsystem lifecycle flags.
 */
struct internal_state_s {
    /** Pool of data log handle instances. */
    DATALOG_HANDLE_ts datalogs[CONFIG_DATALOG_MAX_OBJECTS];

    /** Number of currently active (in-use) log handles. */
    uint32_t datalog_num;

    /** Module identifier used for log messages. */
    MODULES_te subsys;

    /** Active log level for this subsystem. */
    LOG_LEVEL_te log_level;

    /** True after @ref datalog_init_subsys has completed successfully. */
    bool initialized;

    /** True after @ref datalog_start_subsys has been called. */
    bool started;
};

/** @brief Singleton instance of the data log subsystem internal state. */
static struct internal_state_s internal_state;

/* ---- Forward declaration for command handler ---- */
static ERR_te datalog_cmd_list_handler(uint32_t argc, char **argv);

/**
 * @brief Table of CLI commands registered by the data log subsystem.
 *
 * @details
 * Registered with the command subsystem via @ref datalog_cmd_client_info
 * during @ref datalog_init_subsys.
 */
CMD_INFO_ts datalog_cmds[] = {
    {
        .name = "list",
        .help = "Lists active datalog objects, usage: datalog list",
        .handler = datalog_cmd_list_handler
    }
};

/**
 * @brief Registration descriptor passed to the command subsystem.
 *
 * @details
 * Bundles the command table, its size, the subsystem name prefix used
 * on the CLI, and a pointer to the runtime log-level variable.
 */
CMD_CLIENT_INFO_ts datalog_cmd_client_info = {
    .cmds_ptr = datalog_cmds,
    .num_cmds = sizeof(datalog_cmds) / sizeof(datalog_cmds[0]),
    .log_level_ptr = &internal_state.log_level,
    .name = "datalog"
};

/**
 * @defgroup datalog_public_apis Data Log Public APIs
 * @{
 */

/** @brief Initializes the data log subsystem. @see datalog_init_subsys */
ERR_te datalog_init_subsys(void) {
    ERR_te err;

    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    internal_state = (struct internal_state_s){ 0 };

    internal_state.log_level = LOG_LEVEL_INFO;
    internal_state.subsys = MODULES_DATALOG;
    internal_state.initialized = true;
    internal_state.started = false;

    init_log();
    init_systick();
    init_neo6();

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

/** @brief Deinitializes the data log subsystem. @see datalog_deinit_subsys */
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

/** @brief Starts the data log subsystem. @see datalog_start_subsys */
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

/** @brief Stops the data log subsystem. @see datalog_stop_subsys */
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

/** @brief Initializes and registers a data log handle. @see datalog_init_handle */
ERR_te datalog_init_handle(DATALOG_CFG_ts const *datalog_cfg, DATALOG_HANDLE_ts **datalog_handle_o) {
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

    if(!datalog_cfg) {
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
        return ERR_INITIALIZATION_FAILURE;
    }

    f_close(&internal_state.datalogs[free_index].fil);

    str_cpy(
        internal_state.datalogs[free_index].name,
        datalog_cfg->name,
        get_str_len(datalog_cfg->name) + 1
    );
    internal_state.datalogs[free_index].datalog_time = datalog_cfg->datalog_time;
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

/** @brief Deinitializes a data log handle. @see datalog_deinit_handle */
ERR_te datalog_deinit_handle(DATALOG_HANDLE_ts const *datalog_handle) {
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

/** @brief Writes a GPS data entry to the log file if the logging interval has elapsed. @see datalog_run_handle */
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
    str_cpy(lon_str, neo6_info->lon, lon_len - 2);
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
    str_cpy(ort_height_str, neo6_info->ort_height, ort_height_len - 2);
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
    str_cpy(mov_speed_str, neo6_info->mov_speed, mov_speed_len - 2);
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
    str_cpy(mov_dir_str, neo6_info->mov_dir, mov_dir_len - 2);
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

/** @} */

/**
 * @defgroup datalog_command_handlers Data Log Command Handlers
 * @{
 */

/**
 * @brief CLI handler for the "list" command. Logs the names of all active data log handles.
 *
 * @details
 * Expected invocation: `datalog list`
 *
 * Iterates over the internal handle pool and logs the name of every
 * slot that is currently in use.
 *
 * @param[in] argc Argument count. Must be exactly 2.
 * @param[in] argv Argument list: argv[0] = "datalog", argv[1] = "list".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 2
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

/** @} */