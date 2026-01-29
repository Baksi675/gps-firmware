/**
 * @file cortex_m4_nvic.c
 * @author github.com/Baksi675
 * @brief Arm Cortex M4 nested vectored interrupt controller implementation
 * @version 0.1
 * @date 2025-11-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "arm_cortex_m4_nvic.h"
#include "arm_cortex_m4.h"
#include "common.h"
#include "stm32f401re.h"

 /** 
 * @defgroup CORTEX_M4_NVIC_Public_APIs CORTEX_M4_NVIC Public APIs
 * @{
 */

/**
 * @brief Enables or disables the interrupt line in the NVIC of the given interrupt.
 * 
 * @param interrupt_line The interrupt line to enable in the NVIC.
 * @param en_status Whether to enable or disable the interrupt line in the NVIC.
 */
void nvic_set_interrupt(IRQn_te interrupt_line, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		if(interrupt_line < 32) {
			NVIC_ISER->NVIC_ISER_[0] |= 0x1 << interrupt_line;
		}
		else if(interrupt_line < 64) {
			NVIC_ISER->NVIC_ISER_[1] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 63 && interrupt_line < 96) {
			NVIC_ISER->NVIC_ISER_[2] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 95 && interrupt_line < 128) {
			NVIC_ISER->NVIC_ISER_[3] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 127 && interrupt_line < 160) {
			NVIC_ISER->NVIC_ISER_[3] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 159 && interrupt_line < 192) {
			NVIC_ISER->NVIC_ISER_[4] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 191 && interrupt_line < 224) {
			NVIC_ISER->NVIC_ISER_[5] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 223 && interrupt_line < 256) {
			NVIC_ISER->NVIC_ISER_[6] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 255 && interrupt_line < 288) {
			NVIC_ISER->NVIC_ISER_[7] |= 0x1 << (interrupt_line % 32);
		}
	}
	else if(en_status == DISABLE) {
		if(interrupt_line < 32) {
			NVIC_ICER->NVIC_ICER_[0] |= 0x1 << interrupt_line;
		}
		else if(interrupt_line < 64) {
			NVIC_ICER->NVIC_ICER_[1] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 63 && interrupt_line < 96) {
			NVIC_ICER->NVIC_ICER_[2] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 95 && interrupt_line < 128) {
			NVIC_ICER->NVIC_ICER_[3] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 127 && interrupt_line < 160) {
			NVIC_ICER->NVIC_ICER_[3] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 159 && interrupt_line < 192) {
			NVIC_ICER->NVIC_ICER_[4] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 191 && interrupt_line < 224) {
			NVIC_ICER->NVIC_ICER_[5] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 223 && interrupt_line < 256) {
			NVIC_ICER->NVIC_ICER_[6] |= 0x1 << (interrupt_line % 32);
		}
		else if(interrupt_line > 255 && interrupt_line < 288) {
			NVIC_ICER->NVIC_ICER_[7] |= 0x1 << (interrupt_line % 32);
		}
	}
}

  /** @} */