/**
 * @file modules.c
 * @author github.com/Baksi675
 * @brief System module name string table.
 * @version 0.1
 * @date 2026-01-28
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "modules.h"

/**
 * @brief Maps each @ref MODULES_te enumerator to its human-readable name string.
 *
 * @details
 * Indexed by @ref MODULES_te values. Used by the log subsystem to prefix
 * each log message with the originating subsystem name, and by the CLI
 * to display module names.
 *
 * The order and count of entries must remain in sync with @ref MODULES_te.
 */
const char *modules_names[] = {
    [MODULES_CMD]     = "cmd",
    [MODULES_SSD1309] = "ssd1309",
    [MODULES_NEO6]    = "neo6",
    [MODULES_IO]      = "io",
    [MODULES_BUTTON]  = "button",
    [MODULES_MENU]    = "menu",
    [MODULES_SD]      = "sd",
    [MODULES_DATALOG] = "datalog"
};