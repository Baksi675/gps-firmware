/**
 * @file ssd1309.c
 * @author github.com/Baksi675
 * @brief SSD1309 display driver implementation file.
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>

#include "ssd1309.h"
#include "common.h"
#include "err.h"
#include "modules.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_i2c.h"
#include "log.h"
#include "cmd.h"

#define SSD1309_ADDR_W			0x3C			// Write mode
#define SSD1309_ADDR_R			0x3D			// Read mode
#define SSD1309_PAGE_NUM		8
#define SSD1309_BITS_IN_PAGE	8
#define SSD1309_COL_NUM			128
#define SSD1309_MAX_LINES		8

const uint8_t fonts8x8[95][8] = {

/* 0x20 ' ' */
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},

/* 0x21 '!' */
{0x00,0x00,0x00,0x5F,0x00,0x00,0x00,0x00},

/* 0x22 '"' */
{0x00,0x07,0x00,0x07,0x00,0x00,0x00,0x00},

/* 0x23 '#' */
{0x14,0x7F,0x14,0x7F,0x14,0x00,0x00,0x00},

/* 0x24 '$' */
{0x24,0x2A,0x7F,0x2A,0x12,0x00,0x00,0x00},

/* 0x25 '%' */
{0x23,0x13,0x08,0x64,0x62,0x00,0x00,0x00},

/* 0x26 '&' */
{0x36,0x49,0x55,0x22,0x50,0x00,0x00,0x00},

/* 0x27 ''' */
{0x00,0x05,0x03,0x00,0x00,0x00,0x00,0x00},

/* 0x28 '(' */
{0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00},

/* 0x29 ')' */
{0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00},

/* 0x2A '*' */
{0x14,0x08,0x3E,0x08,0x14,0x00,0x00,0x00},

/* 0x2B '+' */
{0x08,0x08,0x3E,0x08,0x08,0x00,0x00,0x00},

/* 0x2C ',' */
{0x00,0x50,0x30,0x00,0x00,0x00,0x00,0x00},

/* 0x2D '-' */
{0x08,0x08,0x08,0x08,0x08,0x00,0x00,0x00},

/* 0x2E '.' */
{0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00},

/* 0x2F '/' */
{0x20,0x10,0x08,0x04,0x02,0x00,0x00,0x00},

/* 0x30 '0' */
{0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00},

/* 0x31 '1' */
{0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00},

/* 0x32 '2' */
{0x62,0x51,0x49,0x49,0x46,0x00,0x00,0x00},

/* 0x33 '3' */
{0x22,0x41,0x49,0x49,0x36,0x00,0x00,0x00},

/* 0x34 '4' */
{0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00},

/* 0x35 '5' */
{0x2F,0x49,0x49,0x49,0x31,0x00,0x00,0x00},

/* 0x36 '6' */
{0x3E,0x49,0x49,0x49,0x32,0x00,0x00,0x00},

/* 0x37 '7' */
{0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00},

/* 0x38 '8' */
{0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00},

/* 0x39 '9' */
{0x26,0x49,0x49,0x49,0x3E,0x00,0x00,0x00},

/* 0x3A ':' */
{0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00},

/* 0x3B ';' */
{0x00,0x56,0x36,0x00,0x00,0x00,0x00,0x00},

/* 0x3C '<' */
{0x08,0x14,0x22,0x41,0x00,0x00,0x00,0x00},

/* 0x3D '=' */
{0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00},

/* 0x3E '>' */
{0x41,0x22,0x14,0x08,0x00,0x00,0x00,0x00},

/* 0x3F '?' */
{0x02,0x01,0x51,0x09,0x06,0x00,0x00,0x00},

/* 0x40 '@' */
{0x32,0x49,0x79,0x41,0x3E,0x00,0x00,0x00},

/* 0x41 'A' */
{0x00,0x7E,0x09,0x09,0x09,0x09,0x7E,0x00},

/* 0x42 'B' */
{0x00,0x7F,0x49,0x49,0x49,0x49,0x36,0x00},

/* 0x43 'C' */
{0x00,0x3E,0x41,0x41,0x41,0x41,0x22,0x00},

/* 0x44 'D' */
{0x00,0x7F,0x41,0x41,0x41,0x22,0x1C,0x00},

/* 0x45 'E' */
{0x00,0x7F,0x49,0x49,0x49,0x49,0x41,0x00},

/* 0x46 'F' */
{0x00,0x7F,0x09,0x09,0x09,0x09,0x01,0x00},

/* 0x47 'G' */
{0x00,0x3E,0x41,0x41,0x49,0x49,0x7A,0x00},

/* 0x48 'H' */
{0x00,0x7F,0x08,0x08,0x08,0x08,0x7F,0x00},

/* 0x49 'I' */
{0x00,0x41,0x41,0x7F,0x41,0x41,0x00,0x00},

/* 0x4A 'J' */
{0x00,0x20,0x40,0x41,0x3F,0x01,0x00,0x00},

/* 0x4B 'K' */
{0x00,0x7F,0x08,0x14,0x22,0x41,0x00,0x00},

/* 0x4C 'L' */
{0x00,0x7F,0x40,0x40,0x40,0x40,0x40,0x00},

/* 0x4D 'M' */
{0x00,0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00},

/* 0x4E 'N' */
{0x00,0x7F,0x04,0x08,0x10,0x7F,0x00,0x00},

/* 0x4F 'O' */
{0x00,0x3E,0x41,0x41,0x41,0x41,0x3E,0x00},

/* 0x50 'P' */
{0x00,0x7F,0x09,0x09,0x09,0x09,0x06,0x00},

/* 0x51 'Q' */
{0x00,0x3E,0x41,0x51,0x21,0x5E,0x00,0x00},

/* 0x52 'R' */
{0x00,0x7F,0x09,0x19,0x29,0x46,0x00,0x00},

/* 0x53 'S' */
{0x00,0x26,0x49,0x49,0x49,0x49,0x32,0x00},

/* 0x54 'T' */
{0x00,0x01,0x01,0x7F,0x01,0x01,0x00,0x00},

/* 0x55 'U' */
{0x00,0x3F,0x40,0x40,0x40,0x40,0x3F,0x00},

/* 0x56 'V' */
{0x00,0x1F,0x20,0x40,0x20,0x1F,0x00,0x00},

/* 0x57 'W' */
{0x00,0x7F,0x20,0x18,0x20,0x7F,0x00,0x00},

/* 0x58 'X' */
{0x00,0x63,0x14,0x08,0x14,0x63,0x00,0x00},

/* 0x59 'Y' */
{0x00,0x03,0x04,0x78,0x04,0x03,0x00,0x00},

/* 0x5A 'Z' */
{0x00,0x61,0x51,0x49,0x45,0x43,0x00,0x00},

/* 0x5B '[' */
{0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00},

/* 0x5C '\' */
{0x02,0x04,0x08,0x10,0x20,0x00,0x00,0x00},

/* 0x5D ']' */
{0x00,0x41,0x41,0x7F,0x00,0x00,0x00,0x00},

/* 0x5E '^' */
{0x04,0x02,0x01,0x02,0x04,0x00,0x00,0x00},

/* 0x5F '_' */
{0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00},

/* 0x60 '`' */
{0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00},

/* 0x61 'a' */
{0x00,0x20,0x54,0x54,0x54,0x54,0x78,0x00},

/* 0x62 'b' */
{0x00,0x7F,0x48,0x48,0x48,0x48,0x30,0x00},

/* 0x63 'c' */
{0x00,0x38,0x44,0x44,0x44,0x44,0x28,0x00},

/* 0x64 'd' */
{0x00,0x30,0x48,0x48,0x48,0x48,0x7F,0x00},

/* 0x65 'e' */
{0x00,0x38,0x54,0x54,0x54,0x54,0x18,0x00},

/* 0x66 'f' */
{0x00,0x08,0x7E,0x09,0x01,0x02,0x00,0x00},

/* 0x67 'g' */
{0x00,0x0C,0x52,0x52,0x52,0x52,0x3E,0x00},

/* 0x68 'h' */
{0x00,0x7F,0x08,0x08,0x08,0x08,0x70,0x00},

/* 0x69 'i' */
{0x00,0x00,0x48,0x7A,0x40,0x00,0x00,0x00},

/* 0x6A 'j' */
{0x00,0x20,0x40,0x48,0x3A,0x00,0x00,0x00},

/* 0x6B 'k' */
{0x00,0x7F,0x10,0x28,0x44,0x00,0x00,0x00},

/* 0x6C 'l' */
{0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00},

/* 0x6D 'm' */
{0x00,0x7C,0x04,0x18,0x04,0x78,0x00,0x00},

/* 0x6E 'n' */
{0x00,0x7C,0x08,0x04,0x04,0x78,0x00,0x00},

/* 0x6F 'o' */
{0x00,0x38,0x44,0x44,0x44,0x44,0x38,0x00},

/* 0x70 'p' */
{0x00,0x7E,0x12,0x12,0x12,0x12,0x0C,0x00},

/* 0x71 'q' */
{0x00,0x0C,0x12,0x12,0x12,0x12,0x7E,0x00},

/* 0x72 'r' */
{0x00,0x7C,0x08,0x04,0x04,0x08,0x00,0x00},

/* 0x73 's' */
{0x00,0x48,0x54,0x54,0x54,0x54,0x20,0x00},

/* 0x74 't' */
{0x00,0x04,0x3F,0x44,0x40,0x20,0x00,0x00},

/* 0x75 'u' */
{0x00,0x3C,0x40,0x40,0x40,0x20,0x7C,0x00},

/* 0x76 'v' */
{0x00,0x1C,0x20,0x40,0x20,0x1C,0x00,0x00},

/* 0x77 'w' */
{0x00,0x3C,0x40,0x30,0x40,0x3C,0x00,0x00},

/* 0x78 'x' */
{0x00,0x44,0x28,0x10,0x28,0x44,0x00,0x00},

/* 0x79 'y' */
{0x00,0x0C,0x50,0x50,0x50,0x50,0x3C,0x00},

/* 0x7A 'z' */
{0x00,0x44,0x64,0x54,0x4C,0x44,0x00,0x00},

/* 0x7B '{' */
{0x00,0x08,0x36,0x41,0x00,0x00,0x00,0x00},

/* 0x7C '|' */
{0x00,0x00,0x7F,0x00,0x00,0x00,0x00,0x00},

/* 0x7D '}' */
{0x00,0x41,0x36,0x08,0x00,0x00,0x00,0x00},

/* 0x7E '~' */
{0x10,0x08,0x08,0x10,0x08,0x08,0x00,0x00}

};

struct ssd1309_handle_s {
	I2C_REGDEF_ts *i2c_instance;
	GPIO_REGDEF_ts *scl_gpio_port;
	GPIO_PIN_te scl_gpio_pin;
	GPIO_REGDEF_ts *sda_gpio_port;
	GPIO_PIN_te sda_gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
	SSD1309_LOW_COL_START_ADDR_PAM_te low_col_start_addr_pam;
	SSD1309_HIGH_COL_START_ADDR_PAM_te high_col_start_addr_pam;
	SSD1309_MEM_ADDR_MODE_te mem_addr_mode;
	SSD1309_COL_ADDR_START_HAM_VAM_te col_addr_start_ham_vam;
	SSD1309_COL_ADDR_END_HAM_VAM_te col_addr_end_ham_vam;
	SSD1309_PAGE_ADDR_START_HAM_VAM_te page_addr_start_ham_vam;
	SSD1309_PAGE_ADDR_END_HAM_VAM_te page_addr_end_ham_vam;
	SSD1309_START_LINE_te start_line;
	SSD1309_CONTRAST_te contrast;
	SSD1309_HORIZONTAL_FLIP_te horizontal_flip;
	SSD1309_INVERSE_MODE_te inverse_mode;
	SSD1309_MULTIPLEX_RATIO_te multiplex_ratio;
	SSD1309_PAGE_START_ADDR_PAM_te page_start_addr_pam;
	SSD1309_VERTICAL_FLIP_te vertical_flip;
	SSD1309_OFFSET_te offset;
	SSD1309_CLK_DIV_RATIO_te clk_div_ratio;
	SSD1309_CLK_SPEED_LVL_te clk_speed_lvl;
	SSD1309_PHASE1_PRECHARGE_DCLK_te phase1_precharge_dclk;
	SSD1309_PHASE2_PRECHARGE_DCLK_te phase2_precharge_dclk;
	SSD1309_VCOMH_DESELECT_LVL_te vcomh_deselect_lvl;
	bool initialized;
};

struct internal_state_s {
	SSD1309_HANDLE_ts ssd1309_handle;
	uint8_t fb[SSD1309_PAGE_NUM][SSD1309_COL_NUM];
	MODULES_te subsys;
	LOG_LEVEL_te log_level;
	bool initialized;
	bool started;
};
static struct internal_state_s internal_state;

static ERR_te ssd1309_cmd_fillrect_handler(uint32_t argc, char **argv);
static ERR_te ssd1309_cmd_clearrect_handler(uint32_t argc, char **argv);
static ERR_te ssd1309_cmd_invertrect_handler(uint32_t argc, char **argv);
static ERR_te ssd1309_cmd_drawtext_handler(uint32_t argc, char **argv);
static ERR_te ssd1309_cmd_clearline_handler(uint32_t argc, char **argv);
static ERR_te ssd1309_cmd_invertline_handler(uint32_t argc, char **argv);

static CMD_INFO_ts ssd1309_cmds[] = {
	{
		.name = "fillrect",
		.help = "Fills a rectangle, usage: ssd1309 fillrect <x1,y1,x2,y2>",
		.handler = ssd1309_cmd_fillrect_handler
	},
	{
		.name = "clearrect",
		.help = "Clears a rectangle, usage: ssd1309 clearrect <x1,y1,x2,y2>",
		.handler = ssd1309_cmd_clearrect_handler
	},
	{
		.name = "invertrect",
		.help = "Inverts the pixels of a rectangle, usage: ssd1309 invertrect <x1,y1,x2,y2>",
		.handler = ssd1309_cmd_invertrect_handler
	},
	{
		.name = "drawtext",
		.help = "Draws text in a line, usage: ssd1309 drawtext <text> <line>",
		.handler = ssd1309_cmd_drawtext_handler
	},
	{
		.name = "clearline",
		.help = "Clears a line, usage: ssd1309 clearline <line>",
		.handler = ssd1309_cmd_clearline_handler
	},
	{
		.name = "invertline",
		.help = "Inverts a line, usage: ssd1309 invertline <line>",
		.handler = ssd1309_cmd_invertline_handler
	}
};

static CMD_CLIENT_INFO_ts ssd1309_cmd_client_info = {
	.cmds_ptr = ssd1309_cmds,
	.num_cmds = sizeof(ssd1309_cmds) / sizeof(ssd1309_cmds[0]),
	.name = "ssd1309",
	.log_level_ptr = &internal_state.log_level
};

/**
 * @brief Initializes the SSD1309 subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te ssd1309_init_subsys(void) {
	ERR_te err;
	
	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };
	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_SSD1309;
	internal_state.initialized = true;
	internal_state.started = false;

	err = cmd_register(&ssd1309_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_init_subsys: cmd_register error"
		);

		return err;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"ssd1309_init_subsys: subsys initialized"
	);

	return ERR_OK;
}

/**
 * @brief Deinitializes the SSD1309 subsystem.
 * 
 * @return ERR_te Eror generated during execution.
 */
ERR_te ssd1309_deinit_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state = (struct internal_state_s){ 0 };

		ERR_te err = cmd_deregister(&ssd1309_cmd_client_info);
		if(err != ERR_OK) {
			return err;
		}
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_deinit_subsys: subsys is not initialized or stopped"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"ssd1309_deinit_subsys: subsystem deinitialized"
	);
	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te ssd1309_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_start_subsys: subsys not initialized or already started"
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
ERR_te ssd1309_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;	
}

/**
 * @brief Initializes the SSD1309 object to a default configuration:
 * - Memory address mode: HAM
 * - RAM column address start: 0,
 * - RAM column address end: 127,
 * - RAM page address start: 0,
 * - RAM page address end: 7,
 * - SSD1309 start line: 0,
 * - Contrast level: 100,
 * - Horizontal flip: false,
 * - Inverse mode: false,
 * - Multiplex ratio: 64,
 * - Vertical flip: false,
 * - ROW offset: 0,
 * - Clock divide ratio: 1,
 * - Clock frequency: ? (set to RESET value),
 * - Phase 1 precharge DCLK cycle: 2,
 * - Phase 2 precharge DCLK cycle: 2,
 * - V_COMH deselect level: 0x78 * VCC,
 * - 
 * @param[in] ssd1309_handle The SSD1309 object created by the user.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_get_def_conf(SSD1309_CONFIG_ts *ssd1309_config_o) {
	ssd1309_config_o->mem_addr_mode = SSD1309_MEM_ADDR_MODE_HAM;
	ssd1309_config_o->col_addr_start_ham_vam = SSD1309_COL_ADDR_START_HAM_VAM_0;
	ssd1309_config_o->col_addr_end_ham_vam = SSD1309_COL_ADDR_END_HAM_VAM_127;
	ssd1309_config_o->page_addr_start_ham_vam = SSD1309_PAGE_ADDR_START_HAM_VAM_0;
	ssd1309_config_o->page_addr_end_ham_vam = SSD1309_PAGE_ADDR_END_HAM_VAM_7;
	ssd1309_config_o->start_line = SSD1309_START_LINE_0;
	ssd1309_config_o->contrast = SSD1309_CONTRAST_10;
	ssd1309_config_o->horizontal_flip = SSD1309_HORIZONTAL_FLIP_TRUE;
	ssd1309_config_o->inverse_mode = SSD1309_INVERSE_MODE_FALSE;
	ssd1309_config_o->multiplex_ratio = SSD1309_MULTIPLEX_RATIO_64;
	ssd1309_config_o->vertical_flip = SSD1309_VERTICAL_FLIP_TRUE;
	ssd1309_config_o->offset = SSD1309_OFFSET_0;
	ssd1309_config_o->clk_div_ratio = SSD1309_CLK_DIV_RATIO_1;
	ssd1309_config_o->clk_speed_lvl = SSD1309_CLK_SPEED_LVL_LVL_15;
	ssd1309_config_o->phase1_precharge_dclk = SSD1309_PHASE1_PRECHARGE_DCLK_2;
	ssd1309_config_o->phase2_precharge_dclk = SSD1309_PHASE2_PRECHARGE_DCLK_2;
	ssd1309_config_o->vcomh_deselect_lvl = SSD1309_VCOMH_DESELECT_LVL_MED;

	return ERR_OK;
}

/**
 * @brief Initializes the SSD1309 display.
 * 
 * @param[in] ssd1309_handle The handle of the SSD1309 module.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_init_handle(SSD1309_CONFIG_ts *ssd1309_config, SSD1309_HANDLE_ts **ssd1309_handle_o) {
	if(internal_state.ssd1309_handle.initialized == true) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_init_handle: handle already initialized"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(ssd1309_config->i2c_instance == (void*)0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_init_handle: No I2C peripheral given"
		);

		return ERR_INITIALIZATION_FAILURE;
	}
	else if(ssd1309_config->scl_gpio_port == (void*)0 || ssd1309_config->sda_gpio_port == (void*)0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"ssd1309_init_handle: No GPIO peripheral given"
		);	
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state.ssd1309_handle.i2c_instance = ssd1309_config->i2c_instance;
	internal_state.ssd1309_handle.scl_gpio_port = ssd1309_config->scl_gpio_port;
	internal_state.ssd1309_handle.scl_gpio_pin = ssd1309_config->scl_gpio_pin;
	internal_state.ssd1309_handle.sda_gpio_port = ssd1309_config->sda_gpio_port;
	internal_state.ssd1309_handle.sda_gpio_pin = ssd1309_config->sda_gpio_pin;
	internal_state.ssd1309_handle.gpio_alternate_function = ssd1309_config->gpio_alternate_function;
	internal_state.ssd1309_handle.low_col_start_addr_pam = ssd1309_config->low_col_start_addr_pam;
	internal_state.ssd1309_handle.high_col_start_addr_pam = ssd1309_config->high_col_start_addr_pam;
	internal_state.ssd1309_handle.mem_addr_mode = ssd1309_config->mem_addr_mode;
	internal_state.ssd1309_handle.col_addr_start_ham_vam = ssd1309_config->col_addr_start_ham_vam;
	internal_state.ssd1309_handle.col_addr_end_ham_vam = ssd1309_config->col_addr_end_ham_vam;
	internal_state.ssd1309_handle.page_addr_start_ham_vam = ssd1309_config->page_addr_start_ham_vam;
	internal_state.ssd1309_handle.page_addr_end_ham_vam = ssd1309_config->page_addr_end_ham_vam;
	internal_state.ssd1309_handle.start_line = ssd1309_config->start_line;
	internal_state.ssd1309_handle.contrast = ssd1309_config->contrast;
	internal_state.ssd1309_handle.horizontal_flip = ssd1309_config->horizontal_flip;
	internal_state.ssd1309_handle.inverse_mode = ssd1309_config->inverse_mode;
	internal_state.ssd1309_handle.multiplex_ratio = ssd1309_config->multiplex_ratio;
	internal_state.ssd1309_handle.page_start_addr_pam = ssd1309_config->page_start_addr_pam;
	internal_state.ssd1309_handle.vertical_flip = ssd1309_config->vertical_flip;
	internal_state.ssd1309_handle.offset = ssd1309_config->offset;
	internal_state.ssd1309_handle.clk_div_ratio = ssd1309_config->clk_div_ratio;
	internal_state.ssd1309_handle.clk_speed_lvl =ssd1309_config->clk_speed_lvl;
	internal_state.ssd1309_handle.phase1_precharge_dclk = ssd1309_config->phase1_precharge_dclk;
	internal_state.ssd1309_handle.phase2_precharge_dclk = ssd1309_config->phase2_precharge_dclk;
	internal_state.ssd1309_handle.vcomh_deselect_lvl = ssd1309_config->vcomh_deselect_lvl;

	GPIO_HANDLE_ts ssd1309_scl = { 0 };
	GPIO_HANDLE_ts ssd1309_sda = { 0 };

	ssd1309_scl.port = ssd1309_config->scl_gpio_port;
	ssd1309_scl.pin = ssd1309_config->scl_gpio_pin;
	ssd1309_scl.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	ssd1309_scl.alternate_function = ssd1309_config->gpio_alternate_function;
	ssd1309_scl.output_type = GPIO_OUTPUT_TYPE_OPENDRAIN;

	ssd1309_sda.port = ssd1309_config->sda_gpio_port;
	ssd1309_sda.pin = ssd1309_config->sda_gpio_pin;
	ssd1309_sda.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	ssd1309_sda.alternate_function = ssd1309_config->gpio_alternate_function;
	ssd1309_sda.output_type = GPIO_OUTPUT_TYPE_OPENDRAIN;

	gpio_init(&ssd1309_scl);
	gpio_init(&ssd1309_sda);

	I2C_HANDLE_ts ssd1309_i2c = { 0 };
	ssd1309_i2c.i2c_clock_strech = I2C_CLOCK_STRETCH_ON;
	ssd1309_i2c.i2c_instance = internal_state.ssd1309_handle.i2c_instance;
	ssd1309_i2c.i2c_speed = I2C_SPEED_400kHz;

	i2c_init(&ssd1309_i2c); 

	i2c_master_set_comm(internal_state.ssd1309_handle.i2c_instance, ENABLE);
	
	uint8_t cmd_buf[64]; 
	uint8_t idx = 0;

	// Control byte: commands
	cmd_buf[idx++] = 0x00;

	// SSD1309 OFF
	cmd_buf[idx++] = 0xAE;

	// Memory addressing mode
	cmd_buf[idx++] = 0x20;
	cmd_buf[idx++] = ssd1309_config->mem_addr_mode;

	// Addressing mode dependent setup
	if (ssd1309_config->mem_addr_mode == SSD1309_MEM_ADDR_MODE_HAM ||
		ssd1309_config->mem_addr_mode == SSD1309_MEM_ADDR_MODE_VAM) {

		// Column start/end
		cmd_buf[idx++] = 0x21;
		cmd_buf[idx++] = ssd1309_config->col_addr_start_ham_vam;
		cmd_buf[idx++] = ssd1309_config->col_addr_end_ham_vam;

		// Page start/end
		cmd_buf[idx++] = 0x22;
		cmd_buf[idx++] = ssd1309_config->page_addr_start_ham_vam;
		cmd_buf[idx++] = ssd1309_config->page_addr_end_ham_vam;
	}
	else {
		// Page start (PAM)
		cmd_buf[idx++] = 0xB0 | ssd1309_config->page_start_addr_pam;

		// Column start (PAM)
		cmd_buf[idx++] = ssd1309_config->low_col_start_addr_pam;
		cmd_buf[idx++] = 0x10 | ssd1309_config->high_col_start_addr_pam;
	}

	// SSD1309 start line 
	cmd_buf[idx++] = 0x40 | ssd1309_config->start_line;

	// Contrast
	cmd_buf[idx++] = 0x81;
	cmd_buf[idx++] = ssd1309_config->contrast;

	// Horizontal flip 
	cmd_buf[idx++] = 0xA0 | ssd1309_config->horizontal_flip;

	// Inverse mode
	cmd_buf[idx++] = 0xA6 | ssd1309_config->inverse_mode;

	// Multiplex ratio
	cmd_buf[idx++] = 0xA8;
	cmd_buf[idx++] = ssd1309_config->multiplex_ratio;

	// Vertical flip
	cmd_buf[idx++] = 0xC0 | ssd1309_config->vertical_flip;

	// SSD1309 offset 
	cmd_buf[idx++] = 0xD3;
	cmd_buf[idx++] = ssd1309_config->offset;

	// Clock divide + oscillator frequency
	cmd_buf[idx++] = 0xD5;
	cmd_buf[idx++] =
		(ssd1309_config->clk_div_ratio & 0x0F) |
		(ssd1309_config->clk_speed_lvl << 4);

	// Pre-charge period
	cmd_buf[idx++] = 0xD9;
	cmd_buf[idx++] =
		(ssd1309_config->phase1_precharge_dclk & 0x0F) |
		(ssd1309_config->phase2_precharge_dclk << 4);

	// VCOMH deselect level
	cmd_buf[idx++] = 0xDB;
	cmd_buf[idx++] = ssd1309_config->vcomh_deselect_lvl;

	// Turn on SSD1309
	cmd_buf[idx++] = 0xAF;

	// Output follows RAM content
	cmd_buf[idx++] = 0xA4;

	i2c_master_send(internal_state.ssd1309_handle.i2c_instance, SSD1309_ADDR_W, cmd_buf, idx);

	i2c_master_set_comm(internal_state.ssd1309_handle.i2c_instance, DISABLE);

	internal_state.ssd1309_handle.initialized = true;

	*ssd1309_handle_o = &internal_state.ssd1309_handle;

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"ssd1309_init_handle: ssd1309 handle initialized"
	);

	return ERR_OK;
}

/**
 * @brief Draws a text on the display.
 * 
 * @param[in] text The text to draw on the display.
 * @param[in] text_len The length of the text to draw on the display.
 * @param[in] line The line where to draw the text.
 * @return ERR_te Error generated during execution.
 */
ERR_te ssd1309_draw_text(char const *text, uint8_t text_len, uint8_t line) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

    if (line > SSD1309_PAGE_NUM || line == 0) {
        return ERR_INVALID_ARGUMENT; // invalid line
    }

	line--;

    uint8_t x = 0; // start at left-most column
    for (uint8_t i = 0; i < text_len; i++) {
		// check if character fits horizontally
		if (x + 8 > SSD1309_WIDTH && line < SSD1309_PAGE_NUM) {
			line++; // stop drawing if we run out of columns
			x = 0;
		}

        // draw 8x8 character
        for (uint8_t col = 0; col < 8; col++) {
			internal_state.fb[line][x + col] = fonts8x8[text[i] - 32][col];
        }

        x += 8; // move to next character position
    }

    return ERR_OK;
}

/**
 * @brief Draws a rectangle on the display.
 * 
 * @param[in] x_src Starting X coord.
 * @param[in] y_src Starting Y coord.
 * @param[in] x_dest Destination X coord.
 * @param[in] y_dest Destination Y coord.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_draw_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(x_src > x_dest || y_src > y_dest || x_src < 1 || x_dest > SSD1309_WIDTH ||
	y_src < 1 || y_dest > SSD1309_HEIGHT) {
		// Invalid arguments
		return ERR_INVALID_ARGUMENT;
	}

    // Convert to 0-based coordinates
	uint8_t xs = x_src  - 1;
    uint8_t ys = y_src  - 1;
    uint8_t xd = x_dest - 1;
    uint8_t yd = y_dest - 1;

    uint8_t page_src = ys / SSD1309_BITS_IN_PAGE;
    uint8_t bit_src  = ys % SSD1309_BITS_IN_PAGE;

    uint8_t page_dest = yd / SSD1309_BITS_IN_PAGE;
    uint8_t bit_dest  = yd % SSD1309_BITS_IN_PAGE;

    for (uint8_t p = page_src; p <= page_dest; p++) {
        for (uint8_t x = xs; x <= xd; x++) {

            if (p == page_src && p == page_dest) {
                /* Single page */
                uint8_t mask = (uint8_t)((0xFFu << bit_src) & ((1u << (bit_dest + 1)) - 1));
                internal_state.fb[p][x] |= mask;
            }
            else if (p == page_src) {
                /* First page */
                uint8_t mask = (uint8_t)(0xFFu << bit_src);
                internal_state.fb[p][x] |= mask;
            }
            else if (p == page_dest) {
                /* Last page */
                uint8_t mask = (uint8_t)((1u << (bit_dest + 1)) - 1);
                internal_state.fb[p][x] |= mask;
            }
            else {
                /* Full pages in between */
                internal_state.fb[p][x] = 0xFF;
            }
        }
    }

	return ERR_OK;
}

/**
 * @brief Clears a line on the display.
 * 
 * @param[in] line The line to clear. 
 * @return ERR_te Error generated during execution.
 */
ERR_te ssd1309_clear_line(uint8_t line) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

    if (line > SSD1309_PAGE_NUM || line == 0) {
        return ERR_INVALID_ARGUMENT; // invalid line
    }

	line--;

	ssd1309_clear_rect(1, line * SSD1309_BITS_IN_PAGE + 1, 128, line * SSD1309_BITS_IN_PAGE + 8);

	return ERR_OK;
}

/**
 * @brief Inverts the pixels in a line.
 * 
 * @param[in] line The line to invert the pixels of.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_invert_line(uint8_t line) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

    if (line > SSD1309_PAGE_NUM || line == 0) {
        return ERR_INVALID_ARGUMENT; // invalid line
    }

	line--;

	ssd1309_invert_rect(1, line * SSD1309_BITS_IN_PAGE + 1, 128, line * SSD1309_BITS_IN_PAGE + 8);

	return ERR_OK;
}

/**
 * @brief Clears a rectangle on the display.
 * 
 * @param[in] x_src Starting X coord.
 * @param[in] y_src Starting Y coord.
 * @param[in] x_dest Destination X coord.
 * @param[in] y_dest Destination Y coord.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_clear_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(x_src > x_dest || y_src > y_dest || x_src < 1 || x_dest > SSD1309_WIDTH ||
	y_src < 1 || y_dest > SSD1309_HEIGHT) {
		// Invalid arguments
		return ERR_INVALID_ARGUMENT;
	}

    // Convert to 0-based coordinates
	uint8_t xs = x_src  - 1;
    uint8_t ys = y_src  - 1;
    uint8_t xd = x_dest - 1;
    uint8_t yd = y_dest - 1;

    uint8_t page_src = ys / SSD1309_BITS_IN_PAGE;
    uint8_t bit_src  = ys % SSD1309_BITS_IN_PAGE;

    uint8_t page_dest = yd / SSD1309_BITS_IN_PAGE;
    uint8_t bit_dest  = yd % SSD1309_BITS_IN_PAGE;

    for (uint8_t p = page_src; p <= page_dest; p++) {
        for (uint8_t x = xs; x <= xd; x++) {

            if (p == page_src && p == page_dest) {
                /* Single page */
                uint8_t mask = (uint8_t)((0xFFu << bit_src) & ((1u << (bit_dest + 1)) - 1));
                internal_state.fb[p][x] &= ~mask;
            }
            else if (p == page_src) {
                /* First page */
                uint8_t mask = (uint8_t)(0xFFu << bit_src);
                internal_state.fb[p][x] &= ~mask;
            }
            else if (p == page_dest) {
                /* Last page */
                uint8_t mask = (uint8_t)((1u << (bit_dest + 1)) - 1);
                internal_state.fb[p][x] &= ~mask;
            }
            else {
                /* Full pages in between */
                internal_state.fb[p][x] = 0x00;
            }
        }
    }

	return ERR_OK;
}

/**
 * @brief Inverts a rectangle on the display.
 * 
 * @param[in] x_src Starting X coord.
 * @param[in] y_src Starting Y coord.
 * @param[in] x_dest Destination X coord.
 * @param[in] y_dest Destination Y coord.
 * @return ERR_te Error generated during execution. 
 */
ERR_te ssd1309_invert_rect(uint8_t x_src, uint8_t y_src, uint8_t x_dest, uint8_t y_dest) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(x_src > x_dest || y_src > y_dest || x_src < 1 || x_dest > SSD1309_WIDTH ||
	y_src < 1 || y_dest > SSD1309_HEIGHT) {
		// Invalid arguments
		return ERR_INVALID_ARGUMENT;
	}	

// Convert to 0-based coordinates
	uint8_t xs = x_src  - 1;
    uint8_t ys = y_src  - 1;
    uint8_t xd = x_dest - 1;
    uint8_t yd = y_dest - 1;

    uint8_t page_src = ys / SSD1309_BITS_IN_PAGE;
    uint8_t bit_src  = ys % SSD1309_BITS_IN_PAGE;

    uint8_t page_dest = yd / SSD1309_BITS_IN_PAGE;
    uint8_t bit_dest  = yd % SSD1309_BITS_IN_PAGE;


    for (uint8_t p = page_src; p <= page_dest; p++) {
        for (uint8_t x = xs; x <= xd; x++) {

            if (p == page_src && p == page_dest) {
                /* Single page */
                uint8_t mask = (uint8_t)((0xFFu << bit_src) & ((1u << (bit_dest + 1)) - 1));
                internal_state.fb[p][x] ^= mask;
            }
            else if (p == page_src) {
                /* First page */
                uint8_t mask = (uint8_t)(0xFFu << bit_src);
                internal_state.fb[p][x] ^= mask;
            }
            else if (p == page_dest) {
                /* Last page */
                uint8_t mask = (uint8_t)((1u << (bit_dest + 1)) - 1);
                internal_state.fb[p][x] ^= mask;
            }
            else {
                /* Full pages in between */
                internal_state.fb[p][x] ^= 0xFF;
            }
        }
    }

	return ERR_OK;
}

/**
 * @brief Updates the display.
 * 
 * @return ERR_te Error generated during exection,
 */
ERR_te ssd1309_update(void) {
	if(!internal_state.ssd1309_handle.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_update: handle not initialized or subsystem not started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	i2c_master_set_comm(internal_state.ssd1309_handle.i2c_instance, ENABLE);

    uint8_t cmd = 0x40 ;
    i2c_master_send(internal_state.ssd1309_handle.i2c_instance,
                    SSD1309_ADDR_W, &cmd, 1);

    /* Framebuffer */
    i2c_master_send_continue(internal_state.ssd1309_handle.i2c_instance,
                    (uint8_t *)internal_state.fb,
                    SSD1309_WIDTH * SSD1309_PAGE_NUM);

	i2c_master_set_comm(internal_state.ssd1309_handle.i2c_instance, DISABLE);

	return ERR_OK;
}

/**
 * @brief Command handler routine for fillrect command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_fillrect_handler(uint32_t argc, char **argv) {
	ERR_te err;
	
	if(argc != 6) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_fillrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	int x1 = str_to_int(argv[2]);
	int y1 = str_to_int(argv[3]);
	int x2 = str_to_int(argv[4]);
	int y2 = str_to_int(argv[5]);

	if(
		(x1 < 1 || x1 > SSD1309_WIDTH) ||
		(y1 < 1 || y1 > SSD1309_HEIGHT) ||
		(x2 < 1 || x2 > SSD1309_WIDTH) ||
		(y2 < 1 || y2 > SSD1309_HEIGHT)
	) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_fillrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_draw_rect(x1, y1, x2, y2);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_fillrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	ssd1309_update();
	
	return ERR_OK;
}

/**
 * @brief Command handler routine for clearrect command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_clearrect_handler(uint32_t argc, char **argv) {
	ERR_te err;
	
	if(argc != 6) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	int x1 = str_to_int(argv[2]);
	int y1 = str_to_int(argv[3]);
	int x2 = str_to_int(argv[4]);
	int y2 = str_to_int(argv[5]);

	if(
		(x1 < 1 || x1 > SSD1309_WIDTH) ||
		(y1 < 1 || y1 > SSD1309_HEIGHT) ||
		(x2 < 1 || x2 > SSD1309_WIDTH) ||
		(y2 < 1 || y2 > SSD1309_HEIGHT)
	) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_clear_rect(x1, y1, x2, y2);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	ssd1309_update();
	
	return ERR_OK;
}

/**
 * @brief Command handler routine for invertrect command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_invertrect_handler(uint32_t argc, char **argv) {
	ERR_te err;
	
	if(argc != 6) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_invertrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	int x1 = str_to_int(argv[2]);
	int y1 = str_to_int(argv[3]);
	int x2 = str_to_int(argv[4]);
	int y2 = str_to_int(argv[5]);

	if(
		(x1 < 1 || x1 > SSD1309_WIDTH) ||
		(y1 < 1 || y1 > SSD1309_HEIGHT) ||
		(x2 < 1 || x2 > SSD1309_WIDTH) ||
		(y2 < 1 || y2 > SSD1309_HEIGHT)
	) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_invertrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_invert_rect(x1, y1, x2, y2);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_invertrect_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	ssd1309_update();
	
	return ERR_OK;
}

/**
 * @brief Command handler routine for drawtext command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_drawtext_handler(uint32_t argc, char **argv) {
	ERR_te err;

	if(argc != 4) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_drawtext_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	uint8_t line = str_to_int(argv[3]);
	if(line > SSD1309_MAX_LINES) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_drawtext_handler: invalid arguments"
		);	

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_draw_text(argv[2], get_str_len(argv[2]), line);
	if(err != ERR_OK) {
		return err;
	}

	ssd1309_update();

	return ERR_OK;
}

/**
 * @brief Command handler routine for clearline command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_clearline_handler(uint32_t argc, char **argv) {
	ERR_te err;

	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearline_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	uint8_t line = str_to_int(argv[2]);
	if(line > SSD1309_MAX_LINES) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearline_handler: invalid arguments"
		);	

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_clear_line(line);
	if(err != ERR_OK) {
		return err;
	}

	ssd1309_update();

	return ERR_OK;
}

/**
 * @brief Command handler routine for invertline command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te ssd1309_cmd_invertline_handler(uint32_t argc, char **argv) {
	ERR_te err;

	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearline_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	uint8_t line = str_to_int(argv[2]);
	if(line > SSD1309_MAX_LINES) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"ssd1309_cmd_clearline_handler: invalid arguments"
		);	

		return ERR_INVALID_ARGUMENT;
	}

	err = ssd1309_invert_line(line);
	if(err != ERR_OK) {
		return err;
	}

	ssd1309_update();

	return ERR_OK;
}