/**
 * @file arm_cortex_m4_nvic.h
 * @author github.com/Baksi675
 * @brief Header file for Arm Cortex M4 nested vectored interrupt controller.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef ARM_CORTEX_M4_NVIC_H__
#define ARM_CORTEX_M4_NVIC_H__

#include "stm32f401re.h"
#include "common.h"

void nvic_set_interrupt(IRQn_te interrupt_line, EN_STATUS_te en_status);

#endif