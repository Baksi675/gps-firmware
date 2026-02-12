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

#include <stdbool.h>
#include <string.h>

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
#include "arm_cortex_m4_systick.h"

/**
 * @brief OCR register bit positions.
 * 
 */
typedef enum {
	OCR_1_7V_1_6V = 4,
	OCR_1_8V_1_7V,
	OCR_1_9V_1_8V,
	OCR_2_0V_1_9V,
	OCR_2_1V_2_0V,
	OCR_2_2V_2_1V,
	OCR_2_3V_2_2V,
	OCR_2_4V_2_3V,
	OCR_2_5V_2_4V,
	OCR_2_6V_2_5V,
	OCR_2_7V_2_6V,
	OCR_2_8V_2_7V,
	OCR_2_9V_2_8V,
	OCR_3_0V_2_9V,
	OCR_3_1V_3_0V,
	OCR_3_2V_3_1V,
	OCR_3_3V_3_2V,
	OCR_3_4V_3_3V,
	OCR_3_5V_3_4V,
	OCR_3_6V_3_5V,
	OCR_CAPACITY_STATUS = 30,
	OCR_PWRUP_STATUS
}OCR_te;

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
	SD_PWRUP_STATUS_te pwrup_status;
	SD_TYPE_te type;
	SD_MIN_OPERATING_VOLTAGE_te min_operating_voltage;
	SD_MAX_OPERATIING_VOLTAGE_te max_operating_voltage;
	uint32_t sector_size;
	bool initialized;
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

static ERR_te sd_go_idle_state(void);

static ERR_te sd_send_cmd(SPI_REGDEF_ts *spi_instance, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o);
static ERR_te sd_cease_comms_helper(SD_HANDLE_ts *sd_handle, bool deinit);

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

	if(internal_state.initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_subsys: subsys is already initialized"
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
	if(internal_state.initialized) {
		internal_state = (struct internal_state_s){ 0 };

		cmd_deregister(&sd_cmd_client_info);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_deinit_subsys: subsys is not initialized"
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
	uint8_t free_index;

	if(!internal_state.initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_init_handle: subsys not initialized"
		);

		return ERR_INITIALIZATION_FAILURE;
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

	for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
		if(internal_state.sds[i].in_use == false) {
			free_index = i;	
			break;
		}
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

	spi_init(&sd_spi);

	str_cpy(
		internal_state.sds[free_index].name,
		sd_config->name,
		get_str_len(sd_config->name) + 1
	);

	internal_state.sds[free_index].spi_instance = sd_config->spi_instance;
	internal_state.sds[free_index].sclk_gpio_port = sd_config->sclk_gpio_port;
	internal_state.sds[free_index].cs_gpio_port = sd_config->cs_gpio_port;
	internal_state.sds[free_index].sclk_gpio_pin = sd_config->sclk_gpio_pin;
	internal_state.sds[free_index].miso_gpio_port = sd_config->miso_gpio_port;
	internal_state.sds[free_index].mosi_gpio_port = sd_config->mosi_gpio_port;
	internal_state.sds[free_index].cs_gpio_pin = sd_config->cs_gpio_pin;
	internal_state.sds[free_index].sclk_gpio_pin = sd_config->sclk_gpio_pin;
	internal_state.sds[free_index].miso_gpio_pin = sd_config->miso_gpio_pin;
	internal_state.sds[free_index].mosi_gpio_pin = sd_config->mosi_gpio_pin;
	internal_state.sds[free_index].gpio_alternate_function = sd_config->gpio_alternate_function;
	internal_state.sds[free_index].in_use = true;

	*sd_handle_o = &internal_state.sds[free_index];

	internal_state.sd_num++;

	// Wait for >= 1 ms after supply reached at least 2.2V, do it from main?
	DELAY(CONFIG_SD_HW_INIT_WAIT_TIME);

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
	uint8_t try = 1;
	do {
		err = sd_send_cmd(sd_config->spi_instance, 0, 0, false, &cmd_response);
		if(err == ERR_TIMEOUT) {
			LOG_ERROR(internal_state.subsys, 
				internal_state.log_level, 
				"sd_init_handle: sd_send_cmd timeout, retry %d/%d in 100 ms", try, CONFIG_SD_TIMEOUT_RETRY_NUM
			);
			try++;
			DELAY(10);
		}
	} while(err == ERR_TIMEOUT && try <= CONFIG_SD_TIMEOUT_RETRY_NUM);

	if(try > CONFIG_SD_TIMEOUT_RETRY_NUM) {
		LOG_ERROR(internal_state.subsys, 
			internal_state.log_level, 
			"sd_init_handle: initialization failure "
		);
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(cmd_response.r1 == 0x01) {
		cmd_response = (CMD_RESPONSE_ts){ 0 };
		err = sd_send_cmd(sd_config->spi_instance, 8, 0x000001AA, false, &cmd_response);
		if(err != ERR_OK) {
			// Try ACMD41
			cmd_response = (CMD_RESPONSE_ts){ 0 };
			uint32_t try_loop1 = 1;
			do {
				err = sd_send_cmd(sd_config->spi_instance, 41, 0x00000000, true, &cmd_response);
				if(err != ERR_OK) {
					// Try CMD1
					cmd_response = (CMD_RESPONSE_ts){ 0 };
					uint32_t try_loop2 = 1;
					do {
						err = sd_send_cmd(sd_config->spi_instance, 1, 0x00000000, false, &cmd_response);
						if(err != ERR_OK) {
							// Unknown card, error
							sd_cease_comms_helper(&internal_state.sds[free_index], true);
							
							return ERR_UNRECOGNIZED_HW;
						}
					}while(cmd_response.r1 == 0x01 && try_loop2-- <= CONFIG_SD_INVALID_RESP_RETRY_NUM);

					if(try_loop2 > CONFIG_SD_INVALID_RESP_RETRY_NUM) {
						sd_cease_comms_helper(&internal_state.sds[free_index], true);
						
						return ERR_INITIALIZATION_FAILURE;
					}

					if(cmd_response.r1 == 0x00) {
						// Card is MMC Ver. 3
						internal_state.sds[free_index].type = SD_TYPE_MMC;

						cmd_response = (CMD_RESPONSE_ts){ 0 };
						err = sd_send_cmd(sd_config->spi_instance, 16, 0x00000200, false, &cmd_response);
						if(err != ERR_OK) {
							sd_cease_comms_helper(&internal_state.sds[free_index], true);
							
							return ERR_INITIALIZATION_FAILURE;
						}
					}
				}
			}while(cmd_response.r1 == 0x01 && try_loop1-- <= CONFIG_SD_INVALID_RESP_RETRY_NUM);

			if(try_loop1 > CONFIG_SD_INVALID_RESP_RETRY_NUM) {
				sd_cease_comms_helper(&internal_state.sds[free_index], true);

				return ERR_INITIALIZATION_FAILURE;
			}

			if(cmd_response.r1 == 0x00) {
				// Card is SD Ver. 1
				internal_state.sds[free_index].type = SD_TYPE_SC_V1;

				cmd_response = (CMD_RESPONSE_ts){ 0 };
				err = sd_send_cmd(sd_config->spi_instance, 16, 0x00000200, false, &cmd_response);
				if(err != ERR_OK) {
					sd_cease_comms_helper(&internal_state.sds[free_index], true);

					return ERR_INITIALIZATION_FAILURE;
				}
			}
		}
		else {
			// Check if R7 matches 0x000001AA
			uint8_t expected[] = { 0x00, 0x00, 0x01, 0xAA };
			for(uint8_t i = 0; i < 4; i++) {
				// Mismatch
				if(cmd_response.r7[i] != expected[i]) {
					sd_cease_comms_helper(&internal_state.sds[free_index], true);
					
					return ERR_INITIALIZATION_FAILURE;
				}
			}

			// Match
			cmd_response = (CMD_RESPONSE_ts){ 0 };
			uint32_t try_loop1 = 1;
			do{
				err = sd_send_cmd(sd_config->spi_instance, 41, 0x40000000, true, &cmd_response);
				if(err != ERR_OK) {
					// Initialization failure
					sd_cease_comms_helper(&internal_state.sds[free_index], true);
					
					return ERR_INITIALIZATION_FAILURE;
				}
			}while (cmd_response.r1 == 0x01 && --try_loop1 <= CONFIG_SD_INVALID_RESP_RETRY_NUM);

			if(try_loop1 > CONFIG_SD_INVALID_RESP_RETRY_NUM) {
				sd_cease_comms_helper(&internal_state.sds[free_index], true);
				
				return ERR_INITIALIZATION_FAILURE;
			}

			if(cmd_response.r1 == 0x00) {
				cmd_response = (CMD_RESPONSE_ts){ 0 };
				err = sd_send_cmd(sd_config->spi_instance, 58, 0, false, &cmd_response);
				if(err != ERR_OK) {
					sd_cease_comms_helper(&internal_state.sds[free_index], true);

					return ERR_INITIALIZATION_FAILURE;
				}

				// Recreate OCR 
				uint32_t ocr = cmd_response.r3[0] << 24 | 
					cmd_response.r3[1] << 16 | 
					cmd_response.r3[2] << 8 | 
					cmd_response.r3[3];

				// Check and save power-up status
				if((ocr >> OCR_PWRUP_STATUS) & 0x1) {
					internal_state.sds[free_index].pwrup_status = SD_PWRUP_STATUS_READY;
				}
				else {
					internal_state.sds[free_index].pwrup_status = SD_PWRUP_STATUS_BUSY;
				}

				// Check and save card capacity 
				if((ocr >> OCR_CAPACITY_STATUS) & 0x1) {
					internal_state.sds[free_index].type = SD_TYPE_HC;
				}
				else {
					internal_state.sds[free_index].type = SD_TYPE_SC_V2;

					cmd_response = (CMD_RESPONSE_ts){ 0 };
					err = sd_send_cmd(sd_config->spi_instance, 16, 0x00000200, false, &cmd_response);
					if(err != ERR_OK) {
						sd_cease_comms_helper(&internal_state.sds[free_index], true);

						return ERR_INITIALIZATION_FAILURE;
					}
				}

				// Check and save operating voltage range
				bool voltage_low_found = false;
				for(uint8_t ocr_counter = 4; ocr_counter < 24; ocr_counter++) {
					if((ocr >> ocr_counter) & 0x1 && !voltage_low_found) {
						internal_state.sds[free_index].min_operating_voltage = (SD_MIN_OPERATING_VOLTAGE_te)(23 - ocr_counter);
						voltage_low_found = true;
					}
					else if((ocr >> ocr_counter) & 0x1 && voltage_low_found) {
						internal_state.sds[free_index].max_operating_voltage = (SD_MAX_OPERATIING_VOLTAGE_te)(23 - ocr_counter);
					}
				}
			}
		}
	}
	else {
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_INITIALIZATION_FAILURE;		
	}

	// Read CSD register
	cmd_response = (CMD_RESPONSE_ts){ 0 };
	sd_send_cmd(sd_config->spi_instance, 9, 0, false,  &cmd_response);
	if(err != ERR_OK) {
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(cmd_response.r1 != 0x00) {
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_UNKNOWN;
	}

	uint8_t token = 0;
	uint32_t start_time = systick_get_ms();
	uint32_t elapsed_time = 0;
	do {
		spi_receive(sd_config->spi_instance, &token, 1);
		elapsed_time = systick_get_ms() - start_time;
	}while(token != 0xFE && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

	if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_TIMEOUT;
	}

	if(token != 0xFE) {
		sd_cease_comms_helper(&internal_state.sds[free_index], true);

		return ERR_UNKNOWN;
	}

	uint8_t csd[16];
	memset(csd, 0, 16);
	spi_receive(sd_config->spi_instance, csd, 16);

	uint8_t crc[2];
	spi_receive(sd_config->spi_instance, crc, 2);

	sd_cease_comms_helper(&internal_state.sds[free_index], false);

	internal_state.sds[free_index].initialized = true;

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"sd_init_handle: sd handle %s initialized",
		internal_state.sds[free_index].name
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
	if(!sd_handle->initialized) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_deinit_handle: handle not initialized"
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

/**
 * @brief Gets the initialization state of the handle.
 * 
 * @param[in] sd_handle Handle to get the initialization state of. 
 * @param[out] handle_init_o Initialization state.
 * @return ERR_te Error generated during execution.
 */
ERR_te sd_get_handle_init(SD_HANDLE_ts *sd_handle, bool *handle_init_o) {
	*handle_init_o = sd_handle->initialized;

	return ERR_OK;
}

/**
 * @brief Reads data from an SD card.
 * 
 * @param[in] sd_handle The SD card handle to read data from. 
 * @param[out] read_buf The buffer to read data into.
 * @param[in] start_sector The sector to read data from.
 * @param[in] num_sectors Number of sectors to read from start_sector.
 * @return ERR_te Error generated during execution.
 */
ERR_te sd_read(SD_HANDLE_ts *sd_handle, uint8_t *read_buf, uint32_t start_sector, uint32_t num_sectors) {
	ERR_te err;
	uint32_t start_time = 0;
	uint32_t elapsed_time = 0;
	CMD_RESPONSE_ts cmd_response = { 0 };

	if(!internal_state.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_read: subsys not initialized or started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(sd_handle == NULL || num_sectors == 0) {
		return ERR_INVALID_ARGUMENT;
	}
	
	spi_set_comm(sd_handle->spi_instance, ENABLE);
	gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, LOW);

	if(num_sectors == 1) {
		err = sd_send_cmd(sd_handle->spi_instance, 17, start_sector, false, &cmd_response);
		if(err != ERR_OK) {
			sd_cease_comms_helper(sd_handle, true);

			return err;
		}
		
		if(cmd_response.r1 != 0x0) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}

		uint8_t token = 0xFF;
		start_time = systick_get_ms();

		do {
			elapsed_time = systick_get_ms() - start_time;
			spi_receive(sd_handle->spi_instance, &token, 1);
		} while(token == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);
		
		if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_TIMEOUT;
		}

		if(token != 0xFE) {
			// Error token
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}
		
		// Read data
		spi_receive(sd_handle->spi_instance, read_buf, 512);

		// Read CRC
		uint8_t crc[2];
		spi_receive(sd_handle->spi_instance, crc, 2);
	}
	else {
		err = sd_send_cmd(sd_handle->spi_instance, 18, start_sector, false, &cmd_response);
		if(err != ERR_OK) {
			sd_cease_comms_helper(sd_handle, true);

			return err;
		}
		
		if(cmd_response.r1 != 0x0) {
			sd_cease_comms_helper(sd_handle, true);
			return ERR_UNKNOWN;
		}	

		for(uint32_t i = 0; i < num_sectors; i++) {
			uint8_t token = 0xFF;
			start_time = systick_get_ms();

			do {
				elapsed_time = systick_get_ms() - start_time;
				spi_receive(sd_handle->spi_instance, &token, 1);
			} while(token == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);
			
			if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
				sd_cease_comms_helper(sd_handle, true);

				return ERR_TIMEOUT;
			}

			if(token != 0xFE) {
				// Error token
				sd_cease_comms_helper(sd_handle, true);

				return ERR_UNKNOWN;
			}
			
			// Read data
			spi_receive(sd_handle->spi_instance, read_buf + (i * 512), 512);

			// Read CRC
			uint8_t crc[2];
			spi_receive(sd_handle->spi_instance, crc, 2);
		}
		uint8_t retry = 0;
		do {
			err = sd_send_cmd(sd_handle->spi_instance, 12, 0, false, &cmd_response);
			if(err != ERR_OK) {
				sd_cease_comms_helper(sd_handle, true);

				return err;
			}
		}while(cmd_response.r1 != 0x0 && retry++ < CONFIG_SD_INVALID_RESP_RETRY_NUM);

		if(cmd_response.r1 != 0x00) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}
	}

	sd_cease_comms_helper(sd_handle, false);

	return ERR_OK;
}

/**
 * @brief Writes data to an SD card.
 * 
 * @param[in] sd_handle The SD card handle to write data. 
 * @param[out] write Writes data from this buffer.
 * @param[in] start_sector The sector to write data to.
 * @param[in] num_sectors Number of sectors to write from start_sector.
 * @return ERR_te Error generated during execution.
 */
ERR_te sd_write(SD_HANDLE_ts *sd_handle, uint8_t *write_buf, uint32_t start_sector, uint32_t num_sectors) {
	ERR_te err;
	uint32_t start_time = 0;
	uint32_t elapsed_time = 0;

	if(!internal_state.initialized || !internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"sd_write: subsys not initialized or started"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(sd_handle == NULL || num_sectors == 0) {
		return ERR_INVALID_ARGUMENT;
	}

	spi_set_comm(sd_handle->spi_instance, ENABLE);
	gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, LOW);

	CMD_RESPONSE_ts cmd_response = { 0 };

	if(num_sectors == 1) {
		err = sd_send_cmd(sd_handle->spi_instance, 24, start_sector, false, &cmd_response);
		if(err != ERR_OK) {
			sd_cease_comms_helper(sd_handle, true);
		
			return err;
		}
		
		if(cmd_response.r1 != 0x0) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}

		uint8_t token = 0xFE;
		
		// Send token
		spi_send(sd_handle->spi_instance, &token, 1);

		// Send data
		spi_send(sd_handle->spi_instance, write_buf, 512);

		// Send dummy CRC
		uint8_t dummy_crc[2] = { 0xFF, 0xFF };
		spi_send(sd_handle->spi_instance, dummy_crc, 2);

		uint8_t data_resp = 0xFF;
		start_time = systick_get_ms();

		do {
			elapsed_time = systick_get_ms() - start_time;
			spi_receive(sd_handle->spi_instance, &data_resp, 1);
		
		} while(data_resp == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);
		if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_TIMEOUT;
		}

		if((data_resp & 0b11111) != 0b00101) {
			// Data not accepted
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}

		// Wait until card releases MISO
		uint8_t busy = 0x00;
		start_time = systick_get_ms();
		do {
			spi_receive(sd_handle->spi_instance, &busy, 1);
			elapsed_time = systick_get_ms() - start_time;
		} while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

		if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_TIMEOUT;
		}
	}
	else {
		err = sd_send_cmd(sd_handle->spi_instance, 25, start_sector, false, &cmd_response);
		if(err != ERR_OK) {
			sd_cease_comms_helper(sd_handle, true);
		
			return err;
		}

		if(cmd_response.r1 != 0x0) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_UNKNOWN;
		}

		for(uint32_t i = 0; i < num_sectors; i++) {
			uint8_t token = 0xFC;
			
			// Send token
			spi_send(sd_handle->spi_instance, &token, 1);

			// Send data
			spi_send(sd_handle->spi_instance, write_buf + (i * 512), 512);

			// Send dummy CRC
			uint8_t dummy_crc[2] = { 0xFF, 0xFF };
			spi_send(sd_handle->spi_instance, dummy_crc, 2);

			uint8_t data_resp = 0xFF;
			start_time = systick_get_ms();

			do {
				elapsed_time = systick_get_ms() - start_time;
				spi_receive(sd_handle->spi_instance, &data_resp, 1);
			
			} while(data_resp == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);
			if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
				sd_cease_comms_helper(sd_handle, true);

				return ERR_TIMEOUT;
			}

			if((data_resp & 0b11111) != 0b00101) {
				// Data not accepted
				sd_cease_comms_helper(sd_handle, true);

				return ERR_UNKNOWN;
			}

			// Wait until card releases MISO
			uint8_t busy = 0x00;
			start_time = systick_get_ms();
			do {
				spi_receive(sd_handle->spi_instance, &busy, 1);
				elapsed_time = systick_get_ms() - start_time;
			} while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

			if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
				sd_cease_comms_helper(sd_handle, true);

				return ERR_TIMEOUT;
			}
		}

		uint8_t stop_tran_token = 0xFD;
			
		// Send token
		spi_send(sd_handle->spi_instance, &stop_tran_token, 1);

		// Wait until card releases MISO
		uint8_t busy = 0x00;
		start_time = systick_get_ms();
		do {
			spi_receive(sd_handle->spi_instance, &busy, 1);
			elapsed_time = systick_get_ms() - start_time;
		} while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

		if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
			sd_cease_comms_helper(sd_handle, true);

			return ERR_TIMEOUT;
		}
	}

	sd_cease_comms_helper(sd_handle, false);

	return ERR_OK;
}

/**
 * @brief Sends a command to the SD card via SPI.
 * 
 * @param[in] sd_handle The SD handle to send a command through.
 * @param[in] index The command index byte (without the 7th bit set)
 * @param[in] arg Command arguments (4 bytes)
 * @param[in] acmd Whether the command is an application specific command.
 * @param[out] cmd_response_o The response structure containing R1, R3 and R7 responses.
 * @return ERR_te Error generated during execution.
 */
static ERR_te sd_send_cmd(SPI_REGDEF_ts *spi_instance, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o) {	
	ERR_te err;
	uint32_t start_time;
	uint32_t elapsed_time;

	uint8_t crc = 0x00;
	uint8_t cmd[6];

	if(acmd) {
		CMD_RESPONSE_ts dummy;
		err = sd_send_cmd(spi_instance, 55, 0, false, &dummy);
		if(err != ERR_OK) {
			return err;
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
	spi_send(spi_instance, cmd, sizeof(cmd));

	// Special case, if CMD12, discard stuff byte (first byte)
	if(index == 12) {
		uint8_t stuff_byte;
		spi_receive(spi_instance, &stuff_byte, 1);
	}

	uint8_t r1 = 0xFF;
	start_time = systick_get_ms();

	// Receive R1
	do {
		spi_receive(spi_instance, &r1, 1);
		elapsed_time = systick_get_ms() - start_time;
	} while ((r1 & 0x80) && elapsed_time <= CONFIG_SD_R1_RESP_TIMEOUT);

	cmd_response_o->r1 = r1;

	if(elapsed_time > CONFIG_SD_R1_RESP_TIMEOUT) {
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

		spi_receive(spi_instance, cmd_response_o->r3, 4);
	}
	else if(
		index == 8 // CMD8 ==> R7
	) {
		// R3 is irrelevant, thus zeroed out
		memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));

		spi_receive(spi_instance, cmd_response_o->r7, 4);
	}
	else if(
		index == 12 // CMD12 ==> R1B
	) {
		// R3 and R7 are irrelevent, thus zeroed
		memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
		memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));

		uint8_t rx = 0x00;
		start_time = systick_get_ms();

		do{
			spi_receive(spi_instance, &rx, 1);
			elapsed_time = systick_get_ms() - start_time;
		}while(rx == 0x00 && elapsed_time <= CONFIG_SD_R1B_RESP_TIMEOUT);

		if(elapsed_time > CONFIG_SD_R1B_RESP_TIMEOUT) {
			return ERR_TIMEOUT;
		}
	}

	return ERR_OK;
}

/**
 * @brief A helper function to cease communication on error detection.
 * 
 * @param[in] sd_handle The sd_handle to cease communication on.
 * @return ERR_te Error generated during execution.
 */
static ERR_te sd_cease_comms_helper(SD_HANDLE_ts *sd_handle, bool deinit) {
	gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, HIGH);
	
	uint8_t dummy[2] = { 0xFF, 0xFF };
	spi_send(sd_handle->spi_instance, dummy, 2);
	
	spi_set_comm(sd_handle->spi_instance, DISABLE);

	if(deinit) {
		// Bypass init protection of sd_deinit_handle
		sd_handle->initialized = true;
		
		// Return the handle to zero state if some fields have been initialized 
		sd_deinit_handle(sd_handle);
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

static ERR_te sd_go_idle_state(void) {
	
}