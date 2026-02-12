/**
* @file sd.h
* @author github.com/Baksi675
* @brief SD card driver header file
* @version 0.1
* @date 2026-02-05
* 
* @copyright Copyright (c) 2026
* 
*/

#ifndef SD_H__
#define SD_H__

#include "err.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "configuration.h"

/**
 * @brief Describes whether the SD card completed the power-up sequence. (OCR content)
 * 
 */
typedef enum {
	SD_PWRUP_STATUS_BUSY,
	SD_PWRUP_STATUS_READY
}SD_PWRUP_STATUS_te;

/**
 * @brief Describes the type of the SD card. (OCR content)
 * 
 */
typedef enum {
	SD_TYPE_MMC,				// byte addresing, legacy
	SD_TYPE_SC_V1,			// byte addressing
	SD_TYPE_SC_V2,			// byte adressing
	SD_TYPE_HC				// block addressing
}SD_TYPE_te;

/**
 * @brief Describes the minimum allowed operating voltage of the SD card. (OCR content)
 * 
 */
typedef enum {
	SD_MIN_OPERATING_VOLTAGE_3_5V,
	SD_MIN_OPERATING_VOLTAGE_3_4V,
	SD_MIN_OPERATING_VOLTAGE_3_3V,
	SD_MIN_OPERATING_VOLTAGE_3_2V,
	SD_MIN_OPERATING_VOLTAGE_3_1V,
	SD_MIN_OPERATING_VOLTAGE_3_0V,
	SD_MIN_OPERATING_VOLTAGE_2_9V,
	SD_MIN_OPERATING_VOLTAGE_2_8V,
	SD_MIN_OPERATING_VOLTAGE_2_7V,
	SD_MIN_OPERATING_VOLTAGE_2_6V,
	SD_MIN_OPERATING_VOLTAGE_2_5V,
	SD_MIN_OPERATING_VOLTAGE_2_4V,
	SD_MIN_OPERATING_VOLTAGE_2_3V,
	SD_MIN_OPERATING_VOLTAGE_2_2V,
	SD_MIN_OPERATING_VOLTAGE_2_1V,
	SD_MIN_OPERATING_VOLTAGE_2_0V,
	SD_MIN_OPERATING_VOLTAGE_1_9V,
	SD_MIN_OPERATING_VOLTAGE_1_8V,
	SD_MIN_OPERATING_VOLTAGE_1_7V,
	SD_MIN_OPERATING_VOLTAGE_1_6V	
}SD_MIN_OPERATING_VOLTAGE_te;

/**
 * @brief Descries the maximum allowed operating voltage of the SD card. (OCR content)
 * 
 */
typedef enum {
	SD_MAX_OPERATING_VOLTAGE_3_6V,
	SD_MAX_OPERATING_VOLTAGE_3_5V,
	SD_MAX_OPERATING_VOLTAGE_3_4V,
	SD_MAX_OPERATING_VOLTAGE_3_3V,
	SD_MAX_OPERATING_VOLTAGE_3_2V,
	SD_MAX_OPERATING_VOLTAGE_3_1V,
	SD_MAX_OPERATING_VOLTAGE_3_0V,
	SD_MAX_OPERATING_VOLTAGE_2_9V,
	SD_MAX_OPERATING_VOLTAGE_2_8V,
	SD_MAX_OPERATING_VOLTAGE_2_7V,
	SD_MAX_OPERATING_VOLTAGE_2_6V,
	SD_MAX_OPERATING_VOLTAGE_2_5V,
	SD_MAX_OPERATING_VOLTAGE_2_4V,
	SD_MAX_OPERATING_VOLTAGE_2_3V,
	SD_MAX_OPERATING_VOLTAGE_2_2V,
	SD_MAX_OPERATING_VOLTAGE_2_1V,
	SD_MAX_OPERATING_VOLTAGE_2_0V,
	SD_MAX_OPERATING_VOLTAGE_1_9V,
	SD_MAX_OPERATING_VOLTAGE_1_8V,
	SD_MAX_OPERATING_VOLTAGE_1_7V	
}SD_MAX_OPERATIING_VOLTAGE_te;

typedef struct {
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
}SD_CONFIG_ts;

typedef struct sd_handle_s SD_HANDLE_ts;

ERR_te sd_init_subsys(void);
ERR_te sd_deinit_subsys(void);
ERR_te sd_start_subsys(void);
ERR_te sd_stop_subsys(void);
ERR_te sd_init_handle(SD_CONFIG_ts *sd_config, SD_HANDLE_ts **sd_handle_o);		
ERR_te sd_deinit_handle(SD_HANDLE_ts *sd_handle);
ERR_te sd_get_handle_init(SD_HANDLE_ts *sd_handle, bool *handle_init_o);
ERR_te sd_read(SD_HANDLE_ts *sd_handle, uint8_t *read_buf, uint32_t start_sector, uint32_t num_sectors);
ERR_te sd_write(SD_HANDLE_ts *sd_handle, uint8_t *write_buf, uint32_t start_sector, uint32_t num_sectors);
ERR_te sd_ioctl(SD_HANDLE_ts *sd_handle);

#endif