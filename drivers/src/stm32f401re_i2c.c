/**
 * @file stm32f401re_i2c.c
 * @author github.com/Baksi675
 * @brief I2C driver implementation for STM32F401RE.
 * @version 0.1
 * @date 2025-08-11
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "stm32f401re_i2c.h"
#include "stm32f401re_rcc.h"

/* ---- Forward declaration for internal helper ---- */
static void i2c_set_pclk(I2C_REGDEF_ts const *i2c_instance, EN_STATUS_te en_status);

/**
 * @defgroup stm32_i2c_public_apis I2C Public APIs
 * @{
 */

/** @brief Initializes the I2C peripheral with the given configuration. @see i2c_init */
void i2c_init(I2C_CFG_ts *i2c_cfg) {
    // Enable peripheral clock
    i2c_set_pclk(i2c_cfg->instance, ENABLE);

    // Configure clock stretching (NOSTRETCH bit in CR1)
    i2c_cfg->instance->I2C_CR1 &= ~(0x1 << I2C_CR1_NOSTRETCH);
    i2c_cfg->instance->I2C_CR1 |= (i2c_cfg->clock_strech << I2C_CR1_NOSTRETCH);

    // Configure own 7-bit address in OAR1; bit 14 must be kept set per reference manual
    i2c_cfg->instance->I2C_OAR1 &= ~(0x7F << I2C_OAR1_ADD7_1);
    i2c_cfg->instance->I2C_OAR1 |= ((i2c_cfg->address & 0x7F) << I2C_OAR1_ADD7_1);
    i2c_cfg->instance->I2C_OAR1 |= (0x1 << 14);

    // Set CR2 FREQ field to the APB1 clock frequency in MHz
    uint32_t i2c_clock_hz = rcc_get_apb1_clk();
    i2c_cfg->instance->I2C_CR2 &= ~(0x3F << I2C_CR2_FREQ);
    i2c_cfg->instance->I2C_CR2 |= (((i2c_clock_hz / 1000000) & 0x3F) << I2C_CR2_FREQ);

    // Compute CCR and TRISE based on selected speed
    uint16_t ccr_value = 0;
    uint8_t t_rise = 0;
    if(i2c_cfg->speed == I2C_SPEED_100kHz) {
        // Standard mode (SM): 100 kHz, 50% duty cycle
        // CCR = f_PCLK1 / (2 * f_SCL)
        i2c_cfg->instance->I2C_CCR &= ~(0x1 << I2C_CCR_FS);

        ccr_value = i2c_clock_hz / (2 * 100000);

        // TRISE = (t_rise_max_ns / t_PCLK1_ns) + 1 = (1000 ns / (1/f_PCLK1)) + 1
        t_rise = (i2c_clock_hz / 1000000) + 1;
    }
    else if(i2c_cfg->speed == I2C_SPEED_400kHz) {
        // Fast mode (FM): 400 kHz, DUTY = 1 gives t_low/t_high = 16/9
        // CCR = f_PCLK1 / (25 * f_SCL)
        i2c_cfg->instance->I2C_CCR &= ~(0x1 << I2C_CCR_FS);
        i2c_cfg->instance->I2C_CCR |= (0x1 << I2C_CCR_FS);

        i2c_cfg->instance->I2C_CCR &= ~(0x1 << I2C_CCR_DUTY);
        i2c_cfg->instance->I2C_CCR |= (0x1 << I2C_CCR_DUTY);

        ccr_value = i2c_clock_hz / (25 * 400000);

        // TRISE = (t_rise_max_ns * f_PCLK1 / 1e9) + 1 = (300 ns * f_PCLK1 / 1e9) + 1
        t_rise = ((i2c_clock_hz * 300) / 1000000000) + 1;
    }
    i2c_cfg->instance->I2C_CCR &= ~(0xFFF);
    i2c_cfg->instance->I2C_CCR |= (ccr_value << I2C_CCR_CCR);

    i2c_cfg->instance->I2C_TRISE &= ~(0x3F);
    i2c_cfg->instance->I2C_TRISE |= (t_rise);
}

/** @brief Deinitializes the I2C peripheral and disables its clock. @see i2c_deinit */
void i2c_deinit(I2C_REGDEF_ts const *i2c_instance) {
    if(i2c_instance == I2C1) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_I2C1RST);
    }
    else if(i2c_instance == I2C2) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_I2C2RST);
    }
    else if(i2c_instance == I2C3) {
        rcc_reset_periph_apb1(RCC_APB1RSTR_I2C3RST);
    }

    i2c_set_pclk(i2c_instance, DISABLE);
}

/** @brief Blocking I2C master transmit. @see i2c_master_send */
void i2c_master_send(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *tx_buffer, uint32_t len) {
    uint16_t volatile dummy_read = 0;
    (void)dummy_read;

    // Generate START condition
    i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_START);

    // Wait for SB (start bit) flag; reading SR1 clears SB
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_SB) & 0x1));
    dummy_read = i2c_instance->I2C_SR1;

    // Send 7-bit slave address with write bit (LSB = 0)
    i2c_instance->I2C_DR = (slave_addr << 1);

    // Wait for ADDR flag; reading SR1 then SR2 clears ADDR
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
    dummy_read = i2c_instance->I2C_SR1;
    dummy_read = i2c_instance->I2C_SR2;

    // Transmit data bytes
    while(len != 0) {
        while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
        i2c_instance->I2C_DR = *tx_buffer;
        tx_buffer++;
        len--;
    }

    // Wait for TxE and BTF to confirm the last byte has been shifted out
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_BTF) & 0x1));
}

/** @brief Continues a transmission without a new START or address phase. @see i2c_master_send_continue */
void i2c_master_send_continue(I2C_REGDEF_ts *i2c_instance, uint8_t *tx_buffer, uint32_t len) {
    uint16_t volatile dummy_read = 0;
    (void)dummy_read;

    // Transmit additional data bytes into the open transaction
    while(len != 0) {
        while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
        i2c_instance->I2C_DR = *tx_buffer;
        tx_buffer++;
        len--;
    }

    // Wait for TxE and BTF to confirm the last byte has been shifted out
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_BTF) & 0x1));
}

/** @brief Blocking I2C master receive. @see i2c_master_receive */
void i2c_master_receive(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *rx_buffer, uint32_t len) {
    uint16_t volatile dummy_read = 0;
    (void)dummy_read;

    // Generate START condition
    i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_START);

    // Wait for SB flag; reading SR1 clears SB
    while(!((i2c_instance->I2C_SR1 >> I2C_SR1_SB) & 0x1));
    dummy_read = i2c_instance->I2C_SR1;

    // Send 7-bit slave address with read bit (LSB = 1)
    i2c_instance->I2C_DR = ((slave_addr << 1) | 0x1);

    if(len == 1) {
        // Single-byte reception: disable ACK before clearing ADDR so NACK
        // is sent immediately after the byte is received
        i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_ACK);

        while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
        dummy_read = i2c_instance->I2C_SR1;
        dummy_read = i2c_instance->I2C_SR2;

        while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
        *rx_buffer = i2c_instance->I2C_DR;
    }
    else if(len > 1) {
        // Multi-byte reception: ACK all bytes until the second-to-last
        while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
        dummy_read = i2c_instance->I2C_SR1;
        dummy_read = i2c_instance->I2C_SR2;

        while(len != 0) {
            if(len == 2) {
                // Disable ACK before reading the penultimate byte so NACK
                // is sent after the last byte
                while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
                i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_ACK);

                *rx_buffer = i2c_instance->I2C_DR;
                rx_buffer++;
                len--;
            }
            else {
                while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
                *rx_buffer = i2c_instance->I2C_DR;
                rx_buffer++;
                len--;
            }
        }
    }

    // Re-enable ACK for subsequent transactions
    i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_ACK);
}

/** @brief Enables or disables the I2C peripheral and controls bus ownership. @see i2c_master_set_comm */
void i2c_master_set_comm(I2C_REGDEF_ts *i2c_instance, EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        // Enable peripheral and ACK generation
        i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_PE);
        i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_ACK);
    }
    else if(en_status == DISABLE) {
        // Generate STOP condition, wait for bus to go idle, then disable PE
        i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_STOP);
        while((i2c_instance->I2C_SR2 >> I2C_SR2_BUSY) & 0x01);
        i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_PE);
    }
}

/** @brief Returns the name string of an I2C peripheral instance. @see i2c_get_name */
void i2c_get_name(I2C_REGDEF_ts const *i2c_instance, char *name) {
    const char i2c[] = "I2C";
    uint8_t i2c_len = get_str_len(i2c);
    uint8_t pos_counter = 0;

    while(pos_counter != i2c_len) {
        name[pos_counter] = i2c[pos_counter];
        pos_counter++;
    }

    if(i2c_instance == I2C1)      name[pos_counter] = '1';
    else if(i2c_instance == I2C2) name[pos_counter] = '2';
    else if(i2c_instance == I2C3) name[pos_counter] = '3';
    pos_counter++;

    name[pos_counter] = '\0';
}

/** @} */

/**
 * @defgroup stm32_i2c_internal_helpers I2C Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables the peripheral clock for an I2C instance.
 *
 * @details
 * Routes to the appropriate RCC APB1 clock enable/disable call based on
 * the instance pointer. Called by @ref i2c_init and @ref i2c_deinit.
 *
 * @param[in] i2c_instance Pointer to the I2C peripheral instance.
 * @param[in] en_status    @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
static void i2c_set_pclk(I2C_REGDEF_ts const *i2c_instance, EN_STATUS_te en_status) {
    if(i2c_instance == I2C1) {
        rcc_set_pclk_apb1(RCC_APB1ENR_I2C1EN, en_status);
    }
    else if(i2c_instance == I2C2) {
        rcc_set_pclk_apb1(RCC_APB1ENR_I2C2EN, en_status);
    }
    else if(i2c_instance == I2C3) {
        rcc_set_pclk_apb1(RCC_APB1ENR_I2C3EN, en_status);
    }
}

/** @} */