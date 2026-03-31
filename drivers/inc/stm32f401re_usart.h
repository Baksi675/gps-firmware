/**
 * @file stm32f401re_usart.h
 * @author github.com/Baksi675
 * @brief STM32F401RE USART driver public API.
 *
 * @details
 * This module provides a driver for the STM32F401RE USART peripheral,
 * supporting asynchronous (UART) mode with configurable baud rate, parity,
 * data width, stop bits, oversampling, hardware flow control, and
 * interrupt-driven reception.
 *
 * Transmission and reception are controlled independently:
 * - Call @ref usart_set_transmission with @ref ENABLE to enable the TX path.
 * - Call @ref usart_set_reception with @ref ENABLE to enable the RX path.
 *
 * Interrupt-driven reception requires the application to override the weak
 * callback functions:
 * - @ref usart1_irq_data_recv_callback — called by USART1_IRQHandler on RXNE
 * - @ref usart6_irq_data_recv_callback — called by USART6_IRQHandler on RXNE
 *
 * If no override is provided, the default weak alias spins in an infinite loop.
 *
 * @note Only RXNE interrupt is currently implemented. TX interrupts and
 *       synchronous mode (@ref USART_MODE_SYNC) are not yet supported.
 *
 * @version 0.1
 * @date 2026-01-22
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32_usart STM32F401RE USART Driver
 * @brief Asynchronous USART driver for the STM32F401RE.
 * @{
 */

#ifndef STM32F401RE_USART_H__
#define STM32F401RE_USART_H__

#include "stm32f401re.h"
#include "common.h"

/**
 * @defgroup stm32_usart_types USART Types
 * @ingroup stm32_usart
 * @brief Configuration enumerations and structures for the USART driver.
 * @{
 */

/** @brief Length of the null-terminated USART peripheral name string (e.g. `"USART1"`). */
#define USART_NAME_LEN 6

/**
 * @brief USART operating mode.
 *
 * @note Synchronous mode (@ref USART_MODE_SYNC) is declared but not yet implemented.
 */
typedef enum {
    USART_MODE_ASYNC, /**< Asynchronous mode (standard UART). */
    USART_MODE_SYNC   /**< Synchronous mode (not yet implemented). */
} USART_MODE_te;

/**
 * @brief USART baud rate in bits per second.
 *
 * @details
 * The BRR register value is computed automatically from the peripheral clock
 * during @ref usart_init. USART1 and USART6 use APB2; USART2 uses APB1.
 */
typedef enum {
    USART_BAUD_RATE_1200    = 1200,
    USART_BAUD_RATE_2400    = 2400,
    USART_BAUD_RATE_9600    = 9600,
    USART_BAUD_RATE_19200   = 19200,
    USART_BAUD_RATE_38400   = 38400,
    USART_BAUD_RATE_57600   = 57600,
    USART_BAUD_RATE_115200  = 115200,
    USART_BAUD_RATE_230400  = 230400,
    USART_BAUD_RATE_460800  = 460800,
    USART_BAUD_RATE_921600  = 921600,
    USART_BAUD_RATE_2000000 = 2000000,
    USART_BAUD_RATE_3000000 = 3000000
} USART_BAUD_RATE_te;

/**
 * @brief USART parity bit configuration.
 */
typedef enum {
    USART_PARITY_DISABLED, /**< No parity bit. */
    USART_PARITY_EVEN,     /**< Even parity. */
    USART_PARITY_ODD,      /**< Odd parity. */
} USART_PARITY_te;

/**
 * @brief USART data word length.
 */
typedef enum {
    USART_FRAME_DATA_BITS_8, /**< 8-bit data word (standard). */
    USART_FRAME_DATA_BITS_9  /**< 9-bit data word. */
} USART_FRAME_DATA_BITS_te;

/**
 * @brief USART oversampling ratio.
 *
 * @details
 * Oversampling by 16 provides better noise immunity; oversampling by 8
 * allows higher baud rates at a given peripheral clock frequency.
 */
typedef enum {
    USART_OVERSAMPLING_16, /**< Oversample by 16 (OVER8 = 0). Better noise immunity. */
    USART_OVERSAMPLING_8   /**< Oversample by 8 (OVER8 = 1). Allows higher baud rates. */
} USART_OVERSAMPLING_te;

/**
 * @brief USART number of stop bits.
 */
typedef enum {
    USART_FRAME_STOP_BITS_1,   /**< 1 stop bit (standard). */
    USART_FRAME_STOP_BITS_0_5, /**< 0.5 stop bits (smartcard mode). */
    USART_FRAME_STOP_BITS_2,   /**< 2 stop bits. */
    USART_FRAME_STOP_BITS_1_5  /**< 1.5 stop bits (smartcard mode). */
} USART_FRAME_STOP_BITS_te;

/**
 * @brief USART hardware flow control (RTS/CTS).
 */
typedef enum {
    USART_HW_FLOW_CONTROL_DISABLED, /**< No hardware flow control. */
    USART_HW_FLOW_CONTROL_ENABLED   /**< RTS and CTS hardware flow control enabled. */
} USART_HW_FLOW_CONTROL_te;

/**
 * @brief USART receiver sample bit method.
 */
typedef enum {
    USART_SAMPLE_BIT_3, /**< Three-sample bit method (majority vote, better noise immunity). */
    USART_SAMPLE_BIT_1  /**< One-sample bit method (faster, less noise tolerant). */
} USART_SAMPLE_BIT_te;

/**
 * @brief USART RXNE interrupt enable configuration.
 */
typedef enum {
    USART_INTERRUPT_EN_FALSE, /**< RXNE interrupt disabled; use polling via @ref usart_receive. */
    USART_INTERRUPT_EN_TRUE   /**< RXNE interrupt enabled; received bytes delivered via callback. */
} USART_INTERRUPT_EN_te;

/**
 * @brief Configuration structure for initializing a USART peripheral.
 *
 * @details
 * Passed to @ref usart_init. It is recommended to zero-initialize this
 * structure before use. Default values when zero-initialized:
 * - Mode: asynchronous, baud rate: 1200, parity: disabled
 * - Data: 8-bit, oversampling: ×16, stop bits: 1
 * - HW flow control: disabled, sample: 3-bit, interrupt: disabled
 */
typedef struct {
    /** Pointer to the USART peripheral instance (USART1, USART2, or USART6). */
    USART_REGDEF_ts *instance;

    /** Operating mode. */
    USART_MODE_te mode;

    /** Baud rate in bits per second. */
    USART_BAUD_RATE_te baud_rate;

    /** Parity bit configuration. */
    USART_PARITY_te parity;

    /** Data word length. */
    USART_FRAME_DATA_BITS_te frame_data_bits;

    /** Oversampling ratio (affects BRR computation). */
    USART_OVERSAMPLING_te oversampling;

    /** Number of stop bits. */
    USART_FRAME_STOP_BITS_te frame_stop_bits;

    /** Hardware flow control (RTS/CTS). */
    USART_HW_FLOW_CONTROL_te hw_flow_control;

    /** Receiver sample bit method. */
    USART_SAMPLE_BIT_te sample_bit;

    /** Whether to enable the RXNE interrupt and configure the NVIC. */
    USART_INTERRUPT_EN_te interrupt_en;
} USART_CFG_ts;

/** @} */

/**
 * @defgroup stm32_usart_api USART Public API
 * @ingroup stm32_usart
 * @brief Public functions to interact with the USART peripheral.
 * @{
 */

/**
 * @brief Initializes the USART peripheral with the given configuration.
 *
 * @details
 * Enables the peripheral clock, configures the frame format, baud rate,
 * oversampling, parity, hardware flow control, sample method, and (if
 * requested) the RXNE interrupt and NVIC line. Enables the peripheral
 * (UE = 1) at the end. TX and RX must be enabled separately via
 * @ref usart_set_transmission and @ref usart_set_reception.
 *
 * @param[in] usart_cfg Pointer to the USART configuration structure.
 */
void usart_init(USART_CFG_ts *usart_cfg);

/**
 * @brief Deinitializes the USART peripheral, disables its NVIC interrupt, and disables its clock.
 *
 * @param[in] usart_instance Pointer to the USART instance to deinitialize.
 */
void usart_deinit(USART_REGDEF_ts const *usart_instance);

/**
 * @brief Blocking USART transmit. Sends @p len bytes from @p tx_buffer.
 *
 * @details
 * Polls TXE before writing each byte, then waits for TC (transmission
 * complete) after the last byte before returning.
 *
 * @param[in] usart_instance Pointer to the USART peripheral instance.
 * @param[in] tx_buffer      Pointer to the transmit data buffer.
 * @param[in] len            Number of bytes to transmit.
 *
 * @note Transmission must be enabled via @ref usart_set_transmission before
 *       calling this function.
 */
void usart_send(USART_REGDEF_ts *usart_instance, uint8_t *tx_buffer, uint32_t len);

/**
 * @brief Blocking USART receive. Reads @p len bytes into @p rx_buffer.
 *
 * @details
 * Polls RXNE before reading each byte from the data register. Do not use
 * alongside interrupt-driven reception on the same instance, as both
 * compete to read USART_DR.
 *
 * @param[in]  usart_instance Pointer to the USART peripheral instance.
 * @param[out] rx_buffer      Pointer to the receive data buffer.
 * @param[in]  len            Number of bytes to receive.
 */
void usart_receive(USART_REGDEF_ts const *usart_instance, uint8_t *rx_buffer, uint32_t len);

/**
 * @brief Enables or disables the USART transmitter (TE bit).
 *
 * @param[in] usart_instance Pointer to the USART peripheral instance.
 * @param[in] en_status      @ref ENABLE to enable TX, @ref DISABLE to disable it.
 */
void usart_set_transmission(USART_REGDEF_ts *usart_instance, EN_STATUS_te en_status);

/**
 * @brief Enables or disables the USART receiver (RE bit).
 *
 * @param[in] usart_instance Pointer to the USART peripheral instance.
 * @param[in] en_status      @ref ENABLE to enable RX, @ref DISABLE to disable it.
 */
void usart_set_reception(USART_REGDEF_ts *usart_instance, EN_STATUS_te en_status);

/**
 * @brief Returns the name string of a USART peripheral instance (e.g. `"USART1"`).
 *
 * @details
 * The caller must ensure @p name points to a buffer of at least
 * @ref USART_NAME_LEN + 1 bytes.
 *
 * @param[in]  usart_instance Pointer to the USART peripheral instance.
 * @param[out] name           Pointer to the destination buffer.
 */
void usart_get_name(USART_REGDEF_ts const *usart_instance, char *name);

/**
 * @brief RXNE callback for USART1. Called by USART1_IRQHandler on each received byte.
 *
 * @details
 * Defined as a weak alias. Override in application code to handle received
 * bytes (e.g. write to a circular buffer as done in `console.c`).
 * The default implementation spins in an infinite loop.
 *
 * @note Must not be called directly from application code.
 *
 * @param[in] data The byte received from USART1_DR.
 */
void usart1_irq_data_recv_callback(uint8_t data);

/**
 * @brief RXNE callback for USART6. Called by USART6_IRQHandler on each received byte.
 *
 * @details
 * Defined as a weak alias. Override in application code to handle received
 * bytes (e.g. write to a circular buffer as done in `neo6.c`).
 * The default implementation spins in an infinite loop.
 *
 * @note Must not be called directly from application code.
 *
 * @param[in] data The byte received from USART6_DR.
 */
void usart6_irq_data_recv_callback(uint8_t data);

/** @} */

#endif

/** @} */