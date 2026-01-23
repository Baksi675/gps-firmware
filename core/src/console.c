/**
 * @file console.c
 * @author github.com/Baksi675
 * @brief Console module implementation
 * @version 0.1
 * @date 2025-10-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>

#include "console.h"
#include "cbuf.h"
#include "common.h"
#include "err.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "cmd.h"
#include "configuration.h"
#include "log.h"

#define CONSOLE_MODE_CMD		3		// Ctrl + C

static uint8_t usart_cbuf_mem[CONFIG_CONSOLE_USART_CBUF_SIZE];
static uint8_t console_cbuf_mem[CONFIG_CONSOLE_CBUF_SIZE];

struct internal_state_s {
	CBUF_HANDLE_ts usart_data_recv_cbuf;
	CBUF_HANDLE_ts console_cbuf;
	USART_REGDEF_ts *usart_instance;
	bool console_mode;
};

static struct internal_state_s internal_state = {
	.usart_data_recv_cbuf.ptr = usart_cbuf_mem,
	.usart_data_recv_cbuf.size = sizeof(usart_cbuf_mem),
	.usart_data_recv_cbuf.read_position = 0,
	.usart_data_recv_cbuf.write_position = 0,
	.console_cbuf.ptr = console_cbuf_mem,
	.console_cbuf.size = sizeof(console_cbuf_mem),
	.console_cbuf.read_position = 0,
	.console_cbuf.write_position = 0,
	.usart_instance = 0,
	.console_mode = false,
};

 /** 
 * @defgroup CONSOLE_Public_APIs CONSOLE Public APIs
 * @{
 */

/**
 * @brief Initializes the console software module. Configures the internal state, initializes the GPIO pins and the USART peripheral.
 * 
 * @param console_handle A console configuration object.
 * @return ERR_te Error code generated during execution.
 */
ERR_te console_init(CONSOLE_HANDLE_ts *console_handle) {
	if(is_pow(CONFIG_CONSOLE_USART_CBUF_SIZE) == false) {
		return ERR_INVALID_CONFIGURATION;
	}

	internal_state.usart_instance = console_handle->usart_instance;

	GPIO_HANDLE_ts console_rx_pin = { 0 };
	console_rx_pin.port = console_handle->gpio_port;
	console_rx_pin.pin = console_handle->gpio_pin;
	console_rx_pin.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	console_rx_pin.alternate_function = console_handle->gpio_alternate_function;
	gpio_init(&console_rx_pin);

	USART_HANDLE_ts console_usart = { 0 };
	console_usart.instance = console_handle->usart_instance;
	console_usart.baud_rate = console_handle->usart_baud_rate;
	console_usart.interrupt_en = USART_INTERRUPT_EN_TRUE;
	usart_init(&console_usart);

	usart_set_reception(internal_state.usart_instance, ENABLE);

	return ERR_OK;
}

/**
 * @brief Runs the console module as part of the state machine.
 * - Inserts data from the USART circular buffer into the module circular buffer
 * - Handles console mode entering and leaving (Ctrl + C)
 * - Handles command parsing
 * 
 * @return ERR_te Error code generated during execution.
 */
ERR_te console_run(void) {
	ERR_te err;
	uint8_t data_len = 0;

	err = cbuf_len(&internal_state.usart_data_recv_cbuf,  &data_len);
	if(err != ERR_OK) {
		return err;
	}

	if(data_len) {
		uint8_t data[data_len];
		
		err = cbuf_read(&internal_state.usart_data_recv_cbuf, data);
		if(err != ERR_OK) {
			return err;
		}

		if(*data == CONSOLE_MODE_CMD && internal_state.console_mode == false) {
			internal_state.console_mode = true;
			log_set_force_disable(false);
			usart_send(internal_state.usart_instance, (uint8_t*)"\r\n$", 3);
		}
		else if(*data == CONSOLE_MODE_CMD && internal_state.console_mode == true) {
			internal_state.console_mode = false;
			log_set_force_disable(false);
			
			uint8_t console_text_len = 0;
			err = cbuf_len(&internal_state.console_cbuf, &console_text_len);
			if(err != ERR_OK) {
				return err;
			}

			for(uint8_t i = 0; i < console_text_len + 1; i++) {
				usart_send(internal_state.usart_instance, (uint8_t*)"\x7F", 1);
			}
		}

		if(*data != CONSOLE_MODE_CMD && internal_state.console_mode == true) {
			usart_send(internal_state.usart_instance, data, data_len);
			
			err = cbuf_write(&internal_state.console_cbuf, data, data_len);
			if(err != ERR_OK) {
				return err;
			}
			
			for(uint32_t i = 0; i < data_len; i++) {
				if(data[i] == '\r') {
					log_set_force_disable(false);
					
					uint8_t console_text_len = 0;
					err = cbuf_len(&internal_state.console_cbuf, &console_text_len);
					if(err != ERR_OK) {
						return err;
					}

					char console_text[console_text_len];

					err = cbuf_read(&internal_state.console_cbuf, (uint8_t*)console_text);
					if(err != ERR_OK) {
						return err;
					}
					
					console_text[console_text_len - 1] = '\0';

					uint8_t backspace_counter = 0;

					for(int16_t j = console_text_len - 1; j >= 0; j--) {
						while(console_text[j] == 127) {
							backspace_counter++;
							console_text[j] = 0;
							j--;
						}

						while(backspace_counter) {
							console_text[j] = 0;
							backspace_counter--;
							j--;
						}
					}

					arr_cmprs((char*)console_text, console_text_len);
					internal_state.console_mode = false;
					usart_send(internal_state.usart_instance, (uint8_t*)"\r\n", 2);
					cmd_execute(console_text);
				}
			}
		}
	}
	
	return ERR_OK;
}

/** @} */

/**
 * @brief Handles USART data receptions by loading the data into a circular buffer. Gets called by the USART RXNE event.
 * 
 * @param data The data received in the USART RX buffer.
 */
void usart1_irq_data_recv_callback(uint8_t data) {
	cbuf_write(&internal_state.usart_data_recv_cbuf, &data, 1);
}


