/**
 * @file button.h
 * @author github.com/Baksi675
 * @brief Button module header file.
 * @version 0.1
 * @date 2026-01-31
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

#include "io.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "err.h"

/*
typedef enum {
	BUTTON_DEBOUNCE_TIME_INF,			// Button debounce property is disabled
	BUTTON_DEBOUNCE_TIME_SHORT = 500,
	BUTTON_DEBOUNCE_TIME_MID = 2500,
	BUTTON_DEBOUNCE_TIME_LONG = 5000
}BUTTON_DEBOUNCE_TIME_te;

typedef enum {
	BUTTON_HELD_TIME_INF,				// Button held property is disabled
	BUTTON_HELD_TIME_SHORT = 50000,
	BUTTON_HELD_TIME_MID = 100000,
	BUTTON_HELD_TIME_LONG = 200000
}BUTTON_HELD_TIME_te;
*/
typedef enum {
	BUTTON_PUSHED_TYPE_LOW,
	BUTTON_PUSHED_TYPE_HIGH
}BUTTON_PUSHED_TYPE_te;

typedef struct {
	char name[CONFIG_BUTTON_MAX_NAME_LEN];
	GPIO_REGDEF_ts *gpio_port;
	GPIO_PIN_te gpio_pin;
	BUTTON_PUSHED_TYPE_te pushed_type;
	uint32_t debounce_limit_ms;
	uint32_t held_limit_ms;
	//BUTTON_DEBOUNCE_TIME_te debounce_time;
	//BUTTON_HELD_TIME_te held_time;
}BUTTON_CFG_ts;

typedef struct button_handle_s BUTTON_HANDLE_ts;

ERR_te button_init_subsys(void);
ERR_te button_deinit_subsys(void);
ERR_te button_start_subsys(void);
ERR_te button_stop_subsys(void);
// Get default configuration API not needed
ERR_te button_init_handle(BUTTON_CFG_ts *button_cfg, BUTTON_HANDLE_ts **button_handle);
ERR_te button_deinit_handle(BUTTON_HANDLE_ts *button_handle);
ERR_te button_run_handle(BUTTON_HANDLE_ts *button_handle);
ERR_te button_run_handle_all(void);
ERR_te button_get_pushed_state(BUTTON_HANDLE_ts *button_handle, bool *pushed_state_o);
ERR_te button_get_held_state(BUTTON_HANDLE_ts *button_handle, bool *held_state_o);

#endif