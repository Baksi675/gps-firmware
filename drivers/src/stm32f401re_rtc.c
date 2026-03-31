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
#include "err.h"
#include "stm32f401re_rcc.h"
#include "common.h"
#include "stm32f401re.h"

/**
 * @brief Backup register index used to store the initialization sentinel pattern.
 *
 * @details
 * If BKPxR[0] contains @ref RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN after a reset,
 * the RTC backup domain was preserved and the peripheral does not need
 * re-initialization. Call @ref rcc_reset_bkpd to force re-initialization.
 */
#define RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR     0

/**
 * @brief Sentinel value written to BKP0R after successful RTC initialization.
 *
 * @details
 * Chosen to be an unlikely reset value. Presence in BKP0R indicates the
 * RTC has been configured and the backup domain has not been cleared.
 */
#define RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN   0xABCD

/** @brief True after @ref rtc_init has completed successfully in this boot session. */
bool initialized;

/* ---- Forward declaration for internal helper ---- */
static void rtc_set_write_protection(EN_STATUS_te en_status);

/**
 * @defgroup stm32_rtc_public_apis RTC Public APIs
 * @{
 */

/** @brief Initializes the RTC peripheral. @see rtc_init */
ERR_te rtc_init(void) {
    if(initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    // Enable PWR clock to allow access to the backup domain
    rcc_set_pclk_apb1(RCC_APB1ENR_PWREN, ENABLE);

    // Enable backup domain write access
    PWR->PWR_CR |= (1 << PWR_CR_DBP);

    // Check sentinel: if present, the RTC was already configured before this reset
    if((RTC->RTC_BKPxR[RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR]
        & RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN)
       == RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN)
    {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    rtc_set_write_protection(DISABLE);

    // Start LSE oscillator and wait for it to stabilize
    RCC->RCC_BDCR |= (1 << RCC_BDCR_LSEON);
    while(!(RCC->RCC_BDCR & (1 << RCC_BDCR_LSERDY)));

    // Select LSE as the RTC clock source
    RCC->RCC_BDCR &= ~(0x3 << RCC_BDCR_RTCSEL);
    RCC->RCC_BDCR |=  (0x1 << RCC_BDCR_RTCSEL);

    // Enable RTC
    RCC->RCC_BDCR |= (1 << RCC_BDCR_RTCEN);

    // Enter INIT mode and wait for INITF confirmation
    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while(!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    // 24-hour format, prescalers for 1 Hz tick from 32.768 kHz LSE:
    // f_ck_apre = LSE / (PREDIV_A + 1) = 32768 / 128 = 256 Hz
    // f_ck_spre = f_ck_apre / (PREDIV_S + 1) = 256 / 256 = 1 Hz
    RTC->RTC_CR &= ~(1 << RTC_CR_FMT);
    RTC->RTC_PRER =
        (127 << RTC_PRER_PREDIV_A) |
        (255 << RTC_PRER_PREDIV_S);

    // Exit INIT mode
    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while(RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    // Write sentinel to backup register to mark initialization complete
    RTC->RTC_BKPxR[RTC_BKP_DOMAIN_RST_INDICATOR_BKPxR] =
        RTC_BKP_DOMAIN_RST_INDICATOR_PATTERN;

    rtc_set_write_protection(ENABLE);

    initialized = true;

    return ERR_OK;
}

/** @brief Sets the RTC calendar (date and weekday). @see rtc_set_calendar */
void rtc_set_calendar(CALENDAR_ts const *date) {
    rtc_set_write_protection(DISABLE);

    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while(!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    RTC->RTC_DR =
        ((date->date % 10)          << RTC_DR_DU)  |
        ((date->date / 10)          << RTC_DR_DT)  |
        ((date->months % 10)        << RTC_DR_MU)  |
        ((date->months / 10)        << RTC_DR_MT)  |
        ((date->week_days)          << RTC_DR_WDU) |
        (((date->year - 2000) % 10) << RTC_DR_YU)  |
        (((date->year - 2000) / 10) << RTC_DR_YT);

    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while(RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    rtc_set_write_protection(ENABLE);
}

/** @brief Sets the RTC time (hours, minutes, seconds). @see rtc_set_time */
void rtc_set_time(TIME_ts const *time) {
    rtc_set_write_protection(DISABLE);

    RTC->RTC_ISR |= (1 << RTC_ISR_INIT);
    while(!(RTC->RTC_ISR & (1 << RTC_ISR_INITF)));

    RTC->RTC_TR =
        (DEC_TO_BCD(time->hours)   << RTC_TR_HU)  |
        (DEC_TO_BCD(time->minutes) << RTC_TR_MNU) |
        (DEC_TO_BCD(time->seconds) << RTC_TR_SU);

    RTC->RTC_ISR &= ~(1 << RTC_ISR_INIT);
    while(RTC->RTC_ISR & (1 << RTC_ISR_INITF));

    rtc_set_write_protection(ENABLE);
}

/** @brief Reads the current time from the RTC. @see rtc_get_time */
void rtc_get_time(TIME_ts *time) {
    uint32_t tr1, tr2;

    // Double-read to ensure consistency: reading DR after TR unlocks the shadow
    // registers and allows TR to be read again safely
    do {
        tr1 = RTC->RTC_TR;
        (void)RTC->RTC_DR;
        tr2 = RTC->RTC_TR;
    } while(tr1 != tr2);

    time->seconds = BCD_TO_DEC(tr1 & 0x7F);
    time->minutes = BCD_TO_DEC((tr1 >> RTC_TR_MNU) & 0x7F);
    time->hours   = BCD_TO_DEC((tr1 >> RTC_TR_HU)  & 0x7F);
}

/** @} */

/**
 * @defgroup stm32_rtc_internal_helpers RTC Internal Helpers
 * @{
 */

/**
 * @brief Enables or disables write protection of the RTC registers.
 *
 * @details
 * The RTC write protection key sequence is defined by the STM32F401RE
 * reference manual:
 * - **Disable** (unlock): write 0xCA then 0x53 to RTC_WPR.
 * - **Enable** (lock): write any invalid key (0xFF) to RTC_WPR.
 *
 * Must be called before and after any write to RTC configuration registers
 * (RTC_TR, RTC_DR, RTC_CR, RTC_PRER, RTC_ISR).
 *
 * @param[in] en_status @ref ENABLE to re-lock registers, @ref DISABLE to unlock them.
 */
static void rtc_set_write_protection(EN_STATUS_te en_status) {
    if(en_status == ENABLE) {
        RTC->RTC_WPR = 0xFF; // Any invalid key re-enables write protection
    }
    else if(en_status == DISABLE) {
        RTC->RTC_WPR = 0xCA; // First key
        RTC->RTC_WPR = 0x53; // Second key — unlocks write access
    }
}

/** @} */