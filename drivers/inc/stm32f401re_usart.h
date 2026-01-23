/**
 * @file stm32f401re_usart.h
 * @author github.com/Baksi675
 * @brief USART driver header file for STM32F401RE MCU
 * @version 0.1
 * @date 2026-01-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef STM32F401RE_USART_H__
#define STM32F401RE_USART_H__

#include "stm32f401re.h"
#include "common.h"
#include "stm32f401re_rcc.h"

/**
 * @brief Defines the length of a USART peripheral
 * 
 */
#define USART_NAME_LEN			6

/**
 * @brief Defines the mode of the USART peripheral.
 * 
 */
typedef enum {
	USART_MODE_ASYNC,
	USART_MODE_SYNC
}USART_MODE_te;

/**
 * @brief Defines the speed of the USART peripheral.
 * 
 */
typedef enum {
	USART_BAUD_RATE_1200 = 1200,
	USART_BAUD_RATE_2400 = 2400,
	USART_BAUD_RATE_9600 = 9600,
	USART_BAUD_RATE_19200 = 19200,
	USART_BAUD_RATE_38400 = 38400,
	USART_BAUD_RATE_57600 = 57600,
	USART_BAUD_RATE_115200 = 115200,
	USART_BAUD_RATE_230400 = 230400,
	USART_BAUD_RATE_460800 = 460800,
	USART_BAUD_RATE_921600 = 921600,
	USART_BAUD_RATE_2000000 = 2000000,
	USART_BAUD_RATE_3000000 = 3000000
}USART_BAUD_RATE_te;

/**
 * @brief Defines the parity bit possibilities of a USART message.
 * 
 */
typedef enum {
	USART_PARITY_DISABLED, 
	USART_PARITY_EVEN,
	USART_PARITY_ODD,
}USART_PARITY_te;

/**
 * @brief Defines the possible data bit numbers in a USART message.
 * 
 */
typedef enum {
	USART_FRAME_DATA_BITS_8,
	USART_FRAME_DATA_BITS_9
}USART_FRAME_DATA_BITS_te;

/**
 * @brief Defines the possible oversamplings in a USART communication.
 * 
 */
typedef enum {
	USART_OVERSAMPLING_16,
	USART_OVERSAMPLING_8
}USART_OVERSAMPLING_te;

/**
 * @brief Defines the number of STOP bits in a USART message.
 * 
 */
typedef enum {
	USART_FRAME_STOP_BITS_1,
	USART_FRAME_STOP_BITS_0_5,
	USART_FRAME_STOP_BITS_2,
	USART_FRAME_STOP_BITS_1_5
}USART_FRAME_STOP_BITS_te;

/**
 * @brief Defines the possibilities of HW flow control in a USART communication.
 * 
 */
typedef enum {
	USART_HW_FLOW_CONTROL_DISABLED,
	USART_HW_FLOW_CONTROL_ENABLED
}USART_HW_FLOW_CONTROL_te;

/**
 * @brief Defines the number of sample bits in a USART communication.
 * 
 */
typedef enum {
	USART_SAMPLE_BIT_3,
	USART_SAMPLE_BIT_1
}USART_SAMPLE_BIT_te;

/**
 * @brief Defines the possibilities of an interrupt inUSART message handling.
 * 
 */
typedef enum {
	USART_INTERRUPT_EN_FALSE,
	USART_INTERRUPT_EN_TRUE
}USART_INTERRUPT_EN_te;

/**
 * @brief Defines a USART object with the possible configurations.
 * 
 */
typedef struct {
	USART_REGDEF_ts *instance;
	USART_MODE_te mode;
	USART_BAUD_RATE_te baud_rate;
	USART_PARITY_te parity;
	USART_FRAME_DATA_BITS_te frame_data_bits;
	USART_OVERSAMPLING_te oversampling;
	USART_FRAME_STOP_BITS_te frame_stop_bits;
	USART_HW_FLOW_CONTROL_te hw_flow_control;
	USART_SAMPLE_BIT_te sample_bit;
	USART_INTERRUPT_EN_te interrupt_en;
}USART_HANDLE_ts;

void usart_init(USART_HANDLE_ts *usart_object);
void usart_deinit(USART_REGDEF_ts *usart_instance);
void usart_send(USART_REGDEF_ts *usart_instance, uint8_t *tx_buffer, uint32_t len);
void usart_receive(USART_REGDEF_ts *usart_instance, uint8_t *rx_buffer, uint32_t len);
void usart_set_transmission(USART_REGDEF_ts *usart_instance, EN_STATUS_te en_status);
void usart_set_reception(USART_REGDEF_ts *usart_instance, EN_STATUS_te en_status);
void usart_get_name(USART_REGDEF_ts *usart_instance, char *name);
void usart1_irq_data_recv_callback(uint8_t data);
void usart6_irq_data_recv_callback(uint8_t data);

#endif