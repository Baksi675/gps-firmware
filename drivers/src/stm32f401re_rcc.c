/**
 * @file stm32f401re_rcc.c
 * @author Márk Bakulár (github.com/Baksi675)
 * @brief RCC driver implementation for STM32F401RE
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "stm32f401re_rcc.h"

/** 
 * @defgroup RCC_Public_APIs RCC Public APIs
 * @{
 */

/**
 * @brief Obtains then returns the current system clock in Hz.
 * 
 * @return uint32_t The system clock in Hz.
 */
uint32_t rcc_get_sysclk(void) {
	uint8_t rcc_cfgr_sws = ((RCC->RCC_CFGR >> RCC_CFGR_SWS) & 0x3);

	switch(rcc_cfgr_sws) {
		case 0:
			// HSI is the source
			return 16000000;
		case 1:
			// HSE is the source
			return 8000000;
		/*case 2:
			// PLL is the source --> not yet implemented
			break;
		*/
	}

	return 0;
}

/**
 * @brief Obtains then returns the current clock of the AHB in Hz.
 * 
 * @return uint32_t The AHB clock in Hz.
 */
uint32_t rcc_get_ahb_clk(void) {
	uint32_t system_clock = rcc_get_sysclk();
	uint8_t rcc_cfgr_hpre = ((RCC->RCC_CFGR >> RCC_CFGR_HPRE) & 0xF);
	uint16_t ahb_division_factor;
	uint32_t ahb_clock;

	if(rcc_cfgr_hpre <= 7) {
		ahb_division_factor = 1;
	}
	else if (rcc_cfgr_hpre <= 11 && rcc_cfgr_hpre >= 8) {
		ahb_division_factor = 0x1 << (rcc_cfgr_hpre - 7);
	}
	else {
		ahb_division_factor = 0x1 << (rcc_cfgr_hpre - 6);
	}

	ahb_clock = system_clock / ahb_division_factor;

	return ahb_clock;
}

/**
 * @brief Obtains then returns the current clock of the APB1 in Hz.
 * 
 * @return uint32_t The APB1 clock in Hz.
 */
uint32_t rcc_get_apb1_clk(void) {
	uint32_t ahb_clock = rcc_get_ahb_clk();
	uint8_t rcc_cfgr_ppre1 = ((RCC->RCC_CFGR >> RCC_CFGR_PPRE1) & 0x7);
	uint8_t apb1_division_factor;
	uint32_t apb1_clock;

	if(rcc_cfgr_ppre1 < 4) {
		apb1_division_factor = 1;
	}
	else {
		apb1_division_factor = 0x1 << (rcc_cfgr_ppre1 - 3);
	}

	apb1_clock = ahb_clock / apb1_division_factor;

	return apb1_clock;
}

/**
 * @brief Obtains then returns the current clock of the APB2 in Hz.
 * 
 * @return uint32_t The APB2 clock in Hz.
 */
uint32_t rcc_get_apb2_clk(void) {
	uint32_t ahb_clock = rcc_get_ahb_clk();
	uint8_t rcc_cfgr_ppre2 = ((RCC->RCC_CFGR >> RCC_CFGR_PPRE2) & 0x7);
	uint8_t apb2_division_factor;
	uint32_t apb2_clock;

	if(rcc_cfgr_ppre2 < 4) {
		apb2_division_factor = 1;
	}
	else {
		apb2_division_factor = 0x1 << (rcc_cfgr_ppre2 - 3);
	}

	apb2_clock = ahb_clock / apb2_division_factor;

	return apb2_clock;
}

/**
 * @brief Enables or disables the peripheral clock of the given peripheral on the AHB1.
 * 
 * @param periph_position The bit position in the RCC_AHB1ENR register. 
 * @param en_status Whether to enable or disable the peripheral clock.
 */
void rcc_set_pclk_ahb1(RCC_AHB1ENR_te periph_position, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		RCC->RCC_AHB1ENR |= (0x1 << periph_position);
	}
	else if(en_status == DISABLE) {
		RCC->RCC_AHB1ENR &= ~(0x1 << periph_position);
	}
}

/**
 * @brief Enables or disables the peripheral clock of the given peripheral on the APB1.
 * 
 * @param periph_position The bit position in the RCC_APB1ENR register. 
 * @param en_status Whether to enable or disable the peripheral clock.
 */
void rcc_set_pclk_apb1(RCC_APB1ENR_te periph_position, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		RCC->RCC_APB1ENR |= (0x1 << periph_position);
	}
	else if(en_status == DISABLE) {
		RCC->RCC_APB1ENR &= ~(0x1 << periph_position);
	}
}

/**
 * @brief Enables or disables the peripheral clock of the given peripheral on the APB2.
 * 
 * @param periph_position The bit position in the RCC_APB2ENR register. 
 * @param en_status Whether to enable or disable the peripheral clock.
 */
void rcc_set_pclk_apb2(RCC_APB2ENR_te periph_position, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		RCC->RCC_APB2ENR |= (0x1 << periph_position);
	}
	else if(en_status == DISABLE) {
		RCC->RCC_APB2ENR &= ~(0x1 << periph_position);
	}
}

/**
 * @brief Resets the given peripheral on the AHB1.
 * 
 * @param periph_position The bit position in the RCC_AHB1RSTR register. 
 */
void rcc_reset_periph_ahb1(RCC_AHB1RSTR_te periph_position) {
	RCC->RCC_AHB1RSTR |= (0x1 << periph_position);
	RCC->RCC_AHB1RSTR &= ~(0x1 << periph_position);
}

/**
 * @brief Resets the given peripheral on the APB1.
 * 
 * @param periph_position The bit position in the RCC_APB1RSTR register. 
 */
void rcc_reset_periph_apb1(RCC_APB1RSTR_te periph_position) {
	RCC->RCC_APB1RSTR |= (0x1 << periph_position);
	RCC->RCC_APB1RSTR &= ~(0x1 << periph_position);
}

/**
 * @brief Resets the given peripheral on the APB2.
 * 
 * @param periph_position The bit position in the RCC_APB2RSTR register. 
 */
void rcc_reset_periph_apb2(RCC_APB2RSTR_te periph_position) {
	RCC->RCC_APB2RSTR |= (0x1 << periph_position);
	RCC->RCC_APB2RSTR &= ~(0x1 << periph_position);
}

/** @} */