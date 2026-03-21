/**
 * @file render.c
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
#include "ssd1309.h"
#include "err.h"
#include "configuration.h"
#include "log.h"
#include "cmd.h"

struct menu_handle_s {
	char title[SSD1309_MAX_CHARS_IN_LINE];							
	char options[CONFIG_MENU_MAX_OPTIONS][SSD1309_MAX_CHARS_IN_LINE];		
	MENU_TYPE_ts type;										
	ERR_te (*get_value_fn)(uint8_t index, char **value_o);			
	uint8_t line_to_highlight;
	uint8_t options_count;									
	int8_t selected_option;											
	int8_t prev_selected_option;									
	int8_t first_visible_option;									
	int8_t last_visible_option;	
	MENU_HANDLE_ts *prev_menu[CONFIG_MENU_MAX_BUF_SIZE];
	uint8_t prev_menu_count;	
	char name[CONFIG_MENU_MAX_NAME_LEN];
	bool in_use;							
};

struct internal_state_s {
	struct menu_handle_s menus[CONFIG_MENU_MAX_OBJECTS];		
	uint8_t menu_num;	
	LOG_LEVEL_te log_level;
	MODULES_te subsys;
	bool initialized;
	bool started;									
};
static struct internal_state_s internal_state = { 0 };

static ERR_te menu_selectable_run(MENU_HANDLE_ts *menu_handle);
static ERR_te menu_dataview_run(MENU_HANDLE_ts *menu_handle);

static ERR_te menu_cmd_info_handler(uint32_t argc, char **argv);
static ERR_te menu_cmd_scroll_handler(uint32_t argc, char **argv);

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

static CMD_CLIENT_INFO_ts menu_cmd_client_info = {
	.cmds_ptr = menu_cmds,
	.num_cmds = sizeof(menu_cmds) / sizeof(menu_cmds[0]),
	.name = "menu",
	.log_level_ptr = &internal_state.log_level
};

 /** 
 * @defgroup MENU_Public_APIs MENU Public APIs
 * @{
 */

/**
 * @brief Initializes the menu subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te menu_init_subsys(void) {
	ERR_te err = 0;

	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"menu_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };
	
	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_MENU;
	internal_state.initialized = true;
	internal_state.started = false;
	
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

/**
 * @brief Deinitializes the menu subsystem internal state to zeroes.
 * 
 * @return ERR_te The error generated during execution.
 */
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

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
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

/**
 * @brief Stops the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
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

/**
 * @brief Registers a menu to the internal state of the render module. Initializes the menu to work with
 * different number of options.
 * 
 * @param[in] menu_cfg An input pointer to a configuration structure.
 * @param[out] menu_handle_o An output pointer to the handle of the registered and initialzied menu. 
 * @return ERR_te The error generated by the function, ERR_OK means no error occured.
 */
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

			// For dataview mode, options count means now on both the options and the data for the options as well
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

/**
 * @brief Run the menu handle as part of the state machine.
 * 
 * @param[in] menu_handle The menu handle to run. 
 * @return ERR_te Error generated during execution.
 */
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

/**
 * @brief Runs all handles.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te menu_run_handle_all(void) {
	for(uint32_t i = 0; i < CONFIG_MENU_MAX_OBJECTS; i++) {
		if(internal_state.menus[i].in_use == true) {
			menu_run_handle(&internal_state.menus[i]);
		}
	}

	return ERR_OK;
}

/**
 * @brief Gets the selected option of the menu.
 * 
 * @param[in] menu_handle An input pointer to the menu handle. 
 * @param[out] selected_option_o An output pointer to the selected option of the menu. 
 * @return ERR_te The error generated by the function, ERR_OK means no error occured.
 */
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

/**
 * @brief Sets the selected option of the menu.
 * 
 * @param[in] menu_handle An input pointer to the menu handle. 
 * @param[out] selected_option An input value to set the selected option of the menu,
 * @return ERR_te The error generated by the function, ERR_OK means no error occured.
 */
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

/**
 * @brief Obtains the previous menu of the menu handle.
 * 
 * @param[in] menu_handle The menu handle to obtain the previous menu of. 
 * @param[out] prev_menu_handle_o The previous menu handle.
 * @return ERR_te Error generated during execution. 
 */
ERR_te menu_get_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts **prev_menu_handle_o) {
	if(menu_handle->prev_menu_count == 0)
		return ERR_INVALID_ARGUMENT;

	*prev_menu_handle_o = menu_handle->prev_menu[--menu_handle->prev_menu_count];

	return ERR_OK;
}

/**
 * @brief Sets the previous menu of the menu handle.
 * 
 * @param[in] menu_handle The menu handle to set the previous menu of. 
 * @param[in] prev_menu_handle The previous menu handle.
 * @return ERR_te Error generated during execution.
 */
ERR_te menu_set_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts *prev_menu_handle) {
	if(menu_handle->prev_menu_count == CONFIG_MENU_MAX_BUF_SIZE)
		return ERR_NOT_ENOUGH_SPACE;

	menu_handle->prev_menu[menu_handle->prev_menu_count++] = prev_menu_handle;

	return ERR_OK;
}

/** @} */

  /** 
 * @defgroup MENU_INTERNAL_HELPERS MENU INTERNAL HELPERS
 * @{
 */

/**
 * @brief Runs a selectable type menu as part of a state machine.
 * 
 * @param[in] menu_handle The menu handle to run. 
 * @return ERR_te Error generated during execution.
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
		// the windows by one in positive direction
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
 * @brief Runs a dataview type menu as part of a state machine.
 * 
 * @param[in] menu_handle The menu handle to run. 
 * @return ERR_te Error generated during execution.
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
		// the windows by one in positive direction
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
		// Counter is at option
		if(i % 2 == 0) {
			ssd1309_draw_text(menu_handle->options[real_counter], 
				get_str_len(menu_handle->options[real_counter]), line_counter, false);
		}
		// Counter is at data
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
 * @defgroup MENU_COMMAND_HANDLERS MENU COMMAND HANDLERS
 * @{
 */

/**
 * @brief Handler routine for the info command. Shows information about available commands.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
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
 * @brief Handler routine for the scroll command.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
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

			return  ERR_OK;
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




 