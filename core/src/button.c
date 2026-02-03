/**
 * @file button.c
 * @author github.com/Baksi675
 * @brief Button module implementation file.
 * @version 0.1
 * @date 2026-01-09
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 // TODO
 // ADD TIMER FOR DEBOUNCE AND HELD TIME

#include "button.h"
#include "arm_cortex_m4_systick.h"
#include "cmd.h"
#include "common.h"
#include "err.h"
#include "stm32f401re_gpio.h"
#include "configuration.h"
#include "log.h"
#include "modules.h"

struct button_handle_s {
	char name[CONFIG_BUTTON_MAX_NAME_LEN];
	GPIO_REGDEF_ts *gpio_port;
	GPIO_PIN_te gpio_pin;
	BUTTON_PUSHED_TYPE_te pushed_type;
	uint32_t debounce_limit_ms;
	uint32_t held_limit_ms;
	//BUTTON_DEBOUNCE_TIME_te debounce_limit;
	//BUTTON_HELD_TIME_te held_limit;
	uint32_t debounce_started_ms;
	uint32_t held_started_ms;
	bool debounce_started;
	bool held_started;
	bool pushed;
	bool held;
	bool in_use;
};

struct internal_state_s {
	struct button_handle_s buttons[CONFIG_BUTTON_MAX_OBJECTS];		
	uint8_t button_num;
	LOG_LEVEL_te log_level;
	MODULES_te subsys;
	bool initialized;
	bool started;										
};
static struct internal_state_s internal_state;

static ERR_te button_getpushed_handler(uint32_t argc, char **argv);
static ERR_te button_getheld_handler(uint32_t argc, char **argv);
static ERR_te button_cmd_info_handler(uint32_t argc, char **argv);

static CMD_INFO_ts button_cmds[] = {
	{
		.name = "getpushed",
		.help = "Gets the pushed state, usage: button getpushed <button>",
		.handler = button_getpushed_handler
	},
	{
		.name = "getheld",
		.help = "Gets the held state, usage: button getheld <button>",
		.handler = button_getheld_handler
	},
	{
		.name = "info",
		.help = "Shows button information, usage: button info",
		.handler = button_cmd_info_handler
	}
};

static CMD_CLIENT_INFO_ts button_cmd_client_info = {
	.cmds_ptr = button_cmds,
	.num_cmds = sizeof(button_cmds) / sizeof(button_cmds[0]),
	.name = "button",
	.log_level_ptr = &internal_state.log_level
};

/**
 * @brief Initializes the button subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te button_init_subsys(void) {
	ERR_te err;

	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };
	
	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_BUTTON;
	internal_state.initialized = true;
	internal_state.started = false;

	SYSTICK_CFG_ts systick_cfg = { 0 };
	systick_cfg.clk_source = SYSTICK_CLK_SOURCE_PROCESSOR;
	systick_cfg.interrupt = SYSTICK_IT_TRUE;
	systick_init(&systick_cfg);
	
	err = cmd_register(&button_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_init_subsys: cmd_register error"
		);

		return err;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"button_init_subsys: subsys initialized"
	);

	return ERR_OK;	
}

/**
 * @brief Deinitializes the button subsystem internal state to zeroes.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te button_deinit_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state = (struct internal_state_s){ 0 };

		cmd_deregister(&button_cmd_client_info);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_deinit_subsys: subsys is not initialized or not stopped"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"button_deinit_subsys: subsys deinitialized"
	);

	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te button_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"button_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_start_subsys: subsys not initialized or already started"
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
ERR_te button_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"button_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

/**
 * @brief Registers a button to the internal state of the button module.
 * 
 * @param[in] button_cfg An input pointer to a configuration structure. 
 * @param[out] button_handle_o An output pointer to the handle of the registered and initialzied button. 
 * @return ERR_te The error generated by the function, ERR_OK means no error occured.
 */
ERR_te button_init_handle(BUTTON_CFG_ts *button_cfg, BUTTON_HANDLE_ts **button_handle_o) {
	if(!internal_state.initialized) {
		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"button_init_handle: subsys not initialized"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}

	if(internal_state.button_num == CONFIG_BUTTON_MAX_OBJECTS) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_init_handle: subsystem out of memory space"
		);

		return ERR_NOT_ENOUGH_SPACE;
	}

	if(!button_cfg) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_init_handle: invalid argument"
		);
		return ERR_INVALID_ARGUMENT;
	}

	GPIO_HANDLE_ts gpio = { 0 };
	gpio.port = button_cfg->gpio_port;
	gpio.pin = button_cfg->gpio_pin;
	gpio.output_type = GPIO_OUTPUT_TYPE_PUSHPULL;
	gpio.mode = GPIO_MODE_INPUT;

	gpio_init(&gpio);

	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(internal_state.buttons[i].in_use == false) {
			str_cpy(
				internal_state.buttons[i].name,
				button_cfg->name,
				get_str_len(button_cfg->name) + 1
			);

			internal_state.buttons[i].debounce_limit_ms = button_cfg->debounce_limit_ms;
			internal_state.buttons[i].gpio_port = button_cfg->gpio_port;
			internal_state.buttons[i].gpio_pin = button_cfg->gpio_pin;
			internal_state.buttons[i].pushed_type = button_cfg->pushed_type;
			internal_state.buttons[i].held_limit_ms = button_cfg->held_limit_ms;
			internal_state.buttons[i].held = false;
			internal_state.buttons[i].held_started_ms = 0;
			internal_state.buttons[i].debounce_started = false;
			internal_state.buttons[i].held_started = false;
			internal_state.buttons[i].pushed = false;
			internal_state.buttons[i].debounce_started_ms = 0;
			internal_state.buttons[i].in_use = true;

			*button_handle_o = &internal_state.buttons[i];

			internal_state.button_num++;

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"button_init_handle: button handle %s initialized",
				internal_state.buttons[i].name
			);

			break;
		}
	}

	return ERR_OK;
}

/**
 * @brief Deinitializes an button object.
 * 
 * @param[in] button_handle The button handle.
 * @return ERR_te The error code generated during execution.
 */
ERR_te button_deinit_handle(BUTTON_HANDLE_ts const *button_handle) {
	if(internal_state.started) {
		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"button_deinit_handle: subsys not stopped"
		);
		return ERR_ILLEGAL_ACTION;
	}

	if(internal_state.button_num == 0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_deinit_handle: no such handle to deinitialize"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}
	
	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(&internal_state.buttons[i] == button_handle) {
			uint8_t name_len = get_str_len(internal_state.buttons[i].name) + 1;
			char name[name_len];
			str_cpy(name, internal_state.buttons[i].name, name_len);

			internal_state.buttons[i] = (BUTTON_HANDLE_ts){ 0 };

			internal_state.button_num--;

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"button_deinit_handle: io handle %s deinitialized",
				name
			);

			break;
		}

		if(i == CONFIG_BUTTON_MAX_OBJECTS - 1) {
			LOG_ERROR(
				internal_state.subsys, 
				internal_state.log_level,
				"button_deinit_handle: no such handle to deinitialize"
			);
			
			return ERR_UNINITIALZIED_OBJECT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Runs a button handle.
 * 
 * @param[in] button_handle The button handle to run. 
 * @return ERR_te Error generated during execution.
 */
ERR_te button_run_handle(BUTTON_HANDLE_ts *button_handle) {
	if(!internal_state.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"button_run_handle: subsystem not initialized or started"
		);

		return ERR_UNKNOWN;
	}

	PIN_STATUS_te pin_status = gpio_read(button_handle->gpio_port, button_handle->gpio_pin);

	// If the debounce has not yet been completed, complete it (if it's enabled)
	if(button_handle->pushed == false) {
		if((button_handle->pushed_type == BUTTON_PUSHED_TYPE_HIGH && 
		pin_status == HIGH) ||
		(button_handle->pushed_type == BUTTON_PUSHED_TYPE_LOW && 
		pin_status == LOW)) {
			uint32_t current_ms = systick_get_ms();

			if(!button_handle->debounce_started) {
				button_handle->debounce_started = true;
				button_handle->debounce_started_ms = current_ms;
			}

			if(button_handle->debounce_started &&
			current_ms - button_handle->debounce_started_ms >= button_handle->debounce_limit_ms) {
				button_handle->pushed = true;
				button_handle->debounce_started = false;
			}
		}
		else {
			if(button_handle->debounce_started) {
				button_handle->debounce_started = false;
				button_handle->debounce_started_ms = 0;
			}
		}
	}

	// If the held property is enabled, check if button is held for long enough
	if(button_handle->pushed == true && button_handle->held == false) {
		if((button_handle->pushed_type == BUTTON_PUSHED_TYPE_HIGH && 
		pin_status == HIGH) ||
		(button_handle->pushed_type == BUTTON_PUSHED_TYPE_LOW && 
		pin_status == LOW)) {
			uint32_t current_ms = systick_get_ms();

			if(!button_handle->held_started) {
				button_handle->held_started = true;
				button_handle->held_started_ms = current_ms;
			}

			if(button_handle->held_started &&
			current_ms - button_handle->held_started_ms >= button_handle->held_limit_ms) {
				button_handle->held = true;
				button_handle->held_started = false;
			}
		}
		else {
			if(button_handle->held_started) {
				button_handle->held_started = false;
				button_handle->held_started_ms = 0;
			}
		}
	}

	// If the button has been pushed, check for button release then restore states and counters
	if(button_handle->pushed == true) {
		if((button_handle->pushed_type == BUTTON_PUSHED_TYPE_HIGH && 
		pin_status == LOW) ||
		(button_handle->pushed_type == BUTTON_PUSHED_TYPE_LOW && 
		pin_status == HIGH)) {
			uint32_t current_ms = systick_get_ms();

			if(!button_handle->debounce_started) {
				button_handle->debounce_started = true;
				button_handle->debounce_started_ms = current_ms;
			}

			if(button_handle->debounce_started &&
			current_ms - button_handle->debounce_started_ms >= button_handle->debounce_limit_ms) {
				button_handle->pushed = false;
				button_handle->held = false;
				button_handle->debounce_started = false;
				button_handle->held_started = false;
			}
		}
		else {
			if(button_handle->debounce_started) {
				button_handle->debounce_started = false;
				button_handle->debounce_started_ms = 0;
			}
		}
	}

	return ERR_OK;
}

/**
 * @brief Runs all buttons in the internal state.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te button_run_handle_all(void) {
	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(internal_state.buttons[i].in_use == true) {
			ERR_te err = button_run_handle(&internal_state.buttons[i]);
			if(err != ERR_OK) {
				return err;
			}
		}
	}

	return ERR_OK;
}

/**
 * @brief 
 * 
 * @param button_handle 
 * @param pushed_state 
 * @return ERR_te 
 */
ERR_te button_get_pushed_state(BUTTON_HANDLE_ts const *button_handle, bool *pushed_state_o) {
	if(!internal_state.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"button_get_pushed_state: handle not initialized or subsystem not started"
		);

		return ERR_ILLEGAL_ACTION;
	}

	*pushed_state_o = button_handle->pushed;

	return ERR_OK;
}

/**
 * @brief 
 * 
 * @param button_handle 
 * @param held_state 
 * @return ERR_te 
 */
ERR_te button_get_held_state(BUTTON_HANDLE_ts const *button_handle, bool *held_state_o) {
	if(!internal_state.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"button_get_held_state: handle not initialized or subsystem not started"
		);

		return ERR_ILLEGAL_ACTION;
	}

	*held_state_o = button_handle->held;

	return ERR_OK;
}

/**
 * @brief Handler routine for the getheld command. Reads the button pushed state.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te button_getpushed_handler(uint32_t argc, char **argv) {
	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"button_getpushed_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;
	}

	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(str_cmp(argv[2], internal_state.buttons[i].name) == true) {
			bool pushed_state;
			ERR_te err = button_get_pushed_state(&internal_state.buttons[i], &pushed_state);
			if(err != ERR_OK) {
				return err;
			}

			LOG_INFO(
				internal_state.subsys,
				internal_state.log_level,
				"button_getpushed_handler: %s pushed state: %d",
				internal_state.buttons[i].name,
				pushed_state
			);

			return ERR_OK;
		}

		if(i == internal_state.button_num - 1) {
			LOG_ERROR(
				internal_state.subsys,
				internal_state.log_level, 
				"button_getpushed_handler: invalid arguments"
			);
			return ERR_INVALID_ARGUMENT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Handler routine for the getheld command. Reads the button held state.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te button_getheld_handler(uint32_t argc, char **argv) {
	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"button_getheld_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;
	}

	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(str_cmp(argv[2], internal_state.buttons[i].name) == true) {
			bool held_state;
			ERR_te err = button_get_held_state(&internal_state.buttons[i], &held_state);
			if(err != ERR_OK) {
				return err;
			}

			LOG_INFO(
				internal_state.subsys,
				internal_state.log_level,
				"button_getheld_handler: %s held state: %d",
				internal_state.buttons[i].name,
				held_state
			);

			return ERR_OK;
		}

		if(i == internal_state.button_num - 1) {
			LOG_ERROR(
				internal_state.subsys,
				internal_state.log_level, 
				"button_getheld_handler: invalid arguments"
			);
			return ERR_INVALID_ARGUMENT;
		}
	}

	return ERR_OK;
}

/**
 * @brief 
 * 
 * @param argc 
 * @param argv 
 * @return ERR_te 
 */
static ERR_te button_cmd_info_handler(uint32_t argc, char **argv) {
	if(argc != 2) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"button_cmd_info_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;		
	}

	LOG_INFO(internal_state.subsys, internal_state.log_level, "Printing button information:");

	for(uint32_t i = 0; i < CONFIG_BUTTON_MAX_OBJECTS; i++) {
		if(internal_state.buttons[i].in_use == true) {
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"name: %s", 
				internal_state.buttons[i].name
			);
		}
	}

	return ERR_OK;
}