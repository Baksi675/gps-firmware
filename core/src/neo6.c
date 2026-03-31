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
#include "init.h"
#include "log.h"
#include "modules.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "cbuf.h"
#include "configuration.h"

/* ---- VTG sentence field positions ---- */
#define VTG_TMG_T_DEGREE_POS        1   /**< Track made good in degrees relative to true north. */
#define VTG_TMG_T_POS               2   /**< True north indicator. */
#define VTG_TMG_M_DEGREE_POS        3   /**< Track made good in degrees relative to magnetic north. */
#define VTG_TMG_M_POS               4   /**< Magnetic north indicator. */
#define VTG_SPEED_KNOTS_POS         5   /**< Speed over ground in knots. */
#define VTG_SPEED_N_POS             6   /**< Knots indicator. */
#define VTG_SPEED_OVER_GROUND_POS   7   /**< Speed over ground in kph. */
#define VTG_SPEED_OVER_GROUND_K_POS 8   /**< Kph indicator. */
#define VTG_MODE_INDICATOR_POS      9   /**< Mode indicator. */

/* ---- GGA sentence field positions ---- */
#define GGA_UTC_POS                 1   /**< UTC time. */
#define GGA_LAT_POS                 2   /**< Latitude. */
#define GGA_LAT_DIR_POS             3   /**< Latitude hemisphere direction. */
#define GGA_LON_POS                 4   /**< Longitude. */
#define GGA_LON_DIR_POS             5   /**< Longitude hemisphere direction. */
#define GGA_QUALITY_FIX_POS         6   /**< Fix quality indicator. */
#define GGA_NUM_SATS_POS            7   /**< Number of satellites used for the fix. */
#define GGA_HDOP_POS                8   /**< Horizontal dilution of precision. */
#define GGA_ORTH_HEIGHT_POS         9   /**< Orthometric height above mean sea level. */
#define GGA_ORTH_HEIGHT_UNIT_POS    10  /**< Unit of orthometric height. */
#define GGA_GEOID_SEP_POS           11  /**< Geoid separation. */
#define GGA_GEOID_SEP_UNIT_POS      12  /**< Unit of geoid separation. */
#define GGA_DATA_REC_AGE_POS        13  /**< Age of differential GPS data record. */
#define GGA_REF_STAT_ID_POS         14  /**< Reference station ID. */

/* ---- GSA sentence field positions ---- */
#define GSA_MODE_POS                1   /**< Selection mode: automatic or manual. */
#define GSA_FIX_TYPE_POS            2   /**< Fix type: 1 = none, 2 = 2D, 3 = 3D. */
#define GSA_PRN_NUM_POS             3   /**< SV pseudorandom noise code number. */
#define GSA_PDOP_POS                15  /**< Position dilution of precision. */
#define GSA_HDOP_POS                16  /**< Horizontal dilution of precision. */
#define GSA_VDOP_POS                17  /**< Vertical dilution of precision. */

/* ---- GSV sentence field positions ---- */
#define GSV_NUM_OF_MESSAGES_POS     1   /**< Total number of GSV messages in this cycle. */
#define GSV_NUM_OF_THIS_MESSAGE_POS 2   /**< Message number within the cycle. */
#define GSV_NUM_SATS_VISIBLE_POS    3   /**< Total number of satellites in view. */
#define GSV_SV_PRN_NUM_POS          4   /**< Satellite PRN number. */
#define GSV_ELEVATION_POS           5   /**< Satellite elevation in degrees (max 90). */
#define GSV_AZIMUTH_POS             6   /**< Azimuth in degrees from true north (000–359). */
#define GSV_SNR_POS                 7   /**< Signal-to-noise ratio in dB (null when not tracking). */

/**
 * @brief Backing memory for the USART receive circular buffer.
 *
 * @details Written by the USART ISR callback and drained by @ref neo6_run.
 */
static uint8_t usart_data_recv_cbuf_mem[128];

/**
 * @brief Backing memory for the NMEA sentence accumulation buffer.
 *
 * @details Accumulates bytes from the USART buffer until a complete
 * NMEA sentence (terminated by '\\n') is available for processing.
 */
static uint8_t nmea_cbuf_mem[128];

/**
 * @brief Internal structure representing the NEO-6 hardware instance.
 *
 * @details
 * Mirrors the @ref NEO6_CFG_ts fields and adds an initialization guard.
 * Only one instance is supported; it is embedded directly in
 * @ref internal_state_s rather than allocated from a pool.
 */
struct neo6_handle_s {
    /** Pointer to the USART peripheral used for GPS communication. */
    USART_REGDEF_ts *usart_instance;

    /** Configured baud rate of the USART peripheral. */
    USART_BAUD_RATE_te usart_baud_rate;

    /** GPIO port of the USART RX pin. */
    GPIO_REGDEF_ts *rx_gpio_port;

    /** GPIO pin number of the USART RX pin. */
    GPIO_PIN_te rx_gpio_pin;

    /** GPIO port of the USART TX pin. */
    GPIO_REGDEF_ts *tx_gpio_port;

    /** GPIO pin number of the USART TX pin. */
    GPIO_PIN_te tx_gpio_pin;

    /** Alternate function mapping for both RX and TX GPIO pins. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;

    /** True after @ref neo6_init_handle has completed successfully. */
    bool initialized;
};

/**
 * @brief Internal state of the NEO-6 subsystem.
 *
 * @details
 * Holds the single hardware handle, both circular buffers, the parsed
 * GPS info structure, and the subsystem lifecycle flags.
 */
struct internal_state_s {
    /** The single NEO-6 hardware handle. */
    NEO6_HANDLE_ts neo6_handle;

    /** Circular buffer receiving raw bytes from the USART ISR. */
    CBUF_HANDLE_ts usart_data_recv_cbuf;

    /** Circular buffer accumulating bytes until a complete NMEA sentence is available. */
    CBUF_HANDLE_ts nmea_cbuf;

    /** Parsed GPS data, updated on each successful NMEA sentence parse. */
    NEO6_INFO_ts neo6_info;

    /** Module identifier used for log messages. */
    MODULES_te subsys;

    /** Active log level for this subsystem. */
    LOG_LEVEL_te log_level;

    /** When true, raw NMEA sentences are logged via the log subsystem. */
    bool dump_raw_nmea;

    /** True after @ref neo6_init_subsys has completed successfully. */
    bool initialized;

    /** True after @ref neo6_start_subsys has been called. */
    bool started;
};

/** @brief Singleton instance of the NEO-6 subsystem internal state. */
static struct internal_state_s internal_state;

/* ---- Forward declarations for internal helpers ---- */
static ERR_te neo6_calc_checksum(char *msg, uint32_t msg_len, uint8_t *checksum_o);
static ERR_te neo6_process_msg(uint8_t *msg, uint32_t msg_len);
static ERR_te neo6_process_rmc(char **tokens);
static ERR_te neo6_process_vtg(char **tokens);
static ERR_te neo6_process_gga(char **tokens);
static ERR_te neo6_process_gsa(char **tokens);
static ERR_te neo6_process_gsv(char **tokens);

/* ---- Forward declaration for command handler ---- */
static ERR_te neo6_dumpnmea_handler(uint32_t argc, char **argv);

/**
 * @brief Table of CLI commands registered by the NEO-6 subsystem.
 *
 * @details
 * Registered with the command subsystem via @ref neo6_cmd_client_info
 * during @ref neo6_init_subsys.
 */
static CMD_INFO_ts neo6_cmds[] = {
    {
        .name = "dumpnmea",
        .help = "Dumps raw NMEA messages, usage: neo6 dumpnmea <true|false>",
        .handler = neo6_dumpnmea_handler
    },
};

/**
 * @brief Registration descriptor passed to the command subsystem.
 *
 * @details
 * Bundles the command table, its size, the subsystem name prefix used
 * on the CLI, and a pointer to the runtime log-level variable.
 */
static CMD_CLIENT_INFO_ts neo6_cmd_client_info = {
    .cmds_ptr = neo6_cmds,
    .num_cmds = sizeof(neo6_cmds) / sizeof(neo6_cmds[0]),
    .name = "neo6",
    .log_level_ptr = &internal_state.log_level
};

/**
 * @defgroup neo6_public_apis NEO-6 Public APIs
 * @{
 */

/** @brief Initializes the NEO-6 subsystem. @see neo6_init_subsys */
ERR_te neo6_init_subsys(void) {
    ERR_te err;

    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
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

    init_log();

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

/** @brief Deinitializes the NEO-6 subsystem. @see neo6_deinit_subsys */
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

/** @brief Starts the NEO-6 subsystem. @see neo6_start_subsys */
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

/** @brief Stops the NEO-6 subsystem. @see neo6_stop_subsys */
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

/** @brief Initializes the NEO-6 hardware handle. @see neo6_init_handle */
ERR_te neo6_init_handle(NEO6_CFG_ts *neo6_cfg, NEO6_HANDLE_ts **neo6_handle_o) {
    if(internal_state.neo6_handle.initialized == true) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "neo6_init_handle: handle already initialized"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    if(neo6_cfg->usart_instance == (void*)0) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "neo6_init_handle: No USART peripheral given"
        );

        return ERR_INITIALIZATION_FAILURE;
    }
    else if(neo6_cfg->rx_gpio_port == (void*)0 || neo6_cfg->tx_gpio_port == (void*)0) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "neo6_init_handle: No GPIO peripheral given"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    internal_state.neo6_handle.usart_instance = neo6_cfg->usart_instance;
    internal_state.neo6_handle.usart_baud_rate = neo6_cfg->usart_baud_rate;
    internal_state.neo6_handle.rx_gpio_port = neo6_cfg->rx_gpio_port;
    internal_state.neo6_handle.tx_gpio_port = neo6_cfg->tx_gpio_port;
    internal_state.neo6_handle.rx_gpio_pin = neo6_cfg->rx_gpio_pin;
    internal_state.neo6_handle.tx_gpio_pin = neo6_cfg->tx_gpio_pin;
    internal_state.neo6_handle.gpio_alternate_function = neo6_cfg->gpio_alternate_function;

    USART_CFG_ts neo6_usart = { 0 };
    neo6_usart.instance = neo6_cfg->usart_instance;
    neo6_usart.baud_rate = neo6_cfg->usart_baud_rate;
    neo6_usart.interrupt_en = USART_INTERRUPT_EN_TRUE;
    usart_init(&neo6_usart);
    usart_set_reception(neo6_usart.instance, ENABLE);

    GPIO_CFG_ts neo6_rx = { 0 };
    neo6_rx.port = neo6_cfg->rx_gpio_port;
    neo6_rx.pin = neo6_cfg->rx_gpio_pin;
    neo6_rx.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    neo6_rx.alternate_function = neo6_cfg->gpio_alternate_function;
    gpio_init(&neo6_rx);

    GPIO_CFG_ts neo6_tx = { 0 };
    neo6_tx.port = neo6_cfg->tx_gpio_port;
    neo6_tx.pin = neo6_cfg->tx_gpio_pin;
    neo6_tx.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    neo6_tx.alternate_function = neo6_cfg->gpio_alternate_function;
    gpio_init(&neo6_tx);

    internal_state.neo6_handle.initialized = true;

    return ERR_OK;
}

/** @brief Processes received NMEA data. @see neo6_run */
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

/** @brief Returns a pointer to the internal GPS data structure. @see neo6_get_info */
ERR_te neo6_get_info(NEO6_INFO_ts **neo6_info_o) {
    *neo6_info_o = &internal_state.neo6_info;

    return ERR_OK;
}

/** @} */

/**
 * @defgroup neo6_internal_helpers NEO-6 Internal Helpers
 * @{
 */

/**
 * @brief Computes the NMEA checksum of a sentence by XORing all bytes between '$' and '*'.
 *
 * @details
 * Iterates through @p msg, skipping the leading '$', XORing each byte
 * until '*' is encountered. The result is the standard NMEA XOR checksum.
 *
 * @param[in]  msg         Pointer to the null-terminated NMEA sentence string.
 * @param[in]  msg_len     Length of the message in bytes.
 * @param[out] checksum_o  Pointer to a variable that will receive the computed checksum.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if '*' is not found before @p msg_len bytes are consumed
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
 * @brief Verifies the checksum of a received NMEA sentence and dispatches it to the appropriate parser.
 *
 * @details
 * Extracts the two-character hex checksum from the end of the sentence,
 * computes the expected checksum via @ref neo6_calc_checksum, and compares
 * them. On a match, tokenizes the sentence by comma and dispatches to
 * @ref neo6_process_rmc, @ref neo6_process_vtg, @ref neo6_process_gga,
 * @ref neo6_process_gsa, or @ref neo6_process_gsv based on the sentence ID.
 * Unrecognized sentence types are silently ignored.
 *
 * @param[in] msg     Pointer to the raw NMEA sentence bytes.
 * @param[in] msg_len Length of the sentence in bytes.
 *
 * @return
 * - ERR_OK on success or for unrecognized sentence types
 * - ERR_DATA_ACQUISITION_FAILURE if the checksum does not match
 * - Propagated error from @ref neo6_calc_checksum on failure
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
        return ERR_DATA_ACQUISITION_FAILURE;
    }

    return ERR_OK;
}

/**
 * @brief Parses an RMC sentence and updates time, date, latitude, and longitude.
 *
 * @details
 * If the validity flag (token 2) is not `"A"` (active), all four fields
 * are set to `"No data"` and the function returns early.
 * Otherwise: time is formatted as `"HH:MM:SS UTC"`, date as `"DD/MM/YYYY"`,
 * and latitude/longitude have their hemisphere indicator appended.
 *
 * @param[in] tokens Tokenized RMC sentence (comma-separated fields).
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the validity flag is not `"A"`
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

    // UTC — format hhmmss → HH:MM:SS UTC
    uint8_t time_len = 6;
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
    internal_state.neo6_info.lat[lat_txt_len] = tokens[4][0];
    internal_state.neo6_info.lat[lat_txt_len + 1] = '\0';

    // Longitude
    uint8_t lon_txt_len = get_str_len(tokens[5]);
    for(uint8_t j = 0; j < lon_txt_len; j++) {
        internal_state.neo6_info.lon[j] = tokens[5][j];
    }
    internal_state.neo6_info.lon[lon_txt_len] = tokens[6][0];
    internal_state.neo6_info.lon[lon_txt_len + 1] = '\0';

    // Date — format ddmmyy → DD/MM/20YY
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
 * @brief Parses a VTG sentence and updates movement direction and speed.
 *
 * @details
 * Direction is taken from the true-north track field. Speed is taken from
 * the speed-over-ground field and has `" kph"` appended. Empty fields
 * result in `"No data"` being written instead.
 *
 * @param[in] tokens Tokenized VTG sentence.
 *
 * @return
 * - ERR_OK always
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
 * @brief Parses a GGA sentence and updates fix status, satellites used, orthometric height, and geoid separation.
 *
 * @details
 * Fix quality is mapped to a human-readable string: `"No valid fix"` (0),
 * `"GPS fix"` (1), `"DGPS fix"` (2). Height and separation values have
 * their unit character appended. Empty fields result in `"No data"`.
 *
 * @param[in] tokens Tokenized GGA sentence.
 *
 * @return
 * - ERR_OK always
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

    // Number of satellites used
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
 * @brief Parses a GSA sentence and updates fix type, PDOP, HDOP, and VDOP.
 *
 * @details
 * Fix type is mapped to a human-readable string: `"Not available"` (1),
 * `"2D"` (2), `"3D"` (3). VDOP strips the trailing NMEA checksum suffix
 * (`*XX`) before storing. Empty fields result in `"No data"`.
 *
 * @param[in] tokens Tokenized GSA sentence.
 *
 * @return
 * - ERR_OK always
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

    // VDOP: subtract 2 to strip the trailing checksum suffix (*XX)
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
 * @brief Parses a GSV sentence and updates the total satellite count.
 *
 * @details
 * Only the total number of satellites in view is extracted. The per-satellite
 * detail fields (PRN, elevation, azimuth, SNR) are not currently stored.
 * Empty fields result in `"No data"`.
 *
 * @param[in] tokens Tokenized GSV sentence.
 *
 * @return
 * - ERR_OK always
 */
static ERR_te neo6_process_gsv(char **tokens) {
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

/** @} */

/**
 * @defgroup neo6_command_handlers NEO-6 Command Handlers
 * @{
 */

/**
 * @brief CLI handler for the "dumpnmea" command. Enables or disables raw NMEA logging.
 *
 * @details
 * Expected invocation: `neo6 dumpnmea <true|false>`
 *
 * When enabled, each complete NMEA sentence is logged via the log subsystem
 * before being parsed, useful for debugging raw GPS output.
 *
 * @param[in] argc Argument count. Must be exactly 3.
 * @param[in] argv Argument list: argv[0] = "neo6", argv[1] = "dumpnmea",
 *                 argv[2] = "true" or "false".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 3 or argv[2] is not "true" or "false"
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

/** @} */

/**
 * @brief USART RXNE interrupt callback. Writes the received byte into the USART receive buffer.
 *
 * @details
 * Called automatically by the USART6 interrupt handler on each received byte.
 * Writes @p data directly into the internal USART circular buffer for later
 * processing by @ref neo6_run.
 *
 * @note This function must not be called directly from application code.
 *
 * @param[in] data The byte received from the USART RX register.
 */
void usart6_irq_data_recv_callback(uint8_t data) {
    cbuf_write(&internal_state.usart_data_recv_cbuf, &data, 1);
}