/**
 * @file gtu7.h
 * @author github.com/Baksi675
 * @brief GTU7 GPS module header file
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef GTU7_H__
#define GTU7_H__

#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_usart.h"
#include "err.h"

typedef enum {
	GPS_MESSAGE_GGA,
	GPS_MESSAGE_GLL,
	GPS_MESSAGE_GSA,
	GPS_MESSAGE_GSV,
	GPS_MESSAGE_RMC,
	GPS_MESSAGE_VTG,
	GPS_MESSAGE_TXT
}GTU7_MESSAGES_te;

typedef struct {
	char lat[16];					// LOCATION menu			from RMC
	char lon[16];					// LOCATION menu			from RMC
	char ort_height[16];			// LOCATION menu			from GGA
	char geoid_sep[16];			// LOCATION menu			from GGA
	char time[16];					// DATE_TIME menu			combined from RMC
	char date[16];					// DATE TIME MENU			combined from RMC
	char mov_dir[16];				// MOVEMENT menu			from VTG
	char mov_speed[16];			// MOVEMENT menu			from VTG
	char num_sats_used[4];			// SATELLITES menu			from GGA
	//char gps_info_prn_sats_used[12][4];		// SATELLITES MENU			from GSA
	char num_sats_all[4];			// SATELLITES menu			from GSV
	char fix_type[16];				// ACCURACY menu			from GSA
	char pdop[16];					// ACCURACY menu			from GSA
	char hdop[16];					// ACCURACY menu			from GSA
	char vdop[16];					// ACCURACY menu			from GSA
	char fix_status[16];			// ACCURACY menu			from GGA
}GTU7_INFO_ts;

typedef struct {
	USART_REGDEF_ts *usart_instance;
	USART_BAUD_RATE_te usart_baud_rate;
	GPIO_REGDEF_ts *rx_gpio_port;
	GPIO_PIN_te rx_gpio_pin;
	GPIO_REGDEF_ts *tx_gpio_port;
	GPIO_PIN_te tx_gpio_pin;
	GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function;
}GTU7_CONFIG_ts;

typedef struct gtu7_handle_s GTU7_HANDLE_ts;

ERR_te gtu7_init_subsys(void);
ERR_te gtu7_deinit_subsys(void);
ERR_te gtu7_start_subsys(void);
ERR_te gtu7_stop_subsys(void);
ERR_te gtu7_init_handle(GTU7_CONFIG_ts *gtu7_config,  GTU7_HANDLE_ts **gtu7_handle_o);
ERR_te gtu7_run(void);
ERR_te gtu7_get_info(GTU7_INFO_ts **gtu7_info_o);

#endif