/**
 * @file stm32f401re_i2c.h
 * @author github.com/Baksi675
 * @brief STM32F401RE I2C driver public API.
 *
 * @details
 * This module provides a blocking master-mode I2C driver for the STM32F401RE.
 * It supports standard mode (100 kHz) and fast mode (400 kHz) and handles
 * peripheral clock management internally.
 *
 * The driver separates bus ownership from peripheral initialization:
 * - @ref i2c_init configures the peripheral registers but does not enable it.
 * - @ref i2c_master_set_comm enables the peripheral and asserts the bus (ENABLE)
 *   or generates a STOP condition and releases the bus (DISABLE).
 *
 * All transfer functions are blocking — they poll status flags and return
 * only after the transfer completes.
 *
 * Typical usage:
 * - Populate an @ref I2C_CFG_ts and call @ref i2c_init
 * - Call @ref i2c_master_set_comm with @ref ENABLE to take the bus
 * - Call @ref i2c_master_send or @ref i2c_master_receive
 * - Call @ref i2c_master_set_comm with @ref DISABLE to release the bus
 *
 * @version 0.1
 * @date 2026-01-30
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32_i2c STM32F401RE I2C Driver
 * @brief Blocking master-mode I2C driver for the STM32F401RE.
 * @{
 */

#ifndef STM32F401RE_I2C_H__
#define STM32F401RE_I2C_H__

#include "stm32f401re.h"
#include "common.h"

/** @brief Length of the null-terminated I2C peripheral name string (e.g. `"I2C1"`). */
#define I2C_NAME_LEN 4

/**
 * @defgroup stm32_i2c_types I2C Types
 * @ingroup stm32_i2c
 * @brief Configuration enumerations and structures for the I2C driver.
 * @{
 */

/**
 * @brief I2C slave clock stretching enable/disable.
 *
 * @details
 * When enabled, the slave may hold SCL low to pause the master and buy time
 * to prepare data. When disabled, clock stretching is not permitted.
 */
typedef enum {
    I2C_CLOCK_STRETCH_ON,  /**< Allow slave clock stretching (NOSTRETCH = 0). */
    I2C_CLOCK_STRETCH_OFF  /**< Disable slave clock stretching (NOSTRETCH = 1). */
} I2C_CLOCK_STRECH_te;

/**
 * @brief I2C acknowledge control.
 */
typedef enum {
    I2C_ACK_OFF, /**< Disable ACK generation after each received byte. */
    I2C_ACK_ON   /**< Enable ACK generation after each received byte. */
} I2C_ACK_te;

/**
 * @brief I2C bus speed (SCL frequency).
 *
 * @details
 * The CCR and TRISE registers are computed automatically from the APB1
 * peripheral clock during @ref i2c_init based on the selected speed.
 */
typedef enum {
    I2C_SPEED_100kHz = 100000, /**< Standard mode: 100 kHz, 50% duty cycle. */
    I2C_SPEED_400kHz = 400000  /**< Fast mode: 400 kHz, 9/16 duty cycle (DUTY = 1). */
} I2C_SPEED_te;

/**
 * @brief Configuration structure for initializing an I2C peripheral.
 *
 * @details
 * Passed to @ref i2c_init. The GPIO pins must be configured separately
 * before calling @ref i2c_init.
 */
typedef struct {
    /** Pointer to the I2C peripheral instance (I2C1, I2C2, or I2C3). */
    I2C_REGDEF_ts *instance;

    /** Slave clock stretching configuration. */
    I2C_CLOCK_STRECH_te clock_strech;

    /**
     * Own 7-bit I2C address (used when operating as a slave).
     * Set to 0 if the peripheral is used in master-only mode.
     */
    uint8_t address;

    /** SCL bus speed. Determines CCR and TRISE register values. */
    I2C_SPEED_te speed;
} I2C_CFG_ts;

/** @} */

/**
 * @defgroup stm32_i2c_api I2C Public API
 * @ingroup stm32_i2c
 * @brief Public functions to interact with the I2C peripheral.
 * @{
 */

/**
 * @brief Initializes the I2C peripheral with the given configuration.
 *
 * @details
 * Enables the peripheral clock, configures clock stretching, own address,
 * APB1 frequency, CCR, and TRISE. Does not enable the peripheral for
 * communication; call @ref i2c_master_set_comm with @ref ENABLE to do so.
 *
 * Default values if fields are zero-initialized:
 * - Clock stretching: enabled
 * - Own address: 0
 * - Speed: 100 kHz
 *
 * @param[in] i2c_cfg Pointer to the I2C configuration structure.
 *
 * @note It is recommended to zero-initialize @ref I2C_CFG_ts before use
 *       to avoid unintended behaviour from uninitialised fields.
 */
void i2c_init(I2C_CFG_ts *i2c_cfg);

/**
 * @brief Deinitializes the I2C peripheral and disables its clock.
 *
 * @details
 * Triggers an RCC peripheral reset for the given instance and then
 * disables the peripheral clock.
 *
 * @param[in] i2c_instance Pointer to the I2C instance to deinitialize.
 */
void i2c_deinit(I2C_REGDEF_ts const *i2c_instance);

/**
 * @brief Blocking I2C master transmit. Sends a start condition, address, and data.
 *
 * @details
 * Generates a START condition, sends the 7-bit slave address with the
 * write bit, then transmits @p len bytes from @p tx_buffer. Waits for
 * BTF (byte transfer finished) after the last byte before returning.
 * Does not generate a STOP condition — call @ref i2c_master_set_comm
 * with @ref DISABLE afterwards to release the bus.
 *
 * @param[in] i2c_instance Pointer to the I2C peripheral instance.
 * @param[in] slave_addr   7-bit slave address (right-aligned, without the R/W bit).
 * @param[in] tx_buffer    Pointer to the data buffer to transmit.
 * @param[in] len          Number of bytes to transmit.
 *
 * @note The peripheral must be enabled via @ref i2c_master_set_comm before
 *       calling this function.
 */
void i2c_master_send(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *tx_buffer, uint32_t len);

/**
 * @brief Continues a transmission started by @ref i2c_master_send without a new START or address phase.
 *
 * @details
 * Transmits @p len additional bytes into an already-open I2C transaction.
 * Useful for sending a command byte followed by a payload in a single bus
 * transaction (e.g. SSD1309 framebuffer transfer).
 *
 * @param[in] i2c_instance Pointer to the I2C peripheral instance.
 * @param[in] tx_buffer    Pointer to the additional data buffer to transmit.
 * @param[in] len          Number of bytes to transmit.
 */
void i2c_master_send_continue(I2C_REGDEF_ts *i2c_instance, uint8_t *tx_buffer, uint32_t len);

/**
 * @brief Blocking I2C master receive. Sends a start condition, address, and reads data.
 *
 * @details
 * Generates a START condition, sends the 7-bit slave address with the read
 * bit, then receives @p len bytes into @p rx_buffer. ACK/NACK is managed
 * automatically: NACK is sent before the last byte to signal end of reception,
 * then ACK is re-enabled before returning.
 *
 * @param[in]  i2c_instance Pointer to the I2C peripheral instance.
 * @param[in]  slave_addr   7-bit slave address (right-aligned, without the R/W bit).
 * @param[out] rx_buffer    Pointer to the buffer that will receive the data.
 * @param[in]  len          Number of bytes to receive.
 *
 * @note The peripheral must be enabled via @ref i2c_master_set_comm before
 *       calling this function.
 */
void i2c_master_receive(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *rx_buffer, uint32_t len);

/**
 * @brief Enables or disables the I2C peripheral and controls bus ownership.
 *
 * @details
 * - **ENABLE**: sets the PE bit, enabling the peripheral, and enables ACK
 *   generation. Must be called before any transfer function.
 * - **DISABLE**: generates a STOP condition, waits for the bus to become
 *   idle (BUSY = 0), then clears PE to release the bus. Must be called
 *   after each complete transaction.
 *
 * Between ENABLE and DISABLE calls the bus is held and cannot be taken
 * by another master.
 *
 * @param[in] i2c_instance Pointer to the I2C peripheral instance.
 * @param[in] en_status    @ref ENABLE to take the bus, @ref DISABLE to release it.
 */
void i2c_master_set_comm(I2C_REGDEF_ts *i2c_instance, EN_STATUS_te en_status);

/**
 * @brief Returns the name string of an I2C peripheral instance (e.g. `"I2C1"`).
 *
 * @details
 * Writes a null-terminated string of the form `"I2Cx"` into @p name.
 * The caller must ensure @p name points to a buffer of at least
 * @ref I2C_NAME_LEN + 1 bytes.
 *
 * @param[in]  i2c_instance Pointer to the I2C peripheral instance.
 * @param[out] name         Pointer to the destination buffer.
 */
void i2c_get_name(I2C_REGDEF_ts const *i2c_instance, char *name);

/** @} */

#endif

/** @} */