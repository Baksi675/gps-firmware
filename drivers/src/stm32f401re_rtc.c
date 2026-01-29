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

////////////////////// To reset time, call rcc_reset_bkpd //////////////////////

#define RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR			0	
#define RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN		0xABCD

static void rtc_set_write_protection(EN_STATUS_te en_status);

/** 
 * @defgroup STM32F401RE_RTC_DRIVER_Public_APIs STM32F401RE_RTC_DRIVER Public APIs
 * @{
 */

/**
 * @brief Initializes the RTC of the MCU.
 * 
 */
void rtc_init(void)
{
    // Enable PWR clock FIRST
    rcc_set_pclk_apb1(RCC_APB1ENR_PWREN, ENABLE);

    // Enable backup domain access
    PWR->PWR_CR |= (1 << PWR_CR_DBP);

    if ((RTC->RTC_BKPxR[RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR]
         & RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN)
        == RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN)
    {
        // Already initialized
        return;
    }

    // Disable write protection
    rtc_set_write_protection(DISABLE);

    // Enable LSE
    RCC->RCC_BDCR |= (1 << RCC_BDCR_LSEON);
    while (!(RCC->RCC_BDCR & (1 << RCC_BDCR_LSERDY)));

    // Select LSE as RTC clock
    RCC->RCC_BDCR &= ~(0x3 << RCC_BDCR_RTCSEL);
    RCC->RCC_BDCR |=  (0x1 << RCC_BDCR_RTCSEL);

    // Enable RTC
    RCC->RCC_BDCR |= (1 << RCC_BDCR_RTCEN);

    // Enter INIT mode
    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while (!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    // Configure RTC
    RTC->RTC_CR &= ~(1 << RTC_CR_FMT);
    RTC->RTC_PRER =
        (127 << RTC_PRER_PREDIV_A) |
        (255 << RTC_PRER_PREDIV_S);

    // Exit INIT
    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while (RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    // NOW mark RTC as valid
    RTC->RTC_BKPxR[RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR] =
        RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN;

    rtc_set_write_protection(ENABLE);
}


/**
 * @brief Sets the calendar of the RTC.
 * 
 * @param date Calendar object.
 */
void rtc_set_calendar(CALENDAR_ts *date)
{
    rtc_set_write_protection(DISABLE);

    // Enter INIT mode
    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while (!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    RTC->RTC_DR =
        ((date->calendar_date % 10)        << RTC_DR_DU) |
        ((date->calendar_date / 10)        << RTC_DR_DT) |
        ((date->calendar_months % 10)      << RTC_DR_MU) |
        ((date->calendar_months / 10)      << RTC_DR_MT) |
        ((date->calendar_week_days)        << RTC_DR_WDU)|
        (((date->calendar_year - 2000) % 10) << RTC_DR_YU) |
        (((date->calendar_year - 2000) / 10) << RTC_DR_YT);

    // Exit INIT
    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while (RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    rtc_set_write_protection(ENABLE);
}


/**
 * @brief Sets the time of the RTC.
 * 
 * @param time Time object.
 */
void rtc_set_time(TIME_ts *time)
{
    rtc_set_write_protection(DISABLE);

    // Enter INIT mode
    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while (!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    RTC->RTC_TR =
        (DEC_TO_BCD(time->time_hours)   << RTC_TR_HU) |
        (DEC_TO_BCD(time->time_minutes) << RTC_TR_MNU) |
        (DEC_TO_BCD(time->time_seconds) << RTC_TR_SU);

    // Exit INIT
    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while (RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    rtc_set_write_protection(ENABLE);
}


/**
 * @brief Gets the time from the RTC.
 * 
 * @param time The time object of the RTC (output).
 */
void rtc_get_time(TIME_ts *time) {
    uint32_t tr1, tr2;

	// Read time atomically
    do {
        tr1 = RTC->RTC_TR;
        (void)RTC->RTC_DR; 
        tr2 = RTC->RTC_TR;
    } while (tr1 != tr2);

    time->time_seconds = BCD_TO_DEC(tr1 & 0x7F);
    time->time_minutes = BCD_TO_DEC((tr1 >> RTC_TR_MNU) & 0x7F);
    time->time_hours   = BCD_TO_DEC((tr1 >> RTC_TR_HU) & 0x7F);
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