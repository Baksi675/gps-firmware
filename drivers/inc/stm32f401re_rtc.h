/**
 * @file stm32f401re_rtc.h
 * @author github.com/Baksi675
 * @brief RTC header file.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef STM32F401RE_RTC_H__
#define STM32F401RE_RTC_H__

#include <stdint.h>

/**
 * @brief Week days definition.
 * 
 */
typedef enum {
	WEEK_DAYS_MONDAY,
	WEEK_DAYS_TUESDAY,
	WEEK_DAYS_WEDNESDAY,
	WEEK_DAYS_THURSDAY,
	WEEK_DAYS_FRIDAY,
	WEEK_DAYS_SATURDAY,
	WEEK_DAYS_SUNDAY
}WEEK_DAYS_te;

/**
 * @brief Month definitions
 * 
 */
typedef enum {
	MONTHS_JANUARY,
	MONTHS_FEBRUARY,
	MONTHS_MARCH,
	MONTHS_APRIL,
	MONTHS_MAY,
	MONTHS_JUNE,
	MONTHS_JULE,
	MONTHS_AUGUST,
	MONTHS_SEPTEMBER,
	MONTHS_OCTOBER,
	MONTHS_NOVEMBER,
	MONTHS_DECEMBER
}MONTHS_te;

/**
 * @brief Time definitions.
 * 
 */
typedef struct {
	uint8_t time_hours;
	uint8_t time_minutes;
	uint8_t time_seconds;
}TIME_ts;

/**
 * @brief Calendar definition.
 * 
 */
typedef struct {
	uint16_t calendar_year;
	WEEK_DAYS_te calendar_week_days;
	MONTHS_te calendar_months;
	uint8_t calendar_date;
}CALENDAR_ts;

void rtc_init();
void rtc_set_calendar(CALENDAR_ts *date);
//void rtc_get_calendar(CALENDAR_ts *date);		// Not used
void rtc_set_time(TIME_ts *time);
void rtc_get_time(TIME_ts *time);

#endif