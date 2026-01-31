#include <stdbool.h>
#include <stdint.h>
#include "common.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_rtc.h"
#include "log.h"
#include "console.h"
#include "io.h"
#include "ssd1309.h"

static void init_all(void);

#define LED_PIN_NO 5

#define RCC_AHB1ENR 	(*(volatile uint32_t*)0x40023830)
#define GPIOA_MODER		(*(volatile uint32_t*)0x40020000)
#define GPIOA_ODR		(*(volatile uint32_t*)0x40020014)

IO_HANDLE_ts *io_handle;
IO_HANDLE_ts *io_handle1;
SSD1309_HANDLE_ts *ssd1309_handle;

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

uint32_t btn_count = 0;
bool processed = false;

int main(void) {
	init_all();

	while(1) {
		console_run();
	
	}

	return 0;
}

static void init_all(void) {
	GPIO_HANDLE_ts btn = { 0 };
	btn.mode = GPIO_MODE_INPUT;
	btn.port = GPIOC;
	btn.pin = GPIO_PIN_13;
	gpio_init(&btn);

	rtc_init();

	CALENDAR_ts rtc_calendar;
	rtc_calendar.calendar_date = 29; 
	rtc_calendar.calendar_months = MONTHS_JANUARY;
	rtc_calendar.calendar_week_days = WEEK_DAYS_THURSDAY;
	rtc_calendar.calendar_year = 2026;
	rtc_set_calendar(&rtc_calendar);

	TIME_ts rtc_time;
	rtc_time.time_hours = 0;
	rtc_time.time_minutes = 0;
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

	GPIO_HANDLE_ts io_gpio = { 0 };
	io_gpio.mode = GPIO_MODE_OUTPUT;
	io_gpio.pull_mode = GPIO_PULL_MODE_NOPUPD;
	io_gpio.pin = GPIO_PIN_5;
	io_gpio.port = GPIOA;

	IO_CONFIG_ts io_config = { 0 };
	io_config.gpio_handle = &io_gpio;
	str_cpy(io_config.name, "led", get_str_len("led") + 1);

	GPIO_HANDLE_ts io_gpio1 = { 0 };
	io_gpio1.mode = GPIO_MODE_OUTPUT;
	io_gpio1.pull_mode = GPIO_PULL_MODE_NOPUPD;
	io_gpio1.pin = GPIO_PIN_7;
	io_gpio1.port = GPIOC;

	IO_CONFIG_ts io_config1 = { 0 };
	io_config1.gpio_handle = &io_gpio1;
	str_cpy(io_config1.name, "ledr", get_str_len("ledr") + 1);

	io_init_subsys();
	io_init_handle(&io_config, &io_handle);
	io_init_handle(&io_config1, &io_handle1);
	io_start_subsys();

	SSD1309_CONFIG_ts ssd1309_conf = { 0 };
	ssd1309_get_def_conf(&ssd1309_conf);
	ssd1309_conf.i2c_instance = I2C1;
	ssd1309_conf.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF4;
	ssd1309_conf.scl_gpio_pin = GPIO_PIN_8;
	ssd1309_conf.scl_gpio_port = GPIOB;
	ssd1309_conf.sda_gpio_pin = GPIO_PIN_9;
	ssd1309_conf.sda_gpio_port = GPIOB;

	ssd1309_init_subsys();
	ssd1309_init_handle(&ssd1309_conf, &ssd1309_handle);
	ssd1309_start_subsys();
}