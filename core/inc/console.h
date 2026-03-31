/**
 * @file console.h
 * @author github.com/Baksi675
 * @brief Console subsystem public API.
 *
 * @details
 * This module provides a USART-based interactive console that bridges
 * raw serial input to the command dispatch subsystem (@ref cmd_module).
 *
 * Features include:
 * - USART peripheral and GPIO initialization for serial reception
 * - Circular-buffer-backed receive path via an ISR callback
 * - Console mode toggled by Ctrl+C, with local echo and backspace handling
 * - Command execution via @ref cmd_execute on carriage return
 *
 * Typical usage:
 * - Populate a @ref CONSOLE_HANDLE_ts with the target USART instance,
 *   baud rate, and RX GPIO configuration
 * - Call @ref console_init once at startup
 * - Call @ref console_run periodically from the main loop or scheduler
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup console_module Console Module
 * @brief USART-based interactive console with command dispatch.
 * @{
 */

#ifndef CONSOLE_H__
#define CONSOLE_H__

#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "err.h"

/**
 * @defgroup console_types Console Types
 * @ingroup console_module
 * @brief Data types used by the console module.
 * @{
 */

/**
 * @brief Configuration handle for initializing the console subsystem.
 *
 * @details
 * This structure is passed to @ref console_init and provides all
 * hardware parameters needed to configure the USART peripheral
 * and its associated RX GPIO pin.
 */
typedef struct {
    /** Pointer to the USART peripheral instance to use for the console. */
    USART_REGDEF_ts *usart_instance;

    /** Baud rate to configure on the USART peripheral. */
    USART_BAUD_RATE_te usart_baud_rate;

    /** GPIO port of the USART RX pin. */
    GPIO_REGDEF_ts *gpio_port;

    /** GPIO pin number of the USART RX pin. */
    GPIO_PIN_te gpio_pin;

    /** Alternate function mapping to route the pin to the USART peripheral. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
} CONSOLE_HANDLE_ts;

/** @} */

/**
 * @defgroup console_api Console Public API
 * @ingroup console_module
 * @brief Public functions to interact with the console subsystem.
 * @{
 */

/**
 * @brief Initializes the console subsystem.
 *
 * @details
 * Configures the internal state, initializes the RX GPIO pin in alternate
 * function mode, and initializes the USART peripheral with interrupt-driven
 * reception. USART reception is enabled before the function returns.
 *
 * @note @ref CONFIG_CONSOLE_USART_CBUF_SIZE must be a power of two.
 *       Initialization will fail with @ref ERR_INVALID_CONFIGURATION otherwise.
 *
 * @param[in] console_handle Pointer to the console configuration structure.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_CONFIGURATION if the USART circular buffer size is not a power of two
 */
ERR_te console_init(CONSOLE_HANDLE_ts *console_handle);

/**
 * @brief Runs the console state machine. Must be called periodically.
 *
 * @details
 * Each call performs the following steps:
 * -# Drains any bytes received into the USART circular buffer by the ISR.
 * -# Handles console mode entry and exit via the Ctrl+C character.
 * -# In console mode, echoes typed characters and accumulates them in
 *    an internal buffer.
 * -# On carriage return, processes backspace characters, then passes the
 *    resulting command string to @ref cmd_execute.
 *
 * @return
 * - ERR_OK on success
 * - Propagated error from internal circular buffer or command operations
 */
ERR_te console_run(void);

/** @} */

#endif

/** @} */