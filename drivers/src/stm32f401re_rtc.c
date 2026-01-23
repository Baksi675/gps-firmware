/**
 * @file stm32f401re_rtc.c
 * @author github.com/Baksi675
 * @brief RTC driver implementation.
 * @version 0.1
 * @date 2026-01-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "stm32f401re_rtc.h"
#include "stm32f401re_rcc.h"
#include "common.h"
#include "stm32f401re.h"

static void rtc_set_write_protection(EN_STATUS_te en_status);

/** 
 * @defgroup STM32F401RE_RTC_DRIVER_Public_APIs STM32F401RE_RTC_DRIVER Public APIs
 * @{
 */

/**
 * @brief Initializes the RTC of the MCU.
 * 
 */
void rtc_init(){
	// Enable peripheral clock for PWR
	rcc_set_pclk_apb1(RCC_APB1ENR_PWREN, ENABLE);
	
	// Enable write access to backup domain after system reset
	PWR->PWR_CR |= (0x1 << PWR_CR_DBP);

	// Disable write protection on RTC registers
	rtc_set_write_protection(DISABLE);

	// Enable LSE clock
	RCC->RCC_BDCR |= (0x1 << RCC_BDCR_LSEON);
	while(!((RCC->RCC_BDCR >> RCC_BDCR_LSERDY) & 0x1));

	// Set LSE as RTC clock source
	RCC->RCC_BDCR &= ~(0x3 << RCC_BDCR_RTCSEL);
	RCC->RCC_BDCR |= (0x1 << RCC_BDCR_RTCSEL);

	// Enable RTC
	RCC->RCC_BDCR |= (0x1 << RCC_BDCR_RTCEN);

	// Enter initialization mode, wait until entered
	RTC->RTC_ISR |= (0x1 << RTC_ISR_INIT);
	while(!((RTC->RTC_ISR >> RTC_ISR_INITF) & 0x1));

	// Set to 24 hour/day format
	RTC->RTC_CR &= ~(0x1 << RTC_CR_FMT);

	// Exit initialization mode
	RTC->RTC_ISR &= ~(0x1 << RTC_ISR_INIT);
	while(((RTC->RTC_ISR >> RTC_ISR_INITF) & 0x1));

	rtc_set_write_protection(ENABLE);
}

/**
 * @brief Sets the calendar of the RTC.
 * 
 * @param date Calendar object.
 */
void rtc_set_calendar(CALENDAR_ts *date) {
    // Disable write protection on RTC registers
    rtc_set_write_protection(DISABLE);

    // Enter initialization mode, wait until entered
    RTC->RTC_ISR |= (0x1 << RTC_ISR_INIT);
    while (!((RTC->RTC_ISR >> RTC_ISR_INITF) & 0x1));

    // Clear only the relevant fields in DR (preserve reserved bits)
    RTC->RTC_DR &= ~(
        (0xF << RTC_DR_DU)  | // Date units
        (0x3 << RTC_DR_DT)  | // Date tens
        (0xF << RTC_DR_MU)  | // Month units
        (0x1 << RTC_DR_MT)  | // Month tens
        (0x7 << RTC_DR_WDU) | // Weekday
        (0xF << RTC_DR_YU)  | // Year units
        (0xF << RTC_DR_YT)    // Year tens
    );

    // Day
    RTC->RTC_DR |= ((date->calendar_date % 10) << RTC_DR_DU);
    RTC->RTC_DR |= ((date->calendar_date / 10) << RTC_DR_DT);

    // Month
    RTC->RTC_DR |= ((date->calendar_months % 10) << RTC_DR_MU);
    RTC->RTC_DR |= ((date->calendar_months / 10) << RTC_DR_MT);

    // Weekday
    RTC->RTC_DR |= (date->calendar_week_days << RTC_DR_WDU);

    // Year (offset from 2000)
    uint8_t year = date->calendar_year - 2000;
    RTC->RTC_DR |= ((year % 10) << RTC_DR_YU);
    RTC->RTC_DR |= ((year / 10) << RTC_DR_YT);

    // Exit initialization mode
    RTC->RTC_ISR &= ~(0x1 << RTC_ISR_INIT);
    while ((RTC->RTC_ISR >> RTC_ISR_INITF) & 0x1);

    // Re-enable write protection
    rtc_set_write_protection(ENABLE);
}

/**
 * @brief Sets the time of the RTC.
 * 
 * @param time Time object.
 */
void rtc_set_time(TIME_ts *time) {
	// Disable write protection
	rtc_set_write_protection(DISABLE);

	// Enter initialization mode, wait until entered
	RTC->RTC_ISR |= (0x1 << RTC_ISR_INIT);
	while(!((RTC->RTC_ISR >> RTC_ISR_INITF) & 0x1));

	// Configure TR register
	RTC->RTC_TR = (DEC_TO_BCD(time->time_hours) << 16) | (DEC_TO_BCD(time->time_minutes) << 8) | (DEC_TO_BCD(time->time_seconds) << 0); 

	// Exit initialization mode
	RTC->RTC_ISR &= ~(0x1 << RTC_ISR_INIT);

	rtc_set_write_protection(ENABLE);
}

/**
 * @brief Gets the time from the RTC.
 * 
 * @param time The time object of the RTC (output).
 */
void rtc_get_time(TIME_ts *time) {
	uint8_t hours, minutes, seconds;

	// Reset RSF bit (register synchronization flag) in the ISR register ==> Needed if the SW reads the shadow registers faster than 2 RTC CLK cycles
	RTC->RTC_ISR &= ~(0x1 << RTC_ISR_RSF);

	while(!((RTC->RTC_ISR >> RTC_ISR_RSF) & 0x1));

	seconds = BCD_TO_DEC(RTC->RTC_TR & 0x7F);
	minutes = BCD_TO_DEC((RTC->RTC_TR >> RTC_TR_MNU) & 0x7F);
	hours = BCD_TO_DEC((RTC->RTC_TR >> RTC_TR_HU) & 0x7F);

	time->time_seconds = seconds;
	time->time_minutes = minutes;
	time->time_hours = hours;
}

/** @} */

/** 
 * @defgroup STM32F401RE_RTC_DRIVER_Internal_Helper STM32F401RE_RTC_DRIVER Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables write protection of the RTC registers.
 * 
 * @param en_status Whether to enable or disable write protection.
 */
static void rtc_set_write_protection(EN_STATUS_te en_status) {
	// Enable write access to RTC registers after backup domain reset	
	if(en_status == ENABLE) {
		RTC->RTC_WPR = 0xFF;
	}
	else if(en_status == DISABLE) {
		RTC->RTC_WPR = 0xCA;
		RTC->RTC_WPR = 0x53;
	}
}

/** @} */