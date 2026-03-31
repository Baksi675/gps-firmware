/**
 * @file datalog.h
 * @author github.com/Baksi675
 * @brief Data log module public API.
 *
 * @details
 * This module implements periodic GPS data logging to a FAT filesystem
 * (via FatFs) on an SD card. Each log entry records time, latitude,
 * longitude, orthometric height, movement speed, and movement direction
 * as retrieved from the NEO-6 GPS module.
 *
 * Typical usage:
 * - Initialize the subsystem using @ref datalog_init_subsys
 * - Configure a log using @ref DATALOG_CFG_ts
 * - Register a log handle using @ref datalog_init_handle
 * - Start the subsystem using @ref datalog_start_subsys
 * - Periodically call @ref datalog_run_handle to append log entries
 *
 * @version 0.1
 * @date 2026-02-15
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup datalog_module Data Log Module
 * @brief Periodic GPS data logging to SD card via FatFs.
 * @{
 */

#ifndef DATALOG_H__
#define DATALOG_H__

#include "err.h"
#include "configuration.h"

/**
 * @defgroup datalog_types Data Log Types
 * @ingroup datalog_module
 * @brief Data types used by the data log module.
 * @{
 */

/**
 * @brief Logging interval in seconds between consecutive log entries.
 *
 * @details
 * The numeric value of each enumerator equals the interval in seconds,
 * which is used directly in the timing comparison inside @ref datalog_run_handle.
 */
typedef enum {
    DATALOG_TIME_1S  = 1,   /**< Log every 1 second.  */
    DATALOG_TIME_2S  = 2,   /**< Log every 2 seconds. */
    DATALOG_TIME_3S  = 3,   /**< Log every 3 seconds. */
    DATALOG_TIME_4S  = 4,   /**< Log every 4 seconds. */
    DATALOG_TIME_5S  = 5,   /**< Log every 5 seconds. */
    DATALOG_TIME_10S = 10,  /**< Log every 10 seconds. */
    DATALOG_TIME_20S = 20,  /**< Log every 20 seconds. */
    DATALOG_TIME_30S = 30,  /**< Log every 30 seconds. */
    DATALOG_TIME_40S = 40,  /**< Log every 40 seconds. */
    DATALOG_TIME_50S = 50,  /**< Log every 50 seconds. */
    DATALOG_TIME_60S = 60,  /**< Log every 60 seconds. */
} DATALOG_TIME_te;

/**
 * @brief Configuration structure for initializing a data log handle.
 *
 * @details
 * Passed to @ref datalog_init_handle to configure a new log instance.
 */
typedef struct {
    /** Human-readable name of this log instance (null-terminated). */
    char name[CONFIG_SD_MAX_NAME_LEN];

    /** Interval between consecutive log entries. */
    DATALOG_TIME_te datalog_time;
} DATALOG_CFG_ts;

/**
 * @brief Opaque handle representing a data log instance.
 *
 * @details
 * Returned by @ref datalog_init_handle and used for all subsequent
 * operations on a log instance. The internal structure is hidden
 * and must not be accessed directly.
 */
typedef struct datalog_handle_s DATALOG_HANDLE_ts;

/** @} */

/**
 * @defgroup datalog_api Data Log Public API
 * @ingroup datalog_module
 * @brief Public functions to interact with the data log subsystem.
 * @{
 */

/**
 * @brief Initializes the data log subsystem.
 *
 * @details
 * Resets the internal state, sets up dependencies (log, systick, NEO-6),
 * and registers the subsystem CLI commands.
 *
 * Must be called before any other data log API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 *
 * @note Should only be called once during system startup.
 */
ERR_te datalog_init_subsys(void);

/**
 * @brief Deinitializes the data log subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized
 *
 * @note All handles should be deinitialized before calling this function.
 */
ERR_te datalog_deinit_subsys(void);

/**
 * @brief Starts the data log subsystem.
 *
 * @details
 * Enables runtime processing. After calling this function,
 * @ref datalog_run_handle will perform log writes on schedule.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 *
 * @note Must be called after @ref datalog_init_subsys.
 */
ERR_te datalog_start_subsys(void);

/**
 * @brief Stops the data log subsystem.
 *
 * @details
 * Disables runtime processing. After calling this function,
 * no new log entries will be written.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 *
 * @note Call this before deinitializing the subsystem or handles.
 */
ERR_te datalog_stop_subsys(void);

/**
 * @brief Initializes and registers a data log handle.
 *
 * @details
 * Mounts the FatFs filesystem, creates or truncates the log file
 * (`0:datalog.txt`), and allocates an internal handle from the pool.
 *
 * @param[in]  datalog_cfg      Pointer to the log configuration structure.
 * @param[out] datalog_handle_o Pointer to a handle pointer that will be set
 *                              to the allocated log instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if the subsystem is not initialized or
 *   the filesystem mount or file open fails
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of log handles is reached
 * - ERR_INVALID_ARGUMENT if @p datalog_cfg is NULL
 */
ERR_te datalog_init_handle(DATALOG_CFG_ts const *datalog_cfg, DATALOG_HANDLE_ts **datalog_handle_o);

/**
 * @brief Deinitializes a data log handle.
 *
 * @details
 * Releases the internal resources associated with the given handle
 * and marks the slot as available for reuse.
 *
 * @param[in] datalog_handle Pointer to the handle to deinitialize.
 *
 * @return
 * - ERR_OK on success
 * - ERR_ILLEGAL_ACTION if the handle is not marked as initialized
 * - ERR_UNINITIALZIED_OBJECT if the handle is not found in the pool
 */
ERR_te datalog_deinit_handle(DATALOG_HANDLE_ts const *datalog_handle);

/**
 * @brief Writes a GPS data entry to the log file if the logging interval has elapsed.
 *
 * @details
 * Checks whether the configured interval has passed since the last write.
 * If so, opens `0:datalog.txt` in append mode and writes the following
 * fields from the NEO-6 GPS module, each on its own line:
 * time, latitude, longitude, orthometric height, movement speed,
 * and movement direction. A blank line is appended after each entry.
 *
 * Returns immediately without writing if the interval has not yet elapsed.
 *
 * @param[in,out] datalog_handle Pointer to the handle to run.
 *
 * @return
 * - ERR_OK on success or if the logging interval has not elapsed
 * - ERR_UNKNOWN if the file cannot be opened or a write operation fails
 */
ERR_te datalog_run_handle(DATALOG_HANDLE_ts *datalog_handle);

/** @} */

#endif

/** @} */