/**
 * @file arm_cortex_m4.h
 * @author github.com/Baksi675
 * @brief Header file for Arm Cortex M4 CPUs.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef ARM_CORTEX_M4_HEADER__
#define ARM_CORTEX_M4_HEADER__

#include <stdint.h>

#define ADDR_NVIC_ISER			(0xE000E100U)
#define ADDR_NVIC_ICER			(0xE000E180U)
#define ADDR_NVIC_ISPR			(0xE000E200U)
#define ADDR_NVIC_ICPR			(0xE000E280U)
#define ADDR_NVIC_IABR			(0xE000E300U)
#define ADDR_NVIC_IPR			(0xE000E400U)
#define ADDR_NVIC_STIR			(0xE000EF00U)

/**
 * @brief NVIC_ISER register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_ISER_[8];
}NVIC_ISER_REGDEF_ts;
#define NVIC_ISER		((NVIC_ISER_REGDEF_ts*)ADDR_NVIC_ISER)

/**
 * @brief NVIC_ICER register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_ICER_[8];
}NVIC_ICER_REGDEF_ts;
#define NVIC_ICER		((NVIC_ICER_REGDEF_ts*)ADDR_NVIC_ICER)

/**
 * @brief NVIC_ISPR register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_ISPR_[8];
}NVIC_ISPR_REGDEF_ts;
#define NVIC_ISPR		((NVIC_ISPR_REGDEF_ts*)ADDR_NVIC_ISPR)

/**
 * @brief NVIC_ICPR register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_ICPR_[8];
}NVIC_ICPR_REGDEF_ts;
#define NVIC_ICPR		((NVIC_ICPR_REGDEF_ts*)ADDR_NVIC_ICPR)

/**
 * @brief NVIC_IABR register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_IABR_[8];
}NVIC_IABR_REGDEF_ts;
#define NVIC_IABR		((NVIC_IABR_REGDEF_ts*)ADDR_NVIC_IABR)

/**
 * @brief NVIC_IPR register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_IPR_[60];
}NVIC_IPR_REGDEF_ts;
#define NVIC_IPR		((NVIC_IPR_REGDEF_ts*)ADDR_NVIC_IPR)

/**
 * @brief NVIC_STIR register definition.
 * 
 */
typedef struct {
	uint32_t volatile NVIC_STIR_;
}NVIC_STIR_REGDEF_ts;
#define NVIC_STIR		((NVIC_STIR_REGDEF_ts*)ADDR_NVIC_STIR)

#endif