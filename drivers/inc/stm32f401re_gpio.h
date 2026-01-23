/**
 * @file stm32f401re_gpio.h
 * @author github.com/Baksi675
 * @brief Header file for STM32F401RE GPIO driver.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef STM32F401RE_GPIO_DRIVER_H_
#define STM32F401RE_GPIO_DRIVER_H_

#include "common.h"
#include "stm32f401re.h"
#include "stm32f401re.h"

#define GPIO_NAME_LEN			5

/**
 * @brief GPIO pin definitions
 * 
 */
typedef enum {
	GPIO_PIN_0,
	GPIO_PIN_1,
	GPIO_PIN_2,
	GPIO_PIN_3,
	GPIO_PIN_4,
	GPIO_PIN_5,
	GPIO_PIN_6,
	GPIO_PIN_7,
	GPIO_PIN_8,
	GPIO_PIN_9,
	GPIO_PIN_10,
	GPIO_PIN_11,
	GPIO_PIN_12,
	GPIO_PIN_13,
	GPIO_PIN_14,
	GPIO_PIN_15
}GPIO_PIN_te;

/**
 * @brief GPIO mode definitions
 * 
 */
typedef enum {
	GPIO_MODE_INPUT,
	GPIO_MODE_OUTPUT,
	GPIO_MODE_ALTERNATE_FUNCTION,
	GPIO_MODE_ANALOG,
	GPIO_MODE_INTERRUPT
}GPIO_MODE_te;

/**
 * @brief GPIO output type definitions
 * 
 */
typedef enum {
	GPIO_OUTPUT_TYPE_PUSHPULL,
	GPIO_OUTPUT_TYPE_OPENDRAIN
}GPIO_OUTPUT_TYPE_te;

/**
 * @brief GPIO speed definitions
 * 
 */
typedef enum {
	GPIO_OUTPUT_SPEED_LOW,
	GPIO_OUTPUT_SPEED_MEDIUM,
	GPIO_OUTPUT_SPEED_HIGH,
	GPIO_OUTPUT_SPEED_VERYHIGH
}GPIO_OUTPUT_SPEED_te;

/**
 * @brief GPIO pull mode definitions
 * 
 */
typedef enum {
	GPIO_PULL_MODE_NOPUPD,
	GPIO_PULL_MODE_PU,
	GPIO_PULL_MODE_PD 
}GPIO_PULL_MODE_te;

/**
 * @brief GPIO alternate function definitions
 * 
 */
typedef enum {
	GPIO_ALTERNATE_FUNCTION_AF0,
	GPIO_ALTERNATE_FUNCTION_AF1,
	GPIO_ALTERNATE_FUNCTION_AF2,
	GPIO_ALTERNATE_FUNCTION_AF3,
	GPIO_ALTERNATE_FUNCTION_AF4,
	GPIO_ALTERNATE_FUNCTION_AF5,
	GPIO_ALTERNATE_FUNCTION_AF6,
	GPIO_ALTERNATE_FUNCTION_AF7,
	GPIO_ALTERNATE_FUNCTION_AF8,
	GPIO_ALTERNATE_FUNCTION_AF9,
	GPIO_ALTERNATE_FUNCTION_AF10,
	GPIO_ALTERNATE_FUNCTION_AF11,
	GPIO_ALTERNATE_FUNCTION_AF12,
	GPIO_ALTERNATE_FUNCTION_AF13,
	GPIO_ALTERNATE_FUNCTION_AF14,
	GPIO_ALTERNATE_FUNCTION_AF15
}GPIO_ALTERNATE_FUNCTION_te;

/**
 * @brief GPIO interrupt trigger definitions
 * 
 */
typedef enum {
	GPIO_INTERRUPT_TRIGGER_RE,
	GPIO_INTERRUPT_TRIGGER_FE,
	GPIO_INTERRUPT_TRIGGER_RFE
}GPIO_INTERRUPT_TRIGGER_te;

/**
 * @brief 
 * 
 */
typedef struct {
    GPIO_REGDEF_ts *port;             
    GPIO_PIN_te pin;		                                
    GPIO_MODE_te mode;                      
    GPIO_OUTPUT_TYPE_te output_type;               
    GPIO_OUTPUT_SPEED_te output_speed;              
    GPIO_PULL_MODE_te pull_mode;                	
    GPIO_ALTERNATE_FUNCTION_te alternate_function; 		
	GPIO_INTERRUPT_TRIGGER_te interrupt_trigger;				
}GPIO_HANDLE_ts;

void gpio_init(GPIO_HANDLE_ts *gpio_handle);
void gpio_write(GPIO_REGDEF_ts *gpio_port, uint8_t gpio_pin, PIN_STATUS_te pin_status); 
PIN_STATUS_te gpio_read(GPIO_REGDEF_ts *gpio_port, uint8_t gpio_pin);
void gpio_clear_interrupt(EXTI_LINES_te exti_line);
void gpio_get_name(GPIO_REGDEF_ts *gpio_port, char *name);

#endif