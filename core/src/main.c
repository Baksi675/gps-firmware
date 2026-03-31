/**
 * @file main.c
 * @author github.com/Baksi675
 * @brief Application entry point for the GPS tracking device.
 *
 * @details
 * This file contains the top-level application logic for a GPS tracking
 * device built on the STM32F401RE microcontroller. It ties together all
 * subsystems and drives the main loop.
 *
 * The device provides:
 * - Real-time GPS data display on an SSD1309 OLED via a scrollable menu system
 * - Three-button navigation (left scroll, middle select/back-hold, right scroll)
 * - Periodic GPS data logging to SD card via the datalog module
 * - A USART console for runtime command dispatch and log level control
 *
 * Subsystem initialization order in @ref init_all:
 * -# IO (test LED)
 * -# Console (USART1, log dependency initialized internally)
 * -# SSD1309 OLED display
 * -# Buttons (left, middle, right)
 * -# NEO-6 GPS module
 * -# Menu system (main, location, date/time, movement, accuracy, satellites)
 * -# Datalog (SD card, 10-second interval)
 *
 * @attention On the prototype hardware, the FatFs diskio.c CS pin must be
 *            configured as GPIOB pin 6.
 *
 * @version 0.1
 * @date 2026-01-01
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "arm_cortex_m4_systick.h"
#include "button.h"
#include "common.h"
#include "err.h"
#include "io.h"
#include "neo6.h"
#include "menu.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "console.h"
#include "ssd1309.h"
#include "datalog.h"

/* ---- Application-level subsystem handles ---- */
SSD1309_HANDLE_ts *ssd1309_handle;  /**< OLED display handle. */
BUTTON_HANDLE_ts *lbtn;             /**< Left button handle. */
BUTTON_HANDLE_ts *mbtn;             /**< Middle button handle (select / back-hold). */
BUTTON_HANDLE_ts *rbtn;             /**< Right button handle. */
MENU_HANDLE_ts *current_menu;       /**< Pointer to the menu currently displayed. */
MENU_HANDLE_ts *main_menu;          /**< Top-level selectable menu. */
MENU_HANDLE_ts *location_menu;      /**< Data-view menu: latitude, longitude, height, geoid. */
MENU_HANDLE_ts *datetime_menu;      /**< Data-view menu: GPS date and time. */
MENU_HANDLE_ts *movement_menu;      /**< Data-view menu: speed and direction. */
MENU_HANDLE_ts *accuracy_menu;      /**< Data-view menu: fix status, fix type, DOP values. */
MENU_HANDLE_ts *satellites_menu;    /**< Data-view menu: satellite counts. */
NEO6_HANDLE_ts *neo6_handle;        /**< NEO-6 GPS module handle. */
DATALOG_HANDLE_ts *datalog_handle;  /**< SD card data log handle. */

/* ---- Forward declarations ---- */
static void init_all(void);
static void run_all(void);
static ERR_te acquire_neo6_location(uint8_t index, char **value_o);
static ERR_te acquire_datetime(uint8_t index, char **value_o);
static ERR_te acquire_movement(uint8_t index, char **value_o);
static ERR_te acquire_accuracy(uint8_t index, char **value_o);
static ERR_te acquire_satellites(uint8_t index, char **value_o);

/* ---- Button state flags ---- */
bool lbtn_pushed = false;       /**< True while the left button is held down. */
bool mbtn_pushed = false;       /**< True while the middle button is held down. */
bool rbtn_pushed = false;       /**< True while the right button is held down. */
bool can_toggle_lbtn = true;    /**< Edge-detect guard for the left button. */
bool can_toggle_mbtn = true;    /**< Edge-detect guard for the middle button. */
bool can_toggle_rbtn = true;    /**< Edge-detect guard for the right button. */
bool mbtn_held = false;         /**< True when the middle button has been held long enough. */
bool can_hold_mbtn = true;      /**< Edge-detect guard for the middle button hold. */

/** True when the display should be redrawn on the next iteration. */
bool refresh = true;

/** Buffer for the option string returned by @ref menu_get_selected_option. */
char selected_option[SSD1309_MAX_CHARS_IN_LINE];

/** Systick timestamp (ms) of the most recent display refresh. */
uint32_t refreshed_ms;

int main(void) {
	init_all();

	// Delay here is needed to not crash the system
	DELAY(50);

	while(1) {
		run_all();
	}

	return 0;
}

/**
 * @brief Initializes all subsystems and hardware peripherals.
 *
 * @details
 * Called once at startup. Configures every subsystem in dependency order
 * and sets @ref current_menu to the top-level main menu.
 * See the @ref main.c file description for the full initialization sequence.
 */
static void init_all(void) {
	GPIO_CFG_ts test_led = { 0 };
	test_led.mode = GPIO_MODE_OUTPUT;
	test_led.pin = GPIO_PIN_15;
	test_led.port = GPIOB;
	gpio_init(&test_led);
	IO_CFG_ts io_test_led_conf = { 0 };
	IO_HANDLE_ts *io_test_led_handle = { 0 };
	io_test_led_conf.gpio_handle = &test_led;
	str_cpy(io_test_led_conf.name, "test_led", get_str_len("test_led") + 1);
	io_init_subsys();
	io_init_handle(&io_test_led_conf, &io_test_led_handle);
	io_start_subsys();

	CONSOLE_HANDLE_ts console_handle = { 0 };
	console_handle.usart_instance = USART1;
	console_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	console_handle.gpio_port = GPIOA;
	console_handle.gpio_pin = GPIO_PIN_10;
	console_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	console_init(&console_handle);

	SSD1309_CFG_ts ssd1309_conf = { 0 };
	ssd1309_get_def_cfg(&ssd1309_conf);
	ssd1309_conf.i2c_instance = I2C1;
	ssd1309_conf.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF4;
	ssd1309_conf.scl_gpio_pin = GPIO_PIN_8;
	ssd1309_conf.scl_gpio_port = GPIOB;
	ssd1309_conf.sda_gpio_pin = GPIO_PIN_9;
	ssd1309_conf.sda_gpio_port = GPIOB;
	ssd1309_init_subsys();
	ssd1309_init_handle(&ssd1309_conf, &ssd1309_handle);
	ssd1309_start_subsys();

	BUTTON_CFG_ts lbtn_cfg = { 0 };
	lbtn_cfg.gpio_port = GPIOB;
	lbtn_cfg.gpio_pin = GPIO_PIN_5;
	lbtn_cfg.debounce_limit_ms = 50;
	lbtn_cfg.held_limit_ms = 1000;
	lbtn_cfg.pushed_type = BUTTON_PUSHED_TYPE_HIGH;
	str_cpy(lbtn_cfg.name, "leftbtn", get_str_len("leftbtn") + 1);

	BUTTON_CFG_ts mbtn_cfg = { 0 };
	mbtn_cfg.gpio_port = GPIOB;
	mbtn_cfg.gpio_pin = GPIO_PIN_4;
	mbtn_cfg.debounce_limit_ms = 50;
	mbtn_cfg.held_limit_ms = 1000;
	mbtn_cfg.pushed_type = BUTTON_PUSHED_TYPE_HIGH;
	str_cpy(mbtn_cfg.name, "midbtn", get_str_len("midbtn") + 1);

	BUTTON_CFG_ts rbtn_cfg = { 0 };
	rbtn_cfg.gpio_port = GPIOB;
	rbtn_cfg.gpio_pin = GPIO_PIN_10;
	rbtn_cfg.debounce_limit_ms = 50;
	rbtn_cfg.held_limit_ms = 1000;
	rbtn_cfg.pushed_type = BUTTON_PUSHED_TYPE_HIGH;
	str_cpy(rbtn_cfg.name, "rightbtn", get_str_len("rightbtn") + 1);

	button_init_subsys();
	button_init_handle(&lbtn_cfg, &lbtn);
	button_init_handle(&mbtn_cfg, &mbtn);
	button_init_handle(&rbtn_cfg, &rbtn);
	button_start_subsys();

	NEO6_CFG_ts neo6_cfg = { 0 };
	neo6_cfg.usart_instance = USART6;
	neo6_cfg.usart_baud_rate = 9600;
	neo6_cfg.rx_gpio_port = GPIOA;
	neo6_cfg.rx_gpio_pin = GPIO_PIN_12;
	neo6_cfg.tx_gpio_port = GPIOA;
	neo6_cfg.tx_gpio_pin = GPIO_PIN_11;
	neo6_cfg.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF8;
	neo6_init_subsys();
	neo6_init_handle(&neo6_cfg, &neo6_handle);
	neo6_start_subsys();

	MENU_CFG_ts main_menu_cfg = { 0 };
	txt_cpy(main_menu_cfg.name, "main", get_str_len("main"));
	txt_cpy(main_menu_cfg.title, "      MENU      ", 16);
	txt_cpy(main_menu_cfg.options[0], "LOCATION", get_str_len("LOCATION"));
	txt_cpy(main_menu_cfg.options[1], "DATE/TIME", get_str_len("DATE/TIME"));
	txt_cpy(main_menu_cfg.options[2], "MOVEMENT", get_str_len("MOVEMENT"));
	txt_cpy(main_menu_cfg.options[3], "ACCURACY", get_str_len("ACCURACY"));
	txt_cpy(main_menu_cfg.options[4], "SATELLITES", get_str_len("SATELLITES"));
	main_menu_cfg.type = MENU_TYPE_SELECTABLE;

	MENU_CFG_ts location_menu_cfg = { 0 };
	txt_cpy(location_menu_cfg.name, "loc", get_str_len("loc"));
	txt_cpy(location_menu_cfg.title, "    LOCATION    ", 16);
	txt_cpy(location_menu_cfg.options[0], "Latitude:", get_str_len("Latitude:"));
	txt_cpy(location_menu_cfg.options[1], "Longitude:", get_str_len("Longitude:"));
	txt_cpy(location_menu_cfg.options[2], "Orthometric h.:", get_str_len("Orthometric h.:"));
	txt_cpy(location_menu_cfg.options[3], "Geoid sep.:", get_str_len("Geoid sep.:"));
	location_menu_cfg.type = MENU_TYPE_DATA_VIEW;
	location_menu_cfg.get_value_fn = acquire_neo6_location;

	MENU_CFG_ts datetime_menu_cfg = { 0 };
	txt_cpy(datetime_menu_cfg.name, "dt", get_str_len("dt"));
	txt_cpy(datetime_menu_cfg.title, "   DATE/TIME    ", 16);
	txt_cpy(datetime_menu_cfg.options[0], "Current date:", get_str_len("Current date:"));
	txt_cpy(datetime_menu_cfg.options[1], "Current time:", get_str_len("Current time:"));
	datetime_menu_cfg.type = MENU_TYPE_DATA_VIEW;
	datetime_menu_cfg.get_value_fn = acquire_datetime;

	MENU_CFG_ts movement_menu_cfg = { 0 };
	txt_cpy(movement_menu_cfg.name, "mov", get_str_len("mov"));
	txt_cpy(movement_menu_cfg.title, "    MOVEMENT    ", 16);
	txt_cpy(movement_menu_cfg.options[0], "Movement speed:", get_str_len("Movement speed:"));
	txt_cpy(movement_menu_cfg.options[1], "Movement dir.:", get_str_len("Movement dir.:"));
	movement_menu_cfg.type = MENU_TYPE_DATA_VIEW;
	movement_menu_cfg.get_value_fn = acquire_movement;

	MENU_CFG_ts accuracy_menu_cfg = { 0 };
	txt_cpy(accuracy_menu_cfg.name, "acc", get_str_len("acc"));
	txt_cpy(accuracy_menu_cfg.title, "    ACCURACY    ", 16);
	txt_cpy(accuracy_menu_cfg.options[0], "Fix status:", get_str_len("Fix status:"));
	txt_cpy(accuracy_menu_cfg.options[1], "Fix type:", get_str_len("Fix type:"));
	txt_cpy(accuracy_menu_cfg.options[2], "PDOP:", get_str_len("PDOP:"));
	txt_cpy(accuracy_menu_cfg.options[3], "HDOP:", get_str_len("HDOP:"));
	txt_cpy(accuracy_menu_cfg.options[4], "VDOP:", get_str_len("VDOP:"));
	accuracy_menu_cfg.type = MENU_TYPE_DATA_VIEW;
	accuracy_menu_cfg.get_value_fn = acquire_accuracy;

	MENU_CFG_ts satellites_menu_cfg = { 0 };
	txt_cpy(satellites_menu_cfg.name, "sats", get_str_len("sats"));
	txt_cpy(satellites_menu_cfg.title, "   SATELLITES   ", 16);
	txt_cpy(satellites_menu_cfg.options[0], "Num sats:", get_str_len("Num sats:"));
	txt_cpy(satellites_menu_cfg.options[1], "Num sats used:", get_str_len("Num sats used:"));
	satellites_menu_cfg.type = MENU_TYPE_DATA_VIEW;
	satellites_menu_cfg.get_value_fn = acquire_satellites;

	menu_init_subsys();
	menu_init_handle(&main_menu_cfg, &main_menu);
	menu_init_handle(&location_menu_cfg, &location_menu);
	menu_init_handle(&datetime_menu_cfg, &datetime_menu);
	menu_init_handle(&movement_menu_cfg, &movement_menu);
	menu_init_handle(&accuracy_menu_cfg, &accuracy_menu);
	menu_init_handle(&satellites_menu_cfg, &satellites_menu);
	menu_start_subsys();

	current_menu = main_menu;

	DATALOG_CFG_ts datalog_cfg = { 0 };
	str_cpy(datalog_cfg.name, "datalog", get_str_len("datalog") + 1);
	datalog_cfg.datalog_time = DATALOG_TIME_10S;
	datalog_init_subsys();
	datalog_init_handle(&datalog_cfg, &datalog_handle);
	datalog_start_subsys();
}

/**
 * @brief Runs all subsystems for one iteration of the main loop.
 *
 * @details
 * Called on every iteration of the superloop in @ref main. Performs the
 * following steps in order:
 * -# Updates the NEO-6 GPS parser.
 * -# Processes any pending console input and dispatches commands.
 * -# Appends a GPS log entry if the datalog interval has elapsed.
 * -# Debounces and samples all three buttons.
 * -# Handles button events:
 *    - Left button: scroll down.
 *    - Right button: scroll up.
 *    - Middle button press: navigate into the selected submenu.
 *    - Middle button hold: navigate back to the previous menu.
 * -# Forces a display refresh every 1000 ms regardless of button activity.
 * -# Redraws @ref current_menu if @ref refresh is set.
 *
 * @note Button events use an edge-detect pattern (can_toggle / can_hold flags)
 *       to prevent repeated triggering while a button remains held.
 */
static void run_all(void) {
	neo6_run();

	console_run();

	datalog_run_handle(datalog_handle);

	button_run_handle_all();

	button_get_pushed_state(lbtn, &lbtn_pushed);
	button_get_pushed_state(mbtn, &mbtn_pushed);
	button_get_pushed_state(rbtn, &rbtn_pushed);

	button_get_held_state(mbtn, &mbtn_held);

	if(lbtn_pushed && can_toggle_lbtn) {
		menu_scroll(current_menu, DOWN);

		refresh = true;
		can_toggle_lbtn = false;
	}
	else if(!lbtn_pushed && !can_toggle_lbtn) {
		can_toggle_lbtn = true;
	}

	if(mbtn_pushed && can_toggle_mbtn) {
		menu_get_selected_option(current_menu, selected_option);

		if(current_menu == main_menu) {
			if(str_cmp(selected_option, "LOCATION") == true) {
				menu_set_prev_menu(location_menu, current_menu);
				current_menu = location_menu;
				refresh = true;
			}
			else if(str_cmp(selected_option, "DATE/TIME") == true) {
				menu_set_prev_menu(datetime_menu, current_menu);
				current_menu = datetime_menu;
				refresh = true;
			}
			else if(str_cmp(selected_option, "MOVEMENT") == true) {
				menu_set_prev_menu(movement_menu, current_menu);
				current_menu = movement_menu;
				refresh = true;
			}
			else if(str_cmp(selected_option, "ACCURACY") == true) {
				menu_set_prev_menu(accuracy_menu, current_menu);
				current_menu = accuracy_menu;
				refresh = true;
			}
			else if(str_cmp(selected_option, "SATELLITES") == true) {
				menu_set_prev_menu(satellites_menu, current_menu);
				current_menu = satellites_menu;
				refresh = true;
			}
		}

		can_toggle_mbtn = false;
	}
	else if(!mbtn_pushed && !can_toggle_mbtn) {
		can_toggle_mbtn = true;
	}

	if(mbtn_held && can_hold_mbtn) {
		MENU_HANDLE_ts *prev_menu = { 0 };

		menu_get_prev_menu(current_menu, &prev_menu);
		current_menu = prev_menu;
		refresh = true;

		can_hold_mbtn = false;
	}
	else if(!mbtn_held && !can_hold_mbtn) {
		can_hold_mbtn = true;
	}

	if(rbtn_pushed && can_toggle_rbtn) {
		menu_scroll(current_menu, UP);

		refresh = true;
		can_toggle_rbtn = false;
	}
	else if(!rbtn_pushed && !can_toggle_rbtn) {
		can_toggle_rbtn = true;
	}

	if(systick_get_ms() - refreshed_ms >= 1000) {
		refresh = true;
	}

	if(refresh) {
		menu_run_handle(current_menu);
		refresh = false;
		refreshed_ms = systick_get_ms();
	}
}

/**
 * @brief Data-view callback for the location menu.
 *
 * @details
 * Returns GPS location fields from the NEO-6 info structure:
 * index 0 = latitude, 1 = longitude, 2 = orthometric height, 3 = geoid separation.
 */
static ERR_te acquire_neo6_location(uint8_t index, char **value_o) {
	NEO6_INFO_ts *neo6_info = NULL;
	neo6_get_info(&neo6_info);

	switch(index) {
		case 0: *value_o = neo6_info->lat;        break;
		case 1: *value_o = neo6_info->lon;        break;
		case 2: *value_o = neo6_info->ort_height; break;
		case 3: *value_o = neo6_info->geoid_sep;  break;
	}

	return ERR_OK;
}

/**
 * @brief Data-view callback for the date/time menu.
 *
 * @details
 * Returns GPS date/time fields from the NEO-6 info structure:
 * index 0 = date, 1 = time.
 */
static ERR_te acquire_datetime(uint8_t index, char **value_o) {
	NEO6_INFO_ts *neo6_info = NULL;
	neo6_get_info(&neo6_info);

	switch(index) {
		case 0: *value_o = neo6_info->date; break;
		case 1: *value_o = neo6_info->time; break;
	}

	return ERR_OK;
}

/**
 * @brief Data-view callback for the movement menu.
 *
 * @details
 * Returns movement fields from the NEO-6 info structure:
 * index 0 = movement speed, 1 = movement direction.
 */
static ERR_te acquire_movement(uint8_t index, char **value_o) {
	NEO6_INFO_ts *neo6_info = NULL;
	neo6_get_info(&neo6_info);

	switch(index) {
		case 0: *value_o = neo6_info->mov_speed; break;
		case 1: *value_o = neo6_info->mov_dir;   break;
	}

	return ERR_OK;
}

/**
 * @brief Data-view callback for the accuracy menu.
 *
 * @details
 * Returns fix quality fields from the NEO-6 info structure:
 * index 0 = fix status, 1 = fix type, 2 = PDOP, 3 = HDOP, 4 = VDOP.
 */
static ERR_te acquire_accuracy(uint8_t index, char **value_o) {
	NEO6_INFO_ts *neo6_info = NULL;
	neo6_get_info(&neo6_info);

	switch(index) {
		case 0: *value_o = neo6_info->fix_status; break;
		case 1: *value_o = neo6_info->fix_type;   break;
		case 2: *value_o = neo6_info->pdop;       break;
		case 3: *value_o = neo6_info->hdop;       break;
		case 4: *value_o = neo6_info->vdop;       break;
	}

	return ERR_OK;
}

/**
 * @brief Data-view callback for the satellites menu.
 *
 * @details
 * Returns satellite count fields from the NEO-6 info structure:
 * index 0 = total satellites in view, 1 = satellites used in fix.
 */
static ERR_te acquire_satellites(uint8_t index, char **value_o) {
	NEO6_INFO_ts *neo6_info = NULL;
	neo6_get_info(&neo6_info);

	switch(index) {
		case 0: *value_o = neo6_info->num_sats_all;  break;
		case 1: *value_o = neo6_info->num_sats_used; break;
	}

	return ERR_OK;
}