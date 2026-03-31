/**
 * @file init.h
 * @author github.com/Baksi675
 * @brief Common initialization public API.
 *
 * @details
 * This header provides initialization functions for shared hardware and
 * software dependencies used across multiple subsystems. Each function
 * encapsulates the full configuration of its target peripheral or module,
 * keeping subsystem init routines free of hardware-specific setup details.
 *
 * These functions are called from subsystem init routines (e.g.
 * @ref button_init_subsys, @ref datalog_init_subsys) to ensure their
 * required dependencies are ready before use.
 *
 * @version 0.1
 * @date 2026-03-21
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup init_module Initialization Module
 * @brief Shared hardware and software dependency initialization.
 * @{
 */

#ifndef INIT_H__
#define INIT_H__

#include "err.h"

/**
 * @defgroup init_api Initialization Public API
 * @ingroup init_module
 * @brief Public initialization functions for shared dependencies.
 * @{
 */

/**
 * @brief Initializes the logging subsystem.
 *
 * @details
 * Configures the log module with a fixed hardware target:
 * USART1 at 115200 baud on GPIOA pin 9 (alternate function AF7).
 *
 * @return
 * - ERR_OK on success
 * - Propagated error from @ref log_init on failure
 */
ERR_te init_log(void);

/**
 * @brief Initializes the SysTick timer.
 *
 * @details
 * Configures SysTick to use the processor clock source with
 * interrupt generation enabled, providing the system millisecond
 * tick used by @ref systick_get_ms.
 *
 * @return
 * - ERR_OK on success
 * - Propagated error from @ref systick_init on failure
 */
ERR_te init_systick(void);

/**
 * @brief Initializes the NEO-6 GPS module.
 *
 * @details
 * Initializes the NEO-6 subsystem and registers a handle configured
 * for USART6 at 9600 baud, with RX on GPIOA pin 12 and TX on GPIOA
 * pin 11 (alternate function AF8). Starts the subsystem after handle
 * initialization.
 *
 * @return
 * - ERR_OK on success
 * - Propagated error from the last failing NEO-6 call on failure
 *
 * @note Errors from intermediate steps (subsys init, handle init) are
 *       overwritten by subsequent calls. Only the last error is returned.
 */
ERR_te init_neo6(void);

/**
 * @brief Initializes the RTC peripheral and sets a default calendar and time.
 *
 * @details
 * Initializes the RTC hardware. If the RTC is already initialized
 * (@ref ERR_MODULE_ALREADY_INITIALIZED), the function returns immediately
 * without overwriting the current time, preserving any previously set value.
 *
 * If initialization succeeds, sets the calendar to Thursday, 29 January 2026
 * and the time to 00:00:00 as a compile-time default.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the RTC was already initialized
 * - Propagated error from @ref rtc_init on failure
 */
ERR_te init_rtc(void);

/** @} */

#endif

/** @} */