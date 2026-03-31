/**
 * @file menu.c
 * @author github.com/Baksi675
 * @brief Menu implementation file
 * @version 0.1
 * @date 2025-12-21
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "menu.h"
#include "common.h"
#include "init.h"
#include "ssd1309.h"
#include "err.h"
#include "configuration.h"
#include "log.h"
#include "cmd.h"

/**
 * @brief Internal structure representing a single menu instance.
 *
 * @details
 * Holds all runtime state for one menu, including the copied configuration,
 * scroll/selection tracking, the navigation history stack, and lifecycle flags.
 *
 * This structure is opaque to the caller; it is accessed only through
 * the @ref MENU_HANDLE_ts pointer returned by @ref menu_init_handle.
 */
struct menu_handle_s {
    /** Title string displayed at the bottom of the screen. */
    char title[SSD1309_MAX_CHARS_IN_LINE];

    /** Array of option label strings copied from the configuration. */
    char options[CONFIG_MENU_MAX_OPTIONS][SSD1309_MAX_CHARS_IN_LINE];

    /** Rendering and interaction mode of this menu. */
    MENU_TYPE_ts type;

    /**
     * Callback for fetching live values in @ref MENU_TYPE_DATA_VIEW mode.
     * NULL for @ref MENU_TYPE_SELECTABLE menus.
     */
    ERR_te (*get_value_fn)(uint8_t index, char **value_o);

    /** Display line number (1-based) that is currently inverted as the selection highlight. */
    uint8_t line_to_highlight;

    /**
     * Total number of scrollable rows. For @ref MENU_TYPE_DATA_VIEW menus
     * this is twice the number of options (labels + values interleaved).
     */
    uint8_t options_count;

    /** Zero-based index of the currently selected option. */
    int8_t selected_option;

    /** Zero-based index of the selected option on the previous render, used to detect scroll direction. */
    int8_t prev_selected_option;

    /** Zero-based index of the first option row currently visible on screen. */
    int8_t first_visible_option;

    /** Zero-based index of the last option row currently visible on screen. */
    int8_t last_visible_option;

    /** Stack of previous menu handles for back-navigation. */
    MENU_HANDLE_ts *prev_menu[CONFIG_MENU_MAX_BUF_SIZE];

    /** Number of entries currently on the @ref prev_menu stack. */
    uint8_t prev_menu_count;

    /** Human-readable name of this menu instance (null-terminated). */
    char name[CONFIG_MENU_MAX_NAME_LEN];

    /** True when this slot is occupied by an active menu instance. */
    bool in_use;
};

/**
 * @brief Internal state of the menu subsystem.
 *
 * @details
 * Holds the pool of menu handles, the count of active handles,
 * and the subsystem lifecycle flags.
 */
struct internal_state_s {
    /** Pool of menu handle instances. */
    struct menu_handle_s menus[CONFIG_MENU_MAX_OBJECTS];

    /** Number of currently active (in-use) menu handles. */
    uint8_t menu_num;

    /** Active log level for this subsystem. */
    LOG_LEVEL_te log_level;

    /** Module identifier used for log messages. */
    MODULES_te subsys;

    /** True after @ref menu_init_subsys has completed successfully. */
    bool initialized;

    /** True after @ref menu_start_subsys has been called. */
    bool started;
};

/** @brief Singleton instance of the menu subsystem internal state. */
static struct internal_state_s internal_state = { 0 };

/* ---- Forward declarations for internal helpers ---- */
static ERR_te menu_selectable_run(MENU_HANDLE_ts *menu_handle);
static ERR_te menu_dataview_run(MENU_HANDLE_ts *menu_handle);

/* ---- Forward declarations for command handlers ---- */
static ERR_te menu_cmd_info_handler(uint32_t argc, char **argv);
static ERR_te menu_cmd_scroll_handler(uint32_t argc, char **argv);

/**
 * @brief Table of CLI commands registered by the menu subsystem.
 *
 * @details
 * Registered with the command subsystem via @ref menu_cmd_client_info
 * during @ref menu_init_subsys.
 */
static CMD_INFO_ts menu_cmds[] = {
    {
        .name = "scroll",
        .help = "Scroll up or down, usage: menu scroll <menu> <up|down>",
        .handler = menu_cmd_scroll_handler
    },
    {
        .name = "info",
        .help = "Shows menu information, usage: menu info",
        .handler = menu_cmd_info_handler
    }
};

/**
 * @brief Registration descriptor passed to the command subsystem.
 *
 * @details
 * Bundles the command table, its size, the subsystem name prefix used
 * on the CLI, and a pointer to the runtime log-level variable.
 */
static CMD_CLIENT_INFO_ts menu_cmd_client_info = {
    .cmds_ptr = menu_cmds,
    .num_cmds = sizeof(menu_cmds) / sizeof(menu_cmds[0]),
    .name = "menu",
    .log_level_ptr = &internal_state.log_level
};

/**
 * @defgroup menu_public_apis Menu Public APIs
 * @{
 */

/** @brief Initializes the menu subsystem. @see menu_init_subsys */
ERR_te menu_init_subsys(void) {
    ERR_te err = 0;

    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    internal_state = (struct internal_state_s){ 0 };

    internal_state.log_level = LOG_LEVEL_INFO;
    internal_state.subsys = MODULES_MENU;
    internal_state.initialized = true;
    internal_state.started = false;

    init_log();

    err = cmd_register(&menu_cmd_client_info);
    if(err != ERR_OK) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_init_subsys: cmd_register error"
        );

        return err;
    }
    LOG_INFO(
        internal_state.subsys,
        internal_state.log_level,
        "menu_init_subsys: subsys initialized"
    );

    return ERR_OK;
}

/** @brief Deinitializes the menu subsystem. @see menu_deinit_subsys */
ERR_te menu_deinit_subsys(void) {
    if(internal_state.initialized && !internal_state.started) {
        internal_state = (struct internal_state_s){ 0 };

        cmd_deregister(&menu_cmd_client_info);
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_deinit_subsys: subsys is not initialized or not stopped"
        );

        return ERR_DEINITIALIZATION_FAILURE;
    }
    LOG_INFO(
        internal_state.subsys,
        internal_state.log_level,
        "menu_deinit_subsys: subsys deinitialized"
    );

    return ERR_OK;
}

/** @brief Starts the menu subsystem. @see menu_start_subsys */
ERR_te menu_start_subsys(void) {
    if(internal_state.initialized && !internal_state.started) {
        internal_state.started = true;

        LOG_INFO(
            internal_state.subsys,
            internal_state.log_level,
            "menu_start_subsys: subsys started"
        );
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_start_subsys: subsys not initialized or already started"
        );

        return ERR_UNKNOWN;
    }

    return ERR_OK;
}

/** @brief Stops the menu subsystem. @see menu_stop_subsys */
ERR_te menu_stop_subsys(void) {
    if(internal_state.initialized && internal_state.started) {
        internal_state.started = false;

        LOG_INFO(
            internal_state.subsys,
            internal_state.log_level,
            "menu_stop_subsys: subsys stopped"
        );
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_stop_subsys: subsys not initialized or already already stopped"
        );

        return ERR_UNKNOWN;
    }

    return ERR_OK;
}

/** @brief Initializes and registers a menu handle. @see menu_init_handle */
ERR_te menu_init_handle(MENU_CFG_ts *menu_cfg, MENU_HANDLE_ts **menu_handle_o) {
    if(internal_state.menu_num == CONFIG_MENU_MAX_OBJECTS)
        return ERR_NOT_ENOUGH_SPACE;

    // Count number of options in the input data
    uint8_t options_count = 0;
    while(menu_cfg->options[options_count][0] != '\0')
        options_count++;

    if(options_count == 0)
        return ERR_INVALID_ARGUMENT;
    else if(options_count > CONFIG_MENU_MAX_OPTIONS)
        return ERR_NOT_ENOUGH_SPACE;

    for(uint32_t i = 0; i < CONFIG_MENU_MAX_OBJECTS; i++) {
        if(internal_state.menus[i].in_use == false) {
            // Copy input data (menu title) to internal state
            txt_cpy(
                internal_state.menus[i].title,
                menu_cfg->title,
                get_str_len(menu_cfg->title)
            );

            // Copy input data (menu name) to internal state
            txt_cpy(
                internal_state.menus[i].name,
                menu_cfg->name,
                get_str_len(menu_cfg->name)
            );

            // Copy input data (menu options) to internal state
            for(uint8_t j = 0; j < options_count; j++) {
                txt_cpy(
                    internal_state.menus[i].options[j],
                    menu_cfg->options[j],
                    get_str_len(menu_cfg->options[j])
                );
            }

            // For dataview mode, options count covers both the labels and their live values
            if(menu_cfg->type == MENU_TYPE_DATA_VIEW) {
                options_count *= 2;
            }

            if(options_count > 7) {
                internal_state.menus[i].last_visible_option = 6;
            }
            else {
                internal_state.menus[i].last_visible_option = options_count - 1;
            }

            internal_state.menus[i].line_to_highlight = 1;
            internal_state.menus[i].selected_option = 0;
            internal_state.menus[i].prev_selected_option = 0;
            internal_state.menus[i].get_value_fn = menu_cfg->get_value_fn;
            internal_state.menus[i].type = menu_cfg->type;
            internal_state.menus[i].first_visible_option = 0;
            internal_state.menus[i].options_count = options_count;
            internal_state.menus[i].in_use = true;

            *menu_handle_o = &internal_state.menus[i];

            internal_state.menu_num++;

            break;
        }
    }

    return ERR_OK;
}

/** @brief Renders a single menu to the display. @see menu_run_handle */
ERR_te menu_run_handle(MENU_HANDLE_ts *menu_handle) {
    ssd1309_clear_rect(1, 1, 128, 64, false);

    if(menu_handle->type == MENU_TYPE_SELECTABLE) {
        menu_selectable_run(menu_handle);
    }
    else if(menu_handle->type == MENU_TYPE_DATA_VIEW) {
        menu_dataview_run(menu_handle);
    }

    ssd1309_update(false);

    return ERR_OK;
}

/** @brief Renders all registered menus to the display. @see menu_run_handle_all */
ERR_te menu_run_handle_all(void) {
    for(uint32_t i = 0; i < CONFIG_MENU_MAX_OBJECTS; i++) {
        if(internal_state.menus[i].in_use == true) {
            menu_run_handle(&internal_state.menus[i]);
        }
    }

    return ERR_OK;
}

/** @brief Retrieves the string of the currently highlighted option. @see menu_get_selected_option */
ERR_te menu_get_selected_option(MENU_HANDLE_ts const *menu_handle, char *selected_option_o) {
    ERR_te err;

    if(menu_handle->type == MENU_TYPE_DATA_VIEW) {
        return ERR_INVALID_ARGUMENT;
    }

    err = str_cpy(
        selected_option_o,
        menu_handle->options[menu_handle->selected_option],
        get_str_len(menu_handle->options[menu_handle->selected_option]) + 1
    );
    if(err != ERR_OK) {
        return ERR_DATA_COPY_FAILURE;
    }

    return ERR_OK;
}

/** @brief Moves the menu selection cursor by one step in the given direction. @see menu_scroll */
ERR_te menu_scroll(MENU_HANDLE_ts *menu_handle, VERTICAL_DIR_te vertical_dir) {
    switch(vertical_dir) {
        case DOWN:
            menu_handle->selected_option += 1;
            break;

        case UP:
            menu_handle->selected_option -= 1;
            break;
    }

    return ERR_OK;
}

/** @brief Pops and returns the previous menu from the navigation history stack. @see menu_get_prev_menu */
ERR_te menu_get_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts **prev_menu_handle_o) {
    if(menu_handle->prev_menu_count == 0)
        return ERR_INVALID_ARGUMENT;

    *prev_menu_handle_o = menu_handle->prev_menu[--menu_handle->prev_menu_count];

    return ERR_OK;
}

/** @brief Pushes a menu onto the navigation history stack of another menu. @see menu_set_prev_menu */
ERR_te menu_set_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts *prev_menu_handle) {
    if(menu_handle->prev_menu_count == CONFIG_MENU_MAX_BUF_SIZE)
        return ERR_NOT_ENOUGH_SPACE;

    menu_handle->prev_menu[menu_handle->prev_menu_count++] = prev_menu_handle;

    return ERR_OK;
}

/** @} */

/**
 * @defgroup menu_internal_helpers Menu Internal Helpers
 * @{
 */

/**
 * @brief Renders a @ref MENU_TYPE_SELECTABLE menu to the display.
 *
 * @details
 * Clamps the selection index to valid bounds, updates the visible window
 * and highlight line based on scroll direction, draws each visible option
 * string, inverts the highlight line to show selection, and draws the title.
 *
 * The display supports 7 option lines (lines 1–7) plus a title line (line 8).
 * When the option count exceeds 7, a sliding window tracks which options
 * are currently visible.
 *
 * @param[in,out] menu_handle Pointer to the menu handle to render.
 *
 * @return
 * - ERR_OK always
 */
static ERR_te menu_selectable_run(MENU_HANDLE_ts *menu_handle) {
    // Clamp selected item if possible boundaries exceeded
    if(menu_handle->selected_option < 0) {
        menu_handle->selected_option = 0;
    }
    else if(menu_handle->selected_option >= menu_handle->options_count) {
        menu_handle->selected_option = menu_handle->options_count - 1;
    }

    // Items in options list is greater than 7, thus shifting of the currently showed items is needed
    if(menu_handle->options_count > 7) {
        // If selected item is outside (positive direction) of the currently showed items list shift
        // the window by one in positive direction
        if(menu_handle->selected_option > menu_handle->last_visible_option) {
            menu_handle->first_visible_option++;
            menu_handle->last_visible_option++;
        }
        // Opposite direction
        else if (menu_handle->selected_option < menu_handle->first_visible_option) {
            menu_handle->first_visible_option--;
            menu_handle->last_visible_option--;
        }

        // If selection reached last visible item, shift selection in positive direction
        if(menu_handle->selected_option == menu_handle->last_visible_option) {
            menu_handle->line_to_highlight = 7;
        }
        // If selection reached first item, shift selection in negative direction
        else if(menu_handle->selected_option == menu_handle->first_visible_option) {
            menu_handle->line_to_highlight = 1;
        }
        // If selection is in-between, change selection
        else {
            // In positive direction
            if(menu_handle->selected_option > menu_handle->prev_selected_option) {
                menu_handle->line_to_highlight = menu_handle->selected_option + 1;
            }
            // In negative direction
            else if(menu_handle->selected_option < menu_handle->prev_selected_option) {
                menu_handle->line_to_highlight = 7 - (menu_handle->last_visible_option - menu_handle->selected_option);
            }
        }
        menu_handle->prev_selected_option = menu_handle->selected_option;
    }
    // Shifting of the currently showed items is not needed
    else {
        // Select first item in the list
        if(menu_handle->selected_option == menu_handle->first_visible_option) {
            menu_handle->line_to_highlight = 1;
        }
        // Select next item in the list (positive direction)
        else if(menu_handle->selected_option > menu_handle->prev_selected_option) {
            menu_handle->line_to_highlight = menu_handle->selected_option + 1;
        }
        // Select next item in the list (negative direction)
        else if(menu_handle->selected_option < menu_handle->prev_selected_option) {
            menu_handle->line_to_highlight = menu_handle->options_count - (menu_handle->last_visible_option - menu_handle->selected_option);
        }
        else if(menu_handle->selected_option == menu_handle->last_visible_option) {
            menu_handle->line_to_highlight = menu_handle->selected_option + 1;
        }
        menu_handle->prev_selected_option = menu_handle->selected_option;
    }

    // Draw item list on the screen
    uint8_t line_counter = 0;
    for(uint8_t i = menu_handle->first_visible_option; i <= menu_handle->last_visible_option; i++) {
        line_counter++;
        ssd1309_draw_text(menu_handle->options[i],
            get_str_len(menu_handle->options[i]), line_counter, false);
    }

    // Simulate selection by inverting the value of pixels
    ssd1309_invert_line(menu_handle->line_to_highlight, false);

    ssd1309_draw_text(menu_handle->title, 16, 8, false);

    return ERR_OK;
}

/**
 * @brief Renders a @ref MENU_TYPE_DATA_VIEW menu to the display.
 *
 * @details
 * Behaves like @ref menu_selectable_run for window and highlight tracking,
 * but renders alternating rows: even-indexed rows display the static option
 * label, odd-indexed rows display the live value returned by
 * @ref MENU_CFG_ts::get_value_fn for the corresponding option index.
 *
 * @param[in,out] menu_handle Pointer to the menu handle to render.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DATA_ACQUISITION_FAILURE if @ref MENU_CFG_ts::get_value_fn returns an error
 */
static ERR_te menu_dataview_run(MENU_HANDLE_ts *menu_handle) {
    ERR_te err;

    // Clamp selected item if possible boundaries exceeded
    if(menu_handle->selected_option < 0) {
        menu_handle->selected_option = 0;
    }
    else if(menu_handle->selected_option >= menu_handle->options_count) {
        menu_handle->selected_option = menu_handle->options_count - 1;
    }

    // Items in options list is greater than 7, thus shifting of the currently showed items is needed
    if(menu_handle->options_count > 7) {
        // If selected item is outside (positive direction) of the currently showed items list shift
        // the window by one in positive direction
        if(menu_handle->selected_option > menu_handle->last_visible_option) {
            menu_handle->first_visible_option++;
            menu_handle->last_visible_option++;
        }
        // Opposite direction
        else if (menu_handle->selected_option < menu_handle->first_visible_option) {
            menu_handle->first_visible_option--;
            menu_handle->last_visible_option--;
        }

        // If selection reached last visible item, shift selection in positive direction
        if(menu_handle->selected_option == menu_handle->last_visible_option) {
            menu_handle->line_to_highlight = 7;
        }
        // If selection reached first item, shift selection in negative direction
        else if(menu_handle->selected_option == menu_handle->first_visible_option) {
            menu_handle->line_to_highlight = 1;
        }
        // If selection is in-between, change selection
        else {
            // In positive direction
            if(menu_handle->selected_option > menu_handle->prev_selected_option) {
                menu_handle->line_to_highlight = menu_handle->selected_option + 1;
            }
            // In negative direction
            else if(menu_handle->selected_option < menu_handle->prev_selected_option) {
                menu_handle->line_to_highlight = 7 - (menu_handle->last_visible_option - menu_handle->selected_option);
            }
        }
        menu_handle->prev_selected_option = menu_handle->selected_option;
    }
    // Shifting of the currently showed items is not needed
    else {
        // Select first item in the list
        if(menu_handle->selected_option == menu_handle->first_visible_option) {
            menu_handle->line_to_highlight = 1;
        }
        // Select next item in the list (positive direction)
        else if(menu_handle->selected_option > menu_handle->prev_selected_option) {
            menu_handle->line_to_highlight = menu_handle->selected_option + 1;
        }
        // Select next item in the list (negative direction)
        else if(menu_handle->selected_option < menu_handle->prev_selected_option) {
            menu_handle->line_to_highlight = menu_handle->options_count - (menu_handle->last_visible_option - menu_handle->selected_option);
        }
        else if(menu_handle->selected_option == menu_handle->last_visible_option) {
            menu_handle->line_to_highlight = menu_handle->selected_option + 1;
        }
        menu_handle->prev_selected_option = menu_handle->selected_option;
    }

    // Draw item list on the screen
    uint8_t line_counter = 0;
    uint8_t real_counter = 0;

    if(menu_handle->first_visible_option > 0)
        real_counter = menu_handle->first_visible_option / 2;

    for(uint8_t i = menu_handle->first_visible_option; i <= menu_handle->last_visible_option; i++) {
        line_counter++;
        // Counter is at option label row
        if(i % 2 == 0) {
            ssd1309_draw_text(menu_handle->options[real_counter],
                get_str_len(menu_handle->options[real_counter]), line_counter, false);
        }
        // Counter is at live value row
        else {
            char *value;

            err = menu_handle->get_value_fn(real_counter, &value);
            if(err != ERR_OK) {
                return ERR_DATA_ACQUISITION_FAILURE;
            }

            ssd1309_draw_text(value,
                get_str_len(value),
                line_counter, false);

            real_counter++;
        }
    }

    // Simulate selection by inverting the value of pixels
    ssd1309_invert_line(menu_handle->line_to_highlight, false);

    ssd1309_draw_text(menu_handle->title, 16, 8, false);

    return ERR_OK;
}

/** @} */

/**
 * @defgroup menu_command_handlers Menu Command Handlers
 * @{
 */

/**
 * @brief CLI handler for the "info" command. Logs the names of all active menu handles.
 *
 * @details
 * Expected invocation: `menu info`
 *
 * @param[in] argc Argument count. Must be exactly 2.
 * @param[in] argv Argument list: argv[0] = "menu", argv[1] = "info".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 2
 */
static ERR_te menu_cmd_info_handler(uint32_t argc, char **argv) {
    if(argc != 2) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_cmd_info_handler: invalid arguments"
        );

        return ERR_INVALID_ARGUMENT;
    }

    LOG_INFO(internal_state.subsys, internal_state.log_level, "Printing menu information:");

    for(uint32_t i = 0; i < CONFIG_MENU_MAX_OBJECTS; i++) {
        if(internal_state.menus[i].in_use == true) {
            LOG_INFO(
                internal_state.subsys,
                internal_state.log_level,
                "name: %s",
                internal_state.menus[i].name
            );
        }
    }

    return ERR_OK;
}

/**
 * @brief CLI handler for the "scroll" command. Scrolls a named menu and re-renders it.
 *
 * @details
 * Expected invocation: `menu scroll <n> <up|down>`
 *
 * Searches the registered menu pool for a handle whose name matches
 * @c argv[2], calls @ref menu_scroll in the specified direction, then
 * calls @ref menu_run_handle to update the display.
 *
 * @param[in] argc Argument count. Must be exactly 4.
 * @param[in] argv Argument list: argv[0] = "menu", argv[1] = "scroll",
 *                 argv[2] = menu name, argv[3] = "up" or "down".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 4, the name is not found,
 *   or the direction string is not recognized
 */
static ERR_te menu_cmd_scroll_handler(uint32_t argc, char **argv) {
    if(argc != 4) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "menu_cmd_scroll_handler: invalid arguments"
        );

        return ERR_INVALID_ARGUMENT;
    }

    for(uint32_t i = 0; i < CONFIG_MENU_MAX_OBJECTS; i++) {
        if(str_cmp(argv[2], internal_state.menus[i].name) == true) {
            if(str_cmp(argv[3], "up") == true) {
                menu_scroll(&internal_state.menus[i], UP);
            }
            else if(str_cmp(argv[3], "down") == true) {
                menu_scroll(&internal_state.menus[i], DOWN);
            }
            else {
                LOG_ERROR(
                    internal_state.subsys,
                    internal_state.log_level,
                    "menu_cmd_scroll_handler: invalid arguments"
                );
                return ERR_INVALID_ARGUMENT;
            }
            menu_run_handle(&internal_state.menus[i]);

            return ERR_OK;
        }
        if(i == internal_state.menu_num - 1) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "menu_cmd_scroll_handler: invalid arguments"
            );

            return ERR_INVALID_ARGUMENT;
        }
    }

    return ERR_OK;
}

/** @} */