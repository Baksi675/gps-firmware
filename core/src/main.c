#include <stdbool.h>
#include <stdint.h>

#include "arm_cortex_m4_systick.h"
#include "button.h"
#include "common.h"
#include "gtu7.h"
#include "menu.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_rtc.h"
#include "log.h"
#include "console.h"
#include "ssd1309.h"


SSD1309_HANDLE_ts *ssd1309_handle;
BUTTON_HANDLE_ts *lbtn;
BUTTON_HANDLE_ts *mbtn;
BUTTON_HANDLE_ts *rbtn;
MENU_HANDLE_ts *current_menu;
MENU_HANDLE_ts *main_menu;
MENU_HANDLE_ts *location_menu;
MENU_HANDLE_ts *datetime_menu;
MENU_HANDLE_ts *movement_menu;
MENU_HANDLE_ts *accuracy_menu;
MENU_HANDLE_ts *satellites_menu;
GTU7_HANDLE_ts *gtu7_handle;

static void init_all(void);
static void run_all(void);
static ERR_te acquire_gtu7_location(uint8_t index, char **value_o);
static ERR_te acquire_datetime(uint8_t index, char **value_o);
static ERR_te acquire_movement(uint8_t index, char **value_o);
static ERR_te acquire_accuracy(uint8_t index, char **value_o);
static ERR_te acquire_satellites(uint8_t index, char **value_o);

bool lbtn_pushed = false;
bool mbtn_pushed = false;
bool rbtn_pushed = false;
bool can_toggle_lbtn = true;
bool can_toggle_mbtn = true;
bool can_toggle_rbtn = true;
bool mbtn_held = false;
bool can_hold_mbtn = true;
bool refresh = true;

char selected_option[SSD1309_MAX_CHARS_IN_LINE];

uint32_t refreshed_ms;

int main(void) {
	init_all();

	while(1) {
		run_all();
	}

	return 0;
}

static void init_all(void) {
	rtc_init();

	CALENDAR_ts rtc_calendar;
	rtc_calendar.calendar_date = 29; 
	rtc_calendar.calendar_months = MONTHS_JANUARY;
	rtc_calendar.calendar_week_days = WEEK_DAYS_THURSDAY;
	rtc_calendar.calendar_year = 2026;
	rtc_set_calendar(&rtc_calendar);

	TIME_ts rtc_time;
	rtc_time.time_hours = 0;
	rtc_time.time_minutes = 0;
	rtc_time.time_seconds = 0;
	rtc_set_time(&rtc_time);

	LOG_HANDLE_ts log_handle = { 0 };
	log_handle.usart_instance = USART1;
	log_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	log_handle.gpio_port = GPIOA;
	log_handle.gpio_pin = GPIO_PIN_9;
	log_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	log_init(&log_handle);

	CONSOLE_HANDLE_ts console_handle = { 0 };
	console_handle.usart_instance = USART1;
	console_handle.usart_baud_rate = USART_BAUD_RATE_115200;
	console_handle.gpio_port = GPIOA;
	console_handle.gpio_pin = GPIO_PIN_10;
	console_handle.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF7;
	console_init(&console_handle);

	SSD1309_CONFIG_ts ssd1309_conf = { 0 };
	ssd1309_get_def_conf(&ssd1309_conf);
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
	str_cpy(
		lbtn_cfg.name,
		"leftbtn",
		get_str_len("leftbtn") + 1 
	);

	BUTTON_CFG_ts mbtn_cfg = { 0 };
	mbtn_cfg.gpio_port = GPIOB;
	mbtn_cfg.gpio_pin = GPIO_PIN_4;
	mbtn_cfg.debounce_limit_ms = 50;
	mbtn_cfg.held_limit_ms = 1000;
	mbtn_cfg.pushed_type = BUTTON_PUSHED_TYPE_HIGH;
	str_cpy(
		mbtn_cfg.name,
		"midbtn",
		get_str_len("midbtn") + 1 
	);

	BUTTON_CFG_ts rbtn_cfg = { 0 };
	rbtn_cfg.gpio_port = GPIOB;
	rbtn_cfg.gpio_pin = GPIO_PIN_10;
	rbtn_cfg.debounce_limit_ms = 50;
	rbtn_cfg.held_limit_ms = 1000;
	rbtn_cfg.pushed_type = BUTTON_PUSHED_TYPE_HIGH;
	str_cpy(
		rbtn_cfg.name,
		"rightbtn",
		get_str_len("rightbtn") + 1 
	);
	button_init_subsys();
	button_init_handle(&lbtn_cfg, &lbtn);
	button_init_handle(&mbtn_cfg, &mbtn);
	button_init_handle(&rbtn_cfg, &rbtn);
	button_start_subsys();

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
	location_menu_cfg.get_value_fn = acquire_gtu7_location;

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

	GTU7_CONFIG_ts gtu7_config = { 0 };
	gtu7_config.usart_instance = USART6;
	gtu7_config.usart_baud_rate = 9600;
	gtu7_config.rx_gpio_port = GPIOA;
	gtu7_config.rx_gpio_pin = GPIO_PIN_12;
	gtu7_config.tx_gpio_port = GPIOA;
	gtu7_config.tx_gpio_pin = GPIO_PIN_11;
	gtu7_config.gpio_alternate_function = GPIO_ALTERNATE_FUNCTION_AF8;
	gtu7_init_subsys();
	gtu7_init_handle(&gtu7_config, &gtu7_handle);
	gtu7_start_subsys();
}

static void run_all(void) {
	gtu7_run();

	console_run();

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

static ERR_te acquire_gtu7_location(uint8_t index, char **value_o) {
	GTU7_INFO_ts *gtu7_info = NULL;
	gtu7_get_info(&gtu7_info);

	switch(index) {
		case 0:
			*value_o = gtu7_info->lat;
			break;
			
		case 1:
			*value_o = gtu7_info->lon;
			break;

		case 2:
			*value_o = gtu7_info->ort_height;
			break;
			
		case 3:
			*value_o = gtu7_info->geoid_sep;
			break;
		
	}

	return ERR_OK;
}

static ERR_te acquire_datetime(uint8_t index, char **value_o) {
	GTU7_INFO_ts *gtu7_info = NULL;
	gtu7_get_info(&gtu7_info);

	switch(index) {
		case 0:
			*value_o = gtu7_info->date;
			break;
			
		case 1:
			*value_o = gtu7_info->time;
			break;
	}

	return ERR_OK;
}

static ERR_te acquire_movement(uint8_t index, char **value_o) {
	GTU7_INFO_ts *gtu7_info = NULL;
	gtu7_get_info(&gtu7_info);

	switch(index) {
		case 0:
			*value_o = gtu7_info->mov_speed;
			break;
			
		case 1:
			*value_o = gtu7_info->mov_dir;
			break;
	}

	return ERR_OK;
}

static ERR_te acquire_accuracy(uint8_t index, char **value_o) {
	GTU7_INFO_ts *gtu7_info = NULL;
	gtu7_get_info(&gtu7_info);

	switch(index) {
		case 0:
			*value_o = gtu7_info->fix_status;
			break;
			
		case 1:
			*value_o = gtu7_info->fix_type;
			break;

		case 2:
			*value_o = gtu7_info->pdop;
			break;

		case 3:
			*value_o = gtu7_info->hdop;
			break;

		case 4:
			*value_o = gtu7_info->vdop;
			break;
	}

	return ERR_OK;
}

static ERR_te acquire_satellites(uint8_t index, char **value_o) {
	GTU7_INFO_ts *gtu7_info = NULL;
	gtu7_get_info(&gtu7_info);

	switch(index) {
		case 0:
			*value_o = gtu7_info->num_sats_all;
			break;
			
		case 1:
			*value_o = gtu7_info->num_sats_used;
			break;
	}

	return ERR_OK;
}