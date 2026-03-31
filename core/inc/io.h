/**
 * @file io.h
 * @author github.com/Baksi675
 * @brief Digital IO module public API.
 *
 * @details
 * This module provides a named abstraction over GPIO pins, supporting
 * both digital output (write) and digital input (read) operations.
 * It follows the same subsystem lifecycle pattern used across the project:
 * init → start → run → stop → deinit.
 *
 * Typical usage:
 * - Initialize the subsystem using @ref io_init_subsys
 * - Populate an @ref IO_CFG_ts with a GPIO configuration and a name
 * - Register a handle using @ref io_init_handle
 * - Start the subsystem using @ref io_start_subsys
 * - Drive or sample the pin using @ref io_write and @ref io_read
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup io_module IO Module
 * @brief Named digital IO abstraction over GPIO pins.
 * @{
 */

#ifndef IO_H__
#define IO_H__

#include "err.h"
#include "stm32f401re_gpio.h"
#include "configuration.h"

/**
 * @defgroup io_types IO Types
 * @ingroup io_module
 * @brief Data types used by the IO module.
 * @{
 */

/**
 * @brief Configuration structure for initializing an IO handle.
 *
 * @details
 * Passed to @ref io_init_handle to configure a new IO instance.
 * The GPIO pin is initialized internally using @p gpio_handle.
 */
typedef struct {
    /** Pointer to the GPIO configuration for this IO pin. */
    GPIO_CFG_ts *gpio_handle;

    /** Human-readable name of this IO instance (null-terminated). */
    char name[CONFIG_IO_MAX_NAME_LEN];
} IO_CFG_ts;

/**
 * @brief Opaque handle representing an IO instance.
 *
 * @details
 * Returned by @ref io_init_handle and used for all subsequent
 * read and write operations on that pin. The internal structure
 * is hidden and must not be accessed directly.
 */
typedef struct io_handle_s IO_HANDLE_ts;

/** @} */

/**
 * @defgroup io_api IO Public API
 * @ingroup io_module
 * @brief Public functions to interact with the IO subsystem.
 * @{
 */

/**
 * @brief Initializes the IO subsystem.
 *
 * @details
 * Resets the internal state, registers the CLI commands, and
 * initializes the logging dependency.
 *
 * Must be called before any other IO API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 *
 * @note Should only be called once during system startup.
 */
ERR_te io_init_subsys(void);

/**
 * @brief Deinitializes the IO subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 * The subsystem must be stopped before calling this function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized or still running
 *
 * @note All IO handles should be deinitialized before calling this function.
 */
ERR_te io_deinit_subsys(void);

/**
 * @brief Starts the IO subsystem.
 *
 * @details
 * Enables runtime operations. After calling this function,
 * @ref io_write and @ref io_read may be called.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 *
 * @note Must be called after @ref io_init_subsys.
 */
ERR_te io_start_subsys(void);

/**
 * @brief Stops the IO subsystem.
 *
 * @details
 * Disables runtime operations. After calling this function,
 * @ref io_write and @ref io_read will return an error.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 *
 * @note Call this before deinitializing the subsystem or handles.
 */
ERR_te io_stop_subsys(void);

/**
 * @brief Initializes and registers an IO handle.
 *
 * @details
 * Configures the GPIO pin described by @p io_cfg and allocates an
 * internal handle from the pool.
 *
 * @param[in]  io_cfg     Pointer to the IO configuration structure.
 * @param[out] io_handle_o Pointer to a handle pointer that will be set
 *                         to the allocated IO instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of IO handles is reached
 */
ERR_te io_init_handle(IO_CFG_ts *io_cfg, IO_HANDLE_ts **io_handle_o);

/**
 * @brief Deinitializes an IO handle.
 *
 * @details
 * Releases the internal resources associated with the given handle
 * and marks the slot as available for reuse.
 *
 * @param[in] io_handle Pointer to the handle to deinitialize.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNINITIALZIED_OBJECT if no handles exist or the handle is not found
 */
ERR_te io_deinit_handle(IO_HANDLE_ts const *io_handle);

/**
 * @brief Writes a logic level to an IO pin.
 *
 * @param[in] io_handle  Pointer to the IO handle to write to.
 * @param[in] pin_status The logic level to drive on the pin (@ref HIGH or @ref LOW).
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or started
 */
ERR_te io_write(IO_HANDLE_ts *io_handle, PIN_STATUS_te pin_status);

/**
 * @brief Reads the current logic level of an IO pin.
 *
 * @param[in]  io_handle    Pointer to the IO handle to read from.
 * @param[out] pin_status_o Pointer to a variable that will receive the
 *                          current pin level (@ref HIGH or @ref LOW).
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or started
 */
ERR_te io_read(IO_HANDLE_ts const *io_handle, PIN_STATUS_te *pin_status_o);

/** @} */

#endif

/** @} */