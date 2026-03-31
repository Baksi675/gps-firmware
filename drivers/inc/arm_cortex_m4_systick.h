/**
 * @file arm_cortex_m4_systick.h
 * @author github.com/Baksi675
 * @brief Arm Cortex-M4 SysTick driver public API.
 *
 * @details
 * This module provides a millisecond-resolution tick counter backed by the
 * Cortex-M4 SysTick timer. The counter is incremented in the SysTick exception
 * handler and is accessible via @ref systick_get_ms.
 *
 * The reload value is computed automatically from the AHB clock frequency
 * to produce a 1 ms tick period regardless of the system clock speed.
 *
 * Typical usage:
 * - Call @ref systick_get_def_cfg to obtain a default configuration
 * - Call @ref systick_init to start the timer
 * - Call @ref systick_get_ms anywhere in application code for timestamps
 *   and non-blocking delay calculations
 *
 * @version 0.1
 * @date 2026-02-01
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup cortex_m4_systick SysTick Driver
 * @ingroup cortex_m4_core
 * @brief Millisecond tick counter driven by the Cortex-M4 SysTick timer.
 * @{
 */

#ifndef ARM_CORTEX_M4_SYSTICK_H__
#define ARM_CORTEX_M4_SYSTICK_H__

#include <stdint.h>
#include "err.h"

/**
 * @defgroup cortex_m4_systick_types SysTick Types
 * @ingroup cortex_m4_systick
 * @brief Configuration types for the SysTick driver.
 * @{
 */

/**
 * @brief SysTick clock source selection.
 */
typedef enum {
    SYSTICK_CLK_SOURCE_EXTERNAL,  /**< Use the external reference clock (SYST_CSR CLKSOURCE = 0). */
    SYSTICK_CLK_SOURCE_PROCESSOR  /**< Use the processor (AHB) clock (SYST_CSR CLKSOURCE = 1). */
} SYSTICK_CLK_SOURCE_te;

/**
 * @brief SysTick interrupt enable selection.
 */
typedef enum {
    SYSTICK_IT_FALSE, /**< Do not generate a SysTick exception on counter wrap. */
    SYSTICK_IT_TRUE   /**< Generate a SysTick exception on counter wrap (required for @ref systick_get_ms). */
} SYSTICK_IT_te;

/**
 * @brief Configuration structure for initializing the SysTick timer.
 */
typedef struct {
    /** Clock source for the SysTick counter. */
    SYSTICK_CLK_SOURCE_te clk_source;

    /**
     * Whether to generate a SysTick exception on each 1 ms wrap.
     * Must be @ref SYSTICK_IT_TRUE for @ref systick_get_ms to advance.
     */
    SYSTICK_IT_te interrupt;
} SYSTICK_CFG_ts;

/** @} */

/**
 * @defgroup cortex_m4_systick_api SysTick Public API
 * @ingroup cortex_m4_systick
 * @brief Public functions to interact with the SysTick driver.
 * @{
 */

/**
 * @brief Initializes and starts the SysTick timer.
 *
 * @details
 * Computes the reload value from the current AHB clock frequency to achieve
 * a 1 ms tick period, configures the clock source and interrupt settings,
 * clears the current value register, and enables the counter.
 *
 * @param[in] systick_cfg Pointer to the SysTick configuration structure.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if SysTick is already initialized
 * - ERR_UNKNOWN if the computed reload value is zero or exceeds the 24-bit
 *   maximum (0xFFFFFF), indicating an unsupported clock frequency
 *
 * @note Typically called indirectly via @ref init_systick.
 */
ERR_te systick_init(SYSTICK_CFG_ts const *systick_cfg);

/**
 * @brief Deinitializes the SysTick timer.
 *
 * @details
 * Disables the counter, clears the interrupt and clock source configuration,
 * resets the reload and current value registers, and resets the internal state.
 * Does nothing if SysTick has not been initialized.
 */
void systick_deinit(void);

/**
 * @brief Populates a configuration structure with the default SysTick settings.
 *
 * @details
 * Default configuration:
 * - Clock source: processor (AHB) clock
 * - Interrupt: enabled
 *
 * @param[out] systick_cfg_o Pointer to the configuration structure to populate.
 */
void systick_get_def_cfg(SYSTICK_CFG_ts *systick_cfg_o);

/**
 * @brief Returns the number of milliseconds elapsed since SysTick was initialized.
 *
 * @details
 * The counter is incremented by the SysTick exception handler on each 1 ms wrap.
 * Returns 0 if SysTick has not been initialized.
 *
 * @return Elapsed time in milliseconds since @ref systick_init was called.
 *
 * @note The counter is a 32-bit unsigned value and will wrap after approximately
 *       49.7 days of continuous operation.
 */
uint32_t systick_get_ms(void);

/** @} */

#endif

/** @} */