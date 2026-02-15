/**
 * @file datalog.h
 * @author github.com/Baksi675
 * @brief Data log module header file
 * @version 0.1
 * @date 2026-02-15
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef DATALOG_H__
#define DATALOG_H__

#include "err.h"
#include "configuration.h"

/**
 * @brief Time interval between data logs.
 * 
 */
typedef enum {
	DATALOG_TIME_1S = 1,
	DATALOG_TIME_2S = 2,
	DATALOG_TIME_3S = 3,
	DATALOG_TIME_4S = 4,
	DATALOG_TIME_5S = 5,
	DATALOG_TIME_10S = 10,
	DATALOG_TIME_20S = 20,
	DATALOG_TIME_30S = 30,
	DATALOG_TIME_40S = 40,
	DATALOG_TIME_50S = 50,
	DATALOG_TIME_60S = 60,
}DATALOG_TIME_te;

typedef struct {
	char name[CONFIG_SD_MAX_NAME_LEN];
	DATALOG_TIME_te datalog_time;
}DATALOG_CONFIG_ts;

typedef struct datalog_handle_s DATALOG_HANDLE_ts;

ERR_te datalog_init_subsys(void);
ERR_te datalog_deinit_subsys(void);
ERR_te datalog_start_subsys(void);
ERR_te datalog_stop_subsys(void);
ERR_te datalog_init_handle(DATALOG_CONFIG_ts *datalog_config, DATALOG_HANDLE_ts **datalog_handle_o);
ERR_te datalog_deinit_handle(DATALOG_HANDLE_ts *datalog_handle);
ERR_te datalog_run_handle(DATALOG_HANDLE_ts *datalog_handle);

#endif