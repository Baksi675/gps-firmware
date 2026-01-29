/**
 * @file log.h
 * @author github.com/Baksi675
 * @brief Header file log subsystem.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef LOG_H__
#define LOG_H__

#include "stm32f401re.h"
#include "stm32f401re_usart.h"
#include "stm32f401re_gpio.h"
#include "err.h"
#include "configuration.h"
#include "modules.h"

typedef enum {
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_CRITICAL,
	LOG_LEVEL_NONE,
	LOG_LEVEL_COUNT
}LOG_LEVEL_te;

typedef struct {
	USART_REGDEF_ts *usart_instance;
	USART_BAUD_RATE_te usart_baud_rate;
	GPIO_REGDEF_ts *gpio_port;
	GPIO_PIN_te gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
}LOG_HANDLE_ts;

ERR_te log_init(LOG_HANDLE_ts *log_handle);
ERR_te log_deinit();
ERR_te log_print(MODULES_te subsys, LOG_LEVEL_te subsys_log_level, LOG_LEVEL_te log_level, char *msg, ...);
ERR_te log_get_level_name(LOG_LEVEL_te log_level, char *str);
ERR_te log_level_to_int(char const *str, LOG_LEVEL_te *log_level_o);
ERR_te log_set_force_disable(bool bool_status);

#if defined(CONFIG_COMPILE_WITH_LOGGING)

#define LOG_INFO(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#define LOG_DEBUG(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define LOG_WARNING(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

#define LOG_ERROR(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define LOG_CRITICAL(subsys, lvl, fmt, ...) \
    log_print((subsys), (lvl), LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

#else

#define LOG_INFO(subsys, lvl, fmt, ...)  ((void)0)
#define LOG_DEBUG(subsys, lvl, fmt, ...) ((void)0)
#define LOG_WARNING(subsys, lvl, fmt, ...)  ((void)0)
#define LOG_ERROR(subsys, lvl, fmt, ...) ((void)0)
#define LOG_CRITICAL(subsys, lvl, fmt, ...)  ((void)0)

#endif

#endif