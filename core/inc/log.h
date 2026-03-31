/**
 * @file log.h
 * @author github.com/Baksi675
 * @brief Log subsystem public API.
 *
 * @details
 * This module provides serial logging over USART with per-message severity
 * levels and per-subsystem log level filtering. Each log message is prefixed
 * with an RTC timestamp, the severity level, and the originating subsystem name.
 *
 * Logging is controlled at two levels:
 * - **Compile time**: define @ref CONFIG_COMPILE_WITH_LOGGING to enable the
 *   @ref LOG_INFO, @ref LOG_DEBUG, @ref LOG_WARNING, @ref LOG_ERROR, and
 *   @ref LOG_CRITICAL macros. Without this define, all macros expand to
 *   no-ops and the logging overhead is eliminated entirely.
 * - **Runtime**: each subsystem holds a @ref LOG_LEVEL_te variable. A message
 *   is only printed if its severity is greater than or equal to the subsystem's
 *   current level. The level can be adjusted at runtime via the CLI.
 *
 * Typical usage:
 * - Populate a @ref LOG_HANDLE_ts and call @ref log_init once at startup
 *   (typically via @ref init_log)
 * - Use the @ref LOG_INFO / @ref LOG_ERROR / etc. macros throughout the codebase
 * - Call @ref log_set_force_disable to suppress output during console input
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup log_module Log Module
 * @brief USART-based serial logging with severity filtering and RTC timestamps.
 * @{
 */

#ifndef LOG_H__
#define LOG_H__

#include "stm32f401re.h"
#include "stm32f401re_usart.h"
#include "stm32f401re_gpio.h"
#include "err.h"
#include "configuration.h"
#include "modules.h"

/**
 * @defgroup log_types Log Types
 * @ingroup log_module
 * @brief Data types used by the log module.
 * @{
 */

/**
 * @brief Log severity levels, in ascending order of severity.
 *
 * @details
 * A subsystem's configured level acts as a threshold: only messages at or
 * above that level are printed. @ref LOG_LEVEL_NONE suppresses all output.
 */
typedef enum {
    LOG_LEVEL_INFO,     /**< Informational messages for normal operation. */
    LOG_LEVEL_DEBUG,    /**< Detailed diagnostic messages for development. */
    LOG_LEVEL_WARNING,  /**< Non-critical conditions that may warrant attention. */
    LOG_LEVEL_ERROR,    /**< Recoverable error conditions. */
    LOG_LEVEL_CRITICAL, /**< Severe errors that may compromise system operation. */
    LOG_LEVEL_NONE,     /**< Suppress all log output for this subsystem. */
    LOG_LEVEL_COUNT     /**< Total number of log levels. Not a valid log level. */
} LOG_LEVEL_te;

/**
 * @brief Configuration handle for initializing the log subsystem.
 *
 * @details
 * Passed to @ref log_init to configure the USART peripheral and TX GPIO
 * pin used for serial log output.
 */
typedef struct {
    /** Pointer to the USART peripheral instance to use for log output. */
    USART_REGDEF_ts *usart_instance;

    /** Baud rate to configure on the USART peripheral. */
    USART_BAUD_RATE_te usart_baud_rate;

    /** GPIO port of the USART TX pin. */
    GPIO_REGDEF_ts *gpio_port;

    /** GPIO pin number of the USART TX pin. */
    GPIO_PIN_te gpio_pin;

    /** Alternate function mapping to route the pin to the USART peripheral. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
} LOG_HANDLE_ts;

/** @} */

/**
 * @defgroup log_api Log Public API
 * @ingroup log_module
 * @brief Public functions to interact with the log subsystem.
 * @{
 */

/**
 * @brief Initializes the log subsystem.
 *
 * @details
 * Configures the TX GPIO pin in alternate function mode, initializes the
 * USART peripheral, enables transmission, and initializes the RTC dependency.
 *
 * @param[in] log_handle Pointer to the log configuration structure.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the log subsystem is already initialized
 *
 * @note Typically called indirectly via @ref init_log rather than directly.
 */
ERR_te log_init(LOG_HANDLE_ts *log_handle);

/**
 * @brief Deinitializes the log subsystem.
 *
 * @details
 * Resets the USART peripheral registers to their reset values and
 * clears the internal state.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNINITIALZIED_OBJECT if the log subsystem has not been initialized
 */
ERR_te log_deinit(void);

/**
 * @brief Prints a formatted message to the serial terminal if the severity threshold is met.
 *
 * @details
 * The message is only transmitted if @p log_level >= @p subsys_log_level
 * and force-disable is not active. Each message is preceded by a prologue
 * containing the RTC timestamp, severity level, and subsystem name.
 *
 * Supported format specifiers:
 * - `%d` — signed 32-bit integer
 * - `%s` — null-terminated string
 * - `%.Nf` — double with N decimal places (N must be a single digit, 1–9)
 *
 * @param[in] subsys           The subsystem emitting the message.
 * @param[in] subsys_log_level The current log level threshold of the subsystem.
 * @param[in] log_level        The severity level of this message.
 * @param[in] msg              Printf-style format string.
 * @param[in] ...              Arguments for the format specifiers in @p msg.
 *
 * @return
 * - ERR_OK always (filtering is silent, not an error)
 *
 * @note Prefer the @ref LOG_INFO, @ref LOG_ERROR, etc. macros over calling
 *       this function directly.
 */
ERR_te log_print(MODULES_te subsys, LOG_LEVEL_te subsys_log_level, LOG_LEVEL_te log_level, char *msg, ...);

/**
 * @brief Converts a @ref LOG_LEVEL_te value to its lowercase string name.
 *
 * @details
 * Writes the level name (e.g. `"info"`, `"error"`) into @p str.
 * The caller must ensure @p str is large enough to hold the result
 * and a null terminator.
 *
 * @param[in]  log_level The log level to convert.
 * @param[out] str       Pointer to the destination buffer.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if @p log_level is not a recognized value
 */
ERR_te log_get_level_name(LOG_LEVEL_te log_level, char *str);

/**
 * @brief Converts a log level name string to its @ref LOG_LEVEL_te value.
 *
 * @details
 * Accepts the lowercase level names: `"none"`, `"info"`, `"debug"`,
 * `"warning"`, `"error"`, `"critical"`. Unrecognized strings result in
 * @ref LOG_LEVEL_NONE being written to @p log_level_o.
 *
 * @param[in]  str          Pointer to the null-terminated level name string.
 * @param[out] log_level_o  Pointer to a variable that will receive the parsed level.
 *
 * @return
 * - ERR_OK always (unrecognized strings silently default to @ref LOG_LEVEL_NONE)
 */
ERR_te log_level_to_int(char const *str, LOG_LEVEL_te *log_level_o);

/**
 * @brief Enables or disables forced suppression of all log output.
 *
 * @details
 * When force-disable is active, @ref log_print produces no output regardless
 * of severity level or subsystem threshold. Used by the console module to
 * suppress log output during interactive command input.
 *
 * @param[in] bool_status true to suppress all output, false to restore normal operation.
 *
 * @return
 * - ERR_OK always
 */
ERR_te log_set_force_disable(bool bool_status);

/** @} */

/**
 * @defgroup log_macros Log Macros
 * @ingroup log_module
 * @brief Compile-time-gated convenience macros for emitting log messages.
 *
 * @details
 * When @ref CONFIG_COMPILE_WITH_LOGGING is defined, each macro expands to
 * a call to @ref log_print with the appropriate severity level.
 * When the define is absent, all macros expand to `((void)0)`, producing
 * no code and zero runtime overhead.
 *
 * @param subsys The @ref MODULES_te identifier of the calling subsystem.
 * @param lvl    The @ref LOG_LEVEL_te threshold of the calling subsystem.
 * @param fmt    Printf-style format string.
 * @param ...    Arguments for the format specifiers in @p fmt.
 * @{
 */

#if defined(CONFIG_COMPILE_WITH_LOGGING)

/** @brief Emits an informational log message. */
#define LOG_INFO(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

/** @brief Emits a debug log message. */
#define LOG_DEBUG(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

/** @brief Emits a warning log message. */
#define LOG_WARNING(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

/** @brief Emits an error log message. */
#define LOG_ERROR(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/** @brief Emits a critical log message. */
#define LOG_CRITICAL(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

#else

#define LOG_INFO(subsys, lvl, fmt, ...)     ((void)0) /**< No-op when logging is disabled. */
#define LOG_DEBUG(subsys, lvl, fmt, ...)    ((void)0) /**< No-op when logging is disabled. */
#define LOG_WARNING(subsys, lvl, fmt, ...)  ((void)0) /**< No-op when logging is disabled. */
#define LOG_ERROR(subsys, lvl, fmt, ...)    ((void)0) /**< No-op when logging is disabled. */
#define LOG_CRITICAL(subsys, lvl, fmt, ...) ((void)0) /**< No-op when logging is disabled. */

#endif

/** @} */

#endif

/** @} */