/**
 * @file io.h
 * @author github.com/Baksi675
 * @brief IO module header file.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef IO_H__
#define IO_H__

#include "err.h"
#include "stm32f401re_gpio.h"
#include "configuration.h"

/**
 * @brief A configuration structure for an IO object.
 * 
 */
typedef struct {
	GPIO_HANDLE_ts *gpio_handle;
	char name[CONFIG_IO_MAX_NAME_LEN];
}IO_CONFIG_ts;

typedef struct io_handle_s IO_HANDLE_ts;

ERR_te io_init_subsys(void);
ERR_te io_deinit_subsys(void);
ERR_te io_start_subsys(void);
ERR_te io_stop_subsys(void);
//ERR_te io_get_def_conf(IO_CONFIG_ts *io_config); 		// Not needed for this module
ERR_te io_init_handle(IO_CONFIG_ts *io_config, IO_HANDLE_ts **io_handle_o);
ERR_te io_deinit_handle(IO_HANDLE_ts const *io_handle);
//ERR_te io_run_handle(IO_HANDLE_ts *io_handle);		// Not needed for this module
//ERR_te io_run_handle_all(void);						// Not needed for this module
ERR_te io_write(IO_HANDLE_ts *io_handle, PIN_STATUS_te pin_status);		// Custom for this module
ERR_te io_read(IO_HANDLE_ts *io_handle, PIN_STATUS_te *pin_status_o);

#endif