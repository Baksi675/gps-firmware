/**
 * @file modules.h
 * @author github.com/Baksi675
 * @brief System module identifier definitions.
 *
 * @details
 * This header defines the @ref MODULES_te enumeration used to identify
 * each subsystem in log messages and CLI output. The numeric value of
 * each enumerator is used as an index into the @ref modules_names array
 * defined in @ref modules.c.
 *
 * When adding a new module, a new enumerator must be added here and a
 * corresponding name string must be added to @ref modules_names in
 * @ref modules.c.
 *
 * @version 0.1
 * @date 2026-01-28
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup modules_module Module Identifiers
 * @brief System-wide subsystem identifier enumeration.
 * @{
 */

#ifndef MODULES_H__
#define MODULES_H__

/**
 * @brief Identifies a subsystem for use in logging and CLI output.
 *
 * @details
 * Each enumerator corresponds to one subsystem. The numeric value is used
 * as an index into @ref modules_names to look up the subsystem's name string.
 *
 * @note Enumerators must remain contiguous and in the same order as the
 *       entries in @ref modules_names to keep the index mapping valid.
 */
typedef enum {
    MODULES_CMD,     /**< Command dispatch subsystem. */
    MODULES_SSD1309, /**< SSD1309 OLED display subsystem. */
    MODULES_NEO6,    /**< NEO-6 GPS module subsystem. */
    MODULES_IO,      /**< Digital IO subsystem. */
    MODULES_BUTTON,  /**< Button input subsystem. */
    MODULES_MENU,    /**< OLED menu subsystem. */
    MODULES_SD,      /**< SD card subsystem. */
    MODULES_DATALOG, /**< GPS data logging subsystem. */
} MODULES_te;

#endif

/** @} */