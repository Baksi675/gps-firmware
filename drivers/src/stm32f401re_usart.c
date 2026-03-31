/**
 * @file stm32f401re_usart.c
 * @author github.com/Baksi675
 * @brief USART driver implementation for STM32F401RE.
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
#include "stm32f401re_rcc.h"

/* ---- Forward declarations for internal helpers ---- */
static void usart_set_pclk(USART_REGDEF_ts const *instance, EN_STATUS_te en_status);
static void usart_set_baud_rate(USART_CFG_ts *usart_cfg);

/**
 * @defgroup stm32_usart_public_apis USART Public APIs
 * @{
 */

/** @brief Initializes the USART peripheral with the given configuration. @see usart_init */
void usart_init(USART_CFG_ts *usart_cfg) {
    // Enable peripheral clock
    usart_set_pclk(usart_cfg->instance, ENABLE);

    // Configure data word length (8 or 9 bit)
    usart_cfg->instance->USART_CR1 &= ~(0x1 << USART_CR1_M);
    usart_cfg->instance->USART_CR1 |= (usart_cfg->frame_data_bits << USART_CR1_M);

    // Configure stop bit count
    usart_cfg->instance->USART_CR2 &= ~(0x3 << USART_CR2_STOP);
    usart_cfg->instance->USART_CR2 |= (usart_cfg->frame_stop_bits << USART_CR2_STOP);

    // Compute and write BRR from peripheral clock and baud rate
    usart_set_baud_rate(usart_cfg);

    // Configure oversampling ratio
    usart_cfg->instance->USART_CR1 &= ~(0x1 << USART_CR1_OVER8);
    usart_cfg->instance->USART_CR1 |= (usart_cfg->oversampling << USART_CR1_OVER8);

    // Configure parity
    if(usart_cfg->parity != USART_PARITY_DISABLED) {
        usart_cfg->instance->USART_CR1 |= (0x1 << USART_CR1_PCE);

        if(usart_cfg->parity == USART_PARITY_EVEN) {
            usart_cfg->instance->USART_CR1 &= ~(0x1 << USART_CR1_PS);
        }
        else if(usart_cfg->parity == USART_PARITY_ODD) {
            usart_cfg->instance->USART_CR1 |= (0x1 << USART_CR1_PS);
        }
    }

    // Configure hardware flow control (RTS + CTS)
    if(usart_cfg->hw_flow_control == USART_HW_FLOW_CONTROL_ENABLED) {
        usart_cfg->instance->USART_CR3 |= (0x1 << USART_CR3_CTSE);
        usart_cfg->instance->USART_CR3 |= (0x1 << USART_CR3_RTSE);
    }

    // Configure sample bit method
    usart_cfg->instance->USART_CR3 &= ~(0x1 << USART_CR3_ONEBIT);
    usart_cfg->instance->USART_CR3 |= (usart_cfg->sample_bit << USART_CR3_ONEBIT);

    if(usart_cfg->mode == USART_MODE_SYNC) {
        // LBCL, CPHA, CPOL, CLKEN — not yet implemented
    }

    // Enable RXNE interrupt and NVIC line if requested
    if(usart_cfg->interrupt_en == USART_INTERRUPT_EN_TRUE) {
        usart_cfg->instance->USART_CR1 |= (0x1 << USART_CR1_RXNEIE);

        if(usart_cfg->instance == USART1) {
            nvic_set_interrupt(USART1_IRQn, ENABLE);
        }
        else if(usart_cfg->instance == USART2) {
            nvic_set_interrupt(USART2_IRQn, ENABLE);
        }
        else if(usart_cfg->instance == USART6) {
            nvic_set_interrupt(USART6_IRQn, ENABLE);
        }
    }

    // Enable the USART peripheral
    usart_cfg->instance->USART_CR1 |= (0x1 << USART_CR1_UE);
}

/** @brief Deinitializes the USART peripheral, disables its NVIC interrupt, and disables its clock. @see usart_deinit */
void usart_deinit(USART_REGDEF_ts const *instance) {
    if(instance == USART1) {
        rcc_reset_periph_apb2(RCC_APB2RSTR_USART1RST);
        nvic_set_interrupt(USART1_IRQn, DISABLE);
    }
    else if(instance == USART2) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_USART2RST);
        nvic_set_interrupt(USART2_IRQn, DISABLE);
    }
    else if(instance == USART6) {
        rcc_reset_periph_apb2(RCC_APB2RSTR_USART6RST);
        nvic_set_interrupt(USART6_IRQn, DISABLE);
    }

    usart_set_pclk(instance, DISABLE);
}

/** @brief Blocking USART transmit. @see usart_send */
void usart_send(USART_REGDEF_ts *instance, uint8_t *tx_buffer, uint32_t len) {
    while(len != 0) {
        while(!((instance->USART_SR >> USART_SR_TXE) & 0x1));
        instance->USART_DR = *tx_buffer;
        tx_buffer++;
        len--;
    }
    // Wait for TC to confirm the last byte has fully shifted out
    while(!((instance->USART_SR >> USART_SR_TC) & 0x1));
}

/** @brief Blocking USART receive. @see usart_receive */
void usart_receive(USART_REGDEF_ts const *instance, uint8_t *rx_buffer, uint32_t len) {
    while(len != 0) {
        while(!((instance->USART_SR >> USART_SR_RXNE) & 0x1));
        *rx_buffer = instance->USART_DR;
        rx_buffer++;
        len--;
    }
}

/** @brief Enables or disables the USART transmitter. @see usart_set_transmission */
void usart_set_transmission(USART_REGDEF_ts *instance, EN_STATUS_te en_status) {
    instance->USART_CR1 &= ~(0x1 << USART_CR1_TE);
    instance->USART_CR1 |= (en_status << USART_CR1_TE);
}

/** @brief Enables or disables the USART receiver. @see usart_set_reception */
void usart_set_reception(USART_REGDEF_ts *instance, EN_STATUS_te en_status) {
    instance->USART_CR1 &= ~(0x1 << USART_CR1_RE);
    instance->USART_CR1 |= (en_status << USART_CR1_RE);
}

/** @brief Returns the name string of a USART peripheral instance. @see usart_get_name */
void usart_get_name(USART_REGDEF_ts const *instance, char *name) {
    const char usart[] = "USART";
    uint8_t usart_len = get_str_len(usart);
    uint8_t pos_counter = 0;

    while(pos_counter != usart_len) {
        name[pos_counter] = usart[pos_counter];
        pos_counter++;
    }

    if(instance == USART1)      name[pos_counter] = '1';
    else if(instance == USART2) name[pos_counter] = '2';
    else if(instance == USART6) name[pos_counter] = '6';
    pos_counter++;

    name[pos_counter] = '\0';
}

/** @} */

/**
 * @defgroup stm32_usart_internal_helpers USART Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables the peripheral clock for a USART instance.
 *
 * @details
 * Routes to the appropriate RCC APB1 or APB2 clock enable/disable call:
 * - USART1 and USART6 are on APB2.
 * - USART2 is on APB1.
 *
 * Called by @ref usart_init and @ref usart_deinit.
 *
 * @param[in] instance  Pointer to the USART peripheral instance.
 * @param[in] en_status @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
static void usart_set_pclk(USART_REGDEF_ts const *instance, EN_STATUS_te en_status) {
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
 * @brief Computes and writes the BRR register for the configured baud rate.
 *
 * @details
 * Uses the fixed-point scaling method from the STM32 reference manual:
 * - `USARTDIV × 100 = (25 × PCLK) / (multiplier × baud_rate)`, where
 *   multiplier is 2 for OVER8 and 4 for OVER16.
 * - The mantissa is `USARTDIV / 100`.
 * - The fraction is rounded and masked to 3 bits (OVER8) or 4 bits (OVER16).
 *
 * USART1 and USART6 use the APB2 clock; USART2 uses the APB1 clock.
 *
 * @param[in] usart_cfg Pointer to the USART configuration structure.
 */
static void usart_set_baud_rate(USART_CFG_ts *usart_cfg) {
    usart_cfg->instance->USART_BRR &= ~(0xFFFF);
    uint32_t usart_pclk = 0;
    uint32_t temp_reg = 0;
    uint32_t usartdiv = 0;
    uint32_t usartdiv_mantissa;
    uint32_t usartdiv_fraction;

    if(usart_cfg->instance == USART1 || usart_cfg->instance == USART6) {
        usart_pclk = rcc_get_apb2_clk();
    }
    else if(usart_cfg->instance == USART2) {
        usart_pclk = rcc_get_apb1_clk();
    }

    if(usart_cfg->oversampling == USART_OVERSAMPLING_8) {
        usartdiv = ((25 * usart_pclk) / (2 * usart_cfg->baud_rate));
    }
    else if(usart_cfg->oversampling == USART_OVERSAMPLING_16) {
        usartdiv = ((25 * usart_pclk) / (4 * usart_cfg->baud_rate));
    }

    usartdiv_mantissa = usartdiv / 100;
    temp_reg |= usartdiv_mantissa << 4;

    usartdiv_fraction = (usartdiv - (usartdiv_mantissa * 100));

    if(usart_cfg->oversampling == USART_OVERSAMPLING_8) {
        // Round fraction to 3 bits for OVER8
        usartdiv_fraction = (((usartdiv_fraction * 8) + 50) / 100) & ((uint8_t)0x07);
    }
    else if(usart_cfg->oversampling == USART_OVERSAMPLING_16) {
        // Round fraction to 4 bits for OVER16
        usartdiv_fraction = (((usartdiv_fraction * 16) + 50) / 100) & ((uint8_t)0x0F);
    }

    temp_reg |= usartdiv_fraction;
    usart_cfg->instance->USART_BRR = temp_reg;
}

/** @} */

/**
 * @brief USART1 interrupt handler. Dispatches RXNE events to @ref usart1_irq_data_recv_callback.
 *
 * @details
 * Checks the RXNE flag and reads USART_DR to clear it, then passes the
 * received byte to the application callback. Only RXNE is handled;
 * other interrupt sources are not checked.
 */
void USART1_IRQHandler(void) {
    if((USART1->USART_SR >> USART_SR_RXNE) & 0x1) {
        uint8_t data = USART1->USART_DR;
        usart1_irq_data_recv_callback(data);
    }
}

/**
 * @brief USART2 interrupt handler. Currently empty — no interrupt sources configured.
 */
void USART2_IRQHandler(void) {
}

/**
 * @brief USART6 interrupt handler. Dispatches RXNE events to @ref usart6_irq_data_recv_callback.
 *
 * @details
 * Checks the RXNE flag and reads USART_DR to clear it, then passes the
 * received byte to the application callback.
 */
void USART6_IRQHandler(void) {
    if((USART6->USART_SR >> USART_SR_RXNE) & 0x1) {
        uint8_t data = USART6->USART_DR;
        usart6_irq_data_recv_callback(data);
    }
}

/**
 * @brief Default weak implementation of @ref usart1_irq_data_recv_callback.
 *
 * @details
 * Spins in an infinite loop if no application-level override is provided.
 * Override this function in application code to handle received bytes.
 */
void usart1_irq_data_recv_callback(uint8_t data) __attribute__((weak, alias("usart_irq_callback")));

/**
 * @brief Default weak implementation of @ref usart6_irq_data_recv_callback.
 *
 * @details
 * Spins in an infinite loop if no application-level override is provided.
 * Override this function in application code to handle received bytes.
 */
void usart6_irq_data_recv_callback(uint8_t data) __attribute__((weak, alias("usart_irq_callback")));

/**
 * @brief Default fallback handler for unimplemented USART receive callbacks.
 *
 * @details
 * Spins forever. Serves as the weak alias target for both
 * @ref usart1_irq_data_recv_callback and @ref usart6_irq_data_recv_callback
 * when no application override is provided.
 */
void usart_irq_callback(uint8_t data) {
    while(1);
}