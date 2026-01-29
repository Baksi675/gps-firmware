/**
 * @file stm32f401re_gpio.c
 * @author github.com/Baksi675
 * @brief GPIO driver implementation for STM32F401RE.
 * @version 0.1
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "stm32f401re_gpio.h"
#include "common.h"
#include "stm32f401re.h"
#include "stm32f401re_rcc.h"
#include "arm_cortex_m4_nvic.h"

static void gpio_set_pclk(GPIO_REGDEF_ts const *gpio_port, EN_STATUS_te en_status);
static PORT_CODES_ts get_syscfg_code(GPIO_REGDEF_ts const *port);
static uint8_t get_exti_position(GPIO_HANDLE_ts const *gpio_handle);

/** 
 * @defgroup GPIO_Public_APIs GPIO Public APIs
 * @{
 */

 /**
  * @brief Enables the clock of the port and sets the following pin configs: mode, output type, output speed, pull-up or pull-down, alternate functionality
  * 
  * @param gpio_handle GPIO configuration object created in application layer
  */
void gpio_init(GPIO_HANDLE_ts *gpio_handle) {
    // Enable the clock for the GPIO peripheral
	gpio_set_pclk(gpio_handle->port, ENABLE);

	// Configure GPIO output type
	gpio_handle->port->GPIO_OTYPER &= ~(0b1 << gpio_handle->pin);
	gpio_handle->port->GPIO_OTYPER |= gpio_handle->output_type << gpio_handle->pin;

	// Configure GPIO speed
	gpio_handle->port->GPIO_OSPEEDR &= ~(0b11 << gpio_handle->pin * 2);
	gpio_handle->port->GPIO_OSPEEDR |= gpio_handle->output_speed << gpio_handle->pin * 2;

	// Configure GPIO pull-up pull-down
	gpio_handle->port->GPIO_PUPDR &= ~(0b11 << gpio_handle->pin * 2);
	gpio_handle->port->GPIO_PUPDR |= gpio_handle->pull_mode << gpio_handle->pin * 2;

	// Configure GPIO alternate functionality
	uint32_t afr_index = gpio_handle->pin >> 3;          // pin / 8
	uint32_t afr_shift = (gpio_handle->pin & 0x7) * 4;   // (pin % 8) * 4

	gpio_handle->port->GPIO_AFR[afr_index] &=
		~(0xFu << afr_shift);

	gpio_handle->port->GPIO_AFR[afr_index] |=
		((uint32_t)gpio_handle->alternate_function << afr_shift);

	// Configure GPIO mode
	if(gpio_handle->mode != GPIO_MODE_INTERRUPT) {
		gpio_handle->port->GPIO_MODER &= ~(0b11 << gpio_handle->pin * 2);
		gpio_handle->port->GPIO_MODER |= gpio_handle->mode << gpio_handle->pin * 2;
	}
	else {
		// Enable the peripheral clock for SYSCFG
		RCC->RCC_APB2ENR |= (0b1 << 14);

		// Set the pins 0-3 to the correct port
		if(gpio_handle->pin < 4) {
			SYSCFG->SYSCFG_EXTICR1 &= ~(0b1111 << gpio_handle->pin * 4);
			SYSCFG->SYSCFG_EXTICR1 |= (get_syscfg_code(gpio_handle->port)) << gpio_handle->pin * 4;
		}
		// Set the pins 4-7 to the correct port
		else if(gpio_handle->pin < 8) {
			SYSCFG->SYSCFG_EXTICR2 &= ~(0b1111 << (gpio_handle->pin - 4) * 4);
			SYSCFG->SYSCFG_EXTICR2 |= (get_syscfg_code(gpio_handle->port)) << (gpio_handle->pin - 4) * 4;
		}
		// Set the pins 8-11 to the correct port
		else if(gpio_handle->pin < 12) {
			SYSCFG->SYSCFG_EXTICR3 &= ~(0b1111 << (gpio_handle->pin - 8) * 4);
			SYSCFG->SYSCFG_EXTICR3 |= (get_syscfg_code(gpio_handle->port)) << (gpio_handle->pin - 8) * 4;
		}
		// Set the pins 12-15 to the correct port
		else {
			SYSCFG->SYSCFG_EXTICR4 &= ~(0b1111 << (gpio_handle->pin - 12) * 4);
			SYSCFG->SYSCFG_EXTICR4 |= (get_syscfg_code(gpio_handle->port)) << (gpio_handle->pin - 12) * 4;
		}

		// Unmask the interrupt for the GPIO pin
		EXTI->EXTI_IMR |= 0b1 << gpio_handle->pin;

		// Set the pin to trigger on the rising edge, falling edge or both
		switch(gpio_handle->interrupt_trigger) {
			case GPIO_INTERRUPT_TRIGGER_RE:
				EXTI->EXTI_RTSR |= 0b1 << gpio_handle->pin;
				break;
			case GPIO_INTERRUPT_TRIGGER_FE:
				EXTI->EXTI_FTSR |= 0b1 << gpio_handle->pin;
				break;
			case GPIO_INTERRUPT_TRIGGER_RFE:
				EXTI->EXTI_RTSR |= 0b1 << gpio_handle->pin;
				EXTI->EXTI_FTSR |= 0b1 << gpio_handle->pin;
				break;
		}

		// Gets the EXTI position in the vector table and enables the interrupt
		uint8_t exti_position = get_exti_position(gpio_handle);
		nvic_set_interrupt(exti_position, ENABLE);
	}
}

/**
 * @brief Deinitializes the given GPIO peripheral by setting its registers back to their reset values.
 * 
 * @param gpio_port The GPIO port to deinitialize.
 */
void gpio_deinit(GPIO_REGDEF_ts const *gpio_port) {
	if(gpio_port == GPIOA) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIOARST);
		
	}
	else if(gpio_port == GPIOB) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIOBRST);
	}
	else if(gpio_port == GPIOC) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIOCRST);
	}
	else if(gpio_port == GPIOD) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIODRST);
	}
	else if(gpio_port == GPIOE) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIOERST);	
	}
	else if(gpio_port == GPIOH) {
		rcc_reset_periph_ahb1(RCC_AHB1RSTR_GPIOHRST);
	}

	gpio_set_pclk(gpio_port, DISABLE);
}

/**
 * @brief Pulls a GPIO pin either high or low.
 * 
 * @param gpio_port The GPIO port to which the pin belongs
 * @param gpio_pin The GPIO pin to pull down
 * @param pin_status The status to which the pin is to be set
 */
void gpio_write(GPIO_REGDEF_ts *gpio_port, uint8_t gpio_pin, PIN_STATUS_te pin_status) {
	switch(pin_status) {
		case HIGH:
			gpio_port->GPIO_ODR |= (0x1 << gpio_pin);
			break;
		case LOW:
			gpio_port->GPIO_ODR &= ~(0x1 << gpio_pin);
			break;
	}
}

 /**
  * @brief Reads the state of a GPIO pin
  * 
  * @param gpio_port The port on which to read the GPIO pin
  * @param gpio_pin The GPIO pin to read the status of
  * @return PIN_STATUS_te 
  */
PIN_STATUS_te gpio_read(GPIO_REGDEF_ts const *gpio_port, uint8_t gpio_pin) {
	PIN_STATUS_te status = (gpio_port->GPIO_IDR >> gpio_pin) & 0x1;
	return status;
}

/** 
 @brief Clears the interrupt in the ISR.

 @param gpio_pin The EXTI line (GPIO pin number) to clear
 */
void gpio_clear_interrupt(EXTI_LINES_te exti_line) {
	EXTI->EXTI_PR = 0x1 << exti_line;
}

void gpio_get_name(GPIO_REGDEF_ts const *gpio_port, char *name) {
	const char gpio[] = "GPIO";
	uint8_t gpio_len = get_str_len(gpio);
	uint8_t pos_counter = 0;

	while(pos_counter < gpio_len) {
		name[pos_counter] = gpio[pos_counter];
		pos_counter++;
	}
	
	if(gpio_port == GPIOA) {
		name[pos_counter] = 'A';
	}
	else if(gpio_port == GPIOB) {
		name[pos_counter] = 'B';
	}
	else if(gpio_port == GPIOC) {
		name[pos_counter] = 'C';
	}
	else if(gpio_port == GPIOD) {
		name[pos_counter] = 'D';
	}
	else if(gpio_port == GPIOE) {
		name[pos_counter] = 'E';
	}
	else if(gpio_port == GPIOH) {
		name[pos_counter] = 'H';
	}
	pos_counter++;

	name[pos_counter] = '\0';
}

/** @} */

/** 
 * @defgroup GPIO_Internal_Helper GPIO Internal Helpers
 * @{
 */

/*!
 @brief Enables or disables a GPIO peripheral clock

 @param gpio_port The GPIO port instance
 @param en_status Whether to enable or disable the port clock
*/
static void gpio_set_pclk(GPIO_REGDEF_ts const *gpio_port, EN_STATUS_te en_status) {
	if(gpio_port == GPIOA) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIOAEN, en_status);
	}
	else if(gpio_port == GPIOB) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIOBEN, en_status);
	}
	else if(gpio_port == GPIOC) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIOCEN, en_status);
	}
	else if(gpio_port == GPIOD) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIODEN, en_status);
	}
	else if(gpio_port == GPIOE) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIOEEN, en_status);
	}
	else if(gpio_port == GPIOH) {
		rcc_set_pclk_ahb1(RCC_AHB1ENR_GPIOHEN, en_status);
	}
}

/*!
 @brief Returns the code of the given port

 @param gpio_port The GPIO port instance
*/
static PORT_CODES_ts get_syscfg_code(GPIO_REGDEF_ts const *gpio_port) {
	if(gpio_port == GPIOA) {
		return PA;
	}
	else if(gpio_port == GPIOB) {
		return PB;
	}
	else if(gpio_port == GPIOC) {
		return PC;
	}
	else if(gpio_port == GPIOD) {
		return PD;
	}
	else if(gpio_port == GPIOE) {
		return PE;
	}
	return PH;
}

/*!
 @brief Returns the EXTI position in the vector table of the given GPIO object

 @param gpio_handle Pointer to the GPIO object.
*/
static uint8_t get_exti_position(GPIO_HANDLE_ts const *gpio_handle) {
	if(gpio_handle->pin == 0) {
		return EXTI0_IRQn;
	}
	else if(gpio_handle->pin == 1) {
		return EXTI1_IRQn;
	}
	else if(gpio_handle->pin == 2) {
		return EXTI2_IRQn;
	}
	else if(gpio_handle->pin == 3) {
		return EXTI3_IRQn;
	}
	else if(gpio_handle->pin == 4) {
		return EXTI4_IRQn;
	}
	else if(gpio_handle->pin < 10 && gpio_handle->pin > 4) {
		return EXTI9_5_IRQn;
	}
	return EXTI15_10_IRQn;
}

/** @} */