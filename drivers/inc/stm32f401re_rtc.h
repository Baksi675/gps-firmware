/**
 * @file stm32f401re_rtc.h
 * @author github.com/Baksi675
 * @brief STM32F401RE RTC driver public API.
 *
 * @details
 * This module provides a driver for the STM32F401RE Real-Time Clock (RTC)
 * peripheral, supporting calendar and time configuration and readback.
 *
 * The RTC is clocked by the LSE (Low-Speed External, 32.768 kHz) oscillator
 * and is configured with the standard 24-hour format. Prescalers are set to
 * produce a 1 Hz tick (PREDIV_A = 127, PREDIV_S = 255).
 *
 * A backup register pattern (@c 0xABCD in BKP0R) is used as an
 * initialization sentinel. If the pattern is present on startup, the RTC is
 * assumed to already be configured and @ref rtc_init returns
 * @ref ERR_MODULE_ALREADY_INITIALIZED without overwriting the current time.
 * To force re-initialization, call @ref rcc_reset_bkpd first.
 *
 * Typical usage:
 * - Call @ref rtc_init once at startup (via @ref init_rtc)
 * - Call @ref rtc_set_calendar and @ref rtc_set_time to set the initial values
 * - Call @ref rtc_get_time anywhere in application code to read the current time
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32_rtc STM32F401RE RTC Driver
 * @brief Calendar and time driver for the STM32F401RE RTC peripheral.
 * @{
 */

#ifndef STM32F401RE_RTC_H__
#define STM32F401RE_RTC_H__

#include <stdint.h>
#include "err.h"

/**
 * @defgroup stm32_rtc_types RTC Types
 * @ingroup stm32_rtc
 * @brief Data types used by the RTC driver.
 * @{
 */

/**
 * @brief Day of the week.
 *
 * @details
 * Stored in the RTC_DR WDU field. Values map directly to the hardware
 * encoding used by the STM32F401RE RTC (Monday = 0 through Sunday = 6).
 */
typedef enum {
    WEEK_DAYS_MONDAY,    /**< Monday.    */
    WEEK_DAYS_TUESDAY,   /**< Tuesday.   */
    WEEK_DAYS_WEDNESDAY, /**< Wednesday. */
    WEEK_DAYS_THURSDAY,  /**< Thursday.  */
    WEEK_DAYS_FRIDAY,    /**< Friday.    */
    WEEK_DAYS_SATURDAY,  /**< Saturday.  */
    WEEK_DAYS_SUNDAY     /**< Sunday.    */
} WEEK_DAYS_te;

/**
 * @brief Month of the year.
 *
 * @note `MONTHS_JULE` is a typo for July carried from the original implementation.
 */
typedef enum {
    MONTHS_JANUARY,   /**< January.   */
    MONTHS_FEBRUARY,  /**< February.  */
    MONTHS_MARCH,     /**< March.     */
    MONTHS_APRIL,     /**< April.     */
    MONTHS_MAY,       /**< May.       */
    MONTHS_JUNE,      /**< June.      */
    MONTHS_JULE,      /**< July. (note: typo in original) */
    MONTHS_AUGUST,    /**< August.    */
    MONTHS_SEPTEMBER, /**< September. */
    MONTHS_OCTOBER,   /**< October.   */
    MONTHS_NOVEMBER,  /**< November.  */
    MONTHS_DECEMBER   /**< December.  */
} MONTHS_te;

/**
 * @brief Holds a time-of-day value (hours, minutes, seconds).
 *
 * @details
 * Used by @ref rtc_set_time and @ref rtc_get_time. All fields are in
 * decimal (not BCD); conversion to/from the RTC's BCD format is handled
 * internally by the driver.
 */
typedef struct {
    uint8_t hours;   /**< Hour of the day (0–23). */
    uint8_t minutes; /**< Minute of the hour (0–59). */
    uint8_t seconds; /**< Second of the minute (0–59). */
} TIME_ts;

/**
 * @brief Holds a calendar date (year, month, day, weekday).
 *
 * @details
 * Used by @ref rtc_set_calendar. The year is a full four-digit year
 * (e.g. 2026); the driver subtracts 2000 before writing to the RTC,
 * so years before 2000 or after 2099 are not supported.
 */
typedef struct {
    uint16_t year;           /**< Full year (2000–2099). */
    WEEK_DAYS_te week_days;  /**< Day of the week. */
    MONTHS_te months;        /**< Month of the year. */
    uint8_t date;            /**< Day of the month (1–31). */
} CALENDAR_ts;

/** @} */

/**
 * @defgroup stm32_rtc_api RTC Public API
 * @ingroup stm32_rtc
 * @brief Public functions to interact with the RTC peripheral.
 * @{
 */

/**
 * @brief Initializes the RTC peripheral.
 *
 * @details
 * Enables the PWR and backup domain access, starts the LSE oscillator,
 * selects LSE as the RTC clock source, enables the RTC, and configures
 * the prescalers for a 1 Hz tick (PREDIV_A = 127, PREDIV_S = 255) with
 * 24-hour format.
 *
 * If the backup register sentinel pattern is already present, the RTC is
 * assumed to be running with a valid configuration and the function returns
 * immediately without modifying the time or calendar.
 *
 * @return
 * - ERR_OK on success
 * - ERR_MODULE_ALREADY_INITIALIZED if the RTC was already initialized
 *   (either by a prior call in this boot or because the backup domain
 *   was preserved across a reset)
 *
 * @note To force re-initialization and clear the current time, call
 *       @ref rcc_reset_bkpd before calling this function.
 * @note Typically called indirectly via @ref init_rtc.
 */
ERR_te rtc_init(void);

/**
 * @brief Sets the RTC calendar (date and weekday).
 *
 * @details
 * Enters RTC INIT mode, writes the date, month, weekday, and year to
 * RTC_DR in BCD format, then exits INIT mode. Write protection is
 * disabled and re-enabled around the operation.
 *
 * @param[in] date Pointer to the calendar structure to write.
 *
 * @note Years must be in the range 2000–2099.
 */
void rtc_set_calendar(CALENDAR_ts const *date);

/**
 * @brief Sets the RTC time (hours, minutes, seconds).
 *
 * @details
 * Enters RTC INIT mode, writes hours, minutes, and seconds to RTC_TR
 * in BCD format using @ref DEC_TO_BCD, then exits INIT mode.
 * Write protection is disabled and re-enabled around the operation.
 *
 * @param[in] time Pointer to the time structure to write.
 */
void rtc_set_time(TIME_ts const *time);

/**
 * @brief Reads the current time from the RTC.
 *
 * @details
 * Reads RTC_TR atomically using the shadow register double-read technique:
 * reads TR, reads DR (to unlock the shadow registers), reads TR again,
 * and repeats if the two TR reads differ. Converts BCD values to decimal
 * using @ref BCD_TO_DEC before storing in @p time.
 *
 * @param[out] time Pointer to the time structure that will receive the
 *                  current hours, minutes, and seconds.
 */
void rtc_get_time(TIME_ts *time);

/** @} */

#endif

/** @} */