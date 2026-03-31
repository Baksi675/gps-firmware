/**
 * @file stm32f401re_spi.c
 * @author github.com/Baksi675
 * @brief SPI driver implementation for STM32F401RE.
 * @version 0.1
 * @date 2025-07-29
 *
 * @copyright Copyright (c) 2025
 */

#include "stm32f401re_spi.h"
#include "stm32f401re.h"
#include "stm32f401re_rcc.h"

/* ---- Forward declaration for internal helper ---- */
static void spi_set_pclk(SPI_REGDEF_ts const *instance, EN_STATUS_te en_status);

/**
 * @defgroup stm32_spi_public_apis SPI Public APIs
 * @{
 */

/** @brief Initializes the SPI peripheral with the given configuration. @see spi_init */
void spi_init(SPI_CFG_ts *spi_cfg) {
    spi_set_pclk(spi_cfg->instance, ENABLE);

    // Configure data frame format (8-bit or 16-bit)
    spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_DFF);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->data_frame_format << SPI_CR1_DFF);

    // Configure clock polarity (CPOL)
    spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_CPOL);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->clock_polarity << SPI_CR1_CPOL);

    // Configure clock phase (CPHA)
    spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_CPHA);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->clock_phase << SPI_CR1_CPHA);

    // Configure bit order (MSB or LSB first)
    spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_LSBFIRST);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->bit_first << SPI_CR1_LSBFIRST);

    // Configure slave select management (HW or SW)
    spi_cfg->instance->SPI_CR1 &= ~(0b1 << SPI_CR1_SSM);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->slave_select_mode << SPI_CR1_SSM);

    if(spi_cfg->mode == SPI_MODE_MASTER) {
        // Set SSI to prevent MODF fault in software NSS mode
        spi_cfg->instance->SPI_CR1 |= (0x1 << SPI_CR1_SSI);

        // Enable SSOE for hardware NSS output
        if(spi_cfg->slave_select_mode == SPI_SLAVE_SELECT_MODE_HW) {
            spi_cfg->instance->SPI_CR2 &= ~(0x1 << SPI_CR2_SSOE);
            spi_cfg->instance->SPI_CR2 |= (0x1 << SPI_CR2_SSOE);
        }

        // Configure master SCK speed (BR bits)
        spi_cfg->instance->SPI_CR1 &= ~(0x07 << SPI_CR1_BR);
        spi_cfg->instance->SPI_CR1 |= (spi_cfg->master_sclk_speed << SPI_CR1_BR);
    }
    else if(spi_cfg->mode == SPI_MODE_SLAVE) {
        // Clear SSI to pull NSS internally low in slave mode
        spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SSI);
    }

    // Set master/slave mode
    spi_cfg->instance->SPI_CR1 &= ~(0x1 << SPI_CR1_MSTR);
    spi_cfg->instance->SPI_CR1 |= (spi_cfg->mode << SPI_CR1_MSTR);
}

/** @brief Deinitializes the SPI peripheral and disables its clock. @see spi_deinit */
void spi_deinit(SPI_REGDEF_ts const *instance) {
    if(instance == SPI1) {
        rcc_reset_periph_apb2(RCC_APB2RSTR_SPI1RST);
    }
    else if(instance == SPI2) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_SPI2RST);
    }
    else if(instance == SPI3) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_SPI3RST);
    }
    else if(instance == SPI4) {
        rcc_reset_periph_apb2(RCC_APB2RSTR_SPI4RST);
    }

    spi_set_pclk(instance, DISABLE);
}

/** @brief Blocking SPI transmit. @see spi_send */
void spi_send(SPI_REGDEF_ts *instance, uint8_t *tx_buffer, uint32_t len) {
    while(len != 0) {
        if(((instance->SPI_CR1 >> SPI_CR1_DFF) & 0x1) == 0) {
            // 8-bit frame: wait for TXE, write one byte
            while(!((instance->SPI_SR >> SPI_SR_TXE) & 0x1));
            instance->SPI_DR = *tx_buffer;
            tx_buffer++;
            len--;
        }
        else {
            // 16-bit frame: wait for TXE, write two bytes
            while(!((instance->SPI_SR >> SPI_SR_TXE) & 0x1));
            instance->SPI_DR = *((uint16_t*)tx_buffer);
            tx_buffer += 2;
            len -= 2;
        }

        // Drain receive FIFO to prevent overrun in full-duplex mode
        while(!((instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
        uint16_t dummy_rx = instance->SPI_DR;
        (void)dummy_rx;
    }
}

/** @brief Blocking SPI receive. @see spi_receive */
void spi_receive(SPI_REGDEF_ts *instance, uint8_t *rx_buffer, uint32_t len) {
    uint16_t dummy_tx = 0xFFFF;

    while(len != 0) {
        if(((instance->SPI_CR1 >> SPI_CR1_DFF) & 0x1) == 0) {
            // 8-bit frame: send dummy byte to generate clock, then read
            instance->SPI_DR = (uint8_t)dummy_tx;
            while(!((instance->SPI_SR >> SPI_SR_TXE) & 0x1));

            while(!((instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
            *rx_buffer = instance->SPI_DR;
            len--;
            rx_buffer++;
        }
        else {
            // 16-bit frame: send dummy word to generate clock, then read
            instance->SPI_DR = dummy_tx;
            while(!((instance->SPI_SR >> SPI_SR_TXE) & 0x1));

            while(!((instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
            *((uint16_t*)rx_buffer) = instance->SPI_DR;
            len -= 2;
            rx_buffer += 2;
        }
    }
}

/** @brief Changes the SPI master clock speed divisor at runtime. @see spi_set_pclk_div */
void spi_set_pclk_div(SPI_REGDEF_ts *spi_instance, SPI_MASTER_SCLK_SPEED_te pclk_div) {
    spi_instance->SPI_CR1 &= ~(0x7 << SPI_CR1_BR);
    spi_instance->SPI_CR1 |= (pclk_div << SPI_CR1_BR);
}

/** @brief Enables or disables the SPI peripheral. @see spi_set_comm */
void spi_set_comm(SPI_REGDEF_ts *instance, EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SPE);
        instance->SPI_CR1 |= (0x1 << SPI_CR1_SPE);
    }
    else if(en_status == DISABLE) {
        // Wait for any ongoing transfer to complete before clearing SPE
        while((instance->SPI_SR >> SPI_SR_BSY) & 0x01);
        instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SPE);
    }
}

/** @} */

/**
 * @defgroup stm32_spi_internal_helpers SPI Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables the peripheral clock for an SPI instance.
 *
 * @details
 * Routes to the appropriate RCC APB1 or APB2 clock enable/disable call:
 * - SPI1, SPI4 are on APB2.
 * - SPI2, SPI3 are on APB1.
 *
 * Called by @ref spi_init and @ref spi_deinit.
 *
 * @param[in] instance  Pointer to the SPI peripheral instance.
 * @param[in] en_status @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
static void spi_set_pclk(SPI_REGDEF_ts const *instance, EN_STATUS_te en_status) {
    if(instance == SPI1) {
        rcc_set_pclk_apb2(RCC_APB2ENR_SPI1EN, en_status);
    }
    else if(instance == SPI2) {
        rcc_set_pclk_apb1(RCC_APB1ENR_SPI2EN, en_status);
    }
    else if(instance == SPI3) {
        rcc_set_pclk_apb1(RCC_APB1ENR_SPI3EN, en_status);
    }
    else if(instance == SPI4) {
        rcc_set_pclk_apb2(RCC_APB2ENR_SPI4EN, en_status);
    }
}

/** @} */