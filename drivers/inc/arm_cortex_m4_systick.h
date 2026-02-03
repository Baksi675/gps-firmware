/**
 * @file arm_cortex_m4_systick.h
 * @author github.com/Baksi675
 * @brief Header file for Arm Cortex M4 systick.
 * @version 0.1
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef ARM_CORTEX_M4_SYSTICK_H__
#define ARM_CORTEX_M4_SYSTICK_H__

#include <stdint.h>

typedef enum {
	SYSTICK_CLK_SOURCE_EXTERNAL,
	SYSTICK_CLK_SOURCE_PROCESSOR
}SYSTICK_CLK_SOURCE_te;

typedef enum {
	SYSTICK_IT_FALSE,
	SYSTICK_IT_TRUE
}SYSTICK_IT_te;

typedef struct {
	SYSTICK_CLK_SOURCE_te clk_source;
	SYSTICK_IT_te interrupt;
}SYSTICK_CFG_ts;

void systick_init(SYSTICK_CFG_ts const *systick_cfg);
void systick_deinit(void);
void systick_get_def_conf(SYSTICK_CFG_ts *systick_cfg_o);
uint32_t systick_get_ms(void);

#endif