/**
 * @file console.h
 * @author github.com/Baksi675
 * @brief Header file for the console module
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef CONSOLE_H__
#define CONSOLE_H__

#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "err.h"

typedef struct {
	USART_REGDEF_ts *usart_instance;
	USART_BAUD_RATE_te usart_baud_rate;
	GPIO_REGDEF_ts *gpio_port;
	GPIO_PIN_te gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
}CONSOLE_HANDLE_ts;

ERR_te console_init(CONSOLE_HANDLE_ts *console_handle);
ERR_te console_run(void);

#endif