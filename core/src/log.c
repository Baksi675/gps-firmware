/**
 *
 * @file log.c
 * @author github.com/Baksi675
 * @brief Log subsystem implementation.
 * @version 0.1
 * @date 2025-09-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "log.h"
#include "common.h"
#include "err.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "stm32f401re_rtc.h"
#include "stdarg.h"

struct internal_state_s {
	USART_REGDEF_ts *usart_instance;
	bool force_disable;
};
static struct internal_state_s internal_state = { 0 };

inline static ERR_te log_print_subsys(LOG_SUBSYS_te subsys);

/** 
 * @defgroup LOG_Public_APIs LOG Public APIs
 * @{
 */

/**
 * @brief Initializes the LOG software module. Configures the internal state, initializes the GPIO pins and the USART peripheral.
 * 
 * @param log_handle A log configuration object.
 * @return ERR_te Error code generated during execution.
 */
ERR_te log_init(LOG_HANDLE_ts *log_handle) {
	if(internal_state.usart_instance != NULL) {
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state.usart_instance = log_handle->usart_instance;

	GPIO_HANDLE_ts log_tx_pin = { 0 };
	log_tx_pin.port = log_handle->gpio_port;
	log_tx_pin.pin = log_handle->gpio_pin;
	log_tx_pin.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	log_tx_pin.alternate_function = log_handle->gpio_alternate_function;
	gpio_init(&log_tx_pin);

	USART_HANDLE_ts log_usart = { 0 };
	log_usart.instance = log_handle->usart_instance;
	log_usart.baud_rate = log_handle->usart_baud_rate;
	usart_init(&log_usart);

	usart_set_transmission(internal_state.usart_instance, ENABLE);

	return ERR_OK;
}

/**
 * @brief Deinitializes the LOG software module by setting the corresponding USART peripheral registers back to their reset values. Also resets the internal state.
 * 
 * @return ERR_te Error code generated during execution.
 */
ERR_te log_deinit() {
	if(internal_state.usart_instance == NULL) {
		return ERR_UNINITIALZIED_OBJECT;
	}

	usart_deinit(internal_state.usart_instance);
	
	internal_state.usart_instance = 0;
	internal_state.force_disable = 0;

	return ERR_OK;
}

/**
 * @brief Prints a message to the serial terminal via USART.
 * 
 * @param subsys The subsystem where the logging is being carried out from.
 * @param subsys_log_level The current log level of the subsystem.
 * @param log_level The level of the log message.
 * @param msg The message to print out on the serial terminal.
 * @param ... Values given to the format specifiers.
 * @return ERR_te Error generated during execution.
 */
ERR_te log_print(LOG_SUBSYS_te subsys, LOG_LEVEL_te subsys_log_level, LOG_LEVEL_te log_level, char *msg, ...) {
	ERR_te err;
	
	if (log_level >= subsys_log_level && internal_state.force_disable == false) {

		va_list args;
		va_start(args, msg);

		err = log_print_subsys(subsys);
		if(err != ERR_OK) {
			return err;
		}
		
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
					dec_places_str[0] = *msg;						// There can be no more than 9 decimal places!!!
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
	}

	return ERR_OK;
}

/**
 * @brief Fills a char buffer passed as argument with the level name also passed as argument.
 * 
 * @param log_level The logging level to convert into string.
 * @param str Pointer to the character buffer.
 * @return ERR_te Error code generated during execution.
 */
ERR_te log_get_level_name(LOG_LEVEL_te log_level, char *str) {
	switch(log_level) {
		case LOG_LEVEL_NONE:
			str[0] = 'N'; str[1] = 'O'; str[2] = 'N'; str[3] = 'E'; str[4] = '\0';
			break;

		case LOG_LEVEL_INFO:
			str[0] = 'I'; str[1] = 'N'; str[2] = 'F'; str[3] = 'O'; str[4] = '\0';
			break;

		case LOG_LEVEL_DEBUG:
			str[0] = 'D'; str[1] = 'E'; str[2] = 'B'; str[3] = 'U'; str[4] = 'G'; str[5] = '\0';
			break;

		case LOG_LEVEL_WARNING:
			str[0] = 'W'; str[1] = 'A'; str[2] = 'R'; str[3] = 'N'; str[4] = 'I'; str[5] = 'N'; str[6] = 'G'; str[7] = '\0';
			break;

		case LOG_LEVEL_ERROR:
			str[0] = 'E'; str[1] = 'R'; str[2] = 'R'; str[3] = 'O'; str[4] = 'R'; str[5] = '\0';
			break;

		case LOG_LEVEL_CRITICAL:
			str[0] = 'C'; str[1] = 'R'; str[2] = 'I'; str[3] = 'T'; str[4] = 'I'; str[5] = 'C'; str[6] = 'A'; str[7] = 'L'; str[8] = '\0';
			break;

		default:
			return ERR_UNKNOWN;
			break;
	}

	return ERR_OK;
}

/**
 * @brief Converts a log level in string format to the corresponding enum number.
 * 
 * @param str The log level string.
 * @return LOG_LEVEL_te The log level in enum format.
 */
ERR_te log_level_to_int(char *str, LOG_LEVEL_te *log_level) {
	if(str[0] == 'n' && str[1] == 'o' && str[2] == 'n' && str[3] == 'e') {
		*log_level = LOG_LEVEL_NONE;
	}
	else if(str[0] == 'i' && str[1] == 'n' && str[2] == 'f' && str[3] == 'o') {
		*log_level = LOG_LEVEL_INFO;
	}
	else if(str[0] == 'd' && str[1] == 'e' && str[2] == 'b' && str[3] == 'u' && str[4] == 'g') {
		*log_level = LOG_LEVEL_DEBUG;
	}
	else if(str[0] == 'w' && str[1] == 'a' && str[2] == 'r' && str[3] == 'n' && str[4] == 'i' && str[5] == 'n' && str[6] == 'g') {
		*log_level = LOG_LEVEL_WARNING;
	}
	else if(str[0] == 'e' && str[1] == 'r' && str[2] == 'r' && str[3] == 'o' && str[4] == 'r') {
		*log_level = LOG_LEVEL_ERROR;
	}
	else if(str[0] == 'c' && str[1] == 'r' && str[2] == 'i' && str[3] == 't' && str[4] == 'i' && str[5] == 'c' && str[6] == 'a' && str[7] == 'l') {
		*log_level = LOG_LEVEL_CRITICAL;
	}

	*log_level = LOG_LEVEL_NONE;

	return ERR_OK;
}

/**
 * @brief If set to ENABLED, this function force disables logging.
 * 
 * @param en_status Enable or disable logging.
 * @return ERR_te Error code generated during execution. 
 */
ERR_te log_set_force_disable(bool bool_status) {
	internal_state.force_disable = bool_status;

	return ERR_OK;
}

 /** @} */

/** 
 * @defgroup LOG_Internal_Helper LOG Internal Helpers
 * @{
 */

 /**
  * @brief Prints the subsystem name to the beginning of a new line with a prompt mark. 
  * 
  * @param subsys The subsystem to print the name of.
  */
inline static ERR_te log_print_subsys(LOG_SUBSYS_te subsys) {
    TIME_ts rtc_time = { 0 };
	rtc_get_time(&rtc_time);

	char seconds[2];
	int_to_str(rtc_time.time_seconds, seconds);
	if(get_str_len(seconds) == 1) {
		char temp = seconds[0];
		seconds[0] = '0';
		seconds[1] = temp;
	}

	char minutes[2];
	int_to_str(rtc_time.time_minutes, minutes);
	if(get_str_len(minutes) == 1) {
		char temp = minutes[0];
		minutes[0] = '0';
		minutes[1] = temp;
	}
	
	char hours[2];
	int_to_str(rtc_time.time_hours, hours);
	if(get_str_len(hours) == 1) {
		char temp = hours[0];
		hours[0] = '0';
		hours[1] = temp;
	}

	char time_format[] = "(__:__:__)";

	str_set(time_format, seconds, 2, 7);
	str_set(time_format, minutes, 2, 4);
	str_set(time_format, hours, 2, 1);

	usart_send(internal_state.usart_instance, (uint8_t*)"\r\n", 2);
	usart_send(internal_state.usart_instance, (uint8_t*)time_format, 10);

	switch(subsys) {
		case LOG_SUBSYS_SSD1309:
			usart_send(internal_state.usart_instance, (uint8_t*)"SSD1309-> ", 10);
            break;

		case LOG_SUBSYS_GTU7:
			usart_send(internal_state.usart_instance, (uint8_t*)"GTU7-> ", 7);
            break;

		case LOG_SUBSYS_CMD:
			usart_send(internal_state.usart_instance, (uint8_t*)"CMD-> ", 6);
			break;

        default:
			return ERR_UNKNOWN;
            break;
    }

	return ERR_OK;
}

 /** @} */