/**
 * @file arm_cortex_m4_systick.c
 * @author github.com/Baksi675
 * @brief Implementation file for Arm Cortex M4 systick.
 * @version 0.1
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "arm_cortex_m4_systick.h"
#include "arm_cortex_m4.h"
#include "stm32f401re_rcc.h"

struct internal_state_s {
	uint32_t volatile elapsed_ms;
	bool initialized;
};
static struct internal_state_s internal_state;

/**
 * @brief Initializes the systick to the given configuration.
 * 
 * @param[in] systick_cfg Systick configuration structure. 
 */
void systick_init(SYSTICK_CFG_ts const *systick_cfg) {
	if(internal_state.initialized) {
		return;
	} 

	// Disable systick for safety
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_ENABLE);

	// Clear internal state
	internal_state = (struct internal_state_s){ 0 };

	// Clear and set interrupt config
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_TICKINT);
	SYSTICK->SYST_CSR |= (systick_cfg->interrupt << SYST_CSR_TICKINT);

	// Clear and Set clock source config
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_CLKSOURCE);
	SYSTICK->SYST_CSR |= (systick_cfg->clk_source << SYST_CSR_CLKSOURCE);

	// Clear current value
	SYSTICK->SYST_CVR = 0;

	uint32_t cpu_clk = rcc_get_ahb_clk();
	uint32_t one_ms_clk_cycle = cpu_clk / 1000;
	uint32_t reload_value = one_ms_clk_cycle * 1;
	if (reload_value == 0 || reload_value > 0x01000000U) {
		return;
	}

	// Clear RVR and set value
	SYSTICK->SYST_RVR &= ~(0xFFFFFF);
	SYSTICK->SYST_RVR |= (reload_value - 1);

	// Enable systick
	SYSTICK->SYST_CSR |= (1U << SYST_CSR_ENABLE);

	internal_state.initialized = true;
}

/**
 * @brief Deinitializes the systick
 * 
 */
void systick_deinit(void) {
	if(!internal_state.initialized) {
		return;
	}

	// Disable systick
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_ENABLE);

	// Clear internal state
	internal_state = (struct internal_state_s){ 0 };

	// Clear interrupt configuration
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_TICKINT);

	// Clear clock source configuration
	SYSTICK->SYST_CSR &= ~(1U << SYST_CSR_CLKSOURCE);

	// Clear current value
	SYSTICK->SYST_CVR = 0;

	// Clear reload value configuration
	SYSTICK->SYST_RVR &= ~(0xFFFFFF);
}

/**
 * @brief Sets the default configuration for a configuration structure.
 * 
 * @param[in] systick_cfg The systick configuration structure. 
 */
void systick_get_def_conf(SYSTICK_CFG_ts *systick_cfg_o) {
	systick_cfg_o->clk_source = SYSTICK_CLK_SOURCE_PROCESSOR;
	systick_cfg_o->interrupt = SYSTICK_IT_TRUE;
}

/**
 * @brief Returns the elapsed milliseconds since initialization.
 * 
 * @return uint32_t Elapsed milliseconds.
 */
uint32_t systick_get_ms(void) {
	if(!internal_state.initialized) {
		return 0;
	}
	
	return internal_state.elapsed_ms;
}

void SysTick_Handler(void) {
	internal_state.elapsed_ms++;
}

