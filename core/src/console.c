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
#include "init.h"

/**
 * @brief ASCII control code used to enter and exit console mode (Ctrl+C).
 */
#define CONSOLE_MODE_CMD    3

/**
 * @brief Backing memory for the USART receive circular buffer.
 *
 * @details
 * Written by the USART ISR callback and drained by @ref console_run.
 * Size is defined by @ref CONFIG_CONSOLE_USART_CBUF_SIZE and must be
 * a power of two.
 */
static uint8_t usart_cbuf_mem[CONFIG_CONSOLE_USART_CBUF_SIZE];

/**
 * @brief Backing memory for the console command accumulation buffer.
 *
 * @details
 * Accumulates echoed characters between console mode entry and the
 * carriage return that triggers command execution.
 * Size is defined by @ref CONFIG_CONSOLE_CBUF_SIZE.
 */
static uint8_t console_cbuf_mem[CONFIG_CONSOLE_CBUF_SIZE];

/**
 * @brief Internal state of the console subsystem.
 *
 * @details
 * Holds the two circular buffers, a pointer to the active USART instance,
 * and the current console mode flag. Statically initialized at startup.
 */
struct internal_state_s {
    /** Circular buffer used to receive raw bytes from the USART ISR. */
    CBUF_HANDLE_ts usart_data_recv_cbuf;

    /** Circular buffer used to accumulate a command string in console mode. */
    CBUF_HANDLE_ts console_cbuf;

    /** Pointer to the active USART peripheral instance. */
    USART_REGDEF_ts *usart_instance;

    /** True when the console is in interactive input mode. */
    bool console_mode;
};

/** @brief Singleton instance of the console subsystem internal state. */
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
 * @defgroup console_public_apis Console Public APIs
 * @{
 */

/** @brief Initializes the console subsystem. @see console_init */
ERR_te console_init(CONSOLE_HANDLE_ts *console_handle) {
    if(is_pow(CONFIG_CONSOLE_USART_CBUF_SIZE) == false) {
        return ERR_INVALID_CONFIGURATION;
    }

    internal_state.usart_instance = console_handle->usart_instance;

    init_log();

    GPIO_CFG_ts console_rx_pin = { 0 };
    console_rx_pin.port = console_handle->gpio_port;
    console_rx_pin.pin = console_handle->gpio_pin;
    console_rx_pin.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    console_rx_pin.alternate_function = console_handle->gpio_alternate_function;
    gpio_init(&console_rx_pin);

    USART_CFG_ts console_usart = { 0 };
    console_usart.instance = console_handle->usart_instance;
    console_usart.baud_rate = console_handle->usart_baud_rate;
    console_usart.interrupt_en = USART_INTERRUPT_EN_TRUE;
    usart_init(&console_usart);

    usart_set_reception(internal_state.usart_instance, ENABLE);

    return ERR_OK;
}

/** @brief Runs the console state machine. @see console_run */
ERR_te console_run(void) {
    ERR_te err;
    uint8_t data_len = 0;

    err = cbuf_len(&internal_state.usart_data_recv_cbuf, &data_len);
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
            log_set_force_disable(true);
            usart_send(internal_state.usart_instance, (uint8_t*)"$", 1);
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
 * @brief USART RXNE interrupt callback. Writes the received byte into the USART receive buffer.
 *
 * @details
 * Called automatically by the USART1 interrupt handler on each received byte.
 * Writes @p data directly into the internal USART circular buffer for later
 * processing by @ref console_run.
 *
 * @note This function must not be called directly from application code.
 *
 * @param[in] data The byte received from the USART RX register.
 */
void usart1_irq_data_recv_callback(uint8_t data) {
    cbuf_write(&internal_state.usart_data_recv_cbuf, &data, 1);
}