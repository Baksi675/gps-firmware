/**
 * @file arm_cortex_m4.h
 * @author github.com/Baksi675
 * @brief Register definitions and bit position enumerations for the Arm Cortex-M4 core peripherals.
 *
 * @details
 * This header provides memory-mapped register definitions and bit position
 * enumerations for the following Cortex-M4 core peripherals:
 * - **NVIC** (Nested Vectored Interrupt Controller): ISER, ICER, ISPR, ICPR, IABR, IPR, STIR
 * - **SysTick**: CSR, RVR, CVR, CALIB
 *
 * All register structs are accessed via pointer macros cast to their fixed
 * memory-mapped addresses as defined in the ARMv7-M Architecture Reference Manual.
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup cortex_m4_core Cortex-M4 Core Peripherals
 * @brief Register definitions for NVIC and SysTick core peripherals.
 * @{
 */

#ifndef ARM_CORTEX_M4_HEADER__
#define ARM_CORTEX_M4_HEADER__

#include <stdint.h>

/**
 * @defgroup cortex_m4_addresses Cortex-M4 Peripheral Base Addresses
 * @ingroup cortex_m4_core
 * @brief Fixed memory-mapped base addresses for Cortex-M4 core peripherals.
 * @{
 */
#define ADDR_NVIC_ISER  (0xE000E100U) /**< NVIC Interrupt Set-Enable Registers base address. */
#define ADDR_NVIC_ICER  (0xE000E180U) /**< NVIC Interrupt Clear-Enable Registers base address. */
#define ADDR_NVIC_ISPR  (0xE000E200U) /**< NVIC Interrupt Set-Pending Registers base address. */
#define ADDR_NVIC_ICPR  (0xE000E280U) /**< NVIC Interrupt Clear-Pending Registers base address. */
#define ADDR_NVIC_IABR  (0xE000E300U) /**< NVIC Interrupt Active Bit Registers base address. */
#define ADDR_NVIC_IPR   (0xE000E400U) /**< NVIC Interrupt Priority Registers base address. */
#define ADDR_NVIC_STIR  (0xE000EF00U) /**< NVIC Software Trigger Interrupt Register address. */
#define ADDR_SYSTICK    (0xE000E010U) /**< SysTick control registers base address. */
/** @} */

/**
 * @defgroup cortex_m4_nvic_regs NVIC Register Definitions
 * @ingroup cortex_m4_core
 * @brief Memory-mapped register structures for the NVIC.
 * @{
 */

/**
 * @brief NVIC Interrupt Set-Enable Registers (NVIC_ISER).
 *
 * @details
 * Writing 1 to a bit enables the corresponding interrupt. Writing 0 has no effect.
 * Eight 32-bit registers cover up to 256 external interrupt lines.
 */
typedef struct {
    uint32_t volatile NVIC_ISER_[8]; /**< ISER[0]–ISER[7]: each bit enables one interrupt line. */
} NVIC_ISER_REGDEF_ts;
#define NVIC_ISER   ((NVIC_ISER_REGDEF_ts*)ADDR_NVIC_ISER)

/**
 * @brief NVIC Interrupt Clear-Enable Registers (NVIC_ICER).
 *
 * @details
 * Writing 1 to a bit disables the corresponding interrupt. Writing 0 has no effect.
 */
typedef struct {
    uint32_t volatile NVIC_ICER_[8]; /**< ICER[0]–ICER[7]: each bit disables one interrupt line. */
} NVIC_ICER_REGDEF_ts;
#define NVIC_ICER   ((NVIC_ICER_REGDEF_ts*)ADDR_NVIC_ICER)

/**
 * @brief NVIC Interrupt Set-Pending Registers (NVIC_ISPR).
 *
 * @details
 * Writing 1 to a bit sets the corresponding interrupt to pending state.
 */
typedef struct {
    uint32_t volatile NVIC_ISPR_[8]; /**< ISPR[0]–ISPR[7]: each bit sets one interrupt pending. */
} NVIC_ISPR_REGDEF_ts;
#define NVIC_ISPR   ((NVIC_ISPR_REGDEF_ts*)ADDR_NVIC_ISPR)

/**
 * @brief NVIC Interrupt Clear-Pending Registers (NVIC_ICPR).
 *
 * @details
 * Writing 1 to a bit clears the pending state of the corresponding interrupt.
 */
typedef struct {
    uint32_t volatile NVIC_ICPR_[8]; /**< ICPR[0]–ICPR[7]: each bit clears one interrupt pending. */
} NVIC_ICPR_REGDEF_ts;
#define NVIC_ICPR   ((NVIC_ICPR_REGDEF_ts*)ADDR_NVIC_ICPR)

/**
 * @brief NVIC Interrupt Active Bit Registers (NVIC_IABR).
 *
 * @details
 * Read-only. A set bit indicates the corresponding interrupt is currently active
 * (being serviced by the processor or preempted).
 */
typedef struct {
    uint32_t volatile NVIC_IABR_[8]; /**< IABR[0]–IABR[7]: each bit reflects one interrupt active state. */
} NVIC_IABR_REGDEF_ts;
#define NVIC_IABR   ((NVIC_IABR_REGDEF_ts*)ADDR_NVIC_IABR)

/**
 * @brief NVIC Interrupt Priority Registers (NVIC_IPR).
 *
 * @details
 * Each byte sets the priority of one interrupt. On the Cortex-M4 only the
 * upper bits of each priority byte are implemented (implementation-defined width).
 * 60 registers cover up to 240 external interrupt lines (4 per register).
 */
typedef struct {
    uint32_t volatile NVIC_IPR_[60]; /**< IPR[0]–IPR[59]: 4 interrupt priorities per register. */
} NVIC_IPR_REGDEF_ts;
#define NVIC_IPR    ((NVIC_IPR_REGDEF_ts*)ADDR_NVIC_IPR)

/**
 * @brief NVIC Software Trigger Interrupt Register (NVIC_STIR).
 *
 * @details
 * Writing an interrupt number to this register generates that interrupt as
 * if it were triggered by hardware. Requires the USERSETMPEND bit in CCR to
 * be set for unprivileged access.
 */
typedef struct {
    uint32_t volatile NVIC_STIR_; /**< STIR: write an IRQ number to trigger that interrupt in software. */
} NVIC_STIR_REGDEF_ts;
#define NVIC_STIR   ((NVIC_STIR_REGDEF_ts*)ADDR_NVIC_STIR)

/** @} */

/**
 * @defgroup cortex_m4_systick_regs SysTick Register Definitions
 * @ingroup cortex_m4_core
 * @brief Memory-mapped register structure and bit positions for the SysTick timer.
 * @{
 */

/**
 * @brief SysTick control and status register block.
 *
 * @details
 * The SysTick timer is a 24-bit decrementing counter used to generate
 * periodic interrupts for OS tick generation and general-purpose timing.
 */
typedef struct {
    uint32_t volatile SYST_CSR;   /**< Control and Status Register: enable, interrupt, clock source, count flag. */
    uint32_t volatile SYST_RVR;   /**< Reload Value Register: value loaded into CVR on counter wrap. */
    uint32_t volatile SYST_CVR;   /**< Current Value Register: current counter value; write any value to clear. */
    uint32_t volatile SYST_CALIB; /**< Calibration Value Register: 10 ms reference value, skew, and no-ref flags. */
} SYST_REGDEF_ts;
#define SYSTICK     ((SYST_REGDEF_ts*)ADDR_SYSTICK)

/**
 * @brief Bit positions within the SysTick Control and Status Register (SYST_CSR).
 */
typedef enum {
    SYST_CSR_ENABLE    = 0,  /**< Bit 0: Enable the SysTick counter (1 = enabled, 0 = disabled). */
    SYST_CSR_TICKINT   = 1,  /**< Bit 1: Enable SysTick exception request (1 = assert on count to 0). */
    SYST_CSR_CLKSOURCE = 2,  /**< Bit 2: Clock source select (0 = external reference, 1 = processor clock). */
    SYST_CSR_COUNTFLAG = 16  /**< Bit 16: Set to 1 when counter reaches 0 since last read; cleared on read. */
} SYST_CSR_te;

/**
 * @brief Bit positions within the SysTick Reload Value Register (SYST_RVR).
 */
typedef enum {
    SYST_RVR_RELOAD = 0 /**< Bits 23:0: Value to reload into CVR on counter wrap-around. */
} SYST_RVR_te;

/**
 * @brief Bit positions within the SysTick Current Value Register (SYST_CVR).
 */
typedef enum {
    SYST_CVR_CURRENT = 0 /**< Bits 23:0: Current counter value. Writing any value clears both CVR and COUNTFLAG. */
} SYST_CVR_te;

/**
 * @brief Bit positions within the SysTick Calibration Value Register (SYST_CALIB).
 */
typedef enum {
    SYST_CALIB_TENMS = 0,  /**< Bits 23:0: Optionally-implemented reload value for a 10 ms tick period. */
    SYST_CALIB_SKEW  = 30, /**< Bit 30: Set if the 10 ms calibration value is inexact. */
    SYST_CALIB_NOREF = 31  /**< Bit 31: Set if no external reference clock is provided. */
} SYST_CALIB_te;

/** @} */

#endif

/** @} */