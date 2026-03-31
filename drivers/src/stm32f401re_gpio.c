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

/* ---- Forward declarations for internal helpers ---- */
static void gpio_set_pclk(GPIO_REGDEF_ts const *gpio_port, EN_STATUS_te en_status);
static PORT_CODES_ts get_syscfg_code(GPIO_REGDEF_ts const *port);
static uint8_t get_exti_position(GPIO_CFG_ts const *gpio_cfg);

/**
 * @defgroup stm32_gpio_public_apis GPIO Public APIs
 * @{
 */

/** @brief Initializes a GPIO pin according to the given configuration. @see gpio_init */
void gpio_init(GPIO_CFG_ts *gpio_cfg) {
    // Enable the clock for the GPIO peripheral
    gpio_set_pclk(gpio_cfg->port, ENABLE);

    // Configure GPIO output type
    gpio_cfg->port->GPIO_OTYPER &= ~(0b1 << gpio_cfg->pin);
    gpio_cfg->port->GPIO_OTYPER |= gpio_cfg->output_type << gpio_cfg->pin;

    // Configure GPIO speed
    gpio_cfg->port->GPIO_OSPEEDR &= ~(0b11 << gpio_cfg->pin * 2);
    gpio_cfg->port->GPIO_OSPEEDR |= gpio_cfg->output_speed << gpio_cfg->pin * 2;

    // Configure GPIO pull-up pull-down
    gpio_cfg->port->GPIO_PUPDR &= ~(0b11 << gpio_cfg->pin * 2);
    gpio_cfg->port->GPIO_PUPDR |= gpio_cfg->pull_mode << gpio_cfg->pin * 2;

    // Configure GPIO alternate function (2 bits per pin in AFR[0] for pins 0-7, AFR[1] for pins 8-15)
    uint32_t afr_index = gpio_cfg->pin >> 3;        // pin / 8 selects AFR[0] or AFR[1]
    uint32_t afr_shift = (gpio_cfg->pin & 0x7) * 4; // (pin % 8) * 4 gives the bit offset

    gpio_cfg->port->GPIO_AFR[afr_index] &= ~(0xFu << afr_shift);
    gpio_cfg->port->GPIO_AFR[afr_index] |= ((uint32_t)gpio_cfg->alternate_function << afr_shift);

    // Configure GPIO mode
    if(gpio_cfg->mode != GPIO_MODE_INTERRUPT) {
        gpio_cfg->port->GPIO_MODER &= ~(0b11 << gpio_cfg->pin * 2);
        gpio_cfg->port->GPIO_MODER |= gpio_cfg->mode << gpio_cfg->pin * 2;
    }
    else {
        // Enable the peripheral clock for SYSCFG
        RCC->RCC_APB2ENR |= (0b1 << 14);

        // Route the GPIO pin to the EXTI line via SYSCFG_EXTICRx
        if(gpio_cfg->pin < 4) {
            SYSCFG->SYSCFG_EXTICR1 &= ~(0b1111 << gpio_cfg->pin * 4);
            SYSCFG->SYSCFG_EXTICR1 |= (get_syscfg_code(gpio_cfg->port)) << gpio_cfg->pin * 4;
        }
        else if(gpio_cfg->pin < 8) {
            SYSCFG->SYSCFG_EXTICR2 &= ~(0b1111 << (gpio_cfg->pin - 4) * 4);
            SYSCFG->SYSCFG_EXTICR2 |= (get_syscfg_code(gpio_cfg->port)) << (gpio_cfg->pin - 4) * 4;
        }
        else if(gpio_cfg->pin < 12) {
            SYSCFG->SYSCFG_EXTICR3 &= ~(0b1111 << (gpio_cfg->pin - 8) * 4);
            SYSCFG->SYSCFG_EXTICR3 |= (get_syscfg_code(gpio_cfg->port)) << (gpio_cfg->pin - 8) * 4;
        }
        else {
            SYSCFG->SYSCFG_EXTICR4 &= ~(0b1111 << (gpio_cfg->pin - 12) * 4);
            SYSCFG->SYSCFG_EXTICR4 |= (get_syscfg_code(gpio_cfg->port)) << (gpio_cfg->pin - 12) * 4;
        }

        // Unmask the EXTI line for this pin
        EXTI->EXTI_IMR |= 0b1 << gpio_cfg->pin;

        // Configure edge trigger
        switch(gpio_cfg->interrupt_trigger) {
            case GPIO_INTERRUPT_TRIGGER_RE:
                EXTI->EXTI_RTSR |= 0b1 << gpio_cfg->pin;
                break;
            case GPIO_INTERRUPT_TRIGGER_FE:
                EXTI->EXTI_FTSR |= 0b1 << gpio_cfg->pin;
                break;
            case GPIO_INTERRUPT_TRIGGER_RFE:
                EXTI->EXTI_RTSR |= 0b1 << gpio_cfg->pin;
                EXTI->EXTI_FTSR |= 0b1 << gpio_cfg->pin;
                break;
        }

        // Enable the corresponding NVIC interrupt line
        uint8_t exti_position = get_exti_position(gpio_cfg);
        nvic_set_interrupt(exti_position, ENABLE);
    }
}

/** @brief Deinitializes a GPIO port by resetting its registers to reset values. @see gpio_deinit */
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

/** @brief Drives a GPIO output pin high or low. @see gpio_write */
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

/** @brief Reads the current logic level of a GPIO input pin. @see gpio_read */
PIN_STATUS_te gpio_read(GPIO_REGDEF_ts const *gpio_port, uint8_t gpio_pin) {
    PIN_STATUS_te status = (gpio_port->GPIO_IDR >> gpio_pin) & 0x1;
    return status;
}

/** @brief Clears the EXTI pending flag for the given interrupt line. @see gpio_clear_interrupt */
void gpio_clear_interrupt(EXTI_LINES_te exti_line) {
    EXTI->EXTI_PR = 0x1 << exti_line;
}

/** @brief Returns the name string of a GPIO port. @see gpio_get_name */
void gpio_get_name(GPIO_REGDEF_ts const *gpio_port, char *name) {
    const char gpio[] = "GPIO";
    uint8_t gpio_len = get_str_len(gpio);
    uint8_t pos_counter = 0;

    while(pos_counter < gpio_len) {
        name[pos_counter] = gpio[pos_counter];
        pos_counter++;
    }

    if(gpio_port == GPIOA)      name[pos_counter] = 'A';
    else if(gpio_port == GPIOB) name[pos_counter] = 'B';
    else if(gpio_port == GPIOC) name[pos_counter] = 'C';
    else if(gpio_port == GPIOD) name[pos_counter] = 'D';
    else if(gpio_port == GPIOE) name[pos_counter] = 'E';
    else if(gpio_port == GPIOH) name[pos_counter] = 'H';
    pos_counter++;

    name[pos_counter] = '\0';
}

/** @} */

/**
 * @defgroup stm32_gpio_internal_helpers GPIO Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables the peripheral clock for a GPIO port.
 *
 * @details
 * Calls the appropriate RCC AHB1 clock enable/disable function based on
 * the port pointer. Called by @ref gpio_init and @ref gpio_deinit.
 *
 * @param[in] gpio_port Pointer to the GPIO port instance.
 * @param[in] en_status @ref ENABLE to enable the clock, @ref DISABLE to disable it.
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

/**
 * @brief Returns the SYSCFG port code for a GPIO port instance.
 *
 * @details
 * The SYSCFG_EXTICRx registers use a 4-bit code to select which port
 * is routed to a given EXTI line. This function maps port pointers to
 * their corresponding @ref PORT_CODES_ts values.
 *
 * @param[in] gpio_port Pointer to the GPIO port instance.
 *
 * @return The @ref PORT_CODES_ts code for the given port.
 */
static PORT_CODES_ts get_syscfg_code(GPIO_REGDEF_ts const *gpio_port) {
    if(gpio_port == GPIOA) return PA;
    else if(gpio_port == GPIOB) return PB;
    else if(gpio_port == GPIOC) return PC;
    else if(gpio_port == GPIOD) return PD;
    else if(gpio_port == GPIOE) return PE;
    return PH;
}

/**
 * @brief Returns the IRQ number for the EXTI line corresponding to a GPIO pin.
 *
 * @details
 * Maps GPIO pin numbers to NVIC IRQ numbers as defined by the STM32F401RE
 * vector table:
 * - Pins 0–4 have individual EXTI IRQs (EXTI0–EXTI4).
 * - Pins 5–9 share EXTI9_5_IRQn.
 * - Pins 10–15 share EXTI15_10_IRQn.
 *
 * @param[in] gpio_cfg Pointer to the GPIO configuration structure.
 *
 * @return The @ref IRQn_te IRQ number for the pin's EXTI line.
 */
static uint8_t get_exti_position(GPIO_CFG_ts const *gpio_cfg) {
    if(gpio_cfg->pin == 0)                                  return EXTI0_IRQn;
    else if(gpio_cfg->pin == 1)                             return EXTI1_IRQn;
    else if(gpio_cfg->pin == 2)                             return EXTI2_IRQn;
    else if(gpio_cfg->pin == 3)                             return EXTI3_IRQn;
    else if(gpio_cfg->pin == 4)                             return EXTI4_IRQn;
    else if(gpio_cfg->pin < 10 && gpio_cfg->pin > 4)       return EXTI9_5_IRQn;
    return EXTI15_10_IRQn;
}

/** @} */