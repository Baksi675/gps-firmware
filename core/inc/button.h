/**
 * @file button.h
 * @author github.com/Baksi675
 * @brief Button module public API.
 *
 * @details
 * This module provides functionality for handling physical button inputs,
 * including:
 * - Debouncing
 * - Button press detection
 * - Button hold detection
 *
 * The module supports multiple button instances and integrates with
 * the system's GPIO, logging, and command subsystems.
 *
 * Typical usage:
 * - Initialize subsystem using @ref button_init_subsys
 * - Configure button using @ref BUTTON_CFG_ts
 * - Register button using @ref button_init_handle
 * - Periodically call @ref button_run_handle or @ref button_run_handle_all
 *
 * @version 0.1
 * @date 2026-01-31
 *
 * @copyright Copyright (c) 2026
 */

 /**
 * @defgroup BUTTON_MODULE Button Module
 * @brief Button driver/module providing debouncing and state detection.
 * @{
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

#include "io.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "err.h"

/**
 * @defgroup BUTTON_TYPES Button Types
 * @ingroup BUTTON_MODULE
 * @brief Data types used by the button module.
 * @{
 */

/**
 * @brief Defines the active level of a button press.
 *
 * @details
 * This enum specifies which logic level represents a "pressed" state
 * for the button input.
 */
typedef enum {

    /** Button is considered pressed when the GPIO pin reads LOW (active-low). */
    BUTTON_PUSHED_TYPE_LOW,

    /** Button is considered pressed when the GPIO pin reads HIGH (active-high). */
    BUTTON_PUSHED_TYPE_HIGH

} BUTTON_PUSHED_TYPE_te;

/**
 * @brief Configuration structure for initializing a button instance.
 *
 * @details
 * This structure is used to configure a button before registering it
 * with the button subsystem using @ref button_init_handle.
 */
typedef struct {

    /** Human-readable name of the button (null-terminated string). */
    char name[CONFIG_BUTTON_MAX_NAME_LEN];

    /** GPIO port where the button is connected. */
    GPIO_REGDEF_ts *gpio_port;

    /** GPIO pin number where the button is connected. */
    GPIO_PIN_te gpio_pin;

    /**
     * Defines the active level of the button.
     * - BUTTON_PUSHED_TYPE_HIGH: button is pressed when pin is HIGH
     * - BUTTON_PUSHED_TYPE_LOW:  button is pressed when pin is LOW
     */
    BUTTON_PUSHED_TYPE_te pushed_type;

    /** Debounce time in milliseconds. */
    uint32_t debounce_limit_ms;

    /** Time in milliseconds required to detect a "held" button. */
    uint32_t held_limit_ms;

} BUTTON_CFG_ts;

/**
 * @brief Opaque handle representing a button instance.
 *
 * @details
 * This handle is returned by @ref button_init_handle and is used to
 * interact with a specific button instance.
 *
 * The internal structure is hidden and must not be accessed directly.
 */
typedef struct button_handle_s BUTTON_HANDLE_ts;

/** @} */

/**
 * @defgroup BUTTON_API Button Public API
 * @ingroup BUTTON_MODULE
 * @brief Public functions to interact with the button subsystem.
 * @{
 */

/**
 * @brief Initializes the button subsystem.
 *
 * @details
 * This function initializes the internal state of the button module,
 * sets up required dependencies, and registers subsystem commands.
 *
 * It must be called before using any other button API functions.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if the subsystem is already initialized
 * - Other error codes depending on internal failures
 *
 * @note This function should only be called once during system startup.
 */
ERR_te button_init_subsys(void);

/**
 * @brief Deinitializes the button subsystem.
 *
 * @details
 * This function resets the internal state of the button subsystem and
 * deregisters previously registered commands.
 *
 * It must only be called after the subsystem has been initialized and
 * fully stopped.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is still running or not initialized
 *
 * @note All button handles must be deinitialized before calling this function.
 */
ERR_te button_deinit_subsys(void);

/**
 * @brief Starts the button subsystem.
 *
 * @details
 * Enables runtime processing of button handles. After calling this function,
 * button state updates via button_run_handle() or button_run_handle_all()
 * are allowed.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 *
 * @note Must be called after button_init_subsys().
 */
ERR_te button_start_subsys(void);

/**
 * @brief Stops the button subsystem.
 *
 * @details
 * Disables runtime processing of button handles. After calling this function,
 * no button state updates should be performed.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 *
 * @note Call this before deinitializing the subsystem or handles.
 */
ERR_te button_stop_subsys(void);

/**
 * @brief Initializes and registers a button handle.
 *
 * @details
 * Configures the GPIO associated with the button and allocates an internal
 * handle from the subsystem pool.
 *
 * The returned handle must be used for all subsequent operations on the button.
 *
 * @param[in] button_cfg Pointer to the button configuration structure.
 * @param[out] button_handle_o Pointer to a handle pointer that will be set
 *                             to the allocated button instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNINITIALZIED_OBJECT if the subsystem is not initialized
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of buttons is reached
 * - ERR_INVALID_ARGUMENT if input arguments are NULL
 *
 * @note GPIO is configured internally as input during initialization.
 */
ERR_te button_init_handle(BUTTON_CFG_ts *button_cfg, BUTTON_HANDLE_ts **button_handle);

/**
 * @brief Deinitializes a button handle.
 *
 * @details
 * Releases the internal resources associated with the given button handle
 * and marks the slot as available for reuse.
 *
 * @param[in] button_handle Pointer to the button handle to deinitialize.
 *
 * @return
 * - ERR_OK on success
 * - ERR_ILLEGAL_ACTION if the subsystem is still running
 * - ERR_UNINITIALZIED_OBJECT if the handle is invalid or not found
 *
 * @note The subsystem must be stopped before calling this function.
 */
ERR_te button_deinit_handle(BUTTON_HANDLE_ts const *button_handle);

/**
 * @brief Executes the state machine for a single button.
 *
 * @details
 * This function performs debounce handling, press detection, and hold detection
 * for the specified button handle.
 *
 * It should be called periodically (e.g., in a main loop or scheduler).
 *
 * @param[in,out] button_handle Pointer to the button handle to process.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or started
 *
 * @note Timing is based on the system tick (systick_get_ms()).
 */
ERR_te button_run_handle(BUTTON_HANDLE_ts *button_handle);

/**
 * @brief Runs the state machine for all registered buttons.
 *
 * @details
 * Iterates over all active button handles and processes their state machines.
 *
 * @return
 * - ERR_OK on success
 * - First encountered error from button_run_handle()
 */
ERR_te button_run_handle_all(void);

/**
 * @brief Retrieves the pushed (pressed) state of a button.
 *
 * @param[in] button_handle Pointer to the button handle.
 * @param[out] pushed_state_o Pointer to a boolean that will receive the state.
 *
 * @return
 * - ERR_OK on success
 * - ERR_ILLEGAL_ACTION if the subsystem is not initialized or started
 *
 * @note The returned state reflects the debounced logical state.
 */
ERR_te button_get_pushed_state(BUTTON_HANDLE_ts const *button_handle, bool *pushed_state_o);

/**
 * @brief Retrieves the held state of a button.
 *
 * @param[in] button_handle Pointer to the button handle.
 * @param[out] held_state_o Pointer to a boolean that will receive the state.
 *
 * @return
 * - ERR_OK on success
 * - ERR_ILLEGAL_ACTION if the subsystem is not initialized or started
 *
 * @note The held state becomes true after the configured held time threshold.
 */
ERR_te button_get_held_state(BUTTON_HANDLE_ts const *button_handle, bool *held_state_o);

/** @} */

#endif

/** @} */