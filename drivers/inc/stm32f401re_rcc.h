#ifndef STM32F401RE_RCC_H__
#define STM32F401RE_RCC_H__

#include "stm32f401re.h"
#include "common.h"

uint32_t rcc_get_sysclk(void);
uint32_t rcc_get_ahb_clk(void);
uint32_t rcc_get_apb1_clk(void);
uint32_t rcc_get_apb2_clk(void);
void rcc_set_pclk_ahb1(RCC_AHB1ENR_te periph_position, EN_STATUS_te en_status);
void rcc_set_pclk_apb1(RCC_APB1ENR_te periph_position, EN_STATUS_te en_status);
void rcc_set_pclk_apb2(RCC_APB2ENR_te periph_position, EN_STATUS_te en_status);
void rcc_reset_periph_ahb1(RCC_AHB1RSTR_te periph_position);
void rcc_reset_periph_apb1(RCC_APB1RSTR_te periph_position);
void rcc_reset_periph_apb2(RCC_APB2RSTR_te periph_position);

#endif