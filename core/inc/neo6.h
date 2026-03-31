/**
 * @file neo6.h
 * @author github.com/Baksi675
 * @brief NEO-6M GPS module public API.
 *
 * @details
 * This module provides NMEA sentence parsing for the u-blox NEO-6M GPS
 * receiver over USART with interrupt-driven reception. Received bytes are
 * accumulated in a circular buffer and processed sentence-by-sentence by
 * @ref neo6_run.
 *
 * The following NMEA sentence types are parsed:
 * - **GGA** — fix quality, satellites used, orthometric height, geoid separation
 * - **GSA** — fix type, PDOP, HDOP, VDOP
 * - **GSV** — total satellites in view
 * - **RMC** — latitude, longitude, UTC time, date
 * - **VTG** — movement speed (kph), movement direction (degrees true north)
 *
 * Parsed fields are stored in a single @ref NEO6_INFO_ts structure accessible
 * via @ref neo6_get_info. All fields are null-terminated strings; unavailable
 * or invalid data is indicated by the string `"No data"`.
 *
 * Typical usage:
 * - Initialize the subsystem using @ref neo6_init_subsys
 * - Configure a handle using @ref NEO6_CFG_ts and register it with @ref neo6_init_handle
 * - Start the subsystem using @ref neo6_start_subsys
 * - Call @ref neo6_run periodically from the main loop to process received data
 * - Retrieve parsed GPS data via @ref neo6_get_info
 *
 * @version 0.1
 * @date 2026-02-03
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup neo6_module NEO-6 GPS Module
 * @brief NMEA sentence parser for the u-blox NEO-6M GPS receiver.
 * @{
 */

#ifndef NEO6_H__
#define NEO6_H__

#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "err.h"

/**
 * @defgroup neo6_types NEO-6 Types
 * @ingroup neo6_module
 * @brief Data types used by the NEO-6 module.
 * @{
 */

/**
 * @brief NMEA sentence type identifiers.
 *
 * @details
 * Identifies the type of a parsed NMEA sentence. Not all types are
 * currently processed; see the module description for the supported set.
 */
typedef enum {
    GPS_MESSAGE_GGA, /**< Global Positioning System Fix Data. */
    GPS_MESSAGE_GLL, /**< Geographic Position, Latitude/Longitude. */
    GPS_MESSAGE_GSA, /**< GNSS DOP and Active Satellites. */
    GPS_MESSAGE_GSV, /**< GNSS Satellites in View. */
    GPS_MESSAGE_RMC, /**< Recommended Minimum Specific GNSS Data. */
    GPS_MESSAGE_VTG, /**< Course Over Ground and Ground Speed. */
    GPS_MESSAGE_TXT, /**< Text Transmission. */
} NEO6_MESSAGES_te;

/**
 * @brief Holds all GPS data fields parsed from incoming NMEA sentences.
 *
 * @details
 * Updated in place each time a relevant NMEA sentence is successfully
 * parsed by @ref neo6_run. All fields are null-terminated strings.
 * Fields that are unavailable or invalid contain the string `"No data"`.
 *
 * Access this structure via @ref neo6_get_info.
 */
typedef struct {
    char lat[16];           /**< Latitude with hemisphere indicator (e.g. `"4807.038N"`). From RMC. */
    char lon[16];           /**< Longitude with hemisphere indicator (e.g. `"01131.000E"`). From RMC. */
    char ort_height[16];    /**< Orthometric height above mean sea level with unit (e.g. `"545.4 M"`). From GGA. */
    char geoid_sep[16];     /**< Geoid separation with unit (e.g. `"46.9 M"`). From GGA. */
    char time[16];          /**< UTC time formatted as `"HH:MM:SS UTC"`. From RMC. */
    char date[16];          /**< Date formatted as `"DD/MM/YYYY"`. From RMC. */
    char mov_dir[16];       /**< Movement direction in degrees relative to true north. From VTG. */
    char mov_speed[16];     /**< Movement speed with unit (e.g. `"12.3 kph"`). From VTG. */
    char num_sats_used[4];  /**< Number of satellites used in the current fix. From GGA. */
    char num_sats_all[4];   /**< Total number of satellites currently in view. From GSV. */
    char fix_type[16];      /**< Fix type: `"Not available"`, `"2D"`, or `"3D"`. From GSA. */
    char pdop[16];          /**< Position dilution of precision. From GSA. */
    char hdop[16];          /**< Horizontal dilution of precision. From GSA. */
    char vdop[16];          /**< Vertical dilution of precision. From GSA. */
    char fix_status[16];    /**< Fix quality: `"No valid fix"`, `"GPS fix"`, or `"DGPS fix"`. From GGA. */
} NEO6_INFO_ts;

/**
 * @brief Configuration structure for initializing a NEO-6 handle.
 *
 * @details
 * Passed to @ref neo6_init_handle to configure the USART peripheral
 * and the RX/TX GPIO pins connected to the NEO-6M module.
 */
typedef struct {
    /** Pointer to the USART peripheral instance connected to the NEO-6M. */
    USART_REGDEF_ts *usart_instance;

    /** Baud rate for the USART peripheral (typically 9600 for NEO-6M). */
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
} NEO6_CFG_ts;

/**
 * @brief Opaque handle representing the NEO-6 hardware instance.
 *
 * @details
 * Returned by @ref neo6_init_handle. The internal structure is hidden
 * and must not be accessed directly.
 *
 * @note The NEO-6 subsystem supports only a single handle instance.
 */
typedef struct neo6_handle_s NEO6_HANDLE_ts;

/** @} */

/**
 * @defgroup neo6_api NEO-6 Public API
 * @ingroup neo6_module
 * @brief Public functions to interact with the NEO-6 GPS subsystem.
 * @{
 */

/**
 * @brief Initializes the NEO-6 subsystem.
 *
 * @details
 * Resets the internal state, initializes the USART receive circular buffers,
 * initializes the logging dependency, and registers the CLI commands.
 *
 * Must be called before any other NEO-6 API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 */
ERR_te neo6_init_subsys(void);

/**
 * @brief Deinitializes the NEO-6 subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 * The subsystem must be stopped before calling this function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized or still running
 * - Propagated error from @ref cmd_deregister on failure
 */
ERR_te neo6_deinit_subsys(void);

/**
 * @brief Starts the NEO-6 subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 */
ERR_te neo6_start_subsys(void);

/**
 * @brief Stops the NEO-6 subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 */
ERR_te neo6_stop_subsys(void);

/**
 * @brief Initializes the NEO-6 hardware handle.
 *
 * @details
 * Configures the USART peripheral with interrupt-driven reception and
 * initializes both the RX and TX GPIO pins in alternate function mode.
 *
 * @note Only one handle instance is supported. Calling this function
 *       a second time without deinitialization returns an error.
 *
 * @param[in]  neo6_cfg      Pointer to the NEO-6 configuration structure.
 * @param[out] neo6_handle_o Pointer to a handle pointer that will be set
 *                           to the initialized instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if a handle is already initialized, or if
 *   the USART or GPIO pointers in @p neo6_cfg are NULL
 */
ERR_te neo6_init_handle(NEO6_CFG_ts *neo6_cfg, NEO6_HANDLE_ts **neo6_handle_o);

/**
 * @brief Processes received NMEA data. Must be called periodically.
 *
 * @details
 * Drains bytes from the USART receive circular buffer into the NMEA
 * accumulation buffer. Each time a newline character is detected, the
 * complete sentence is extracted, its checksum is verified, and it is
 * dispatched to the appropriate parser (RMC, VTG, GGA, GSA, or GSV).
 *
 * Parsed values are written directly into the internal @ref NEO6_INFO_ts
 * structure and are accessible via @ref neo6_get_info.
 *
 * @note Known limitation: if a received chunk contains the end of one
 *       sentence and the start of the next, the second sentence is lost.
 *       This does not currently cause observable issues.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DATA_ACQUISITION_FAILURE if a sentence fails its checksum verification
 * - ERR_UNKNOWN if a checksum cannot be computed (malformed sentence)
 */
ERR_te neo6_run(void);

/**
 * @brief Returns a pointer to the internal GPS data structure.
 *
 * @details
 * The returned pointer is valid for the lifetime of the subsystem.
 * The structure is updated in place on each successful NMEA parse.
 * The caller must not free or modify the returned pointer.
 *
 * @param[out] neo6_info_o Pointer to a pointer that will be set to the
 *                         internal @ref NEO6_INFO_ts structure.
 *
 * @return
 * - ERR_OK always
 */
ERR_te neo6_get_info(NEO6_INFO_ts **neo6_info_o);

/** @} */

#endif

/** @} */