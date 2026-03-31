/**
 * @file menu.h
 * @author github.com/Baksi675
 * @brief Menu module public API.
 *
 * @details
 * This module provides an OLED display menu system backed by the SSD1309
 * driver. Two menu types are supported:
 *
 * - **Selectable** (@ref MENU_TYPE_SELECTABLE): a scrollable list of static
 *   string options. The currently highlighted option can be retrieved via
 *   @ref menu_get_selected_option.
 * - **Data view** (@ref MENU_TYPE_DATA_VIEW): a scrollable list that
 *   interleaves static option labels with live values fetched at render
 *   time via a @ref MENU_CFG_ts::get_value_fn callback.
 *
 * Both types support a navigation history stack via @ref menu_set_prev_menu
 * and @ref menu_get_prev_menu, allowing a back-navigation pattern.
 *
 * Typical usage:
 * - Initialize the subsystem using @ref menu_init_subsys
 * - Populate a @ref MENU_CFG_ts and register a handle with @ref menu_init_handle
 * - Start the subsystem using @ref menu_start_subsys
 * - Scroll with @ref menu_scroll and render with @ref menu_run_handle
 *
 * @version 0.1
 * @date 2026-02-01
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup menu_module Menu Module
 * @brief OLED display menu system with selectable and data-view modes.
 * @{
 */

#ifndef MENU_H__
#define MENU_H__

#include <stdint.h>

#include "common.h"
#include "ssd1309.h"
#include "err.h"
#include "configuration.h"

/**
 * @defgroup menu_types Menu Types
 * @ingroup menu_module
 * @brief Data types used by the menu module.
 * @{
 */

/**
 * @brief Determines the rendering and interaction mode of a menu.
 */
typedef enum {
    /** Scrollable list of static string options with highlight selection. */
    MENU_TYPE_SELECTABLE,

    /**
     * Scrollable list that alternates between a static label and a live
     * value fetched via @ref MENU_CFG_ts::get_value_fn on each render.
     */
    MENU_TYPE_DATA_VIEW
} MENU_TYPE_ts;

/**
 * @brief Configuration structure for initializing a menu handle.
 *
 * @details
 * Passed to @ref menu_init_handle. The number of options is determined
 * automatically by scanning for the first entry whose first character
 * is `'\0'`, so the options array must be null-terminated in this sense.
 */
typedef struct {
    /** Title string displayed at the bottom of the screen (null-terminated). */
    char title[SSD1309_MAX_CHARS_IN_LINE];

    /**
     * Array of option label strings. Each entry is a null-terminated string
     * of at most @ref SSD1309_MAX_CHARS_IN_LINE characters. The array is
     * scanned until an entry whose first byte is `'\0'` is found, which
     * marks the end of the option list.
     */
    char options[CONFIG_MENU_MAX_OPTIONS][SSD1309_MAX_CHARS_IN_LINE];

    /** Menu rendering and interaction mode. */
    MENU_TYPE_ts type;

    /**
     * Callback used by @ref MENU_TYPE_DATA_VIEW menus to fetch the live
     * value for a given option index at render time.
     *
     * @param[in]  index   Zero-based index of the option whose value is needed.
     * @param[out] value_o Pointer that will be set to the value string.
     * @return ERR_OK on success, any other code signals a data acquisition failure.
     */
    ERR_te (*get_value_fn)(uint8_t index, char **value_o);

    /** Human-readable name of this menu instance (null-terminated). */
    char name[CONFIG_MENU_MAX_NAME_LEN];
} MENU_CFG_ts;

/**
 * @brief Opaque handle representing a menu instance.
 *
 * @details
 * Returned by @ref menu_init_handle and used for all subsequent
 * operations on that menu. The internal structure is hidden and must
 * not be accessed directly.
 */
typedef struct menu_handle_s MENU_HANDLE_ts;

/** @} */

/**
 * @defgroup menu_api Menu Public API
 * @ingroup menu_module
 * @brief Public functions to interact with the menu subsystem.
 * @{
 */

/**
 * @brief Initializes the menu subsystem.
 *
 * @details
 * Resets the internal state, initializes the logging dependency, and
 * registers the CLI commands.
 *
 * Must be called before any other menu API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 *
 * @note Should only be called once during system startup.
 */
ERR_te menu_init_subsys(void);

/**
 * @brief Deinitializes the menu subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 * The subsystem must be stopped before calling this function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized or still running
 */
ERR_te menu_deinit_subsys(void);

/**
 * @brief Starts the menu subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 */
ERR_te menu_start_subsys(void);

/**
 * @brief Stops the menu subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 */
ERR_te menu_stop_subsys(void);

/**
 * @brief Initializes and registers a menu handle.
 *
 * @details
 * Counts the number of options in @p menu_cfg by scanning for the first
 * entry whose first character is `'\0'`, copies all configuration into the
 * internal pool, and sets up initial scroll state.
 *
 * For @ref MENU_TYPE_DATA_VIEW menus, the internal option count is doubled
 * to account for interleaved label and value rows.
 *
 * @param[in]  menu_cfg      Pointer to the menu configuration structure.
 * @param[out] menu_handle_o Pointer to a handle pointer that will be set
 *                           to the allocated menu instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of handles is reached or
 *   the option count exceeds @ref CONFIG_MENU_MAX_OPTIONS
 * - ERR_INVALID_ARGUMENT if no options are found in @p menu_cfg
 */
ERR_te menu_init_handle(MENU_CFG_ts *menu_cfg, MENU_HANDLE_ts **menu_handle_o);

/**
 * @brief Renders a single menu to the display.
 *
 * @details
 * Clears the display area, dispatches to the appropriate internal renderer
 * based on @ref MENU_TYPE_ts, and triggers a display update.
 *
 * @param[in,out] menu_handle Pointer to the menu handle to render.
 *
 * @return
 * - ERR_OK always
 */
ERR_te menu_run_handle(MENU_HANDLE_ts *menu_handle);

/**
 * @brief Renders all registered menus to the display.
 *
 * @details
 * Iterates over all active handles and calls @ref menu_run_handle on each.
 *
 * @return
 * - ERR_OK always
 */
ERR_te menu_run_handle_all(void);

/**
 * @brief Retrieves the string of the currently highlighted option.
 *
 * @details
 * Only valid for @ref MENU_TYPE_SELECTABLE menus. Returns
 * @ref ERR_INVALID_ARGUMENT for @ref MENU_TYPE_DATA_VIEW menus.
 *
 * @param[in]  menu_handle       Pointer to the menu handle to query.
 * @param[out] selected_option_o Pointer to a buffer that will receive the
 *                               selected option string (null-terminated).
 *                               Must be at least @ref SSD1309_MAX_CHARS_IN_LINE bytes.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if the menu type is @ref MENU_TYPE_DATA_VIEW
 * - ERR_DATA_COPY_FAILURE if the string copy fails
 */
ERR_te menu_get_selected_option(MENU_HANDLE_ts const *menu_handle, char *selected_option_o);

/**
 * @brief Moves the menu selection cursor by one step in the given direction.
 *
 * @details
 * Increments or decrements the internal selection index. The actual
 * clamping to valid bounds and display update happen on the next call
 * to @ref menu_run_handle.
 *
 * @param[in,out] menu_handle  Pointer to the menu handle to scroll.
 * @param[in]     vertical_dir @ref UP to move the selection up,
 *                             @ref DOWN to move it down.
 *
 * @return
 * - ERR_OK always
 */
ERR_te menu_scroll(MENU_HANDLE_ts *menu_handle, VERTICAL_DIR_te vertical_dir);

/**
 * @brief Pops and returns the previous menu from the navigation history stack.
 *
 * @details
 * Used to implement back-navigation. The popped handle is removed from
 * the stack.
 *
 * @param[in,out] menu_handle       Pointer to the current menu handle.
 * @param[out]    prev_menu_handle_o Pointer to a handle pointer that will be
 *                                   set to the previous menu.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if the navigation history stack is empty
 */
ERR_te menu_get_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts **prev_menu_handle_o);

/**
 * @brief Pushes a menu onto the navigation history stack of another menu.
 *
 * @details
 * Used to record the menu to return to when the user navigates back.
 * Call this before transitioning to a new menu.
 *
 * @param[in,out] menu_handle      Pointer to the current menu handle whose
 *                                 history stack will be updated.
 * @param[in]     prev_menu_handle Pointer to the menu handle to push onto
 *                                 the history stack.
 *
 * @return
 * - ERR_OK on success
 * - ERR_NOT_ENOUGH_SPACE if the history stack is full
 */
ERR_te menu_set_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts *prev_menu_handle);

/** @} */

#endif

/** @} */