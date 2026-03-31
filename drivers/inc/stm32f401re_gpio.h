/**
 * @file stm32f401re_gpio.h
 * @author github.com/Baksi675
 * @brief STM32F401RE GPIO driver public API.
 *
 * @details
 * This module provides a software interface to the STM32F401RE GPIO peripheral,
 * supporting the following pin modes:
 * - **Input / Output** — standard digital I/O with configurable output type,
 *   speed, and pull-up/pull-down resistors
 * - **Alternate Function** — routes a pin to an on-chip peripheral (USART, I2C, SPI, etc.)
 * - **Analog** — disables the digital input buffer for ADC use
 * - **Interrupt** — configures the EXTI line and NVIC for edge-triggered interrupts
 *
 * The peripheral clock is enabled automatically during @ref gpio_init and
 * disabled during @ref gpio_deinit.
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32_gpio STM32F401RE GPIO Driver
 * @brief GPIO peripheral driver for the STM32F401RE.
 * @{
 */

#ifndef STM32F401RE_GPIO_DRIVER_H_
#define STM32F401RE_GPIO_DRIVER_H_

#include "common.h"
#include "stm32f401re.h"

/** @brief Length of the null-terminated GPIO port name string (e.g. `"GPIOA"`). */
#define GPIO_NAME_LEN 5

/**
 * @defgroup stm32_gpio_types GPIO Types
 * @ingroup stm32_gpio
 * @brief Configuration enumerations and structures for the GPIO driver.
 * @{
 */

/**
 * @brief GPIO pin number within a port (0–15).
 */
typedef enum {
    GPIO_PIN_0,  /**< Pin 0.  */
    GPIO_PIN_1,  /**< Pin 1.  */
    GPIO_PIN_2,  /**< Pin 2.  */
    GPIO_PIN_3,  /**< Pin 3.  */
    GPIO_PIN_4,  /**< Pin 4.  */
    GPIO_PIN_5,  /**< Pin 5.  */
    GPIO_PIN_6,  /**< Pin 6.  */
    GPIO_PIN_7,  /**< Pin 7.  */
    GPIO_PIN_8,  /**< Pin 8.  */
    GPIO_PIN_9,  /**< Pin 9.  */
    GPIO_PIN_10, /**< Pin 10. */
    GPIO_PIN_11, /**< Pin 11. */
    GPIO_PIN_12, /**< Pin 12. */
    GPIO_PIN_13, /**< Pin 13. */
    GPIO_PIN_14, /**< Pin 14. */
    GPIO_PIN_15  /**< Pin 15. */
} GPIO_PIN_te;

/**
 * @brief GPIO pin operating mode.
 *
 * @details
 * - @ref GPIO_MODE_INPUT and @ref GPIO_MODE_OUTPUT write to MODER.
 * - @ref GPIO_MODE_ALTERNATE_FUNCTION enables the AFR registers.
 * - @ref GPIO_MODE_ANALOG disables the digital buffer (for ADC).
 * - @ref GPIO_MODE_INTERRUPT configures SYSCFG EXTICR, EXTI IMR/RTSR/FTSR,
 *   and the NVIC rather than MODER.
 */
typedef enum {
    GPIO_MODE_INPUT,              /**< Digital input mode. */
    GPIO_MODE_OUTPUT,             /**< Digital output mode. */
    GPIO_MODE_ALTERNATE_FUNCTION, /**< Alternate function mode (peripheral routing). */
    GPIO_MODE_ANALOG,             /**< Analog mode (digital buffer disabled). */
    GPIO_MODE_INTERRUPT           /**< External interrupt mode (EXTI + NVIC configured). */
} GPIO_MODE_te;

/**
 * @brief GPIO output driver type.
 */
typedef enum {
    GPIO_OUTPUT_TYPE_PUSHPULL,  /**< Push-pull output (actively drives both HIGH and LOW). */
    GPIO_OUTPUT_TYPE_OPENDRAIN  /**< Open-drain output (actively pulls LOW; HIGH is floating). */
} GPIO_OUTPUT_TYPE_te;

/**
 * @brief GPIO output slew rate / speed.
 *
 * @details
 * Higher speed settings increase current consumption and EMI.
 * Select the lowest speed adequate for the signal frequency.
 */
typedef enum {
    GPIO_OUTPUT_SPEED_LOW,      /**< Low speed (~2 MHz). */
    GPIO_OUTPUT_SPEED_MEDIUM,   /**< Medium speed (~25 MHz). */
    GPIO_OUTPUT_SPEED_HIGH,     /**< High speed (~50 MHz). */
    GPIO_OUTPUT_SPEED_VERYHIGH  /**< Very high speed (~100 MHz). */
} GPIO_OUTPUT_SPEED_te;

/**
 * @brief GPIO internal pull-up / pull-down resistor configuration.
 */
typedef enum {
    GPIO_PULL_MODE_NOPUPD, /**< No pull-up or pull-down resistor. */
    GPIO_PULL_MODE_PU,     /**< Internal pull-up resistor enabled. */
    GPIO_PULL_MODE_PD      /**< Internal pull-down resistor enabled. */
} GPIO_PULL_MODE_te;

/**
 * @brief GPIO alternate function mapping (AF0–AF15).
 *
 * @details
 * The mapping between AF numbers and on-chip peripherals is device-specific.
 * Refer to the STM32F401RE datasheet alternate function table for the correct
 * AF number for each pin/peripheral combination.
 */
typedef enum {
    GPIO_ALTERNATE_FUNCTION_AF0,  /**< Alternate function 0 (e.g. SYS, MCO). */
    GPIO_ALTERNATE_FUNCTION_AF1,  /**< Alternate function 1 (e.g. TIM1/TIM2). */
    GPIO_ALTERNATE_FUNCTION_AF2,  /**< Alternate function 2 (e.g. TIM3–TIM5). */
    GPIO_ALTERNATE_FUNCTION_AF3,  /**< Alternate function 3 (e.g. TIM9–TIM11). */
    GPIO_ALTERNATE_FUNCTION_AF4,  /**< Alternate function 4 (e.g. I2C1–I2C3). */
    GPIO_ALTERNATE_FUNCTION_AF5,  /**< Alternate function 5 (e.g. SPI1–SPI4). */
    GPIO_ALTERNATE_FUNCTION_AF6,  /**< Alternate function 6 (e.g. SPI3). */
    GPIO_ALTERNATE_FUNCTION_AF7,  /**< Alternate function 7 (e.g. USART1–USART2). */
    GPIO_ALTERNATE_FUNCTION_AF8,  /**< Alternate function 8 (e.g. USART6). */
    GPIO_ALTERNATE_FUNCTION_AF9,  /**< Alternate function 9 (e.g. I2C2–I2C3, CAN). */
    GPIO_ALTERNATE_FUNCTION_AF10, /**< Alternate function 10 (e.g. OTG_FS). */
    GPIO_ALTERNATE_FUNCTION_AF11, /**< Alternate function 11 (reserved). */
    GPIO_ALTERNATE_FUNCTION_AF12, /**< Alternate function 12 (e.g. SDIO). */
    GPIO_ALTERNATE_FUNCTION_AF13, /**< Alternate function 13 (reserved). */
    GPIO_ALTERNATE_FUNCTION_AF14, /**< Alternate function 14 (reserved). */
    GPIO_ALTERNATE_FUNCTION_AF15  /**< Alternate function 15 (EVENTOUT). */
} GPIO_ALTERNATE_FUNCTION_te;

/**
 * @brief EXTI edge trigger selection for interrupt mode.
 *
 * @details
 * Only used when @ref GPIO_CFG_ts::mode is @ref GPIO_MODE_INTERRUPT.
 */
typedef enum {
    GPIO_INTERRUPT_TRIGGER_RE,  /**< Trigger on rising edge only. */
    GPIO_INTERRUPT_TRIGGER_FE,  /**< Trigger on falling edge only. */
    GPIO_INTERRUPT_TRIGGER_RFE  /**< Trigger on both rising and falling edges. */
} GPIO_INTERRUPT_TRIGGER_te;

/**
 * @brief Configuration structure for initializing a GPIO pin.
 *
 * @details
 * Passed to @ref gpio_init. Fields that are irrelevant for the selected
 * @ref mode are ignored:
 * - @ref output_type, @ref output_speed are only relevant for output and
 *   alternate function modes.
 * - @ref alternate_function is only relevant for @ref GPIO_MODE_ALTERNATE_FUNCTION.
 * - @ref interrupt_trigger is only relevant for @ref GPIO_MODE_INTERRUPT.
 */
typedef struct {
    /** Pointer to the GPIO port instance (e.g. GPIOA, GPIOB). */
    GPIO_REGDEF_ts *port;

    /** Pin number within the port. */
    GPIO_PIN_te pin;

    /** Operating mode of the pin. */
    GPIO_MODE_te mode;

    /** Output driver type (push-pull or open-drain). */
    GPIO_OUTPUT_TYPE_te output_type;

    /** Output slew rate. */
    GPIO_OUTPUT_SPEED_te output_speed;

    /** Internal pull-up / pull-down configuration. */
    GPIO_PULL_MODE_te pull_mode;

    /** Alternate function mapping (AF0–AF15). */
    GPIO_ALTERNATE_FUNCTION_te alternate_function;

    /** Edge trigger selection for interrupt mode. */
    GPIO_INTERRUPT_TRIGGER_te interrupt_trigger;
} GPIO_CFG_ts;

/** @} */

/**
 * @defgroup stm32_gpio_api GPIO Public API
 * @ingroup stm32_gpio
 * @brief Public functions to interact with the GPIO peripheral.
 * @{
 */

/**
 * @brief Initializes a GPIO pin according to the given configuration.
 *
 * @details
 * Enables the peripheral clock for the port, then configures output type,
 * speed, pull mode, alternate function, and pin mode. For interrupt mode,
 * also configures SYSCFG EXTICR, EXTI IMR, RTSR/FTSR, and enables the
 * corresponding NVIC interrupt line.
 *
 * @param[in] gpio_cfg Pointer to the GPIO configuration structure.
 */
void gpio_init(GPIO_CFG_ts *gpio_cfg);

/**
 * @brief Deinitializes a GPIO port by resetting its registers to reset values.
 *
 * @details
 * Triggers an RCC peripheral reset for the given port and then disables
 * its peripheral clock.
 *
 * @param[in] gpio_port Pointer to the GPIO port instance to deinitialize.
 */
void gpio_deinit(GPIO_REGDEF_ts const *gpio_port);

/**
 * @brief Drives a GPIO output pin high or low.
 *
 * @param[in] gpio_port  Pointer to the GPIO port instance.
 * @param[in] gpio_pin   Pin number to write (0–15).
 * @param[in] pin_status @ref HIGH to drive the pin high, @ref LOW to drive it low.
 */
void gpio_write(GPIO_REGDEF_ts *gpio_port, uint8_t gpio_pin, PIN_STATUS_te pin_status);

/**
 * @brief Reads the current logic level of a GPIO input pin.
 *
 * @param[in] gpio_port Pointer to the GPIO port instance.
 * @param[in] gpio_pin  Pin number to read (0–15).
 *
 * @return @ref HIGH if the pin reads logic high, @ref LOW if it reads logic low.
 */
PIN_STATUS_te gpio_read(GPIO_REGDEF_ts const *gpio_port, uint8_t gpio_pin);

/**
 * @brief Clears the EXTI pending flag for the given interrupt line.
 *
 * @details
 * Must be called at the end of an EXTI interrupt handler to acknowledge
 * the interrupt and prevent immediate re-entry. Writes 1 to the EXTI_PR
 * bit for @p exti_line (write-1-to-clear).
 *
 * @param[in] exti_line The EXTI line number corresponding to the GPIO pin.
 */
void gpio_clear_interrupt(EXTI_LINES_te exti_line);

/**
 * @brief Returns the name string of a GPIO port (e.g. `"GPIOA"`).
 *
 * @details
 * Writes a null-terminated string of the form `"GPIOx"` into @p name.
 * The caller must ensure @p name points to a buffer of at least
 * @ref GPIO_NAME_LEN + 1 bytes.
 *
 * @param[in]  gpio_port Pointer to the GPIO port instance.
 * @param[out] name      Pointer to the destination buffer.
 */
void gpio_get_name(GPIO_REGDEF_ts const *gpio_port, char *name);

/** @} */

#endif

/** @} */