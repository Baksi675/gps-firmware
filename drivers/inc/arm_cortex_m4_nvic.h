/**
 * @file arm_cortex_m4_nvic.h
 * @author github.com/Baksi675
 * @brief Arm Cortex-M4 NVIC driver public API.
 *
 * @details
 * This header exposes a minimal software interface to the Cortex-M4
 * Nested Vectored Interrupt Controller (NVIC), providing enable and
 * disable control over individual interrupt lines.
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup cortex_m4_nvic_api Cortex-M4 NVIC API
 * @ingroup cortex_m4_core
 * @brief Public API for enabling and disabling NVIC interrupt lines.
 * @{
 */

#ifndef ARM_CORTEX_M4_NVIC_H__
#define ARM_CORTEX_M4_NVIC_H__

#include "stm32f401re.h"
#include "common.h"

/**
 * @brief Enables or disables an interrupt line in the NVIC.
 *
 * @details
 * Writes to the appropriate NVIC_ISER register bit to enable the interrupt,
 * or to the NVIC_ICER register bit to disable it. The correct register word
 * is selected automatically based on the interrupt line number.
 *
 * Supports interrupt lines 0–287 (register words 0–7).
 *
 * @param[in] interrupt_line The interrupt line number to configure (@ref IRQn_te).
 * @param[in] en_status      @ref ENABLE to enable the interrupt line,
 *                           @ref DISABLE to disable it.
 *
 * @note Writing 0 to ISER or ICER bits has no effect; only setting bits
 *       takes action, so no read-modify-write is required.
 */
void nvic_set_interrupt(IRQn_te interrupt_line, EN_STATUS_te en_status);

#endif

/** @} */