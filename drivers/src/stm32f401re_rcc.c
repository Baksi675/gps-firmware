/**
 * @file stm32f401re_rcc.c
 * @author github.com/Baksi675
 * @brief RCC driver implementation for STM32F401RE.
 * @version 0.1
 * @date 2025-08-14
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "stm32f401re_rcc.h"
#include "stm32f401re.h"

/**
 * @defgroup stm32_rcc_public_apis RCC Public APIs
 * @{
 */

/** @brief Returns the current system clock frequency in Hz. @see rcc_get_sysclk */
uint32_t rcc_get_sysclk(void) {
    uint8_t rcc_cfgr_sws = ((RCC->RCC_CFGR >> RCC_CFGR_SWS) & 0x3);

    switch(rcc_cfgr_sws) {
        case 0:
            return 16000000; // HSI
        case 1:
            return 8000000;  // HSE
        /* case 2: PLL — not yet implemented */
    }

    return 0;
}

/** @brief Returns the current AHB bus clock frequency in Hz. @see rcc_get_ahb_clk */
uint32_t rcc_get_ahb_clk(void) {
    uint32_t system_clock = rcc_get_sysclk();
    uint8_t rcc_cfgr_hpre = ((RCC->RCC_CFGR >> RCC_CFGR_HPRE) & 0xF);
    uint16_t ahb_division_factor;

    if(rcc_cfgr_hpre <= 7) {
        ahb_division_factor = 1;
    }
    else if(rcc_cfgr_hpre <= 11) {
        ahb_division_factor = 0x1 << (rcc_cfgr_hpre - 7);
    }
    else {
        ahb_division_factor = 0x1 << (rcc_cfgr_hpre - 6);
    }

    return system_clock / ahb_division_factor;
}

/** @brief Returns the current APB1 peripheral bus clock frequency in Hz. @see rcc_get_apb1_clk */
uint32_t rcc_get_apb1_clk(void) {
    uint32_t ahb_clock = rcc_get_ahb_clk();
    uint8_t rcc_cfgr_ppre1 = ((RCC->RCC_CFGR >> RCC_CFGR_PPRE1) & 0x7);
    uint8_t apb1_division_factor;

    if(rcc_cfgr_ppre1 < 4) {
        apb1_division_factor = 1;
    }
    else {
        apb1_division_factor = 0x1 << (rcc_cfgr_ppre1 - 3);
    }

    return ahb_clock / apb1_division_factor;
}

/** @brief Returns the current APB2 peripheral bus clock frequency in Hz. @see rcc_get_apb2_clk */
uint32_t rcc_get_apb2_clk(void) {
    uint32_t ahb_clock = rcc_get_ahb_clk();
    uint8_t rcc_cfgr_ppre2 = ((RCC->RCC_CFGR >> RCC_CFGR_PPRE2) & 0x7);
    uint8_t apb2_division_factor;

    if(rcc_cfgr_ppre2 < 4) {
        apb2_division_factor = 1;
    }
    else {
        apb2_division_factor = 0x1 << (rcc_cfgr_ppre2 - 3);
    }

    return ahb_clock / apb2_division_factor;
}

/** @brief Enables or disables the peripheral clock for an AHB1 peripheral. @see rcc_set_pclk_ahb1 */
void rcc_set_pclk_ahb1(RCC_AHB1ENR_te periph_position, EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        RCC->RCC_AHB1ENR |= (0x1 << periph_position);
    }
    else if(en_status == DISABLE) {
        RCC->RCC_AHB1ENR &= ~(0x1 << periph_position);
    }
}

/** @brief Enables or disables the peripheral clock for an APB1 peripheral. @see rcc_set_pclk_apb1 */
void rcc_set_pclk_apb1(RCC_APB1ENR_te periph_position, EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        RCC->RCC_APB1ENR |= (0x1 << periph_position);
    }
    else if(en_status == DISABLE) {
        RCC->RCC_APB1ENR &= ~(0x1 << periph_position);
    }
}

/** @brief Enables or disables the peripheral clock for an APB2 peripheral. @see rcc_set_pclk_apb2 */
void rcc_set_pclk_apb2(RCC_APB2ENR_te periph_position, EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        RCC->RCC_APB2ENR |= (0x1 << periph_position);
    }
    else if(en_status == DISABLE) {
        RCC->RCC_APB2ENR &= ~(0x1 << periph_position);
    }
}

/** @brief Resets an AHB1 peripheral via RCC_AHB1RSTR. @see rcc_reset_periph_ahb1 */
void rcc_reset_periph_ahb1(RCC_AHB1RSTR_te periph_position) {
    RCC->RCC_AHB1RSTR |= (0x1 << periph_position);
    RCC->RCC_AHB1RSTR &= ~(0x1 << periph_position);
}

/** @brief Resets an APB1 peripheral via RCC_APB1RSTR. @see rcc_reset_periph_apb1 */
void rcc_reset_periph_apb1(RCC_APB1RSTR_te periph_position) {
    RCC->RCC_APB1RSTR |= (0x1 << periph_position);
    RCC->RCC_APB1RSTR &= ~(0x1 << periph_position);
}

/** @brief Resets an APB2 peripheral via RCC_APB2RSTR. @see rcc_reset_periph_apb2 */
void rcc_reset_periph_apb2(RCC_APB2RSTR_te periph_position) {
    RCC->RCC_APB2RSTR |= (0x1 << periph_position);
    RCC->RCC_APB2RSTR &= ~(0x1 << periph_position);
}

/** @brief Resets the backup domain. @see rcc_reset_bkpd */
void rcc_reset_bkpd(void) {
    RCC->RCC_BDCR |= (0x1 << RCC_BDCR_BDRST);
    RCC->RCC_BDCR &= ~(0x1 << RCC_BDCR_BDRST);
}

/** @} */