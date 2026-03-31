/**
 * @file arm_cortex_m4_systick.c
 * @author github.com/Baksi675
 * @brief Arm Cortex-M4 SysTick driver implementation.
 * @version 0.1
 * @date 2026-02-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "arm_cortex_m4_systick.h"
#include "arm_cortex_m4.h"
#include "err.h"
#include "stm32f401re_rcc.h"

/**
 * @brief Internal state of the SysTick driver.
 *
 * @details
 * Holds the millisecond counter incremented by @ref SysTick_Handler and
 * an initialization guard to prevent double-initialization.
 */
struct internal_state_s {
    /** Milliseconds elapsed since @ref systick_init was called. Incremented by the SysTick ISR. */
    uint32_t volatile elapsed_ms;

    /** True after @ref systick_init has completed successfully. */
    bool initialized;
};

/** @brief Singleton instance of the SysTick driver internal state. */
static struct internal_state_s internal_state;

/**
 * @defgroup cortex_m4_systick_public_apis SysTick Public APIs
 * @{
 */

/** @brief Initializes and starts the SysTick timer. @see systick_init */
ERR_te systick_init(SYSTICK_CFG_ts const *systick_cfg) {
    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    // Disable SysTick before reconfiguring
    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_ENABLE);

    // Clear internal state
    internal_state = (struct internal_state_s){ 0 };

    // Configure interrupt enable
    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_TICKINT);
    SYSTICK->SYST_CSR |= (systick_cfg->interrupt << SYST_CSR_TICKINT);

    // Configure clock source
    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_CLKSOURCE);
    SYSTICK->SYST_CSR |= (systick_cfg->clk_source << SYST_CSR_CLKSOURCE);

    // Clear current value
    SYSTICK->SYST_CVR = 0;

    // Compute reload value for a 1 ms tick period from the AHB clock
    uint32_t cpu_clk = rcc_get_ahb_clk();
    uint32_t one_ms_clk_cycle = cpu_clk / 1000;
    uint32_t reload_value = one_ms_clk_cycle + 1;
    if (reload_value == 0 || reload_value > 0x01000000U) {
        return ERR_UNKNOWN;
    }

    // Set reload value (24-bit register)
    SYSTICK->SYST_RVR &= ~(0xFFFFFF);
    SYSTICK->SYST_RVR |= (reload_value - 1);

    // Enable SysTick
    SYSTICK->SYST_CSR |= (1U << SYST_CSR_ENABLE);

    internal_state.initialized = true;

    return ERR_OK;
}

/** @brief Deinitializes the SysTick timer. @see systick_deinit */
void systick_deinit(void) {
    if(!internal_state.initialized) {
        return;
    }

    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_ENABLE);

    internal_state = (struct internal_state_s){ 0 };

    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_TICKINT);
    SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_CLKSOURCE);

    SYSTICK->SYST_CVR = 0;

    SYSTICK->SYST_RVR &= ~(0xFFFFFF);
}

/** @brief Populates a configuration structure with the default SysTick settings. @see systick_get_def_cfg */
void systick_get_def_cfg(SYSTICK_CFG_ts *systick_cfg_o) {
    systick_cfg_o->clk_source = SYSTICK_CLK_SOURCE_PROCESSOR;
    systick_cfg_o->interrupt = SYSTICK_IT_TRUE;
}

/** @brief Returns the number of milliseconds elapsed since SysTick was initialized. @see systick_get_ms */
uint32_t systick_get_ms(void) {
    if(!internal_state.initialized) {
        return 0;
    }

    return internal_state.elapsed_ms;
}

/**
 * @brief SysTick exception handler. Increments the millisecond counter on each 1 ms wrap.
 *
 * @details
 * Called automatically by the Cortex-M4 exception mechanism every time the
 * SysTick counter reaches zero. Must not be called directly from application code.
 */
void SysTick_Handler(void) {
    internal_state.elapsed_ms++;
}

/** @} */