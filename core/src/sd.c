/**
* @file sd.c
* @author github.com/Baksi675
* @brief SD card driver implementation file.
* @version 0.1
* @date 2026-02-05
* 
* @copyright Copyright (c) 2026
* 
*/

#include "sd.h"
#include "common.h"
#include "err.h"
#include "log.h"
#include "modules.h"
#include "cmd.h"
#include "configuration.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_rcc.h"
#include "stm32f401re_spi.h"
#include <stdbool.h>
#include <string.h>

struct sd_handle_s {
	char name[CONFIG_SD_MAX_NAME_LEN];
	SPI_REGDEF_ts *spi_instance;
	GPIO_REGDEF_ts *sclk_gpio_port;
	GPIO_REGDEF_ts *cs_gpio_port;
	GPIO_REGDEF_ts *miso_gpio_port;
	GPIO_REGDEF_ts *mosi_gpio_port;
	GPIO_PIN_te sclk_gpio_pin;
	GPIO_PIN_te cs_gpio_pin;
	GPIO_PIN_te miso_gpio_pin;
	GPIO_PIN_te mosi_gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
	bool in_use;
};

struct internal_state_s {
	SD_HANDLE_ts sds[CONFIG_SD_MAX_OBJECTS];
	MODULES_te subsys;
	LOG_LEVEL_te log_level;
	uint32_t sd_num;
	bool initialized;
	bool started;
};
struct internal_state_s internal_state;

typedef struct {
	uint8_t r1;
	uint8_t r3[4];
	uint8_t r7[4];
}CMD_RESPONSE_ts;

static ERR_te sd_send_cmd(SD_HANDLE_ts *sd_handle, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o);
static ERR_te sd_cmd_info_handler(uint32_t argc, char **argv);

CMD_INFO_ts sd_cmds[] = {
	{
		.name = "info",
		.help = "Shows SD information, usage: sd info",
		.handler = sd_cmd_info_handler
	}
};

CMD_CLIENT_INFO_ts sd_cmd_client_info = {
	.cmds_ptr = sd_cmds,
	.num_cmds = sizeof(sd_cmds) / sizeof(sd_cmds[0]),
	.log_level_ptr = &internal_state.log_level,
	.name = "sd"
};

/**
 * @brief Deinitializes the SD subsystem internal state to zeroes.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te sd_init_subsys(void) {
	ERR_te err;

	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };

	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_SD;
	internal_state.initialized = true;
	internal_state.started = false;

	err = cmd_register(&sd_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_subsys: cmd_register error"
		);
		return err;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"sd_init_subsys: subsys initialized"
	);

	return ERR_OK;
}

/**
 * @brief Deinitializes the SD subsystem internal state to zeroes.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te sd_deinit_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state = (struct internal_state_s){ 0 };

		cmd_deregister(&sd_cmd_client_info);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_deinit_subsys: subsys is not initialized or not stopped"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}
	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"sd_deinit_subsys: subsys deinitialized"
	);

	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te sd_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_start_subsys: subsys not initialized or already started"
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
ERR_te sd_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

/**
 * @brief Initializes an SD handle.
 * 
 * @param[in] sd_config The SD configuration.
 * @param[out] sd_handle_o The initialized handle.
 * @return ERR_te Error generated during execution.
 */
ERR_te sd_init_handle(SD_CONFIG_ts *sd_config, SD_HANDLE_ts **sd_handle_o) {
	ERR_te err;

	if(!internal_state.initialized) {
		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_handle: subsys not initialized"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}
	
	if(internal_state.sd_num == CONFIG_SD_MAX_OBJECTS) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_handle: subsystem out of memory space"
		);

		return ERR_NOT_ENOUGH_SPACE;
	}	

	if(!sd_config) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_handle: invalid argument"
		);
		return ERR_INVALID_ARGUMENT;
	}

	GPIO_HANDLE_ts sd_sclk = { 0 };
	sd_sclk.port = sd_config->sclk_gpio_port;
	sd_sclk.pin = sd_config->sclk_gpio_pin;
	sd_sclk.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	sd_sclk.alternate_function = sd_config->gpio_alternate_function;
	gpio_init(&sd_sclk);

	GPIO_HANDLE_ts sd_cs = { 0 };
	sd_cs.port = sd_config->cs_gpio_port;
	sd_cs.pin = sd_config->cs_gpio_pin;
	sd_cs.mode = GPIO_MODE_OUTPUT;
	gpio_init(&sd_cs);

	GPIO_HANDLE_ts sd_mosi = { 0 };
	sd_mosi.port = sd_config->mosi_gpio_port;
	sd_mosi.pin = sd_config->mosi_gpio_pin;
	sd_mosi.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	sd_mosi.alternate_function = sd_config->gpio_alternate_function;
	gpio_init(&sd_mosi);

	GPIO_HANDLE_ts sd_miso = { 0 };
	sd_miso.port = sd_config->miso_gpio_port;
	sd_miso.pin = sd_config->miso_gpio_pin;
	sd_miso.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	sd_miso.alternate_function = sd_config->gpio_alternate_function;
	gpio_init(&sd_miso);

	// Wait for >= 1 ms after supply reached at least 2.2V, do it from main?
	for(uint32_t i = 0; i < 150000; i++);

	// Configure SPI
	SPI_HANDLE_ts sd_spi = { 0 };
	sd_spi.instance = sd_config->spi_instance;
	sd_spi.bit_first = SPI_BIT_FIRST_MSB;
	sd_spi.clock_phase = SPI_CLOCK_PHASE_FIRST_TRANSITION;
	sd_spi.clock_polarity = SPI_CLOCK_POLARITY_0_IDLE;
	sd_spi.data_frame_format = SPI_DATA_FRAME_FORMATE_8_BIT;
	sd_spi.slave_select_mode = SPI_SLAVE_SELECT_MODE_SW;
	sd_spi.mode = SPI_MODE_MASTER;

	// Set SPI speed between 100 and 400 KHz then initialize
	uint32_t spi_pclk = 0;

	if(sd_config->spi_instance == SPI1 || sd_config->spi_instance == SPI4) {
		spi_pclk = rcc_get_apb2_clk();
	}
	else if(sd_config->spi_instance == SPI2 || sd_config->spi_instance == SPI3) {
		spi_pclk = rcc_get_apb1_clk();
	}

	uint32_t spi_pclk_cpy = spi_pclk;
	uint32_t div_factor = 2;
	while(div_factor <= 256) {
		spi_pclk = spi_pclk / div_factor;
	
		if(spi_pclk >= 100000 && spi_pclk <= 400000) {
			break;
		}

		spi_pclk = spi_pclk_cpy;
		div_factor *= 2;
	}

	switch (div_factor) {
		case 2:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_2;
			break;
		case 4:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_4;
			break;
		case 8:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_8;
			break;
		case 16:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_16;
			break;
		case 32:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_32;
			break;
		case 64:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_64;
			break;
		case 128:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_128;
			break;
		case 256:
			sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_256;
			break;
	}

	for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
		if(internal_state.sds[i].in_use == false) {
			str_cpy(
				internal_state.sds[i].name,
				sd_config->name,
				get_str_len(sd_config->name) + 1
			);

			internal_state.sds[i].spi_instance = sd_config->spi_instance;
			internal_state.sds[i].sclk_gpio_port = sd_config->sclk_gpio_port;
			internal_state.sds[i].cs_gpio_port = sd_config->cs_gpio_port;
			internal_state.sds[i].sclk_gpio_pin = sd_config->sclk_gpio_pin;
			internal_state.sds[i].miso_gpio_port = sd_config->miso_gpio_port;
			internal_state.sds[i].mosi_gpio_port = sd_config->mosi_gpio_port;
			internal_state.sds[i].cs_gpio_pin = sd_config->cs_gpio_pin;
			internal_state.sds[i].sclk_gpio_pin = sd_config->sclk_gpio_pin;
			internal_state.sds[i].miso_gpio_pin = sd_config->miso_gpio_pin;
			internal_state.sds[i].mosi_gpio_pin = sd_config->mosi_gpio_pin;
			internal_state.sds[i].gpio_alternate_function = sd_config->gpio_alternate_function;
			internal_state.sds[i].in_use = true;

			*sd_handle_o = &internal_state.sds[i];

			internal_state.sd_num++;

			break;
		}
	}

	spi_init(&sd_spi);

	// Enable SPI communication
	spi_set_comm(sd_spi.instance, ENABLE);

	// Set CS high
	gpio_write(sd_config->cs_gpio_port, sd_config->cs_gpio_pin, HIGH);

	// Send at least 74 dummy clocks
	uint8_t dummy_tx = 0xFF;
	for(uint8_t i = 0; i < 10; i++) {
		spi_send(sd_config->spi_instance, &dummy_tx, 1);
	}

	CMD_RESPONSE_ts cmd_response;

	gpio_write(sd_config->cs_gpio_port, sd_config->cs_gpio_pin, LOW);

	// Send CMD0
	err = sd_send_cmd(*sd_handle_o, 0, 0, false, &cmd_response);
	if(err != ERR_OK) {
		LOG_ERROR(internal_state.subsys, 
			internal_state.log_level, 
			"sd_init_handle: sd card initialization failure"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(cmd_response.r1 == 0x01) {
		cmd_response = (CMD_RESPONSE_ts){ 0 };
		err = sd_send_cmd(*sd_handle_o, 8, 0x000001AA, false, &cmd_response);
		if(err != ERR_OK) {
			// Try ACMD41
			cmd_response = (CMD_RESPONSE_ts){ 0 };
			uint32_t timeout = 10000;
			do {
				err = sd_send_cmd(*sd_handle_o, 41, 0x00000000, true, &cmd_response);
				if(err != ERR_OK) {
					// Try CMD1
					cmd_response = (CMD_RESPONSE_ts){ 0 };
					timeout = 10000;
					do {
						err = sd_send_cmd(*sd_handle_o, 1, 0x00000000, false, &cmd_response);
						if(err != ERR_OK) {
							// Unknown card, error
							return err;
						}
					}while(cmd_response.r1 == 0x01 && --timeout);

					if(timeout == 0) {
						return ERR_TIMEOUT;
					}

					if(cmd_response.r1 == 0x00) {
						// Card is MMC Ver. 3
						cmd_response = (CMD_RESPONSE_ts){ 0 };
						err = sd_send_cmd(*sd_handle_o, 16, 0x00000200, false, &cmd_response);
						if(err != ERR_OK) {
							return err;
						}

						return ERR_OK;
					}
				}
			}while(cmd_response.r1 == 0x01 && --timeout);

			if(timeout == 0) {
				return ERR_TIMEOUT;
			}

			if(cmd_response.r1 == 0x00) {
				// Card is SD Ver. 1
				cmd_response = (CMD_RESPONSE_ts){ 0 };
				err = sd_send_cmd(*sd_handle_o, 16, 0x00000200, false, &cmd_response);
				if(err != ERR_OK) {
					return err;
				}

				return ERR_OK;
			}
		}

		// Check if R7 matches 0x000001AA
		uint8_t expected[] = { 0x00, 0x00, 0x01, 0xAA };
		for(uint8_t i = 0; i < 4; i++) {
			// Mismatch
			if(cmd_response.r7[i] != expected[i]) {
				return ERR_INITIALIZATION_FAILURE;
			}
		}

		// Match
		cmd_response = (CMD_RESPONSE_ts){ 0 };
		uint32_t timeout = 10000;
		do{
			err = sd_send_cmd(*sd_handle_o, 41, 0x40000000, true, &cmd_response);
			if(err != ERR_OK) {
				// Initialization failure
				return err;
			}
		}while (cmd_response.r1 == 0x01 && --timeout);

		if(timeout == 0) {
			return ERR_TIMEOUT;
		}

		if(cmd_response.r1 == 0x00) {
			cmd_response = (CMD_RESPONSE_ts){ 0 };
			err = sd_send_cmd(*sd_handle_o, 58, 0, false, &cmd_response);
			if(err != ERR_OK) {
				return err;
			}

			if((cmd_response.r3[3] >> 29) & 1) {
				// CCS bit is set
				// Card is SD Ver. 2+ (block address)
				return ERR_OK;
			}
			else {
				// CCS bit is not set
				// Card is SD Ver. 2+ (byte address)
				cmd_response = (CMD_RESPONSE_ts){ 0 };
				err = sd_send_cmd(*sd_handle_o, 16, 0x00000200, false, &cmd_response);
				if(err != ERR_OK) {
					return err;
				}

				return ERR_OK;
			}
		}
	}
	else {
		LOG_ERROR(internal_state.subsys, 
			internal_state.log_level, 
			"sd_init_handle: sd card initialization failure"
		);

		return ERR_INITIALIZATION_FAILURE;		
	}

	// Set CS high
	gpio_write(sd_config->cs_gpio_port, sd_config->cs_gpio_pin, HIGH);

	// Disable SPI
	spi_set_comm(sd_spi.instance, DISABLE);

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"sd_init_handle: sd handle %s initialized",
		sd_config->name
	);

	return ERR_OK;
}

/**
 * @brief Deinitializes an SD handle.
 * 
 * @param[in] sd_handle The SD handle.
 * @return ERR_te The error code generated during execution.
 */
ERR_te sd_deinit_handle(SD_HANDLE_ts *sd_handle) {
	if(internal_state.started) {
		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_deinit_handle: subsys not stopped"
		);
		return ERR_ILLEGAL_ACTION;
	}

	if(internal_state.sd_num == 0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_deinit_handle: no such handle to deinitialize"
		);

		return ERR_UNINITIALZIED_OBJECT;
	}
	
	for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
		if(&internal_state.sds[i] == sd_handle) {
			uint8_t name_len = get_str_len(internal_state.sds[i].name) + 1;
			char name[name_len];
			str_cpy(name, internal_state.sds[i].name, name_len);

			internal_state.sds[i] = (SD_HANDLE_ts){ 0 };

			internal_state.sd_num--;

			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"sd_deinit_handle: sd handle %s deinitialized",
				name
			);

			break;
		}

		if(i == CONFIG_SD_MAX_OBJECTS - 1) {
			LOG_ERROR(
				internal_state.subsys, 
				internal_state.log_level,
				"sd_deinit_handle: no such handle to deinitialize"
			);
			
			return ERR_UNINITIALZIED_OBJECT;
		}
	}

	return ERR_OK;
}

static ERR_te sd_send_cmd(SD_HANDLE_ts *sd_handle, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o) {	
	uint8_t crc = 0x00;
	uint8_t cmd[6];

	if(acmd) {
		CMD_RESPONSE_ts dummy;
		if(sd_send_cmd(sd_handle, 55, 0, false, &dummy) != ERR_OK) {
			return ERR_TIMEOUT;
		}
	}

	// CMD0 CRC needed
	if(index == 0x00) {
		crc = 0x95;
	}
	// CMD8 CRC needed
	else if(index == 0x08) {
		crc = 0x87;
	}

	cmd[0] = 0x40 | index;
	cmd[1] = (arg >> 24) & 0xFF;
	cmd[2] = (arg >> 16) & 0xFF;
	cmd[3] = (arg >> 8) & 0xFF;
	cmd[4] = arg & 0xFF;
	cmd[5] = crc;

	// Send command
	spi_send(sd_handle->spi_instance, cmd, sizeof(cmd));

	// Receive R1
	uint8_t r1 = 0xFF;
	uint32_t timeout = 1000;

	do {
		spi_receive(sd_handle->spi_instance, &r1, 1);
	} while (r1 == 0xFF && --timeout);

	cmd_response_o->r1 = r1;

	if(timeout == 0) {
		spi_set_comm(sd_handle->spi_instance, DISABLE);
		gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, HIGH);

		return ERR_TIMEOUT;
	}

	// Command response is R1
	if(
		index == 0 ||	// CMD0 ==> R1
		index == 1 ||	// CMD1 ==> R1
		index == 41 ||	// CMD41 ==> R1
		index == 9 ||	// CMD9 ==> R1
		index == 10 ||	// CMD10 ==> R1
		index == 16 ||	// CMD16 ==> R1
		index == 17 ||	// CMD17 ==> R1
		index == 18 ||	// CMD18 ==> R1
		index == 23 ||	// CMD23 ==> R1
		index == 24 ||	// CMD24 ==> R1
		index == 25 ||	// CMD25 ==> R1
		index == 55	// CMD55 ==> R1
	) {
		// R3 and R7 are irrelevent, thus zeroed
		memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
		memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));
	}
	else if(
		index == 58 // CMD58 ==> R3
	) {
		// R7 is irrelevant, thus zeroed
		memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));

		spi_receive(sd_handle->spi_instance, cmd_response_o->r3, 4);
	}
	else if(
		index == 8 // CMD8 ==> R7
	) {
		// R3 is irrelevant, thus zeroed out
		memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));

		spi_receive(sd_handle->spi_instance, cmd_response_o->r7, 4);
	}
	else if(
		index == 12 // CMD8 ==> R1B
	) {
		// R3 and R7 are irrelevent, thus zeroed
		memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
		memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));

		uint8_t rx = 0x00;
		timeout = 100000;

		do{
			spi_receive(sd_handle->spi_instance, &rx, 1);
		}while(rx == 0x00 && --timeout);

		if(timeout == 0) {
			spi_set_comm(sd_handle->spi_instance, DISABLE);
			gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, HIGH);

			return ERR_TIMEOUT;
		}
	}

	return ERR_OK;
}

/**
 * @brief Handler routine for the info command. Shows information about objects commands.
 * 
 * @param[in] argc Argument count.
 * @param[in] argv Argument list.
 * @return ERR_te Error generated during execution.
 */
static ERR_te sd_cmd_info_handler(uint32_t argc, char **argv) {
	if(argc != 2) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"sd_cmd_info_handler: invalid arguments"
		);
		return ERR_INVALID_ARGUMENT;		
	}

	for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
		if(internal_state.sds[i].in_use == true) {
			LOG_INFO(
				internal_state.subsys, 
				internal_state.log_level,
				"name: %s", 
				internal_state.sds[i].name
			);
		}
	}

	return ERR_OK;
}