/**
 * @file menu.h
 * @author github.com/Baksi675
 * @brief Menu header file
 * @version 0.1
 * @date 2026-02-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef MENU_H__
#define MENU_H__

#include <stdint.h>

#include "common.h"
#include "ssd1309.h"
#include "err.h"
#include "configuration.h"

typedef enum {
	MENU_TYPE_SELECTABLE,
	MENU_TYPE_DATA_VIEW
}MENU_TYPE_ts;

typedef struct {
	char title[SSD1309_MAX_CHARS_IN_LINE];									
	char options[CONFIG_MENU_MAX_OPTIONS][SSD1309_MAX_CHARS_IN_LINE];		
	MENU_TYPE_ts type;												
	ERR_te (*get_value_fn)(uint8_t index, char **value_o);
	char name[CONFIG_MENU_MAX_NAME_LEN];		
}MENU_CFG_ts;

typedef struct menu_handle_s MENU_HANDLE_ts;

ERR_te menu_init_subsys(void);
ERR_te menu_deinit_subsys(void);
ERR_te menu_start_subsys(void);
ERR_te menu_stop_subsys(void);
ERR_te menu_init_handle(MENU_CFG_ts *menu_cfg, MENU_HANDLE_ts **menu_handle_o);
ERR_te menu_run_handle(MENU_HANDLE_ts *menu_handle);
ERR_te menu_run_handle_all(void);
ERR_te menu_get_selected_option(MENU_HANDLE_ts const *menu_handle, char *selected_option_o);
ERR_te menu_scroll(MENU_HANDLE_ts *menu_handle, VERTICAL_DIR_te vertical_dir);
ERR_te menu_get_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts **prev_menu_handle_o);
ERR_te menu_set_prev_menu(MENU_HANDLE_ts *menu_handle, MENU_HANDLE_ts *prev_menu_handle);

#endif