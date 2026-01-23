/**
 * @file stm32f401re_usart_driver.c
 * @author github.com/Baksi675
 * @brief USART driver implementation for STM32F401RE
 * @version 0.1
 * @date 2025-09-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "stm32f401re_usart.h"
#include "common.h"
#include "arm_cortex_m4_nvic.h"
#include "stm32f401re.h"

static void usart_set_pclk(USART_REGDEF_ts *instance, EN_STATUS_te en_status);
static void usart_set_baud_rate(USART_HANDLE_ts *usart_object);

/** 
 * @defgroup USART_Public_APIs USART Public APIs
 * @{
 */

/**
 * @brief Initializes the USART peripheral. If no configuration is given in the application layer, then the USART is initialized with the following defaults:
 * - 8 bit data frame length
 * - 1 stop bit in frame
 * - 1200 baud rate
 * - Parity bit DISABLE in frame
 * - Oversampling by 16
 * - HW flow control DISABLE
 * - Three sample bit method
 * - Interrupts DISABLE
 *
 * @param usart_object USART configuration object.
 */
void usart_init(USART_HANDLE_ts *usart_object) {
	// Enable peripheral clock
	usart_set_pclk(usart_object->instance, ENABLE);

	// Define frame data length (8 or 9 bit)
	usart_object->instance->USART_CR1 &= ~(0x1 << USART_CR1_M);
	usart_object->instance->USART_CR1 |= (usart_object->frame_data_bits << USART_CR1_M);

	// Define frame stop bit count (0.5, 1, 1.5 or 2)
	usart_object->instance->USART_CR2 &= ~(0x3 << USART_CR2_STOP);
	usart_object->instance->USART_CR2 |= (usart_object->frame_stop_bits << USART_CR2_STOP);

	// Set baud rate
	usart_set_baud_rate(usart_object);

	// Set oversampling
	usart_object->instance->USART_CR1 &= ~(0x1 << USART_CR1_OVER8);
	usart_object->instance->USART_CR1 |= (usart_object->oversampling << USART_CR1_OVER8);

	// Set parity
	if(usart_object->parity != USART_PARITY_DISABLED) {
		usart_object->instance->USART_CR1 |= (0x1 << USART_CR1_PCE);

		if(usart_object->parity == USART_PARITY_EVEN) {
			usart_object->instance->USART_CR1 &= ~(0x1 << USART_CR1_PS);
		}
		else if(usart_object->parity == USART_PARITY_ODD) {
			usart_object->instance->USART_CR1 |= (0x1 << USART_CR1_PS);
		}
	}

	// Set HW flow control
	if(usart_object->hw_flow_control == USART_HW_FLOW_CONTROL_ENABLED) {
		usart_object->instance->USART_CR3 |= (0x1 << USART_CR3_CTSE);

		usart_object->instance->USART_CR3 |= (0x1 << USART_CR3_RTSE);
	}

	// Set sample bit number
	usart_object->instance->USART_CR3 &= ~(0x1 << USART_CR3_ONEBIT);
	usart_object->instance->USART_CR3 |= (usart_object->sample_bit << USART_CR3_ONEBIT);

	if(usart_object->mode == USART_MODE_SYNC) {
		// LBCL, CPHA, CPOL, CLKEN --> YET TO BE IMPLEMENTED
	}

	// Enable interrupts (only RXNE as of yet, can be extended)
	if(usart_object->interrupt_en == USART_INTERRUPT_EN_TRUE) {
		usart_object->instance->USART_CR1 |= (0x1 << USART_CR1_RXNEIE);

		if(usart_object->instance == USART1) {
			nvic_set_interrupt(USART1_IRQn, ENABLE);
		}
		else if(usart_object->instance == USART2) {
			nvic_set_interrupt(USART2_IRQn, ENABLE);
		}
		else if(usart_object->instance == USART6) {
			nvic_set_interrupt(USART6_IRQn, ENABLE);
		}
	}

	// Enable the peripheral
	usart_object->instance->USART_CR1 |= (0x1 << USART_CR1_UE);
}

/**
 * @brief Deinitializes the given USART peripheral by setting its registers back to their reset values. It also turns of the peripherals clock.
 * 
 * @param instance The USART instance to deinitialize.
 */
void usart_deinit(USART_REGDEF_ts *instance) {
	if(instance == USART1) {
		rcc_reset_periph_apb2(RCC_APB2RSTR_USART1RST);
	}
	else if(instance == USART2) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_USART2RST);
	}
	else if(instance == USART6) {
		rcc_reset_periph_apb2(RCC_APB2RSTR_USART6RST);
	}

	nvic_set_interrupt(USART1_IRQn, DISABLE);
	nvic_set_interrupt(USART2_IRQn, DISABLE);
	nvic_set_interrupt(USART6_IRQn, DISABLE);

	usart_set_pclk(instance, DISABLE);
}

/**
 * @brief A blocking USART send function. Sends data, blocks until it's completed.
 *
 * Comments about interface:
 * - Before calling this function the USART transmission must be ENABLE with \ref usart_set_transmission if it hasn't been done so.
 *
 * @param instance The USART instance on which to send data.
 * @param tx_buffer A pointer to the buffer containing the data to be sent.
 * @param len The length of the buffer to be sent.
 */
void usart_send(USART_REGDEF_ts *instance, uint8_t *tx_buffer, uint32_t len) {
	while(len != 0) {
		while(!((instance->USART_SR >> USART_SR_TXE) & 0x1));
		instance->USART_DR = *tx_buffer;
		tx_buffer++;
		len--;
	}
	while(!((instance->USART_SR >> USART_SR_TC) & 0x1));
}

/**
 * @brief A blocking USART master receive function. Receives data, blocks until completed.
 * 
 * @param instance The USART instance on which to receive data.
 * @param rx_buffer A pointer to the buffer that will store the received data.
 * @param len The length of the data to be received.
 */
void usart_receive(USART_REGDEF_ts *instance, uint8_t *rx_buffer, uint32_t len) {
	while(len != 0) {
		while(!((instance->USART_SR >> USART_SR_RXNE) & 0x1));
		*rx_buffer = instance->USART_DR;
		rx_buffer++;
		len--;
	}
}

/**
 * @brief Enables or disables the peripheral transmission. On disabling the transmission the MCU consumes less power.
 * 
 * @param instance The USART instance to be ENABLE or DISABLE.
 * @param en_status Whether to enable or disable the transmission.
 */
void usart_set_transmission(USART_REGDEF_ts *instance, EN_STATUS_te en_status) {
	instance->USART_CR1 &= ~(0x1 << USART_CR1_TE);
	instance->USART_CR1 |= (en_status << USART_CR1_TE);
}

/**
 * @brief Enables or disables the peripheral reception. On disabling the reception the MCU consumes less power.
 * 
 * @param instance The USART instance to be ENABLE or DISABLE.
 * @param en_status Whether to enable or disable the reception.
 */
void usart_set_reception(USART_REGDEF_ts *instance, EN_STATUS_te en_status) {
	instance->USART_CR1 &= ~(0x1 << USART_CR1_RE);
	instance->USART_CR1 |= (en_status << USART_CR1_RE);
}

/**
 * @brief Returns the name in strings of the USART peripheral.
 * 
 * @param instance The USART peripheral.
 * @param name A pointer to a buffer which will contain the name.
 */
void usart_get_name(USART_REGDEF_ts *instance, char *name) {
	char usart[] = "USART";
	uint8_t usart_len = get_str_len(usart);
	uint8_t pos_counter = 0;

	while(pos_counter != usart_len) {
		name[pos_counter] = usart[pos_counter];
		pos_counter++;
	}
	
	if(instance == USART1) {
		name[pos_counter] = '1';
	}
	else if(instance == USART2) {
		name[pos_counter] = '2';
	}
	else if(instance == USART6) {
		name[pos_counter] = '6';
	}
	pos_counter++;

	name[pos_counter] = '\0';
}

/** @} */

/** 
 * @defgroup USART_Internal_Helper USART Internal Helpers
 * @{
 */

/**
 * @brief Enable or disable the peripheral clock of the given USART instance.
 * 
 * @param instance The USART instance.
 * @param en_status Whether to enable or disable the peripheral clock.
 */
static void usart_set_pclk(USART_REGDEF_ts *instance, EN_STATUS_te en_status) {
	if(instance == USART1) {
		rcc_set_pclk_apb2(RCC_APB2ENR_USART1EN, en_status);
	}
	else if(instance == USART2) {
		rcc_set_pclk_apb1(RCC_APB1ENR_USART2EN, en_status);
	}
	else if(instance == USART6) {
		rcc_set_pclk_apb2(RCC_APB2ENR_USART6EN, en_status);
	}
}

/**
 * @brief Sets the baud rate of the USART peripheral.
 * 
 * @param usart_object USART configuration object.
 */
static void usart_set_baud_rate(USART_HANDLE_ts *usart_object) {
	// Set baud rate
	usart_object->instance->USART_BRR &= ~(0xFFFF);
	uint32_t usart_pclk = 0;
	uint32_t temp_reg = 0;
	uint32_t usartdiv = 0;
	uint32_t usartdiv_mantissa;
	uint32_t usartdiv_fraction;

	if(usart_object->instance == USART1 || usart_object->instance == USART6) {
		usart_pclk = rcc_get_apb2_clk();
	}
	else if(usart_object->instance == USART2) {
		usart_pclk = rcc_get_apb1_clk();
	}

	if(usart_object->oversampling == USART_OVERSAMPLING_8) {
		usartdiv = ((25 * usart_pclk) / (2 *  usart_object->baud_rate));
	}
	else if(usart_object->oversampling == USART_OVERSAMPLING_16) {
		usartdiv = ((25 * usart_pclk) / (4 *  usart_object->baud_rate));

	}
	usartdiv_mantissa = usartdiv / 100;
	
	temp_reg |= usartdiv_mantissa << 4;

	usartdiv_fraction = (usartdiv - (usartdiv_mantissa * 100));

	if(usart_object->oversampling == USART_OVERSAMPLING_8) {
		usartdiv_fraction = (((usartdiv_fraction * 8) + 50) / 100) & ((uint8_t)0x07);

	}
	else if(usart_object->oversampling == USART_OVERSAMPLING_16) {
		usartdiv_fraction = (((usartdiv_fraction * 16) + 50) / 100) & ((uint8_t)0x0F);
	}

	temp_reg |= usartdiv_fraction;

	usart_object->instance->USART_BRR = temp_reg;
}

/** @} */

void USART1_IRQHandler(void) {
	// Only RXNE interrupt implementation as of yet (can be extended)
	if((USART1->USART_SR >> USART_SR_RXNE) & 0x1) {
		uint8_t data = USART1->USART_DR;
		usart1_irq_data_recv_callback(data);
	}
} 

void USART2_IRQHandler(void) {
	
} 

void USART6_IRQHandler(void) {
	if((USART6->USART_SR >> USART_SR_RXNE) & 0x1) {
		uint8_t data = USART6->USART_DR;
		usart6_irq_data_recv_callback(data);
	}
} 

void usart1_irq_data_recv_callback(uint8_t data) __attribute__((weak, alias("usart_irq_callback")));
void usart6_irq_data_recv_callback(uint8_t data) __attribute__((weak, alias("usart_irq_callback")));

void usart_irq_callback() {
	while(1);
}