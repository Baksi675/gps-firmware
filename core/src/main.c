#include <stdint.h>
#include "stm32f401re_gpio.h"
#include "stm32f401re_rtc.h"
#include "log.h"
#include "console.h"

static void init_all(void);

#define LED_PIN_NO 5

#define RCC_AHB1ENR 	(*(volatile uint32_t*)0x40023830)
#define GPIOA_MODER		(*(volatile uint32_t*)0x40020000)
#define GPIOA_ODR		(*(volatile uint32_t*)0x40020014)

void led_init(void) {
	// Enable GPIOA peripheral
	RCC_AHB1ENR |= 0b1 << 0;

	// Clear and set to output mode
	GPIOA_MODER &= ~(0b11 << LED_PIN_NO);
	GPIOA_MODER |= (0b1 << (LED_PIN_NO * 2));
}

void led_toggle(void) {
	// XOR the output
	GPIOA_ODR ^= (0b1 << LED_PIN_NO);
}

uint8_t count = 0;

/*
int main(void) {
	led_init();

	while(1) {
		if(count < 10) {
			led_toggle();
			for(uint32_t i = 0; i < 200000; i++);
			count++;
		}
	
	}

	return 0;
}*/

int main(void) {
	init_all();

	ERR_te err;
	err = log_print(LOG_SUBSYS_CMD, LOG_LEVEL_INFO, LOG_LEVEL_INFO, 
			"test");
		err = log_print(LOG_SUBSYS_CMD, LOG_LEVEL_INFO, LOG_LEVEL_INFO, 
			"test");
				err = log_print(LOG_SUBSYS_CMD, LOG_LEVEL_INFO, LOG_LEVEL_INFO, 
			"test");
				err = log_print(LOG_SUBSYS_CMD, LOG_LEVEL_INFO, LOG_LEVEL_INFO, 
			"test");
				err = log_print(LOG_SUBSYS_CMD, LOG_LEVEL_INFO, LOG_LEVEL_INFO, 
			"test");
	if(err != ERR_OK) {
		return err;
	}

	while(1) {
		console_run();
	
	}

	return 0;
}

static void init_all(void) {
	rtc_init();

	CALENDAR_ts rtc_calendar;
	rtc_calendar.calendar_date = 19; 
	rtc_calendar.calendar_months = MONTHS_SEPTEMBER;
	rtc_calendar.calendar_week_days = WEEK_DAYS_FRIDAY;
	rtc_calendar.calendar_year = 2025;
	rtc_set_calendar(&rtc_calendar);

	TIME_ts rtc_time;
	rtc_time.time_hours = 9;
	rtc_time.time_minutes = 4;
	rtc_time.time_seconds = 0;
	rtc_set_time(&rtc_time);

	LOG_HANDLE_ts log_handle = { 0 };
	log_handle.usart_instance = USART1;
	log_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	log_handle.gpio_port = GPIOA;
	log_handle.gpio_pin = GPIO_PIN_9;
	log_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	log_init(&log_handle);

	CONSOLE_HANDLE_ts console_handle = { 0 };
	console_handle.usart_instance = USART1;
	console_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	console_handle.gpio_port = GPIOA;
	console_handle.gpio_pin = GPIO_PIN_10;
	console_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	console_init(&console_handle);
}