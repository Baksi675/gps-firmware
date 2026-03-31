/**
 * @file log.c
 * @author github.com/Baksi675
 * @brief Log subsystem implementation.
 * @version 0.1
 * @date 2025-09-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdarg.h>

#include "log.h"
#include "common.h"
#include "err.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "stm32f401re_rtc.h"
#include "modules.h"
#include "init.h"

/** @brief External array mapping @ref MODULES_te values to their name strings. */
extern const char *modules_names[];

/**
 * @brief Internal state of the log subsystem.
 *
 * @details
 * Holds the active USART instance, the force-disable flag used by the
 * console module during interactive input, and an initialization guard.
 */
struct internal_state_s {
    /** Pointer to the USART peripheral used for log output. */
    USART_REGDEF_ts *usart_instance;

    /** When true, all log output is suppressed regardless of level. */
    bool force_disable;

    /** True after @ref log_init has completed successfully. */
    bool initialized;
};

/** @brief Singleton instance of the log subsystem internal state. */
static struct internal_state_s internal_state = { 0 };

/* ---- Forward declaration for internal helper ---- */
inline static ERR_te log_print_prologue(MODULES_te subsys, LOG_LEVEL_te log_level);

/**
 * @defgroup log_public_apis Log Public APIs
 * @{
 */

/** @brief Initializes the log subsystem. @see log_init */
ERR_te log_init(LOG_HANDLE_ts *log_handle) {
    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    internal_state.usart_instance = log_handle->usart_instance;

    init_rtc();

    GPIO_CFG_ts log_tx_pin = { 0 };
    log_tx_pin.port = log_handle->gpio_port;
    log_tx_pin.pin = log_handle->gpio_pin;
    log_tx_pin.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    log_tx_pin.alternate_function = log_handle->gpio_alternate_function;
    gpio_init(&log_tx_pin);

    USART_CFG_ts log_usart = { 0 };
    log_usart.instance = log_handle->usart_instance;
    log_usart.baud_rate = log_handle->usart_baud_rate;
    usart_init(&log_usart);

    usart_set_transmission(internal_state.usart_instance, ENABLE);

    internal_state.initialized = true;

    return ERR_OK;
}

/** @brief Deinitializes the log subsystem. @see log_deinit */
ERR_te log_deinit() {
    if(internal_state.usart_instance == (void*)0) {
        return ERR_UNINITIALZIED_OBJECT;
    }

    usart_deinit(internal_state.usart_instance);

    internal_state.usart_instance = 0;
    internal_state.force_disable = 0;

    return ERR_OK;
}

/** @brief Prints a formatted message to the serial terminal if the severity threshold is met. @see log_print */
ERR_te log_print(MODULES_te subsys, LOG_LEVEL_te subsys_log_level, LOG_LEVEL_te log_level, char *msg, ...) {
    if (log_level >= subsys_log_level && internal_state.force_disable == false) {
        va_list args;
        va_start(args, msg);

        log_print_prologue(subsys, log_level);

        while (*msg != '\0') {
            if (*msg == '%') {
                msg++;

                if (*msg == 'd') {
                    int32_t num = va_arg(args, int);
                    char num_buf[12];
                    int_to_str(num, num_buf);
                    usart_send(internal_state.usart_instance, (uint8_t*)num_buf, get_str_len(num_buf));
                }
                else if (*msg == 's') {
                    char *str = va_arg(args, char*);
                    usart_send(internal_state.usart_instance, (uint8_t*)str, get_str_len(str));
                }
                else if(*msg == '.') {
                    msg++;

                    char dec_places_str[2];
                    dec_places_str[0] = *msg;   // There can be no more than 9 decimal places
                    dec_places_str[1] = '\0';

                    msg++;

                    if(*msg == 'f') {
                        double num = va_arg(args, double);
                        char num_buf[20] = { 0 };
                        double_to_str(num, num_buf, str_to_int(dec_places_str));
                        usart_send(internal_state.usart_instance, (uint8_t*)num_buf, get_str_len(num_buf));
                    }
                }
            }
            else {
                usart_send(internal_state.usart_instance, (uint8_t*)msg, 1);
            }

            msg++;
        }

        va_end(args);

        usart_send(internal_state.usart_instance, (uint8_t*)"\r\n", 2);
    }

    return ERR_OK;
}

/** @brief Converts a @ref LOG_LEVEL_te value to its lowercase string name. @see log_get_level_name */
ERR_te log_get_level_name(LOG_LEVEL_te log_level, char *str) {
    char const *name;

    switch(log_level) {
        case LOG_LEVEL_NONE:
            name = "none";
            break;

        case LOG_LEVEL_INFO:
            name = "info";
            break;

        case LOG_LEVEL_DEBUG:
            name = "debug";
            break;

        case LOG_LEVEL_WARNING:
            name = "warning";
            break;

        case LOG_LEVEL_ERROR:
            name = "error";
            break;

        case LOG_LEVEL_CRITICAL:
            name = "critical";
            break;

        default:
            return ERR_UNKNOWN;
            break;
    }

    str_cpy(str, name, get_str_len(name) + 1);

    return ERR_OK;
}

/** @brief Converts a log level name string to its @ref LOG_LEVEL_te value. @see log_level_to_int */
ERR_te log_level_to_int(char const *str, LOG_LEVEL_te *log_level) {
    if(str_cmp(str, "none") == true) {
        *log_level = LOG_LEVEL_NONE;
    }
    else if(str_cmp(str, "info") == true) {
        *log_level = LOG_LEVEL_INFO;
    }
    else if(str_cmp(str, "debug") == true) {
        *log_level = LOG_LEVEL_DEBUG;
    }
    else if(str_cmp(str, "warning") == true) {
        *log_level = LOG_LEVEL_WARNING;
    }
    else if(str_cmp(str, "error") == true) {
        *log_level = LOG_LEVEL_ERROR;
    }
    else if(str_cmp(str, "critical") == true) {
        *log_level = LOG_LEVEL_CRITICAL;
    }
    else {
        *log_level = LOG_LEVEL_NONE;
    }

    return ERR_OK;
}

/** @brief Enables or disables forced suppression of all log output. @see log_set_force_disable */
ERR_te log_set_force_disable(bool bool_status) {
    internal_state.force_disable = bool_status;

    return ERR_OK;
}

/** @} */

/**
 * @defgroup log_internal_helpers Log Internal Helpers
 * @{
 */

/**
 * @brief Prints the log message prologue: timestamp, severity, and subsystem name.
 *
 * @details
 * Reads the current time from the RTC and formats it as `(HH:MM:SS)`,
 * zero-padding single-digit values. Follows with the severity level in
 * parentheses and the subsystem name, then `"-> "` to precede the message body.
 *
 * Output format: `(HH:MM:SS)(level)SUBSYSTEM_NAME-> `
 *
 * @param[in] subsys    The subsystem emitting the message.
 * @param[in] log_level The severity level of the message.
 *
 * @return
 * - ERR_OK always
 */
inline static ERR_te log_print_prologue(MODULES_te subsys, LOG_LEVEL_te log_level) {
    TIME_ts rtc_time = { 0 };
    rtc_get_time(&rtc_time);

    char seconds[2];
    int_to_str(rtc_time.seconds, seconds);
    if(get_str_len(seconds) == 1) {
        char temp = seconds[0];
        seconds[0] = '0';
        seconds[1] = temp;
    }

    char minutes[2];
    int_to_str(rtc_time.minutes, minutes);
    if(get_str_len(minutes) == 1) {
        char temp = minutes[0];
        minutes[0] = '0';
        minutes[1] = temp;
    }

    char hours[2];
    int_to_str(rtc_time.hours, hours);
    if(get_str_len(hours) == 1) {
        char temp = hours[0];
        hours[0] = '0';
        hours[1] = temp;
    }

    char time_format[] = "(__:__:__)";

    str_set(time_format, seconds, 2, 7);
    str_set(time_format, minutes, 2, 4);
    str_set(time_format, hours, 2, 1);

    usart_send(internal_state.usart_instance, (uint8_t*)time_format, 10);

    char log_level_str[16];
    uint8_t log_level_len = 0;
    log_level_str[0] = '(';
    log_get_level_name(log_level, log_level_str + 1);
    log_level_len = get_str_len(log_level_str);
    log_level_str[log_level_len] = ')';
    log_level_str[log_level_len + 1] = '\0';

    usart_send(
        internal_state.usart_instance,
        (uint8_t*)log_level_str,
        get_str_len(log_level_str) + 1
    );

    usart_send(
        internal_state.usart_instance,
        (uint8_t*)modules_names[subsys],
        get_str_len((char*)modules_names[subsys])
    );

    usart_send(
        internal_state.usart_instance,
        (uint8_t*)"-> ",
        3
    );

    return ERR_OK;
}

/** @} */