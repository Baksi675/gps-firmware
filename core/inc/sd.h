/**
 * @file sd.h
 * @author github.com/Baksi675
 * @brief SD card driver public API.
 *
 * @details
 * This module provides an SPI-based SD card driver supporting SDSC and SDHC
 * cards in SPI mode. It handles card initialization, type detection, CSD
 * register parsing, and sector-level read/write operations. The driver is
 * designed to be used as the low-level backend for a FatFs diskio layer.
 *
 * Card initialization in @ref sd_init_handle performs the full SPI-mode
 * power-up sequence:
 * -# Sends at least 74 dummy clock cycles with CS high
 * -# Issues CMD0 to enter idle state
 * -# Issues CMD8 to detect SD Ver.2 vs Ver.1 / MMC
 * -# Issues ACMD41 or CMD1 to complete initialization
 * -# Reads the OCR and CSD registers to determine capacity and addressing mode
 * -# Ramps the SPI clock to maximum speed after initialization
 *
 * Typical usage:
 * - Initialize the subsystem using @ref sd_init_subsys
 * - Populate an @ref SD_CFG_ts and register a handle with @ref sd_init_handle
 * - Start the subsystem using @ref sd_start_subsys
 * - Use @ref sd_read and @ref sd_write for sector-level access
 * - Query card properties with @ref sd_get_sector_count and @ref sd_get_sector_size
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup sd_module SD Card Module
 * @brief SPI-mode SD card driver supporting SDSC and SDHC cards.
 * @{
 */

#ifndef SD_H__
#define SD_H__

#include "err.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "configuration.h"

/**
 * @defgroup sd_types SD Card Types
 * @ingroup sd_module
 * @brief Data types used by the SD card module.
 * @{
 */

/**
 * @brief Configuration structure for initializing an SD card handle.
 *
 * @details
 * Passed to @ref sd_init_handle to configure the SPI peripheral and
 * all four SPI GPIO pins (SCLK, CS, MISO, MOSI) connected to the SD card.
 *
 * @note The CS pin is configured as a GPIO output and driven manually.
 *       SCLK, MISO, and MOSI are configured in alternate function mode.
 */
typedef struct {
    /** Human-readable name for this SD card instance (null-terminated). */
    char name[CONFIG_SD_MAX_NAME_LEN];

    /** Pointer to the SPI peripheral instance connected to the SD card. */
    SPI_REGDEF_ts *spi_instance;

    /** GPIO port of the SPI SCLK pin. */
    GPIO_REGDEF_ts *sclk_gpio_port;

    /** GPIO port of the SPI CS (chip select) pin. */
    GPIO_REGDEF_ts *cs_gpio_port;

    /** GPIO port of the SPI MISO pin. */
    GPIO_REGDEF_ts *miso_gpio_port;

    /** GPIO port of the SPI MOSI pin. */
    GPIO_REGDEF_ts *mosi_gpio_port;

    /** GPIO pin number of the SPI SCLK pin. */
    GPIO_PIN_te sclk_gpio_pin;

    /** GPIO pin number of the SPI CS pin. */
    GPIO_PIN_te cs_gpio_pin;

    /** GPIO pin number of the SPI MISO pin. */
    GPIO_PIN_te miso_gpio_pin;

    /** GPIO pin number of the SPI MOSI pin. */
    GPIO_PIN_te mosi_gpio_pin;

    /** Alternate function mapping for SCLK, MISO, and MOSI GPIO pins. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
} SD_CFG_ts;

/**
 * @brief Opaque handle representing an SD card instance.
 *
 * @details
 * Returned by @ref sd_init_handle and used for all subsequent read,
 * write, and query operations. The internal structure is hidden and
 * must not be accessed directly.
 */
typedef struct sd_handle_s SD_HANDLE_ts;

/** @} */

/**
 * @defgroup sd_api SD Card Public API
 * @ingroup sd_module
 * @brief Public functions to interact with the SD card subsystem.
 * @{
 */

/**
 * @brief Initializes the SD card subsystem.
 *
 * @details
 * Resets the internal state, initializes logging and systick dependencies,
 * and registers the CLI commands.
 *
 * Must be called before any other SD API function.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the subsystem is already initialized
 * - Propagated error from @ref cmd_register on failure
 */
ERR_te sd_init_subsys(void);

/**
 * @brief Deinitializes the SD card subsystem.
 *
 * @details
 * Resets the internal state to zero and deregisters the CLI commands.
 *
 * @return
 * - ERR_OK on success
 * - ERR_DEINITIALIZATION_FAILURE if the subsystem is not initialized
 */
ERR_te sd_deinit_subsys(void);

/**
 * @brief Starts the SD card subsystem.
 *
 * @details
 * Enables @ref sd_read and @ref sd_write operations.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already started
 */
ERR_te sd_start_subsys(void);

/**
 * @brief Stops the SD card subsystem.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNKNOWN if the subsystem is not initialized or already stopped
 */
ERR_te sd_stop_subsys(void);

/**
 * @brief Initializes an SD card handle and performs the full SPI-mode power-up sequence.
 *
 * @details
 * Configures all GPIO pins and the SPI peripheral, then executes the SD card
 * initialization sequence (idle state, interface condition check, ACMD41/CMD1,
 * OCR read, CSD read). On success, the SPI clock is ramped to maximum speed.
 *
 * If any step of the initialization sequence fails, the handle is automatically
 * deinitialized and an error is returned.
 *
 * @param[in]  sd_cfg      Pointer to the SD card configuration structure.
 * @param[out] sd_handle_o Pointer to a handle pointer that will be set to
 *                         the allocated SD card instance.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if the subsystem is not initialized
 * - ERR_NOT_ENOUGH_SPACE if the maximum number of handles is reached
 * - ERR_INVALID_ARGUMENT if @p sd_cfg is NULL
 * - ERR_UNSUCCESFUL_ACTION if any step of the SD initialization sequence fails
 */
ERR_te sd_init_handle(SD_CFG_ts *sd_cfg, SD_HANDLE_ts **sd_handle_o);

/**
 * @brief Deinitializes an SD card handle and resets the SPI peripheral.
 *
 * @param[in] sd_handle Pointer to the handle to deinitialize.
 *
 * @return
 * - ERR_OK on success
 * - ERR_ILLEGAL_ACTION if the handle is not initialized
 * - ERR_UNINITIALZIED_OBJECT if the handle is not found in the pool
 */
ERR_te sd_deinit_handle(SD_HANDLE_ts *sd_handle);

/**
 * @brief Retrieves the initialization state of an SD card handle.
 *
 * @param[in]  sd_handle     Pointer to the SD card handle to query.
 * @param[out] handle_init_o Pointer to a boolean that will receive the
 *                           initialization state.
 *
 * @return
 * - ERR_OK always
 */
ERR_te sd_get_handle_init(SD_HANDLE_ts *sd_handle, bool *handle_init_o);

/**
 * @brief Retrieves the total sector (block) count of the SD card.
 *
 * @details
 * The value is determined from the CSD register during @ref sd_init_handle
 * and is used by the FatFs diskio layer.
 *
 * @param[in]  sd_handle      Pointer to the SD card handle.
 * @param[out] sector_count_o Pointer to a variable that will receive the sector count.
 *
 * @return
 * - ERR_OK always
 */
ERR_te sd_get_sector_count(SD_HANDLE_ts *sd_handle, uint32_t *sector_count_o);

/**
 * @brief Retrieves the sector (block) size of the SD card in bytes.
 *
 * @details
 * For SDHC cards this is always 512 bytes. For SDSC cards it is determined
 * from the CSD register. Used by the FatFs diskio layer.
 *
 * @param[in]  sd_handle     Pointer to the SD card handle.
 * @param[out] sector_size_o Pointer to a variable that will receive the sector size in bytes.
 *
 * @return
 * - ERR_OK always
 */
ERR_te sd_get_sector_size(SD_HANDLE_ts *sd_handle, uint32_t *sector_size_o);

/**
 * @brief Reads one or more sectors from the SD card.
 *
 * @details
 * Reads @p num_sectors consecutive sectors starting at @p start_sector into
 * @p read_buf. Uses CMD17 for single-sector reads and CMD18 + CMD12 for
 * multi-sector reads. Handles both byte-addressed (SDSC) and block-addressed
 * (SDHC) cards transparently.
 *
 * @param[in]  sd_handle    Pointer to the SD card handle.
 * @param[out] read_buf     Pointer to the destination buffer. Must be at least
 *                          @p num_sectors × sector size bytes.
 * @param[in]  start_sector Zero-based index of the first sector to read.
 * @param[in]  num_sectors  Number of consecutive sectors to read.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if the subsystem or handle is not initialized or started
 * - ERR_INVALID_ARGUMENT if @p sd_handle is NULL or @p num_sectors is 0
 * - ERR_TIMEOUT if the data token is not received within the configured timeout
 * - ERR_UNKNOWN if the card returns an error token or an unexpected response
 */
ERR_te sd_read(SD_HANDLE_ts *sd_handle, uint8_t *read_buf, uint32_t start_sector, uint32_t num_sectors);

/**
 * @brief Writes one or more sectors to the SD card.
 *
 * @details
 * Writes @p num_sectors consecutive sectors from @p write_buf starting at
 * @p start_sector. Uses CMD24 for single-sector writes and CMD25 + CMD12
 * for multi-sector writes. Handles both byte-addressed (SDSC) and
 * block-addressed (SDHC) cards transparently.
 *
 * @param[in] sd_handle    Pointer to the SD card handle.
 * @param[in] write_buf    Pointer to the source data buffer. Must contain at
 *                         least @p num_sectors × sector size bytes.
 * @param[in] start_sector Zero-based index of the first sector to write.
 * @param[in] num_sectors  Number of consecutive sectors to write.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INITIALIZATION_FAILURE if the subsystem is not initialized or started
 * - ERR_INVALID_ARGUMENT if @p sd_handle is NULL or @p num_sectors is 0
 * - ERR_TIMEOUT if the card does not complete the write within the configured timeout
 * - ERR_UNKNOWN if the card returns an unexpected response
 */
ERR_te sd_write(SD_HANDLE_ts *sd_handle, uint8_t *write_buf, uint32_t start_sector, uint32_t num_sectors);

/**
 * @brief Executes an IOCTL control command on the SD card handle.
 *
 * @details
 * Used by the FatFs diskio layer to perform control operations such as
 * flushing write buffers or querying card parameters.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 */
ERR_te sd_ioctl(SD_HANDLE_ts *sd_handle);

/** @} */

#endif

/** @} */