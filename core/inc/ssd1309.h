/**
 * @file ssd1309.h
 * @author github.com/Baksi675
 * @brief SSD1309 OLED display driver public API.
 *
 * @details
 * This module provides an I2C-based driver for the Solomon Systech SSD1309
 * 128×64 OLED display controller. It maintains an internal framebuffer and
 * exposes high-level drawing operations that are flushed to the display
 * on demand via @ref ssd1309_update.
 *
 * The display is organized as 8 pages of 8 rows each (64 rows total) by
 * 128 columns. All drawing functions operate on the framebuffer in RAM;
 * changes are not visible until @ref ssd1309_update is called.
 *
 * Coordinates used by drawing functions are 1-based pixel coordinates:
 * - X: 1–128 (left to right)
 * - Y: 1–64 (top to bottom)
 * - Line: 1–8 (one 8-pixel-tall character row per line)
 *
 * A `force` parameter on each drawing function bypasses the initialized /
 * started guard, allowing calls from CLI command handlers before the
 * subsystem is fully started.
 *
 * Typical usage:
 * - Call @ref ssd1309_get_def_cfg to obtain a sensible default configuration
 * - Adjust any fields in @ref SSD1309_CFG_ts as needed
 * - Initialize the subsystem with @ref ssd1309_init_subsys
 * - Initialize the display with @ref ssd1309_init_handle
 * - Start the subsystem with @ref ssd1309_start_subsys
 * - Draw content using @ref ssd1309_draw_text, @ref ssd1309_draw_rect, etc.
 * - Flush to the display with @ref ssd1309_update
 *
 * @version 0.1
 * @date 2026-01-31
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup ssd1309_module SSD1309 Display Module
 * @brief I2C driver for the SSD1309 128×64 OLED display controller.
 * @{
 */

#ifndef SSD1309_H__
#define SSD1309_H__

#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "err.h"

/**
 * @defgroup ssd1309_macros SSD1309 Display Dimensions
 * @ingroup ssd1309_module
 * @brief Physical and logical dimension constants for the SSD1309.
 * @{
 */

#define SSD1309_WIDTH            128 /**< Display width in pixels. */
#define SSD1309_HEIGHT            64 /**< Display height in pixels. */
#define SSD1309_CHAR_WIDTH         8 /**< Width of one character glyph in pixels. */
#define SSD1309_CHAR_HEIGHT        8 /**< Height of one character glyph in pixels. */
#define SSD1309_MAX_CHARS_IN_LINE 16 /**< Maximum number of characters that fit on one display line. */

/** @} */

/**
 * @defgroup ssd1309_types SSD1309 Types
 * @ingroup ssd1309_module
 * @brief Configuration enumerations and data types for the SSD1309 driver.
 * @{
 */

/**
 * @brief Lower column start address (lower 4 bit part of 8 bit address) for RAM scanning of the SSD1309. 
 * Only relevant for PAM.
 * 
 */
typedef enum {
	SSD1309_LOW_COL_START_ADDR_PAM_0,
	SSD1309_LOW_COL_START_ADDR_PAM_1,
	SSD1309_LOW_COL_START_ADDR_PAM_2,
	SSD1309_LOW_COL_START_ADDR_PAM_3,
	SSD1309_LOW_COL_START_ADDR_PAM_4,
	SSD1309_LOW_COL_START_ADDR_PAM_5,
	SSD1309_LOW_COL_START_ADDR_PAM_6,
	SSD1309_LOW_COL_START_ADDR_PAM_7,
	SSD1309_LOW_COL_START_ADDR_PAM_8,
	SSD1309_LOW_COL_START_ADDR_PAM_9,
	SSD1309_LOW_COL_START_ADDR_PAM_10,
	SSD1309_LOW_COL_START_ADDR_PAM_11,
	SSD1309_LOW_COL_START_ADDR_PAM_12,
	SSD1309_LOW_COL_START_ADDR_PAM_13,
	SSD1309_LOW_COL_START_ADDR_PAM_14,
	SSD1309_LOW_COL_START_ADDR_PAM_15
}SSD1309_LOW_COL_START_ADDR_PAM_te;

/**
 * @brief Higher column start address (upper 4 bit part of 8 bit address) for RAM scanning of the SSD1309.  
 * Only relevant for PAM.
 * SSD1309 is 128 pixels wide, thus only 3 bits are allowed in the higher nibble.
 */
 typedef enum {
	SSD1309_HIGH_COL_START_ADDR_PAM_0,
	SSD1309_HIGH_COL_START_ADDR_PAM_1,
	SSD1309_HIGH_COL_START_ADDR_PAM_2,
	SSD1309_HIGH_COL_START_ADDR_PAM_3,
	SSD1309_HIGH_COL_START_ADDR_PAM_4,
	SSD1309_HIGH_COL_START_ADDR_PAM_5,
	SSD1309_HIGH_COL_START_ADDR_PAM_6,
	SSD1309_HIGH_COL_START_ADDR_PAM_7,
}SSD1309_HIGH_COL_START_ADDR_PAM_te;

/**
 * @brief Memory addressing mode:
 * - PAM (Page Addressing Mode)
 * - HAM (Horizontal Addressing Mode)
 * - VAM (Vertical Addressing Mode)
 */
typedef enum {
	SSD1309_MEM_ADDR_MODE_HAM,
	SSD1309_MEM_ADDR_MODE_VAM,
	SSD1309_MEM_ADDR_MODE_PAM
}SSD1309_MEM_ADDR_MODE_te;

/**
 * @brief Column start address for RAM scanning of the SSD1309.
 * Only relevant for HAM and VAM.
 */
typedef enum {
    SSD1309_COL_ADDR_START_HAM_VAM_0,
    SSD1309_COL_ADDR_START_HAM_VAM_1,
    SSD1309_COL_ADDR_START_HAM_VAM_2,
    SSD1309_COL_ADDR_START_HAM_VAM_3,
    SSD1309_COL_ADDR_START_HAM_VAM_4,
    SSD1309_COL_ADDR_START_HAM_VAM_5,
    SSD1309_COL_ADDR_START_HAM_VAM_6,
    SSD1309_COL_ADDR_START_HAM_VAM_7,
    SSD1309_COL_ADDR_START_HAM_VAM_8,
    SSD1309_COL_ADDR_START_HAM_VAM_9,
    SSD1309_COL_ADDR_START_HAM_VAM_10,
    SSD1309_COL_ADDR_START_HAM_VAM_11,
    SSD1309_COL_ADDR_START_HAM_VAM_12,
    SSD1309_COL_ADDR_START_HAM_VAM_13,
    SSD1309_COL_ADDR_START_HAM_VAM_14,
    SSD1309_COL_ADDR_START_HAM_VAM_15,
    SSD1309_COL_ADDR_START_HAM_VAM_16,
    SSD1309_COL_ADDR_START_HAM_VAM_17,
    SSD1309_COL_ADDR_START_HAM_VAM_18,
    SSD1309_COL_ADDR_START_HAM_VAM_19,
    SSD1309_COL_ADDR_START_HAM_VAM_20,
    SSD1309_COL_ADDR_START_HAM_VAM_21,
    SSD1309_COL_ADDR_START_HAM_VAM_22,
    SSD1309_COL_ADDR_START_HAM_VAM_23,
    SSD1309_COL_ADDR_START_HAM_VAM_24,
    SSD1309_COL_ADDR_START_HAM_VAM_25,
    SSD1309_COL_ADDR_START_HAM_VAM_26,
    SSD1309_COL_ADDR_START_HAM_VAM_27,
    SSD1309_COL_ADDR_START_HAM_VAM_28,
    SSD1309_COL_ADDR_START_HAM_VAM_29,
    SSD1309_COL_ADDR_START_HAM_VAM_30,
    SSD1309_COL_ADDR_START_HAM_VAM_31,
    SSD1309_COL_ADDR_START_HAM_VAM_32,
    SSD1309_COL_ADDR_START_HAM_VAM_33,
    SSD1309_COL_ADDR_START_HAM_VAM_34,
    SSD1309_COL_ADDR_START_HAM_VAM_35,
    SSD1309_COL_ADDR_START_HAM_VAM_36,
    SSD1309_COL_ADDR_START_HAM_VAM_37,
    SSD1309_COL_ADDR_START_HAM_VAM_38,
    SSD1309_COL_ADDR_START_HAM_VAM_39,
    SSD1309_COL_ADDR_START_HAM_VAM_40,
    SSD1309_COL_ADDR_START_HAM_VAM_41,
    SSD1309_COL_ADDR_START_HAM_VAM_42,
    SSD1309_COL_ADDR_START_HAM_VAM_43,
    SSD1309_COL_ADDR_START_HAM_VAM_44,
    SSD1309_COL_ADDR_START_HAM_VAM_45,
    SSD1309_COL_ADDR_START_HAM_VAM_46,
    SSD1309_COL_ADDR_START_HAM_VAM_47,
    SSD1309_COL_ADDR_START_HAM_VAM_48,
    SSD1309_COL_ADDR_START_HAM_VAM_49,
    SSD1309_COL_ADDR_START_HAM_VAM_50,
    SSD1309_COL_ADDR_START_HAM_VAM_51,
    SSD1309_COL_ADDR_START_HAM_VAM_52,
    SSD1309_COL_ADDR_START_HAM_VAM_53,
    SSD1309_COL_ADDR_START_HAM_VAM_54,
    SSD1309_COL_ADDR_START_HAM_VAM_55,
    SSD1309_COL_ADDR_START_HAM_VAM_56,
    SSD1309_COL_ADDR_START_HAM_VAM_57,
    SSD1309_COL_ADDR_START_HAM_VAM_58,
    SSD1309_COL_ADDR_START_HAM_VAM_59,
    SSD1309_COL_ADDR_START_HAM_VAM_60,
    SSD1309_COL_ADDR_START_HAM_VAM_61,
    SSD1309_COL_ADDR_START_HAM_VAM_62,
    SSD1309_COL_ADDR_START_HAM_VAM_63,
    SSD1309_COL_ADDR_START_HAM_VAM_64,
    SSD1309_COL_ADDR_START_HAM_VAM_65,
    SSD1309_COL_ADDR_START_HAM_VAM_66,
    SSD1309_COL_ADDR_START_HAM_VAM_67,
    SSD1309_COL_ADDR_START_HAM_VAM_68,
    SSD1309_COL_ADDR_START_HAM_VAM_69,
    SSD1309_COL_ADDR_START_HAM_VAM_70,
    SSD1309_COL_ADDR_START_HAM_VAM_71,
    SSD1309_COL_ADDR_START_HAM_VAM_72,
    SSD1309_COL_ADDR_START_HAM_VAM_73,
    SSD1309_COL_ADDR_START_HAM_VAM_74,
    SSD1309_COL_ADDR_START_HAM_VAM_75,
    SSD1309_COL_ADDR_START_HAM_VAM_76,
    SSD1309_COL_ADDR_START_HAM_VAM_77,
    SSD1309_COL_ADDR_START_HAM_VAM_78,
    SSD1309_COL_ADDR_START_HAM_VAM_79,
    SSD1309_COL_ADDR_START_HAM_VAM_80,
    SSD1309_COL_ADDR_START_HAM_VAM_81,
    SSD1309_COL_ADDR_START_HAM_VAM_82,
    SSD1309_COL_ADDR_START_HAM_VAM_83,
    SSD1309_COL_ADDR_START_HAM_VAM_84,
    SSD1309_COL_ADDR_START_HAM_VAM_85,
    SSD1309_COL_ADDR_START_HAM_VAM_86,
    SSD1309_COL_ADDR_START_HAM_VAM_87,
    SSD1309_COL_ADDR_START_HAM_VAM_88,
    SSD1309_COL_ADDR_START_HAM_VAM_89,
    SSD1309_COL_ADDR_START_HAM_VAM_90,
    SSD1309_COL_ADDR_START_HAM_VAM_91,
    SSD1309_COL_ADDR_START_HAM_VAM_92,
    SSD1309_COL_ADDR_START_HAM_VAM_93,
    SSD1309_COL_ADDR_START_HAM_VAM_94,
    SSD1309_COL_ADDR_START_HAM_VAM_95,
    SSD1309_COL_ADDR_START_HAM_VAM_96,
    SSD1309_COL_ADDR_START_HAM_VAM_97,
    SSD1309_COL_ADDR_START_HAM_VAM_98,
    SSD1309_COL_ADDR_START_HAM_VAM_99,
    SSD1309_COL_ADDR_START_HAM_VAM_100,
    SSD1309_COL_ADDR_START_HAM_VAM_101,
    SSD1309_COL_ADDR_START_HAM_VAM_102,
    SSD1309_COL_ADDR_START_HAM_VAM_103,
    SSD1309_COL_ADDR_START_HAM_VAM_104,
    SSD1309_COL_ADDR_START_HAM_VAM_105,
    SSD1309_COL_ADDR_START_HAM_VAM_106,
    SSD1309_COL_ADDR_START_HAM_VAM_107,
    SSD1309_COL_ADDR_START_HAM_VAM_108,
    SSD1309_COL_ADDR_START_HAM_VAM_109,
    SSD1309_COL_ADDR_START_HAM_VAM_110,
    SSD1309_COL_ADDR_START_HAM_VAM_111,
    SSD1309_COL_ADDR_START_HAM_VAM_112,
    SSD1309_COL_ADDR_START_HAM_VAM_113,
    SSD1309_COL_ADDR_START_HAM_VAM_114,
    SSD1309_COL_ADDR_START_HAM_VAM_115,
    SSD1309_COL_ADDR_START_HAM_VAM_116,
    SSD1309_COL_ADDR_START_HAM_VAM_117,
    SSD1309_COL_ADDR_START_HAM_VAM_118,
    SSD1309_COL_ADDR_START_HAM_VAM_119,
    SSD1309_COL_ADDR_START_HAM_VAM_120,
    SSD1309_COL_ADDR_START_HAM_VAM_121,
    SSD1309_COL_ADDR_START_HAM_VAM_122,
    SSD1309_COL_ADDR_START_HAM_VAM_123,
    SSD1309_COL_ADDR_START_HAM_VAM_124,
    SSD1309_COL_ADDR_START_HAM_VAM_125,
    SSD1309_COL_ADDR_START_HAM_VAM_126,
    SSD1309_COL_ADDR_START_HAM_VAM_127
}SSD1309_COL_ADDR_START_HAM_VAM_te;

/**
 * @brief Column end address for RAM scanning of the SSD1309.
 * Only relevant for HAM and VAM.
 */
typedef enum {
    SSD1309_COL_ADDR_END_HAM_VAM_0,
    SSD1309_COL_ADDR_END_HAM_VAM_1,
    SSD1309_COL_ADDR_END_HAM_VAM_2,
    SSD1309_COL_ADDR_END_HAM_VAM_3,
    SSD1309_COL_ADDR_END_HAM_VAM_4,
    SSD1309_COL_ADDR_END_HAM_VAM_5,
    SSD1309_COL_ADDR_END_HAM_VAM_6,
    SSD1309_COL_ADDR_END_HAM_VAM_7,
    SSD1309_COL_ADDR_END_HAM_VAM_8,
    SSD1309_COL_ADDR_END_HAM_VAM_9,
    SSD1309_COL_ADDR_END_HAM_VAM_10,
    SSD1309_COL_ADDR_END_HAM_VAM_11,
    SSD1309_COL_ADDR_END_HAM_VAM_12,
    SSD1309_COL_ADDR_END_HAM_VAM_13,
    SSD1309_COL_ADDR_END_HAM_VAM_14,
    SSD1309_COL_ADDR_END_HAM_VAM_15,
    SSD1309_COL_ADDR_END_HAM_VAM_16,
    SSD1309_COL_ADDR_END_HAM_VAM_17,
    SSD1309_COL_ADDR_END_HAM_VAM_18,
    SSD1309_COL_ADDR_END_HAM_VAM_19,
    SSD1309_COL_ADDR_END_HAM_VAM_20,
    SSD1309_COL_ADDR_END_HAM_VAM_21,
    SSD1309_COL_ADDR_END_HAM_VAM_22,
    SSD1309_COL_ADDR_END_HAM_VAM_23,
    SSD1309_COL_ADDR_END_HAM_VAM_24,
    SSD1309_COL_ADDR_END_HAM_VAM_25,
    SSD1309_COL_ADDR_END_HAM_VAM_26,
    SSD1309_COL_ADDR_END_HAM_VAM_27,
    SSD1309_COL_ADDR_END_HAM_VAM_28,
    SSD1309_COL_ADDR_END_HAM_VAM_29,
    SSD1309_COL_ADDR_END_HAM_VAM_30,
    SSD1309_COL_ADDR_END_HAM_VAM_31,
    SSD1309_COL_ADDR_END_HAM_VAM_32,
    SSD1309_COL_ADDR_END_HAM_VAM_33,
    SSD1309_COL_ADDR_END_HAM_VAM_34,
    SSD1309_COL_ADDR_END_HAM_VAM_35,
    SSD1309_COL_ADDR_END_HAM_VAM_36,
    SSD1309_COL_ADDR_END_HAM_VAM_37,
    SSD1309_COL_ADDR_END_HAM_VAM_38,
    SSD1309_COL_ADDR_END_HAM_VAM_39,
    SSD1309_COL_ADDR_END_HAM_VAM_40,
    SSD1309_COL_ADDR_END_HAM_VAM_41,
    SSD1309_COL_ADDR_END_HAM_VAM_42,
    SSD1309_COL_ADDR_END_HAM_VAM_43,
    SSD1309_COL_ADDR_END_HAM_VAM_44,
    SSD1309_COL_ADDR_END_HAM_VAM_45,
    SSD1309_COL_ADDR_END_HAM_VAM_46,
    SSD1309_COL_ADDR_END_HAM_VAM_47,
    SSD1309_COL_ADDR_END_HAM_VAM_48,
    SSD1309_COL_ADDR_END_HAM_VAM_49,
    SSD1309_COL_ADDR_END_HAM_VAM_50,
    SSD1309_COL_ADDR_END_HAM_VAM_51,
    SSD1309_COL_ADDR_END_HAM_VAM_52,
    SSD1309_COL_ADDR_END_HAM_VAM_53,
    SSD1309_COL_ADDR_END_HAM_VAM_54,
    SSD1309_COL_ADDR_END_HAM_VAM_55,
    SSD1309_COL_ADDR_END_HAM_VAM_56,
    SSD1309_COL_ADDR_END_HAM_VAM_57,
    SSD1309_COL_ADDR_END_HAM_VAM_58,
    SSD1309_COL_ADDR_END_HAM_VAM_59,
    SSD1309_COL_ADDR_END_HAM_VAM_60,
    SSD1309_COL_ADDR_END_HAM_VAM_61,
    SSD1309_COL_ADDR_END_HAM_VAM_62,
    SSD1309_COL_ADDR_END_HAM_VAM_63,
    SSD1309_COL_ADDR_END_HAM_VAM_64,
    SSD1309_COL_ADDR_END_HAM_VAM_65,
    SSD1309_COL_ADDR_END_HAM_VAM_66,
    SSD1309_COL_ADDR_END_HAM_VAM_67,
    SSD1309_COL_ADDR_END_HAM_VAM_68,
    SSD1309_COL_ADDR_END_HAM_VAM_69,
    SSD1309_COL_ADDR_END_HAM_VAM_70,
    SSD1309_COL_ADDR_END_HAM_VAM_71,
    SSD1309_COL_ADDR_END_HAM_VAM_72,
    SSD1309_COL_ADDR_END_HAM_VAM_73,
    SSD1309_COL_ADDR_END_HAM_VAM_74,
    SSD1309_COL_ADDR_END_HAM_VAM_75,
    SSD1309_COL_ADDR_END_HAM_VAM_76,
    SSD1309_COL_ADDR_END_HAM_VAM_77,
    SSD1309_COL_ADDR_END_HAM_VAM_78,
    SSD1309_COL_ADDR_END_HAM_VAM_79,
    SSD1309_COL_ADDR_END_HAM_VAM_80,
    SSD1309_COL_ADDR_END_HAM_VAM_81,
    SSD1309_COL_ADDR_END_HAM_VAM_82,
    SSD1309_COL_ADDR_END_HAM_VAM_83,
    SSD1309_COL_ADDR_END_HAM_VAM_84,
    SSD1309_COL_ADDR_END_HAM_VAM_85,
    SSD1309_COL_ADDR_END_HAM_VAM_86,
    SSD1309_COL_ADDR_END_HAM_VAM_87,
    SSD1309_COL_ADDR_END_HAM_VAM_88,
    SSD1309_COL_ADDR_END_HAM_VAM_89,
    SSD1309_COL_ADDR_END_HAM_VAM_90,
    SSD1309_COL_ADDR_END_HAM_VAM_91,
    SSD1309_COL_ADDR_END_HAM_VAM_92,
    SSD1309_COL_ADDR_END_HAM_VAM_93,
    SSD1309_COL_ADDR_END_HAM_VAM_94,
    SSD1309_COL_ADDR_END_HAM_VAM_95,
    SSD1309_COL_ADDR_END_HAM_VAM_96,
    SSD1309_COL_ADDR_END_HAM_VAM_97,
    SSD1309_COL_ADDR_END_HAM_VAM_98,
    SSD1309_COL_ADDR_END_HAM_VAM_99,
    SSD1309_COL_ADDR_END_HAM_VAM_100,
    SSD1309_COL_ADDR_END_HAM_VAM_101,
    SSD1309_COL_ADDR_END_HAM_VAM_102,
    SSD1309_COL_ADDR_END_HAM_VAM_103,
    SSD1309_COL_ADDR_END_HAM_VAM_104,
    SSD1309_COL_ADDR_END_HAM_VAM_105,
    SSD1309_COL_ADDR_END_HAM_VAM_106,
    SSD1309_COL_ADDR_END_HAM_VAM_107,
    SSD1309_COL_ADDR_END_HAM_VAM_108,
    SSD1309_COL_ADDR_END_HAM_VAM_109,
    SSD1309_COL_ADDR_END_HAM_VAM_110,
    SSD1309_COL_ADDR_END_HAM_VAM_111,
    SSD1309_COL_ADDR_END_HAM_VAM_112,
    SSD1309_COL_ADDR_END_HAM_VAM_113,
    SSD1309_COL_ADDR_END_HAM_VAM_114,
    SSD1309_COL_ADDR_END_HAM_VAM_115,
    SSD1309_COL_ADDR_END_HAM_VAM_116,
    SSD1309_COL_ADDR_END_HAM_VAM_117,
    SSD1309_COL_ADDR_END_HAM_VAM_118,
    SSD1309_COL_ADDR_END_HAM_VAM_119,
    SSD1309_COL_ADDR_END_HAM_VAM_120,
    SSD1309_COL_ADDR_END_HAM_VAM_121,
    SSD1309_COL_ADDR_END_HAM_VAM_122,
    SSD1309_COL_ADDR_END_HAM_VAM_123,
    SSD1309_COL_ADDR_END_HAM_VAM_124,
    SSD1309_COL_ADDR_END_HAM_VAM_125,
    SSD1309_COL_ADDR_END_HAM_VAM_126,
    SSD1309_COL_ADDR_END_HAM_VAM_127
}SSD1309_COL_ADDR_END_HAM_VAM_te;

/**
 * @brief Page address start for RAM scanning of the SSD1309.
 * Only relevant for HAM and VAM.
 */
typedef enum {
	SSD1309_PAGE_ADDR_START_HAM_VAM_0,
	SSD1309_PAGE_ADDR_START_HAM_VAM_1,
	SSD1309_PAGE_ADDR_START_HAM_VAM_2,
	SSD1309_PAGE_ADDR_START_HAM_VAM_3,
	SSD1309_PAGE_ADDR_START_HAM_VAM_4,
	SSD1309_PAGE_ADDR_START_HAM_VAM_5,
	SSD1309_PAGE_ADDR_START_HAM_VAM_6,
	SSD1309_PAGE_ADDR_START_HAM_VAM_7
}SSD1309_PAGE_ADDR_START_HAM_VAM_te;

/**
 * @brief Page address end for RAM scanning of the SSD1309.
 * Only relevant for HAM and VAM.
 */
 typedef enum {
	SSD1309_PAGE_ADDR_END_HAM_VAM_0,
	SSD1309_PAGE_ADDR_END_HAM_VAM_1,
	SSD1309_PAGE_ADDR_END_HAM_VAM_2,
	SSD1309_PAGE_ADDR_END_HAM_VAM_3,
	SSD1309_PAGE_ADDR_END_HAM_VAM_4,
	SSD1309_PAGE_ADDR_END_HAM_VAM_5,
	SSD1309_PAGE_ADDR_END_HAM_VAM_6,
	SSD1309_PAGE_ADDR_END_HAM_VAM_7
}SSD1309_PAGE_ADDR_END_HAM_VAM_te;

/**
 * @brief Which row in RAM to appear at the top of the screen.
 * 
 */
typedef enum {
    SSD1309_START_LINE_0,
    SSD1309_START_LINE_1,
    SSD1309_START_LINE_2,
    SSD1309_START_LINE_3,
    SSD1309_START_LINE_4,
    SSD1309_START_LINE_5,
    SSD1309_START_LINE_6,
    SSD1309_START_LINE_7,
    SSD1309_START_LINE_8,
    SSD1309_START_LINE_9,
    SSD1309_START_LINE_10,
    SSD1309_START_LINE_11,
    SSD1309_START_LINE_12,
    SSD1309_START_LINE_13,
    SSD1309_START_LINE_14,
    SSD1309_START_LINE_15,
    SSD1309_START_LINE_16,
    SSD1309_START_LINE_17,
    SSD1309_START_LINE_18,
    SSD1309_START_LINE_19,
    SSD1309_START_LINE_20,
    SSD1309_START_LINE_21,
    SSD1309_START_LINE_22,
    SSD1309_START_LINE_23,
    SSD1309_START_LINE_24,
    SSD1309_START_LINE_25,
    SSD1309_START_LINE_26,
    SSD1309_START_LINE_27,
    SSD1309_START_LINE_28,
    SSD1309_START_LINE_29,
    SSD1309_START_LINE_30,
    SSD1309_START_LINE_31,
    SSD1309_START_LINE_32,
    SSD1309_START_LINE_33,
    SSD1309_START_LINE_34,
    SSD1309_START_LINE_35,
    SSD1309_START_LINE_36,
    SSD1309_START_LINE_37,
    SSD1309_START_LINE_38,
    SSD1309_START_LINE_39,
    SSD1309_START_LINE_40,
    SSD1309_START_LINE_41,
    SSD1309_START_LINE_42,
    SSD1309_START_LINE_43,
    SSD1309_START_LINE_44,
    SSD1309_START_LINE_45,
    SSD1309_START_LINE_46,
    SSD1309_START_LINE_47,
    SSD1309_START_LINE_48,
    SSD1309_START_LINE_49,
    SSD1309_START_LINE_50,
    SSD1309_START_LINE_51,
    SSD1309_START_LINE_52,
    SSD1309_START_LINE_53,
    SSD1309_START_LINE_54,
    SSD1309_START_LINE_55,
    SSD1309_START_LINE_56,
    SSD1309_START_LINE_57,
    SSD1309_START_LINE_58,
    SSD1309_START_LINE_59,
    SSD1309_START_LINE_60,
    SSD1309_START_LINE_61,
    SSD1309_START_LINE_62,
    SSD1309_START_LINE_63
}SSD1309_START_LINE_te;

/**
 * @brief Current control (brightness) of the LEDs in the SSD1309.
 * 
 */
typedef enum {
    SSD1309_CONTRAST_0,
    SSD1309_CONTRAST_1,
    SSD1309_CONTRAST_2,
    SSD1309_CONTRAST_3,
    SSD1309_CONTRAST_4,
    SSD1309_CONTRAST_5,
    SSD1309_CONTRAST_6,
    SSD1309_CONTRAST_7,
    SSD1309_CONTRAST_8,
    SSD1309_CONTRAST_9,
    SSD1309_CONTRAST_10,
    SSD1309_CONTRAST_11,
    SSD1309_CONTRAST_12,
    SSD1309_CONTRAST_13,
    SSD1309_CONTRAST_14,
    SSD1309_CONTRAST_15,
    SSD1309_CONTRAST_16,
    SSD1309_CONTRAST_17,
    SSD1309_CONTRAST_18,
    SSD1309_CONTRAST_19,
    SSD1309_CONTRAST_20,
    SSD1309_CONTRAST_21,
    SSD1309_CONTRAST_22,
    SSD1309_CONTRAST_23,
    SSD1309_CONTRAST_24,
    SSD1309_CONTRAST_25,
    SSD1309_CONTRAST_26,
    SSD1309_CONTRAST_27,
    SSD1309_CONTRAST_28,
    SSD1309_CONTRAST_29,
    SSD1309_CONTRAST_30,
    SSD1309_CONTRAST_31,
    SSD1309_CONTRAST_32,
    SSD1309_CONTRAST_33,
    SSD1309_CONTRAST_34,
    SSD1309_CONTRAST_35,
    SSD1309_CONTRAST_36,
    SSD1309_CONTRAST_37,
    SSD1309_CONTRAST_38,
    SSD1309_CONTRAST_39,
    SSD1309_CONTRAST_40,
    SSD1309_CONTRAST_41,
    SSD1309_CONTRAST_42,
    SSD1309_CONTRAST_43,
    SSD1309_CONTRAST_44,
    SSD1309_CONTRAST_45,
    SSD1309_CONTRAST_46,
    SSD1309_CONTRAST_47,
    SSD1309_CONTRAST_48,
    SSD1309_CONTRAST_49,
    SSD1309_CONTRAST_50,
    SSD1309_CONTRAST_51,
    SSD1309_CONTRAST_52,
    SSD1309_CONTRAST_53,
    SSD1309_CONTRAST_54,
    SSD1309_CONTRAST_55,
    SSD1309_CONTRAST_56,
    SSD1309_CONTRAST_57,
    SSD1309_CONTRAST_58,
    SSD1309_CONTRAST_59,
    SSD1309_CONTRAST_60,
    SSD1309_CONTRAST_61,
    SSD1309_CONTRAST_62,
    SSD1309_CONTRAST_63,
    SSD1309_CONTRAST_64,
    SSD1309_CONTRAST_65,
    SSD1309_CONTRAST_66,
    SSD1309_CONTRAST_67,
    SSD1309_CONTRAST_68,
    SSD1309_CONTRAST_69,
    SSD1309_CONTRAST_70,
    SSD1309_CONTRAST_71,
    SSD1309_CONTRAST_72,
    SSD1309_CONTRAST_73,
    SSD1309_CONTRAST_74,
    SSD1309_CONTRAST_75,
    SSD1309_CONTRAST_76,
    SSD1309_CONTRAST_77,
    SSD1309_CONTRAST_78,
    SSD1309_CONTRAST_79,
    SSD1309_CONTRAST_80,
    SSD1309_CONTRAST_81,
    SSD1309_CONTRAST_82,
    SSD1309_CONTRAST_83,
    SSD1309_CONTRAST_84,
    SSD1309_CONTRAST_85,
    SSD1309_CONTRAST_86,
    SSD1309_CONTRAST_87,
    SSD1309_CONTRAST_88,
    SSD1309_CONTRAST_89,
    SSD1309_CONTRAST_90,
    SSD1309_CONTRAST_91,
    SSD1309_CONTRAST_92,
    SSD1309_CONTRAST_93,
    SSD1309_CONTRAST_94,
    SSD1309_CONTRAST_95,
    SSD1309_CONTRAST_96,
    SSD1309_CONTRAST_97,
    SSD1309_CONTRAST_98,
    SSD1309_CONTRAST_99,
    SSD1309_CONTRAST_100,
    SSD1309_CONTRAST_101,
    SSD1309_CONTRAST_102,
    SSD1309_CONTRAST_103,
    SSD1309_CONTRAST_104,
    SSD1309_CONTRAST_105,
    SSD1309_CONTRAST_106,
    SSD1309_CONTRAST_107,
    SSD1309_CONTRAST_108,
    SSD1309_CONTRAST_109,
    SSD1309_CONTRAST_110,
    SSD1309_CONTRAST_111,
    SSD1309_CONTRAST_112,
    SSD1309_CONTRAST_113,
    SSD1309_CONTRAST_114,
    SSD1309_CONTRAST_115,
    SSD1309_CONTRAST_116,
    SSD1309_CONTRAST_117,
    SSD1309_CONTRAST_118,
    SSD1309_CONTRAST_119,
    SSD1309_CONTRAST_120,
    SSD1309_CONTRAST_121,
    SSD1309_CONTRAST_122,
    SSD1309_CONTRAST_123,
    SSD1309_CONTRAST_124,
    SSD1309_CONTRAST_125,
    SSD1309_CONTRAST_126,
    SSD1309_CONTRAST_127,
    SSD1309_CONTRAST_128,
    SSD1309_CONTRAST_129,
    SSD1309_CONTRAST_130,
    SSD1309_CONTRAST_131,
    SSD1309_CONTRAST_132,
    SSD1309_CONTRAST_133,
    SSD1309_CONTRAST_134,
    SSD1309_CONTRAST_135,
    SSD1309_CONTRAST_136,
    SSD1309_CONTRAST_137,
    SSD1309_CONTRAST_138,
    SSD1309_CONTRAST_139,
    SSD1309_CONTRAST_140,
    SSD1309_CONTRAST_141,
    SSD1309_CONTRAST_142,
    SSD1309_CONTRAST_143,
    SSD1309_CONTRAST_144,
    SSD1309_CONTRAST_145,
    SSD1309_CONTRAST_146,
    SSD1309_CONTRAST_147,
    SSD1309_CONTRAST_148,
    SSD1309_CONTRAST_149,
    SSD1309_CONTRAST_150,
    SSD1309_CONTRAST_151,
    SSD1309_CONTRAST_152,
    SSD1309_CONTRAST_153,
    SSD1309_CONTRAST_154,
    SSD1309_CONTRAST_155,
    SSD1309_CONTRAST_156,
    SSD1309_CONTRAST_157,
    SSD1309_CONTRAST_158,
    SSD1309_CONTRAST_159,
    SSD1309_CONTRAST_160,
    SSD1309_CONTRAST_161,
    SSD1309_CONTRAST_162,
    SSD1309_CONTRAST_163,
    SSD1309_CONTRAST_164,
    SSD1309_CONTRAST_165,
    SSD1309_CONTRAST_166,
    SSD1309_CONTRAST_167,
    SSD1309_CONTRAST_168,
    SSD1309_CONTRAST_169,
    SSD1309_CONTRAST_170,
    SSD1309_CONTRAST_171,
    SSD1309_CONTRAST_172,
    SSD1309_CONTRAST_173,
    SSD1309_CONTRAST_174,
    SSD1309_CONTRAST_175,
    SSD1309_CONTRAST_176,
    SSD1309_CONTRAST_177,
    SSD1309_CONTRAST_178,
    SSD1309_CONTRAST_179,
    SSD1309_CONTRAST_180,
    SSD1309_CONTRAST_181,
    SSD1309_CONTRAST_182,
    SSD1309_CONTRAST_183,
    SSD1309_CONTRAST_184,
    SSD1309_CONTRAST_185,
    SSD1309_CONTRAST_186,
    SSD1309_CONTRAST_187,
    SSD1309_CONTRAST_188,
    SSD1309_CONTRAST_189,
    SSD1309_CONTRAST_190,
    SSD1309_CONTRAST_191,
    SSD1309_CONTRAST_192,
    SSD1309_CONTRAST_193,
    SSD1309_CONTRAST_194,
    SSD1309_CONTRAST_195,
    SSD1309_CONTRAST_196,
    SSD1309_CONTRAST_197,
    SSD1309_CONTRAST_198,
    SSD1309_CONTRAST_199,
    SSD1309_CONTRAST_200,
    SSD1309_CONTRAST_201,
    SSD1309_CONTRAST_202,
    SSD1309_CONTRAST_203,
    SSD1309_CONTRAST_204,
    SSD1309_CONTRAST_205,
    SSD1309_CONTRAST_206,
    SSD1309_CONTRAST_207,
    SSD1309_CONTRAST_208,
    SSD1309_CONTRAST_209,
    SSD1309_CONTRAST_210,
    SSD1309_CONTRAST_211,
    SSD1309_CONTRAST_212,
    SSD1309_CONTRAST_213,
    SSD1309_CONTRAST_214,
    SSD1309_CONTRAST_215,
    SSD1309_CONTRAST_216,
    SSD1309_CONTRAST_217,
    SSD1309_CONTRAST_218,
    SSD1309_CONTRAST_219,
    SSD1309_CONTRAST_220,
    SSD1309_CONTRAST_221,
    SSD1309_CONTRAST_222,
    SSD1309_CONTRAST_223,
    SSD1309_CONTRAST_224,
    SSD1309_CONTRAST_225,
    SSD1309_CONTRAST_226,
    SSD1309_CONTRAST_227,
    SSD1309_CONTRAST_228,
    SSD1309_CONTRAST_229,
    SSD1309_CONTRAST_230,
    SSD1309_CONTRAST_231,
    SSD1309_CONTRAST_232,
    SSD1309_CONTRAST_233,
    SSD1309_CONTRAST_234,
    SSD1309_CONTRAST_235,
    SSD1309_CONTRAST_236,
    SSD1309_CONTRAST_237,
    SSD1309_CONTRAST_238,
    SSD1309_CONTRAST_239,
    SSD1309_CONTRAST_240,
    SSD1309_CONTRAST_241,
    SSD1309_CONTRAST_242,
    SSD1309_CONTRAST_243,
    SSD1309_CONTRAST_244,
    SSD1309_CONTRAST_245,
    SSD1309_CONTRAST_246,
    SSD1309_CONTRAST_247,
    SSD1309_CONTRAST_248,
    SSD1309_CONTRAST_249,
    SSD1309_CONTRAST_250,
    SSD1309_CONTRAST_251,
    SSD1309_CONTRAST_252,
    SSD1309_CONTRAST_253,
    SSD1309_CONTRAST_254,
    SSD1309_CONTRAST_255
}SSD1309_CONTRAST_te;

/**
 * @brief Flip the image horizontally.
 * 
 */
typedef enum {
	SSD1309_HORIZONTAL_FLIP_FALSE,
	SSD1309_HORIZONTAL_FLIP_TRUE
}SSD1309_HORIZONTAL_FLIP_te;

/**
 * @brief Causes the values stored in RAM to have the opposite effect. (1 to turn off pixel, 0 to turn on pixel)
 * 
 */
typedef enum {
	SSD1309_INVERSE_MODE_FALSE,
	SSD1309_INVERSE_MODE_TRUE
}SSD1309_INVERSE_MODE_te;

/**
 * @brief Number of SSD1309 LED lines activated.
 * 
 */
typedef enum {
    SSD1309_MULTIPLEX_RATIO_16 = 15,
    SSD1309_MULTIPLEX_RATIO_17,
    SSD1309_MULTIPLEX_RATIO_18,
    SSD1309_MULTIPLEX_RATIO_19,
    SSD1309_MULTIPLEX_RATIO_20,
    SSD1309_MULTIPLEX_RATIO_21,
    SSD1309_MULTIPLEX_RATIO_22,
    SSD1309_MULTIPLEX_RATIO_23,
    SSD1309_MULTIPLEX_RATIO_24,
    SSD1309_MULTIPLEX_RATIO_25,
    SSD1309_MULTIPLEX_RATIO_26,
    SSD1309_MULTIPLEX_RATIO_27,
    SSD1309_MULTIPLEX_RATIO_28,
    SSD1309_MULTIPLEX_RATIO_29,
    SSD1309_MULTIPLEX_RATIO_30,
    SSD1309_MULTIPLEX_RATIO_31,
    SSD1309_MULTIPLEX_RATIO_32,
    SSD1309_MULTIPLEX_RATIO_33,
    SSD1309_MULTIPLEX_RATIO_34,
    SSD1309_MULTIPLEX_RATIO_35,
    SSD1309_MULTIPLEX_RATIO_36,
    SSD1309_MULTIPLEX_RATIO_37,
    SSD1309_MULTIPLEX_RATIO_38,
    SSD1309_MULTIPLEX_RATIO_39,
    SSD1309_MULTIPLEX_RATIO_40,
    SSD1309_MULTIPLEX_RATIO_41,
    SSD1309_MULTIPLEX_RATIO_42,
    SSD1309_MULTIPLEX_RATIO_43,
    SSD1309_MULTIPLEX_RATIO_44,
    SSD1309_MULTIPLEX_RATIO_45,
    SSD1309_MULTIPLEX_RATIO_46,
    SSD1309_MULTIPLEX_RATIO_47,
    SSD1309_MULTIPLEX_RATIO_48,
    SSD1309_MULTIPLEX_RATIO_49,
    SSD1309_MULTIPLEX_RATIO_50,
    SSD1309_MULTIPLEX_RATIO_51,
    SSD1309_MULTIPLEX_RATIO_52,
    SSD1309_MULTIPLEX_RATIO_53,
    SSD1309_MULTIPLEX_RATIO_54,
    SSD1309_MULTIPLEX_RATIO_55,
    SSD1309_MULTIPLEX_RATIO_56,
    SSD1309_MULTIPLEX_RATIO_57,
    SSD1309_MULTIPLEX_RATIO_58,
    SSD1309_MULTIPLEX_RATIO_59,
    SSD1309_MULTIPLEX_RATIO_60,
    SSD1309_MULTIPLEX_RATIO_61,
    SSD1309_MULTIPLEX_RATIO_62,
    SSD1309_MULTIPLEX_RATIO_63,
    SSD1309_MULTIPLEX_RATIO_64
}SSD1309_MULTIPLEX_RATIO_te;

/**
 * @brief Page address start for RAM scanning of the SSD1309.
 * Only relevant for PAM.
 */
typedef enum {
	SSD1309_PAGE_START_ADDR_PAM_0,
	SSD1309_PAGE_START_ADDR_PAM_1,
	SSD1309_PAGE_START_ADDR_PAM_2,
	SSD1309_PAGE_START_ADDR_PAM_3,
	SSD1309_PAGE_START_ADDR_PAM_4,
	SSD1309_PAGE_START_ADDR_PAM_5,
	SSD1309_PAGE_START_ADDR_PAM_6,
	SSD1309_PAGE_START_ADDR_PAM_7,
}SSD1309_PAGE_START_ADDR_PAM_te;

/**
 * @brief Flip the image vertically.
 * 
 */
typedef enum {
	SSD1309_VERTICAL_FLIP_FALSE,
	SSD1309_VERTICAL_FLIP_TRUE = 8
}SSD1309_VERTICAL_FLIP_te;

/**
 * @brief selects which RAM row is internally linked to COM0.
 * 
 */
typedef enum {
    SSD1309_OFFSET_0 = 0,
    SSD1309_OFFSET_1,
    SSD1309_OFFSET_2,
    SSD1309_OFFSET_3,
    SSD1309_OFFSET_4,
    SSD1309_OFFSET_5,
    SSD1309_OFFSET_6,
    SSD1309_OFFSET_7,
    SSD1309_OFFSET_8,
    SSD1309_OFFSET_9,
    SSD1309_OFFSET_10,
    SSD1309_OFFSET_11,
    SSD1309_OFFSET_12,
    SSD1309_OFFSET_13,
    SSD1309_OFFSET_14,
    SSD1309_OFFSET_15,
    SSD1309_OFFSET_16,
    SSD1309_OFFSET_17,
    SSD1309_OFFSET_18,
    SSD1309_OFFSET_19,
    SSD1309_OFFSET_20,
    SSD1309_OFFSET_21,
    SSD1309_OFFSET_22,
    SSD1309_OFFSET_23,
    SSD1309_OFFSET_24,
    SSD1309_OFFSET_25,
    SSD1309_OFFSET_26,
    SSD1309_OFFSET_27,
    SSD1309_OFFSET_28,
    SSD1309_OFFSET_29,
    SSD1309_OFFSET_30,
    SSD1309_OFFSET_31,
    SSD1309_OFFSET_32,
    SSD1309_OFFSET_33,
    SSD1309_OFFSET_34,
    SSD1309_OFFSET_35,
    SSD1309_OFFSET_36,
    SSD1309_OFFSET_37,
    SSD1309_OFFSET_38,
    SSD1309_OFFSET_39,
    SSD1309_OFFSET_40,
    SSD1309_OFFSET_41,
    SSD1309_OFFSET_42,
    SSD1309_OFFSET_43,
    SSD1309_OFFSET_44,
    SSD1309_OFFSET_45,
    SSD1309_OFFSET_46,
    SSD1309_OFFSET_47,
    SSD1309_OFFSET_48,
    SSD1309_OFFSET_49,
    SSD1309_OFFSET_50,
    SSD1309_OFFSET_51,
    SSD1309_OFFSET_52,
    SSD1309_OFFSET_53,
    SSD1309_OFFSET_54,
    SSD1309_OFFSET_55,
    SSD1309_OFFSET_56,
    SSD1309_OFFSET_57,
    SSD1309_OFFSET_58,
    SSD1309_OFFSET_59,
    SSD1309_OFFSET_60,
    SSD1309_OFFSET_61,
    SSD1309_OFFSET_62,
    SSD1309_OFFSET_63
}SSD1309_OFFSET_te;

/**
 * @brief The divide ratio of the SSD1309 clock.
 * 
 */
typedef enum {
    SSD1309_CLK_DIV_RATIO_1 = 1,
    SSD1309_CLK_DIV_RATIO_2,
    SSD1309_CLK_DIV_RATIO_3,
    SSD1309_CLK_DIV_RATIO_4,
    SSD1309_CLK_DIV_RATIO_5,
    SSD1309_CLK_DIV_RATIO_6,
    SSD1309_CLK_DIV_RATIO_7,
    SSD1309_CLK_DIV_RATIO_8,
    SSD1309_CLK_DIV_RATIO_9,
    SSD1309_CLK_DIV_RATIO_10,
    SSD1309_CLK_DIV_RATIO_11,
    SSD1309_CLK_DIV_RATIO_12,
    SSD1309_CLK_DIV_RATIO_13,
    SSD1309_CLK_DIV_RATIO_14,
    SSD1309_CLK_DIV_RATIO_15,
    SSD1309_CLK_DIV_RATIO_16
}SSD1309_CLK_DIV_RATIO_te;

/**
 * @brief The clock speed of the SSD1309.
 * 
 */
typedef enum {
    SSD1309_CLK_SPEED_LVL_0,
    SSD1309_CLK_SPEED_LVL_LVL_1,
    SSD1309_CLK_SPEED_LVL_LVL_2,
    SSD1309_CLK_SPEED_LVL_LVL_3,
    SSD1309_CLK_SPEED_LVL_LVL_4,
    SSD1309_CLK_SPEED_LVL_LVL_5,
    SSD1309_CLK_SPEED_LVL_LVL_6,
    SSD1309_CLK_SPEED_LVL_LVL_7,
    SSD1309_CLK_SPEED_LVL_LVL_8,
    SSD1309_CLK_SPEED_LVL_LVL_9,
    SSD1309_CLK_SPEED_LVL_LVL_10,
    SSD1309_CLK_SPEED_LVL_LVL_11,
    SSD1309_CLK_SPEED_LVL_LVL_12,
    SSD1309_CLK_SPEED_LVL_LVL_13,
    SSD1309_CLK_SPEED_LVL_LVL_14,
    SSD1309_CLK_SPEED_LVL_LVL_15
}SSD1309_CLK_SPEED_LVL_te;

/**
 * @brief Phase 1 (discharge) length of the segment (pixel) output wave form.
 * 
 */
typedef enum {
    SSD1309_PHASE1_PRECHARGE_DCLK_1 = 1,
    SSD1309_PHASE1_PRECHARGE_DCLK_2,
    SSD1309_PHASE1_PRECHARGE_DCLK_3,
    SSD1309_PHASE1_PRECHARGE_DCLK_4,
    SSD1309_PHASE1_PRECHARGE_DCLK_5,
    SSD1309_PHASE1_PRECHARGE_DCLK_6,
    SSD1309_PHASE1_PRECHARGE_DCLK_7,
    SSD1309_PHASE1_PRECHARGE_DCLK_8,
    SSD1309_PHASE1_PRECHARGE_DCLK_9,
    SSD1309_PHASE1_PRECHARGE_DCLK_10,
    SSD1309_PHASE1_PRECHARGE_DCLK_11,
    SSD1309_PHASE1_PRECHARGE_DCLK_12,
    SSD1309_PHASE1_PRECHARGE_DCLK_13,
    SSD1309_PHASE1_PRECHARGE_DCLK_14,
    SSD1309_PHASE1_PRECHARGE_DCLK_15
}SSD1309_PHASE1_PRECHARGE_DCLK_te;

/**
 * @brief Phase 2 (charge) length of the segment (pixel) output wave form.
 * 
 */
typedef enum {
    SSD1309_PHASE2_PRECHARGE_DCLK_1 = 1,
    SSD1309_PHASE2_PRECHARGE_DCLK_2,
    SSD1309_PHASE2_PRECHARGE_DCLK_3,
    SSD1309_PHASE2_PRECHARGE_DCLK_4,
    SSD1309_PHASE2_PRECHARGE_DCLK_5,
    SSD1309_PHASE2_PRECHARGE_DCLK_6,
    SSD1309_PHASE2_PRECHARGE_DCLK_7,
    SSD1309_PHASE2_PRECHARGE_DCLK_8,
    SSD1309_PHASE2_PRECHARGE_DCLK_9,
    SSD1309_PHASE2_PRECHARGE_DCLK_10,
    SSD1309_PHASE2_PRECHARGE_DCLK_11,
    SSD1309_PHASE2_PRECHARGE_DCLK_12,
    SSD1309_PHASE2_PRECHARGE_DCLK_13,
    SSD1309_PHASE2_PRECHARGE_DCLK_14,
    SSD1309_PHASE2_PRECHARGE_DCLK_15
}SSD1309_PHASE2_PRECHARGE_DCLK_te;

/**
 * @brief The deselect level of a LED row in the SSD1309.
 * LOW -> 0.64 * Vcc
 * MED -> 0.78 * Vcc
 * HIGH -> 0.84 * Vcc
 */
typedef enum {
	SSD1309_VCOMH_DESELECT_LVL_LOW,
	SSD1309_VCOMH_DESELECT_LVL_MED = 13,
	SSD1309_VCOMH_DESELECT_LVL_HIGH = 15
}SSD1309_VCOMH_DESELECT_LVL_te;

/**
 * @brief Full configuration structure for initializing an SSD1309 display handle.
 *
 * @details
 * Passed to @ref ssd1309_init_handle. Use @ref ssd1309_get_def_cfg to obtain
 * a sensible default configuration and then override specific fields as needed.
 *
 * Fields are grouped by addressing mode relevance:
 * - PAM fields: @ref low_col_start_addr_pam, @ref high_col_start_addr_pam,
 *   @ref page_start_addr_pam
 * - HAM/VAM fields: @ref col_addr_start_ham_vam, @ref col_addr_end_ham_vam,
 *   @ref page_addr_start_ham_vam, @ref page_addr_end_ham_vam
 */
typedef struct {
    /** Pointer to the I2C peripheral instance connected to the display. */
    I2C_REGDEF_ts *i2c_instance;

    /** GPIO port of the I2C SCL pin. */
    GPIO_REGDEF_ts *scl_gpio_port;

    /** GPIO pin number of the I2C SCL pin. */
    GPIO_PIN_te scl_gpio_pin;

    /** GPIO port of the I2C SDA pin. */
    GPIO_REGDEF_ts *sda_gpio_port;

    /** GPIO pin number of the I2C SDA pin. */
    GPIO_PIN_te sda_gpio_pin;

    /** Alternate function mapping for both SCL and SDA GPIO pins. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;

    /** Lower nibble of the column start address (PAM only). */
    SSD1309_LOW_COL_START_ADDR_PAM_te low_col_start_addr_pam;

    /** Upper nibble of the column start address (PAM only). */
    SSD1309_HIGH_COL_START_ADDR_PAM_te high_col_start_addr_pam;

    /** Memory addressing mode: HAM, VAM, or PAM. */
    SSD1309_MEM_ADDR_MODE_te mem_addr_mode;

    /** Column start address for RAM scanning (HAM and VAM only). */
    SSD1309_COL_ADDR_START_HAM_VAM_te col_addr_start_ham_vam;

    /** Column end address for RAM scanning (HAM and VAM only). */
    SSD1309_COL_ADDR_END_HAM_VAM_te col_addr_end_ham_vam;

    /** Page start address for RAM scanning (HAM and VAM only). */
    SSD1309_PAGE_ADDR_START_HAM_VAM_te page_addr_start_ham_vam;

    /** Page end address for RAM scanning (HAM and VAM only). */
    SSD1309_PAGE_ADDR_END_HAM_VAM_te page_addr_end_ham_vam;

    /** RAM row that maps to the top of the display. */
    SSD1309_START_LINE_te start_line;

    /** Display brightness (current control level, 0–255). */
    SSD1309_CONTRAST_te contrast;

    /** Horizontal segment re-map (left/right flip). */
    SSD1309_HORIZONTAL_FLIP_te horizontal_flip;

    /** Inverse video mode (swap on/off pixel polarity). */
    SSD1309_INVERSE_MODE_te inverse_mode;

    /** Number of active COM lines (display height in rows). */
    SSD1309_MULTIPLEX_RATIO_te multiplex_ratio;

    /** Page start address for the first displayed page (PAM only). */
    SSD1309_PAGE_START_ADDR_PAM_te page_start_addr_pam;

    /** COM output scan direction (top/bottom flip). */
    SSD1309_VERTICAL_FLIP_te vertical_flip;

    /** COM pin vertical shift (maps RAM row 0 to a different COM output). */
    SSD1309_OFFSET_te offset;

    /** Display clock divide ratio. */
    SSD1309_CLK_DIV_RATIO_te clk_div_ratio;

    /** Display oscillator frequency level. */
    SSD1309_CLK_SPEED_LVL_te clk_speed_lvl;

    /** Phase 1 (discharge) duration of the pixel drive waveform in DCLKs. */
    SSD1309_PHASE1_PRECHARGE_DCLK_te phase1_precharge_dclk;

    /** Phase 2 (charge) duration of the pixel drive waveform in DCLKs. */
    SSD1309_PHASE2_PRECHARGE_DCLK_te phase2_precharge_dclk;

    /** VCOMH deselect voltage level relative to Vcc. */
    SSD1309_VCOMH_DESELECT_LVL_te vcomh_deselect_lvl;
} SSD1309_CFG_ts;

/**
 * @brief Opaque handle representing an SSD1309 display instance.
 *
 * @details
 * Returned by @ref ssd1309_init_handle and used for all subsequent
 * display operations. The internal structure is hidden and must not
 * be accessed directly.
 *
 * @note Only one handle instance is supported by the subsystem.
 */
typedef struct ssd1309_handle_s SSD1309_HANDLE_ts;

/** @} */

/**
 * @defgroup ssd1309_api SSD1309 Public API
 * @ingroup ssd1309_module
 * @brief Public functions to interact with the SSD1309 display subsystem.
 * @{
 */

/**
 * @brief Initializes the SSD1309 subsystem.
 *
 * @details
 * Resets the internal state, initializes the logging dependency, and
 * registers the CLI commands.
 *
 * Must be called before any other SSD1309 API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 */
ERR_te ssd1309_init_subsys(void);

/**
 * @brief Deinitializes the SSD1309 subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 * The subsystem must be stopped before calling this function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized or still running
 * - Propagated error from @ref cmd_deregister on failure
 */
ERR_te ssd1309_deinit_subsys(void);

/**
 * @brief Starts the SSD1309 subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 */
ERR_te ssd1309_start_subsys(void);

/**
 * @brief Stops the SSD1309 subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 */
ERR_te ssd1309_stop_subsys(void);

/**
 * @brief Populates a configuration structure with sensible default values.
 *
 * @details
 * The default configuration uses:
 * - Horizontal addressing mode (HAM), full 128×64 column and page range
 * - Contrast level 10, both horizontal and vertical flip enabled
 * - Multiplex ratio 64, no display offset, clock divide ratio 1
 * - Clock speed level 15, phase 1 and phase 2 precharge of 2 DCLKs
 * - VCOMH deselect level: medium (0.78 × Vcc)
 *
 * The I2C instance and GPIO fields are not set and must be filled in
 * by the caller before passing to @ref ssd1309_init_handle.
 *
 * @param[out] ssd1309_cfg_o Pointer to the configuration structure to populate.
 *
 * @return
 * - ERR_OK always
 */
ERR_te ssd1309_get_def_cfg(SSD1309_CFG_ts *ssd1309_cfg_o);

/**
 * @brief Initializes the SSD1309 display and sends the full configuration sequence over I2C.
 *
 * @details
 * Configures the SCL and SDA GPIO pins in open-drain alternate function mode,
 * initializes the I2C peripheral at 400 kHz, and transmits the complete
 * SSD1309 initialization command sequence derived from @p ssd1309_cfg.
 *
 * The display is turned off during configuration and turned on at the end.
 *
 * @note Only one handle instance is supported. Calling this function a second
 *       time without deinitialization returns an error.
 *
 * @param[in]  ssd1309_cfg      Pointer to the display configuration structure.
 * @param[out] ssd1309_handle_o Pointer to a handle pointer that will be set
 *                              to the initialized display instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if a handle is already initialized, or if
 *   the I2C or GPIO pointers in @p ssd1309_cfg are NULL
 */
ERR_te ssd1309_init_handle(SSD1309_CFG_ts *ssd1309_cfg, SSD1309_HANDLE_ts **ssd1309_handle_o);

/**
 * @brief Draws a text string into the framebuffer at the specified line.
 *
 * @details
 * Renders characters from the built-in 8×8 font table into the framebuffer.
 * Each character occupies 8 columns; characters wrap to the next line if they
 * exceed the display width. The framebuffer is not flushed to the display;
 * call @ref ssd1309_update to make the changes visible.
 *
 * Supports printable ASCII characters (0x20–0x7E).
 *
 * @param[in] text     Pointer to the text string to render.
 * @param[in] text_len Number of characters to render from @p text.
 * @param[in] line     Display line (1–8, top to bottom) where text begins.
 * @param[in] force    If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if @p line is out of range
 */
ERR_te ssd1309_draw_text(char const *text, uint8_t text_len, uint8_t line, bool force);

/**
 * @brief Draws a filled rectangle into the framebuffer.
 *
 * @details
 * Sets all pixels within the rectangle defined by (@p x_src, @p y_src) to
 * (@p x_dest, @p y_dest) in the framebuffer. Coordinates are 1-based.
 * The framebuffer is not flushed; call @ref ssd1309_update to display the result.
 *
 * @param[in] x_src  Left edge of the rectangle (1–128).
 * @param[in] y_src  Top edge of the rectangle (1–64).
 * @param[in] x_dest Right edge of the rectangle (1–128, must be ≥ @p x_src).
 * @param[in] y_dest Bottom edge of the rectangle (1–64, must be ≥ @p y_src).
 * @param[in] force  If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if coordinates are out of range or @p x_src > @p x_dest
 */
ERR_te ssd1309_draw_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest, bool force);

/**
 * @brief Clears a single display line in the framebuffer (sets all pixels off).
 *
 * @details
 * Equivalent to calling @ref ssd1309_clear_rect over the full width of the
 * given line. The framebuffer is not flushed; call @ref ssd1309_update afterwards.
 *
 * @param[in] line  Display line to clear (1–8).
 * @param[in] force If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if @p line is out of range
 */
ERR_te ssd1309_clear_line(uint8_t line, bool force);

/**
 * @brief Inverts all pixels in a single display line in the framebuffer.
 *
 * @details
 * Equivalent to calling @ref ssd1309_invert_rect over the full width of the
 * given line. Used by the menu module to simulate a selection highlight.
 * The framebuffer is not flushed; call @ref ssd1309_update afterwards.
 *
 * @param[in] line  Display line to invert (1–8).
 * @param[in] force If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if @p line is out of range
 */
ERR_te ssd1309_invert_line(uint8_t line, bool force);

/**
 * @brief Clears a rectangular region in the framebuffer (sets all pixels off).
 *
 * @details
 * Clears all pixels within the rectangle defined by (@p x_src, @p y_src) to
 * (@p x_dest, @p y_dest). Coordinates are 1-based.
 * The framebuffer is not flushed; call @ref ssd1309_update to display the result.
 *
 * @param[in] x_src  Left edge (1–128).
 * @param[in] y_src  Top edge (1–64).
 * @param[in] x_dest Right edge (1–128, must be ≥ @p x_src).
 * @param[in] y_dest Bottom edge (1–64, must be ≥ @p y_src).
 * @param[in] force  If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if coordinates are out of range or @p x_src > @p x_dest
 */
ERR_te ssd1309_clear_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest, bool force);

/**
 * @brief Inverts all pixels in a rectangular region of the framebuffer.
 *
 * @details
 * XORs all pixels within the rectangle defined by (@p x_src, @p y_src) to
 * (@p x_dest, @p y_dest). Coordinates are 1-based.
 * The framebuffer is not flushed; call @ref ssd1309_update to display the result.
 *
 * @param[in] x_src  Left edge (1–128).
 * @param[in] y_src  Top edge (1–64).
 * @param[in] x_dest Right edge (1–128, must be ≥ @p x_src).
 * @param[in] y_dest Bottom edge (1–64, must be ≥ @p y_src).
 * @param[in] force  If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 * - ERR_INVALID_ARGUMENT if coordinates are out of range or @p x_src > @p x_dest
 */
ERR_te ssd1309_invert_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest, bool force);

/**
 * @brief Flushes the internal framebuffer to the display over I2C.
 *
 * @details
 * Transmits the full 128×8-page framebuffer to the SSD1309 over I2C,
 * making all pending framebuffer changes visible on the display.
 *
 * @param[in] force If true, bypasses the initialized/started guard.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if not initialized/started and @p force is false
 */
ERR_te ssd1309_update(bool force);

/** @} */

#endif

/** @} */