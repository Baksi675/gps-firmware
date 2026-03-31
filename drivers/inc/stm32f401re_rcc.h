/**
 * @file stm32f401re_rcc.h
 * @author github.com/Baksi675
 * @brief STM32F401RE RCC driver public API.
 *
 * @details
 * This module provides a software interface to the STM32F401RE Reset and
 * Clock Control (RCC) peripheral, covering:
 * - Reading the current system, AHB, APB1, and APB2 clock frequencies
 * - Enabling and disabling peripheral clocks on AHB1, APB1, and APB2
 * - Resetting peripherals on AHB1, APB1, and APB2 via the reset registers
 * - Resetting the backup domain
 *
 * Clock frequency functions read the current CFGR prescaler fields at
 * call time, so they always reflect the live clock tree configuration.
 *
 * @note PLL as system clock source is not yet implemented. Only HSI (16 MHz)
 *       and HSE (8 MHz) are currently supported by @ref rcc_get_sysclk.
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32_rcc STM32F401RE RCC Driver
 * @brief Clock control and peripheral reset driver for the STM32F401RE.
 * @{
 */

#ifndef STM32F401RE_RCC_H__
#define STM32F401RE_RCC_H__

#include "stm32f401re.h"
#include "common.h"

/**
 * @defgroup stm32_rcc_api RCC Public API
 * @ingroup stm32_rcc
 * @brief Public functions to interact with the RCC peripheral.
 * @{
 */

/**
 * @brief Returns the current system clock frequency in Hz.
 *
 * @details
 * Reads the SWS (system clock switch status) field in RCC_CFGR to determine
 * the active clock source:
 * - SWS = 0: HSI (16 MHz)
 * - SWS = 1: HSE (8 MHz)
 * - SWS = 2: PLL (not yet implemented, returns 0)
 *
 * @return System clock frequency in Hz, or 0 if the source is unrecognized.
 */
uint32_t rcc_get_sysclk(void);

/**
 * @brief Returns the current AHB bus clock frequency in Hz.
 *
 * @details
 * Reads the HPRE prescaler field from RCC_CFGR and divides the system
 * clock accordingly. The prescaler values map as follows:
 * - HPRE ≤ 7: divide by 1 (no prescaling)
 * - HPRE 8–11: divide by 2, 4, 8, 16
 * - HPRE 12–15: divide by 64, 128, 256, 512
 *
 * @return AHB clock frequency in Hz.
 */
uint32_t rcc_get_ahb_clk(void);

/**
 * @brief Returns the current APB1 (low-speed) peripheral bus clock frequency in Hz.
 *
 * @details
 * Reads the PPRE1 prescaler field from RCC_CFGR and divides the AHB clock
 * accordingly. PPRE1 < 4 maps to no prescaling; PPRE1 ≥ 4 maps to
 * divide by 2^(PPRE1 - 3).
 *
 * @return APB1 clock frequency in Hz.
 */
uint32_t rcc_get_apb1_clk(void);

/**
 * @brief Returns the current APB2 (high-speed) peripheral bus clock frequency in Hz.
 *
 * @details
 * Reads the PPRE2 prescaler field from RCC_CFGR and divides the AHB clock
 * accordingly. Same mapping as @ref rcc_get_apb1_clk but using PPRE2.
 *
 * @return APB2 clock frequency in Hz.
 */
uint32_t rcc_get_apb2_clk(void);

/**
 * @brief Enables or disables the peripheral clock for an AHB1 peripheral.
 *
 * @param[in] periph_position Bit position in RCC_AHB1ENR for the target peripheral.
 * @param[in] en_status       @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
void rcc_set_pclk_ahb1(RCC_AHB1ENR_te periph_position, EN_STATUS_te en_status);

/**
 * @brief Enables or disables the peripheral clock for an APB1 peripheral.
 *
 * @param[in] periph_position Bit position in RCC_APB1ENR for the target peripheral.
 * @param[in] en_status       @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
void rcc_set_pclk_apb1(RCC_APB1ENR_te periph_position, EN_STATUS_te en_status);

/**
 * @brief Enables or disables the peripheral clock for an APB2 peripheral.
 *
 * @param[in] periph_position Bit position in RCC_APB2ENR for the target peripheral.
 * @param[in] en_status       @ref ENABLE to enable the clock, @ref DISABLE to disable it.
 */
void rcc_set_pclk_apb2(RCC_APB2ENR_te periph_position, EN_STATUS_te en_status);

/**
 * @brief Resets an AHB1 peripheral via RCC_AHB1RSTR.
 *
 * @details
 * Sets the reset bit then clears it, triggering a peripheral reset pulse.
 *
 * @param[in] periph_position Bit position in RCC_AHB1RSTR for the target peripheral.
 */
void rcc_reset_periph_ahb1(RCC_AHB1RSTR_te periph_position);

/**
 * @brief Resets an APB1 peripheral via RCC_APB1RSTR.
 *
 * @details
 * Sets the reset bit then clears it, triggering a peripheral reset pulse.
 *
 * @param[in] periph_position Bit position in RCC_APB1RSTR for the target peripheral.
 */
void rcc_reset_periph_apb1(RCC_APB1RSTR_te periph_position);

/**
 * @brief Resets an APB2 peripheral via RCC_APB2RSTR.
 *
 * @details
 * Sets the reset bit then clears it, triggering a peripheral reset pulse.
 *
 * @param[in] periph_position Bit position in RCC_APB2RSTR for the target peripheral.
 */
void rcc_reset_periph_apb2(RCC_APB2RSTR_te periph_position);

/**
 * @brief Resets the backup domain (RTC, backup registers, backup SRAM).
 *
 * @details
 * Sets then clears the BDRST bit in RCC_BDCR, resetting all backup domain
 * logic including the RTC, backup registers, and backup SRAM controller.
 *
 * @note This also clears the RTC clock source selection. Re-configure the
 *       RTC clock source after calling this function.
 */
void rcc_reset_bkpd(void);

/** @} */

#endif

/** @} */