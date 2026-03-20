/**
 * @file neo6.c
 * @author github.com/Baksi675
 * @brief NEO-6M GPS module implementation file.
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdint.h>

#include "neo6.h"
#include "cmd.h"
#include "common.h"
#include "err.h"
#include "log.h"
#include "modules.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "cbuf.h"
#include "configuration.h"

#define VTG_TMG_T_DEGREE_POS			1			// Track made good relative to true north
#define VTG_TMG_T_POS					2			// Indicator of true north
#define VTG_TMG_M_DEGREE_POS			3			// Track made good relative to magnetic north
#define VTG_TMG_M_POS					4			// Indicator of magnetic north
#define VTG_SPEED_KNOTS_POS				5			// Speed in knots
#define VTG_SPEED_N_POS					6			// Knots indicator
#define VTG_SPEED_OVER_GROUND_POS		7			// Speed over ground in kph
#define VTG_SPEED_OVER_GROUND_K_POS		8			// Kph indicator
#define VTG_MODE_INDICATOR_POS			9			// Mode indicator

#define GGA_UTC_POS						1			// UTC time
#define GGA_LAT_POS						2			// Latutide
#define GGA_LAT_DIR_POS					3			// Latitude direction
#define GGA_LON_POS						4			// Longitude
#define GGA_LON_DIR_POS					5			// Longitude direction
#define GGA_QUALITY_FIX_POS				6			// Quality of the fix
#define GGA_NUM_SATS_POS				7			// Number of satellites used for calculation
#define GGA_HDOP_POS					8			// HDOP 
#define GGA_ORTH_HEIGHT_POS				9			// Orthometric height
#define GGA_ORTH_HEIGHT_UNIT_POS		10			// Unit of orthometric height
#define GGA_GEOID_SEP_POS				11			// Geoid separation
#define GGA_GEOID_SEP_UNIT_POS			12			// Unit of geoid separation
#define GGA_DATA_REC_AGE_POS			13			// Age of data record
#define GGA_REF_STAT_ID_POS				14			// Reference station ID

#define GSA_MODE_POS					1			// Mode (Automatic, manual)
#define GSA_FIX_TYPE_POS				2			// Fix type 
#define GSA_PRN_NUM_POS					3			// SV pseudorandom code
#define GSA_PDOP_POS					15			// Position dillution of precision
#define GSA_HDOP_POS					16			// Horizontal dillution of precision
#define GSA_VDOP_POS					17			// Vertical dillution of precision

#define GSV_NUM_OF_MESSAGES_POS			1			// Total number of messages of this type in this cycle
#define GSV_NUM_OF_THIS_MESSAGE_POS		2			// Message number
#define GSV_NUM_SATS_VISIBLE_POS		3			// Total number of SVs visible	
#define GSV_SV_PRN_NUM_POS				4			// SV PRN number
#define GSV_ELEVATION_POS				5			// Elevation, in degrees, 90° maximum
#define GSV_AZIMUTH_POS					6			// Azimuth, degrees from True North, 000° through 359°
#define GSV_SNR_POS						7			// SNR, 00 through 99 dB (null when not tracking)

static uint8_t usart_data_recv_cbuf_mem[128];
static uint8_t nmea_cbuf_mem[128];

struct neo6_handle_s {
	USART_REGDEF_ts *usart_instance;
	USART_BAUD_RATE_te usart_baud_rate;
	GPIO_REGDEF_ts *rx_gpio_port;
	GPIO_PIN_te rx_gpio_pin;
	GPIO_REGDEF_ts *tx_gpio_port;
	GPIO_PIN_te tx_gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
	bool initialized;
};

struct internal_state_s {
	NEO6_HANDLE_ts neo6_handle;
	CBUF_HANDLE_ts usart_data_recv_cbuf;
	CBUF_HANDLE_ts nmea_cbuf;
	NEO6_INFO_ts neo6_info;
	MODULES_te subsys;
	LOG_LEVEL_te log_level;
	bool dump_raw_nmea;
	bool initialized;
	bool started;
};
static struct internal_state_s internal_state;

static ERR_te neo6_calc_checksum(char *msg, uint32_t msg_len, uint8_t *checksum_o);
static ERR_te neo6_process_msg(uint8_t *msg, uint32_t msg_len);
static ERR_te neo6_process_rmc(char **tokens);
static ERR_te neo6_process_vtg(char **tokens);
static ERR_te neo6_process_gga(char **tokens);
static ERR_te neo6_process_gsa(char **tokens);
static ERR_te neo6_process_gsv(char **tokens);

static ERR_te neo6_dumpnmea_handler(uint32_t argc, char **argv);

static CMD_INFO_ts neo6_cmds[] = {
	{
		.name = "dumpnmea",
		.help = "Dumps raw NMEA messages, usage: neo6 dumpnmea <true|false>",
		.handler = neo6_dumpnmea_handler
	},
};

static CMD_CLIENT_INFO_ts neo6_cmd_client_info = {
	.cmds_ptr = neo6_cmds,
	.num_cmds = sizeof(neo6_cmds) / sizeof(neo6_cmds[0]),
	.name = "neo6",
	.log_level_ptr = &internal_state.log_level
};


/** 
 * @defgroup neo6_Public_APIs GPS Public APIs
 * @{
 */

/**
 * @brief Initializes the NEO6 subsystem internal state to a clean state and registers the subsystem commands.
 * 
 * @return ERR_te The error generated during execution.
 */
ERR_te neo6_init_subsys(void) {
	ERR_te err;
	
	if(internal_state.initialized || internal_state.started) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_init_subsys: subsys is already initialized or started"
		);
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state = (struct internal_state_s){ 0 };
	internal_state.log_level = LOG_LEVEL_INFO;
	internal_state.subsys = MODULES_NEO6;
	internal_state.usart_data_recv_cbuf.ptr = usart_data_recv_cbuf_mem;
	internal_state.usart_data_recv_cbuf.size = sizeof(usart_data_recv_cbuf_mem);
	internal_state.nmea_cbuf.ptr = nmea_cbuf_mem;
	internal_state.nmea_cbuf.size = sizeof(nmea_cbuf_mem);
	internal_state.dump_raw_nmea = false;
	internal_state.initialized = true;
	internal_state.started = false;

	err = cmd_register(&neo6_cmd_client_info);
	if(err != ERR_OK) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_init_subsys: cmd_register error"
		);

		return err;
	}

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"neo6_init_subsys: subsys initialized"
	);

	return ERR_OK;	
}

/**
 * @brief Deinitializes the neo6 subsystem.
 * 
 * @return ERR_te Eror generated during execution.
 */
ERR_te neo6_deinit_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state = (struct internal_state_s){ 0 };

		ERR_te err = cmd_deregister(&neo6_cmd_client_info);
		if(err != ERR_OK) {
			return err;
		}
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_deinit_subsys: subsys is not initialized or stopped"
		);

		return ERR_DEINITIALIZATION_FAILURE;
	}

	LOG_INFO(
		internal_state.subsys, 
		internal_state.log_level,
		"neo6_deinit_subsys: subsystem deinitialized"
	);
	return ERR_OK;
}

/**
 * @brief Starts the subsystem.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te neo6_start_subsys(void) {
	if(internal_state.initialized && !internal_state.started) {
		internal_state.started = true;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_start_subsys: subsys started"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_start_subsys: subsys not initialized or already started"
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
ERR_te neo6_stop_subsys(void) {
	if(internal_state.initialized && internal_state.started) {
		internal_state.started = false;

		LOG_INFO(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_stop_subsys: subsys stopped"
		);
	}
	else {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_stop_subsys: subsys not initialized or already already stopped"
		);
		
		return ERR_UNKNOWN;
	}

	return ERR_OK;
}

/**
 * @brief Initializes the GPS software module. Configures the internal state, initializes the GPIO pins and the USART peripheral.
 * 
 * @param[in] neo6_config GPS configuration object.
 * @param[out] neo6_handle_o Initialized handle.
 * @return ERR_te Error generated during execution.
 */
ERR_te neo6_init_handle(NEO6_CONFIG_ts *neo6_config,  NEO6_HANDLE_ts **neo6_handle_o) {
	if(internal_state.neo6_handle.initialized == true) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"neo6_init_handle: handle already initialized"
		);

		return ERR_INITIALIZATION_FAILURE;
	}

	if(neo6_config->usart_instance == (void*)0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_init_handle: No USART peripheral given"
		);

		return ERR_INITIALIZATION_FAILURE;
	}
	else if(neo6_config->rx_gpio_port == (void*)0 || neo6_config->tx_gpio_port == (void*)0) {
		LOG_ERROR(
			internal_state.subsys, 
			internal_state.log_level,
			"neo6_init_handle: No GPIO peripheral given"
		);	
		
		return ERR_INITIALIZATION_FAILURE;
	}

	internal_state.neo6_handle.usart_instance = neo6_config->usart_instance;
	internal_state.neo6_handle.usart_baud_rate = neo6_config->usart_baud_rate;
	internal_state.neo6_handle.rx_gpio_port = neo6_config->rx_gpio_port;
	internal_state.neo6_handle.tx_gpio_port = neo6_config->tx_gpio_port;
	internal_state.neo6_handle.rx_gpio_pin = neo6_config->rx_gpio_pin;
	internal_state.neo6_handle.tx_gpio_pin = neo6_config->tx_gpio_pin;
	internal_state.neo6_handle.gpio_alternate_function = neo6_config->gpio_alternate_function;

	USART_HANDLE_ts neo6_usart = { 0 };
	neo6_usart.instance = neo6_config->usart_instance;
	neo6_usart.baud_rate = neo6_config->usart_baud_rate;
	neo6_usart.interrupt_en = USART_INTERRUPT_EN_TRUE;
	usart_init(&neo6_usart);
	usart_set_reception(neo6_usart.instance, ENABLE);

	GPIO_HANDLE_ts neo6_rx = { 0 };
	neo6_rx.port = neo6_config->rx_gpio_port;
	neo6_rx.pin = neo6_config->rx_gpio_pin;
	neo6_rx.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	neo6_rx.alternate_function = neo6_config->gpio_alternate_function;
	gpio_init(&neo6_rx);

	GPIO_HANDLE_ts neo6_tx = { 0 };
	neo6_tx.port = neo6_config->tx_gpio_port;
	neo6_tx.pin = neo6_config->tx_gpio_pin;
	neo6_tx.mode = GPIO_MODE_ALTERNATE_FUNCTION;
	neo6_tx.alternate_function = neo6_config->gpio_alternate_function;
	gpio_init(&neo6_tx);

	internal_state.neo6_handle.initialized = true;

	return ERR_OK;
}

// Possible bug in the code --> When characters are received in a way, that the characters include the end of a message and the beginning of a new message, the new message is lost.
// Currently doesn't cause any issues
// change uint8_t to uint32_t
/**
 * @brief Runs the neo6 subsystem as part of the state machine.
 * 
 * @return ERR_te Error generated during execution.
 */
ERR_te neo6_run(void) {
	uint8_t data_len = 0;
	cbuf_len(&internal_state.usart_data_recv_cbuf, &data_len);

	if(data_len) {
		uint8_t data[data_len];

		cbuf_read(&internal_state.usart_data_recv_cbuf, data);

		cbuf_write(&internal_state.nmea_cbuf, data, data_len);

		for(uint32_t i = 0; i < data_len; i++) {
			if(data[i] == '\n') {
				uint8_t nmea_len = 0;
				cbuf_len(&internal_state.nmea_cbuf, &nmea_len);

				uint8_t nmea_msg[nmea_len];
				cbuf_read(&internal_state.nmea_cbuf, nmea_msg);

				nmea_msg[nmea_len - 2] = '\0';
				nmea_msg[nmea_len - 1] = '\0';

				if(internal_state.dump_raw_nmea == true) {
					LOG_INFO(
						internal_state.subsys,
						internal_state.log_level,
						"%s", nmea_msg
					);
				} 

				// Process NMEA message
				ERR_te err = neo6_process_msg(nmea_msg, nmea_len);
				if(err != ERR_OK) {
					return err;
				}
			}
		}
	}

	return ERR_OK;
}

/**
 * @brief Gets the neo6 information from the internal state.
 * 
 * @param[out] neo6_info_o The neo6 GPS information.
 * @return ERR_te Error generated during execution.
 */
ERR_te neo6_get_info(NEO6_INFO_ts **neo6_info_o) {
	*neo6_info_o = &internal_state.neo6_info;

	return ERR_OK;
}
/** @} */

/** 
 * @defgroup neo6_Internal_Helper GPS Internal Helpers
 * @{
 */

/**
 * @brief Calculates then returns the checksum of the NMEA message.
 * 
 * @param cmd Pointer to the NMEA message.
 * @return uint8_t The 1 byte checksum of the NMEA message.
 */
static ERR_te neo6_calc_checksum(char *msg, uint32_t msg_len, uint8_t *checksum_o) {
	uint8_t xor_val = 0;
	uint32_t char_count = 0;

	while(*msg != '*' && char_count < msg_len) {
		if(*msg == '$') {
			msg++;
			continue;
		}

		xor_val ^= *msg;

		msg++;
		char_count++;

		if(char_count == msg_len) {
			return ERR_UNKNOWN;
		}
	}
	*checksum_o = xor_val;

	return ERR_OK;
}

/**
 * @brief Calculates the checksum of a received message and processes it by message type.
 * 
 * @param[in] msg The message received.
 * @param[out] msg_len The length of the received message. 
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_msg(uint8_t *msg, uint32_t msg_len) {
	ERR_te err;
	
	uint8_t checksum_recv = ascii_hex_to_byte(msg[msg_len - 4], msg[msg_len - 3]);
	
	uint8_t checksum_calc = 0;
	err = neo6_calc_checksum((char*)msg, msg_len, &checksum_calc);
	if(err != ERR_OK) {
		return err;
	}

	char *tokens[CONFIG_NEO6_MAX_TOKENS];
	uint16_t num_tokens = 0;

	if(checksum_recv == checksum_calc) {
		str_tokenize((char*)msg, ",", CONFIG_NEO6_MAX_TOKENS, tokens, &num_tokens);

		if(str_cmp("$GPRMC", tokens[0]) == true) {
			neo6_process_rmc(tokens);
		}
		else if(str_cmp("$GPVTG", tokens[0]) == true) {
			neo6_process_vtg(tokens);
		}
		else if(str_cmp("$GPGGA", tokens[0]) == true) {
			neo6_process_gga(tokens);
		}
		else if(str_cmp("$GPGSA", tokens[0]) == true) {
			neo6_process_gsa(tokens);
		}
		else if(str_cmp("$GPGSV", tokens[0]) == true) {
			neo6_process_gsv(tokens);
		}
	}
	else {
		// Invalid checksum
		return ERR_DATA_ACQUISITION_FAILURE;
	}

	return ERR_OK;
}

/**
 * @brief Processes a RMC message.
 * 
 * @param[in] tokens Tokenized RMC message.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_rmc(char **tokens) {
	if(str_cmp(tokens[2], "A") == false) {
		str_cpy(internal_state.neo6_info.time, "No data", 
			get_str_len("No data") + 1);

		str_cpy(internal_state.neo6_info.date, "No data", 
			get_str_len("No data") + 1);

		str_cpy(internal_state.neo6_info.lon, "No data", 
			get_str_len("No data") + 1);

		str_cpy(internal_state.neo6_info.lat, "No data", 
			get_str_len("No data") + 1);

		return ERR_UNKNOWN;
	}

	// UTC
	uint8_t time_len = 6; // hhmmss
	int8_t real_counter = 0;
	for(uint8_t j = 0; j < time_len; j++) {
		internal_state.neo6_info.time[real_counter++] = tokens[1][j];

		if((real_counter + 1) % 3 == 0 && real_counter <= 5) {
			internal_state.neo6_info.time[real_counter++] = ':';
		}
	}
	internal_state.neo6_info.time[real_counter++] = ' ';
	internal_state.neo6_info.time[real_counter++] = 'U';
	internal_state.neo6_info.time[real_counter++] = 'T';
	internal_state.neo6_info.time[real_counter++] = 'C';
	internal_state.neo6_info.time[real_counter++] = '\0';

	// Latitude
	uint8_t lat_txt_len = get_str_len(tokens[3]);
	for(uint8_t j = 0; j < lat_txt_len; j++) {
		internal_state.neo6_info.lat[j] = tokens[3][j];
	}
	// Add hemisphere indicators
	internal_state.neo6_info.lat[lat_txt_len] = tokens[4][0];
	internal_state.neo6_info.lat[lat_txt_len + 1] = '\0';

	// Longitude
	uint8_t lon_txt_len = get_str_len(tokens[5]);
	for(uint8_t j = 0; j < lon_txt_len; j++) {
		internal_state.neo6_info.lon[j] = tokens[5][j];
	}
	// Add hemisphere indicators
	internal_state.neo6_info.lon[lon_txt_len] = tokens[6][0];
	internal_state.neo6_info.lon[lon_txt_len + 1] = '\0';

	// Date
	internal_state.neo6_info.date[0] = tokens[9][0];
	internal_state.neo6_info.date[1] = tokens[9][1];
	internal_state.neo6_info.date[2] = '/';
	internal_state.neo6_info.date[3] = tokens[9][2];
	internal_state.neo6_info.date[4] = tokens[9][3];
	internal_state.neo6_info.date[5] = '/';
	internal_state.neo6_info.date[6] = '2';
	internal_state.neo6_info.date[7] = '0';
	internal_state.neo6_info.date[8] = tokens[9][4];
	internal_state.neo6_info.date[9] = tokens[9][5];
	internal_state.neo6_info.date[10] = '\0';

	return ERR_OK;
}

/**
 * @brief Processes a VTG message.
 * 
 * @param[in] tokens Tokenized VTG message.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_vtg(char **tokens) {
	// Movement direction (relative to true north)
	uint8_t track_made_good_len = get_str_len(tokens[VTG_TMG_T_DEGREE_POS]);
	if(track_made_good_len == 0) {
		str_cpy(internal_state.neo6_info.mov_dir, "No data", 
			get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.mov_dir, &tokens[VTG_TMG_T_DEGREE_POS][0], 
			track_made_good_len + 1);
	}

	// Movement speed (kph)
	uint8_t kph_len = get_str_len(tokens[VTG_SPEED_OVER_GROUND_POS]);
	if(kph_len == 0) {
		str_cpy(internal_state.neo6_info.mov_speed, "No data", 
			get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.mov_speed, &tokens[VTG_SPEED_OVER_GROUND_POS][0], 
			kph_len + 1);
		internal_state.neo6_info.mov_speed[kph_len++] = ' ';
		internal_state.neo6_info.mov_speed[kph_len++] = 'k';
		internal_state.neo6_info.mov_speed[kph_len++] = 'p';
		internal_state.neo6_info.mov_speed[kph_len++] = 'h';
		internal_state.neo6_info.mov_speed[kph_len++] = '\0';
	}

	return ERR_OK;
}

/**
 * @brief Processes a GGA message.
 * 
 * @param[in] tokens Tokenized GGA message.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_gga(char **tokens) {
	// Quality of the fix
	uint8_t fix_status_len = get_str_len(tokens[GGA_QUALITY_FIX_POS]);
	if(fix_status_len == 0) {
		str_cpy(internal_state.neo6_info.fix_status, "No data",
				get_str_len("No data") + 1);
	}
	else if(str_cmp(tokens[GGA_QUALITY_FIX_POS], "0") == true) {
		char msg[] = "No valid fix";
		str_cpy(internal_state.neo6_info.fix_status, msg, get_str_len(msg) + 1);
	}
	else if(str_cmp(tokens[GGA_QUALITY_FIX_POS], "1") == true) {
		char msg[] = "GPS fix";
		str_cpy(internal_state.neo6_info.fix_status, msg, get_str_len(msg) + 1);
	}
	else if(str_cmp(tokens[GGA_QUALITY_FIX_POS], "2") == true) {
		char msg[] = "DGPS fix";
		str_cpy(internal_state.neo6_info.fix_status, msg, get_str_len(msg) + 1);
	}
	else {
		char msg[] = "?";
		str_cpy(internal_state.neo6_info.fix_status, msg, get_str_len(msg) + 1);
	}

	// Number of satellites
	uint8_t num_sats_used_len = get_str_len(tokens[GGA_NUM_SATS_POS]);
	if(num_sats_used_len == 0) {
		str_cpy(internal_state.neo6_info.num_sats_used, "No data",
				get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.num_sats_used, tokens[GGA_NUM_SATS_POS], 
			get_str_len(tokens[GGA_NUM_SATS_POS]) + 1);
	}

	// Orthometric height
	uint8_t ort_height_len = get_str_len(tokens[GGA_ORTH_HEIGHT_POS]);
	if(ort_height_len == 0) {
		str_cpy(internal_state.neo6_info.ort_height, "No data",
				get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.ort_height, tokens[GGA_ORTH_HEIGHT_POS],
			ort_height_len + 1);
		internal_state.neo6_info.ort_height[ort_height_len] = ' ';
		internal_state.neo6_info.ort_height[ort_height_len + 1] = tokens[GGA_ORTH_HEIGHT_UNIT_POS][0];
		internal_state.neo6_info.ort_height[ort_height_len + 2] = '\0';
	}

	// Geoid separation
	uint8_t geoid_sep_len = get_str_len(tokens[GGA_GEOID_SEP_POS]);
	if(geoid_sep_len == 0) {
		str_cpy(internal_state.neo6_info.geoid_sep, "No data",
				get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.geoid_sep, tokens[GGA_GEOID_SEP_POS],
				geoid_sep_len + 1);
		internal_state.neo6_info.geoid_sep[geoid_sep_len] = ' ';
		internal_state.neo6_info.geoid_sep[geoid_sep_len + 1] = tokens[GGA_GEOID_SEP_UNIT_POS][0];
		internal_state.neo6_info.geoid_sep[geoid_sep_len + 2] = '\0';
	}

	return ERR_OK;
}

/**
 * @brief Processes a GSA message.
 * 
 * @param[in] tokens Tokenized GSA message.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_gsa(char **tokens) {
	uint8_t msg_part_len = get_str_len(tokens[GSA_FIX_TYPE_POS]);
	if(msg_part_len == 0) {
		str_cpy(internal_state.neo6_info.fix_type, "No data",
		get_str_len("No data") + 1);
	}
	else {
		if(str_cmp(tokens[GSA_FIX_TYPE_POS], "1") == true) {
			str_cpy(internal_state.neo6_info.fix_type, "Not available",
			get_str_len("Not available") + 1);
		}
		else if(str_cmp(tokens[GSA_FIX_TYPE_POS], "2") == true) {
			str_cpy(internal_state.neo6_info.fix_type, "2D",
			get_str_len("2D") + 1);
		}
		else if(str_cmp(tokens[GSA_FIX_TYPE_POS], "3") == true) {
			str_cpy(internal_state.neo6_info.fix_type, "3D",
			get_str_len("3D") + 1);
		}
	}

	msg_part_len = get_str_len(tokens[GSA_PDOP_POS]);
	if(msg_part_len == 0) {
		str_cpy(internal_state.neo6_info.pdop, "No data",
		get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.pdop, tokens[GSA_PDOP_POS],
		msg_part_len + 1);
	}

	msg_part_len = get_str_len(tokens[GSA_HDOP_POS]);
	if(msg_part_len == 0) {
		str_cpy(internal_state.neo6_info.hdop, "No data",
		get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.hdop, tokens[GSA_HDOP_POS],
		msg_part_len + 1);
	}

	msg_part_len = get_str_len(tokens[GSA_VDOP_POS]) - 2;
	if(msg_part_len == 0) {
		str_cpy(internal_state.neo6_info.vdop, "No data",
		get_str_len("No data") + 1);
	}
	else {
		str_cpy(internal_state.neo6_info.vdop, tokens[GSA_VDOP_POS],
		msg_part_len);
		internal_state.neo6_info.vdop[msg_part_len] = '\0';
	}

	return ERR_OK;
}

/**
 * @brief Processes a GSV message.
 * 
 * @param[in] tokens Tokenized GSV message.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_process_gsv(char **tokens) {
	// Total number of satellites visible
	uint8_t msg_part_len = get_str_len(tokens[GSV_NUM_SATS_VISIBLE_POS]);
	if(msg_part_len == 0) {
		str_cpy(internal_state.neo6_info.num_sats_all, "No data",
		get_str_len("No data") + 1);		
	}
	else {
		str_cpy(internal_state.neo6_info.num_sats_all, tokens[GSV_NUM_SATS_VISIBLE_POS],
		msg_part_len + 1);
	}

	return ERR_OK;
}

/**
 * @brief Command handler routine for dumpnmea command.
 * 
 * @param[in] argc Number of arguments.
 * @param[in] argv Arguments.
 * @return ERR_te Error generated during execution.
 */
static ERR_te neo6_dumpnmea_handler(uint32_t argc, char **argv) {
	if(argc != 3) {
		LOG_ERROR(
			internal_state.subsys,
			internal_state.log_level,
			"neo6_dumpnmea_handler: invalid arguments"
		);

		return ERR_INVALID_ARGUMENT;
	}

	if(str_cmp(argv[2], "true") == true) {
		internal_state.dump_raw_nmea = true;
	}
	else if(str_cmp(argv[2], "false") == true) {
		internal_state.dump_raw_nmea = false;
	}
	else {
	LOG_ERROR(
		internal_state.subsys,
		internal_state.log_level,
		"neo6_dumpnmea_handler: invalid arguments"
	);

		return ERR_INVALID_ARGUMENT;
	}

	return ERR_OK;
}

void usart6_irq_data_recv_callback(uint8_t data) {
	cbuf_write(&internal_state.usart_data_recv_cbuf, &data, 1);
}

/** @} */