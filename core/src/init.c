/**
 * @file init.c
 * @author github.com/Baksi675
 * @brief Common initialization implementation file.
 * @version 0.1
 * @date 2026-03-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "init.h"
#include "err.h"
#include "log.h"
#include "arm_cortex_m4_systick.h"
#include "neo6.h"
#include "stm32f401re_rtc.h"

/**
 * @defgroup init_public_apis Initialization Public APIs
 * @{
 */

/** @brief Initializes the logging subsystem. @see init_log */
ERR_te init_log(void) {
	ERR_te err;
	
	LOG_HANDLE_ts log_handle = { 0 };
	log_handle.usart_instance = USART1;
	log_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	log_handle.gpio_port = GPIOA;
	log_handle.gpio_pin = GPIO_PIN_9;
	log_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	err = log_init(&log_handle);

	return err;
}

/** @brief Initializes the SysTick timer. @see init_systick */
ERR_te init_systick(void) {
	ERR_te err;

	SYSTICK_CFG_ts systick_cfg = { 0 };
	systick_cfg.clk_source = SYSTICK_CLK_SOURCE_PROCESSOR;
	systick_cfg.interrupt = SYSTICK_IT_TRUE;
	err = systick_init(&systick_cfg);

	return err;
}

/** @brief Initializes the NEO-6 GPS module. @see init_neo6 */
ERR_te init_neo6(void) {
	ERR_te err;
	NEO6_HANDLE_ts *neo6_handle = { 0 }; 
	
	NEO6_CFG_ts neo6_cfg = { 0 };
	neo6_cfg.usart_instance = USART6;
	neo6_cfg.usart_baud_rate = 9600;
	neo6_cfg.rx_gpio_port = GPIOA;
	neo6_cfg.rx_gpio_pin = GPIO_PIN_12;
	neo6_cfg.tx_gpio_port = GPIOA;
	neo6_cfg.tx_gpio_pin = GPIO_PIN_11;
	neo6_cfg.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF8;
	err = neo6_init_subsys();
	err = neo6_init_handle(&neo6_cfg, &neo6_handle);
	err = neo6_start_subsys();

	return err;
}

/** @brief Initializes the RTC peripheral and sets a default calendar and time. @see init_rtc */
ERR_te init_rtc(void) {
	ERR_te err;
	
	err = rtc_init();

	if(err == ERR_MODULE_ALREADY_INITIALIZED) {
		return err;
	}

	CALENDAR_ts rtc_calendar;
	rtc_calendar.date = 29; 
	rtc_calendar.months = MONTHS_JANUARY;
	rtc_calendar.week_days = WEEK_DAYS_THURSDAY;
	rtc_calendar.year = 2026;
	rtc_set_calendar(&rtc_calendar);

	TIME_ts rtc_time;
	rtc_time.hours = 0;
	rtc_time.minutes = 0;
	rtc_time.seconds = 0;
	rtc_set_time(&rtc_time);

	return ERR_OK;
}

/** @} */