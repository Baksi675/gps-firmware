/**
 * @file stm32f401re.h
 * @author github.com/Baksi675
 * @brief STM32F401RE MCU-specific peripheral register definitions and bit position enumerations.
 *
 * @details
 * This header is the central hardware abstraction layer for the STM32F401RE.
 * It provides:
 * - **Peripheral base addresses** for APB1, APB2, AHB1, and AHB2 buses
 * - **Register struct definitions** for GPIO, RCC, EXTI, SYSCFG, RTC, PWR,
 *   SPI, I2C, and USART peripherals, cast via accessor macros (e.g. @ref RCC,
 *   @ref GPIOA)
 * - **Bit position enumerations** for every configured register, used with
 *   the shift-and-mask pattern throughout the driver layer
 * - **IRQ number enumeration** (@ref IRQn_te) for use with the NVIC driver
 * - **EXTI line enumeration** (@ref EXTI_LINES_te) and **SYSCFG port codes**
 *   (@ref PORT_CODES_ts) for interrupt routing
 * - **Legacy clock enable/disable macros** (GPIOx_CLK_EN / GPIOx_CLK_DIS)
 *   superseded by @ref rcc_set_pclk_ahb1
 *
 * @version 0.1
 * @date 2026-01-22
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup stm32f401re_hal STM32F401RE Hardware Abstraction Layer
 * @brief Peripheral register definitions and bit positions for the STM32F401RE.
 * @{
 */

#ifndef STM32F401RE_H_
#define STM32F401RE_H_

#include <stdint.h>

/**
 * @defgroup stm32f401re_addresses Peripheral Base Addresses
 * @ingroup stm32f401re_hal
 * @brief Memory-mapped base addresses for all STM32F401RE peripherals.
 * @{
 */

/** @brief APB1 peripheral bus base address (0x40000000). */
#define ADDR_APB1               (0x40000000U)
#define ADDR_SPI3               (ADDR_APB1 + 0x3C00) /**< SPI3 base address.   */
#define ADDR_SPI2               (ADDR_APB1 + 0x3800) /**< SPI2 base address.   */
#define ADDR_RTC                (ADDR_APB1 + 0x2800) /**< RTC base address.    */
#define ADDR_PWR                (ADDR_APB1 + 0x7000) /**< PWR base address.    */
#define ADDR_I2C3               (ADDR_APB1 + 0x5C00) /**< I2C3 base address.   */
#define ADDR_I2C2               (ADDR_APB1 + 0x5800) /**< I2C2 base address.   */
#define ADDR_I2C1               (ADDR_APB1 + 0x5400) /**< I2C1 base address.   */
#define ADDR_USART2             (ADDR_APB1 + 0x4400) /**< USART2 base address. */

/** @brief APB2 peripheral bus base address (0x40010000). */
#define ADDR_APB2               (0x40010000U)
#define ADDR_EXTI               (ADDR_APB2 + 0x3C00) /**< EXTI base address.   */
#define ADDR_SYSCFG             (ADDR_APB2 + 0x3800) /**< SYSCFG base address. */
#define ADDR_SPI4               (ADDR_APB2 + 0x3400) /**< SPI4 base address.   */
#define ADDR_SPI1               (ADDR_APB2 + 0x3000) /**< SPI1 base address.   */
#define ADDR_USART6             (ADDR_APB2 + 0x1400) /**< USART6 base address. */
#define ADDR_USART1             (ADDR_APB2 + 0x1000) /**< USART1 base address. */

/** @brief AHB1 peripheral bus base address (0x40020000). */
#define ADDR_AHB1               (0x40020000U)
#define ADDR_GPIOA              (ADDR_AHB1)           /**< GPIOA base address. */
#define ADDR_GPIOB              (ADDR_AHB1 + 0x0400)  /**< GPIOB base address. */
#define ADDR_GPIOC              (ADDR_AHB1 + 0x0800)  /**< GPIOC base address. */
#define ADDR_GPIOD              (ADDR_AHB1 + 0x0C00)  /**< GPIOD base address. */
#define ADDR_GPIOE              (ADDR_AHB1 + 0x1000)  /**< GPIOE base address. */
#define ADDR_GPIOH              (ADDR_AHB1 + 0x1C00)  /**< GPIOH base address. */
#define ADDR_RCC                (ADDR_AHB1 + 0x3800)  /**< RCC base address.   */

/** @brief AHB2 peripheral bus base address (0x50000000). */
#define ADDR_AHB2               (0x50000000U)

/** @} */

/**
 * @defgroup stm32f401re_regdefs Peripheral Register Definitions
 * @ingroup stm32f401re_hal
 * @brief Memory-mapped register structures for STM32F401RE peripherals.
 * @{
 */

/**
 * @brief GPIO peripheral register map.
 *
 * @details
 * Each GPIO port occupies 0x400 bytes and contains the following registers.
 * Instantiated via the @ref GPIOA … @ref GPIOH accessor macros.
 */
typedef struct {
    uint32_t volatile GPIO_MODER;   /**< Mode register: 2 bits per pin — input, output, AF, or analog. */
    uint32_t volatile GPIO_OTYPER;  /**< Output type register: 0 = push-pull, 1 = open-drain. */
    uint32_t volatile GPIO_OSPEEDR; /**< Output speed register: 2 bits per pin — low/medium/high/very high. */
    uint32_t volatile GPIO_PUPDR;   /**< Pull-up/pull-down register: 2 bits per pin — none/PU/PD. */
    uint32_t volatile GPIO_IDR;     /**< Input data register: read-only, reflects pin logic levels. */
    uint32_t volatile GPIO_ODR;     /**< Output data register: write the desired output level per pin. */
    uint32_t volatile GPIO_BSRR;    /**< Bit set/reset register: upper 16 bits reset, lower 16 bits set. */
    uint32_t volatile GPIO_LCKR;    /**< Lock register: freezes pin configuration until next reset. */
    uint32_t volatile GPIO_AFR[2];  /**< Alternate function registers: AFR[0] for pins 0–7, AFR[1] for pins 8–15. */
} GPIO_REGDEF_ts;

/**
 * @brief RCC (Reset and Clock Control) peripheral register map.
 *
 * @details
 * Provides clock source selection, PLL configuration, bus prescalers,
 * peripheral clock enables, peripheral resets, low-power clock enables,
 * and the backup domain control register.
 * Instantiated via the @ref RCC accessor macro.
 */
typedef struct {
    uint32_t volatile RCC_CR;           /**< Clock control register: HSI/HSE/PLL enable and ready flags. */
    uint32_t volatile RCC_PLLCFGR;      /**< PLL configuration register: M, N, P, Q dividers and source. */
    uint32_t volatile RCC_CFGR;         /**< Clock configuration register: system clock switch, bus prescalers. */
    uint32_t volatile RCC_CIR;          /**< Clock interrupt register: ready flags and interrupt enables. */
    uint32_t volatile RCC_AHB1RSTR;     /**< AHB1 peripheral reset register. */
    uint32_t volatile RCC_AHB2RSTR;     /**< AHB2 peripheral reset register. */
    uint32_t RESERVED0[2];
    uint32_t volatile RCC_APB1RSTR;     /**< APB1 peripheral reset register. */
    uint32_t volatile RCC_APB2RSTR;     /**< APB2 peripheral reset register. */
    uint32_t RESERVED1[2];
    uint32_t volatile RCC_AHB1ENR;      /**< AHB1 peripheral clock enable register. */
    uint32_t volatile RCC_AHB2ENR;      /**< AHB2 peripheral clock enable register. */
    uint32_t RESERVED2[2];
    uint32_t volatile RCC_APB1ENR;      /**< APB1 peripheral clock enable register. */
    uint32_t volatile RCC_APB2ENR;      /**< APB2 peripheral clock enable register. */
    uint32_t RESERVED3[2];
    uint32_t volatile RCC_AHB1LPENR;    /**< AHB1 peripheral low-power clock enable register. */
    uint32_t volatile RCC_AHB2LPENR;    /**< AHB2 peripheral low-power clock enable register. */
    uint32_t RESERVED4[2];
    uint32_t volatile RCC_APB1LPENR;    /**< APB1 peripheral low-power clock enable register. */
    uint32_t volatile RCC_APB2LPENR;    /**< APB2 peripheral low-power clock enable register. */
    uint32_t RESERVED5[2];
    uint32_t volatile RCC_BDCR;         /**< Backup domain control register: LSE, RTC clock source, backup reset. */
    uint32_t volatile RCC_CSR;          /**< Control/status register: LSI enable/ready, reset flags. */
    uint32_t RESERVED6[2];
    uint32_t volatile RCC_SSCFGR;       /**< Spread spectrum clock generation register. */
    uint32_t volatile RCC_PLLI2SCFGR;   /**< PLLI2S configuration register. */
    uint32_t RESERVED7;
    uint32_t volatile RCC_DCKCFGR;      /**< Dedicated clocks configuration register. */
} RCC_REGDEF_ts;

/**
 * @brief EXTI (External Interrupt/Event Controller) peripheral register map.
 */
typedef struct {
    uint32_t volatile EXTI_IMR;   /**< Interrupt mask register: 1 = interrupt unmasked for this line. */
    uint32_t volatile EXTI_EMR;   /**< Event mask register: 1 = event unmasked for this line. */
    uint32_t volatile EXTI_RTSR;  /**< Rising trigger selection register: 1 = rising edge triggers this line. */
    uint32_t volatile EXTI_FTSR;  /**< Falling trigger selection register: 1 = falling edge triggers this line. */
    uint32_t volatile EXTI_SWIER; /**< Software interrupt event register: write 1 to trigger line in software. */
    uint32_t volatile EXTI_PR;    /**< Pending register: write 1 to clear a pending interrupt flag. */
} EXTI_REGDEF_ts;

/**
 * @brief SYSCFG (System Configuration Controller) peripheral register map.
 *
 * @details
 * Used primarily to route GPIO pins to EXTI lines via the EXTICRx registers.
 */
typedef struct {
    uint32_t volatile SYSCFG_MEMRMP;  /**< Memory remap register. */
    uint32_t volatile SYSCFG_PMC;     /**< Peripheral mode configuration register. */
    uint32_t volatile SYSCFG_EXTICR1; /**< External interrupt configuration register 1 (pins 0–3). */
    uint32_t volatile SYSCFG_EXTICR2; /**< External interrupt configuration register 2 (pins 4–7). */
    uint32_t volatile SYSCFG_EXTICR3; /**< External interrupt configuration register 3 (pins 8–11). */
    uint32_t volatile SYSCFG_EXTICR4; /**< External interrupt configuration register 4 (pins 12–15). */
    uint32_t volatile SYSCFG_CMPCR;   /**< Compensation cell control register. */
} SYSCFG_REGDEF_ts;

/**
 * @brief RTC peripheral register map.
 *
 * @details
 * All registers are write-protected by default. Unlock by writing 0xCA then
 * 0x53 to @ref RTC_WPR. Re-lock by writing any invalid key.
 * Most configuration registers also require INIT mode (RTC_ISR_INIT = 1).
 */
typedef struct {
    uint32_t volatile RTC_TR;       /**< Time register: BCD-encoded hours, minutes, seconds, AM/PM flag. */
    uint32_t volatile RTC_DR;       /**< Date register: BCD-encoded year, month, day, weekday. */
    uint32_t volatile RTC_CR;       /**< Control register: format, wakeup timer, alarm, timestamp enables. */
    uint32_t volatile RTC_ISR;      /**< Initialization and status register: INIT mode, shadow register sync. */
    uint32_t volatile RTC_PRER;     /**< Prescaler register: PREDIV_A (async) and PREDIV_S (sync) fields. */
    uint32_t volatile RTC_WUTR;     /**< Wakeup timer register. */
    uint32_t volatile RTC_CALIBR;   /**< Calibration register. */
    uint32_t volatile RTC_ALRMAR;   /**< Alarm A register. */
    uint32_t volatile RTC_ALRMBR;   /**< Alarm B register. */
    uint32_t volatile RTC_WPR;      /**< Write protection register: write 0xCA then 0x53 to unlock. */
    uint32_t volatile RTC_SSR;      /**< Sub-second register. */
    uint32_t RESERVED0;
    uint32_t volatile RTC_TSTR;     /**< Timestamp time register. */
    uint32_t RESERVED1;
    uint32_t volatile RTC_TSSSR;    /**< Timestamp sub-second register. */
    uint32_t volatile RTC_CALR;     /**< Smooth digital calibration register. */
    uint32_t volatile RTC_TAFCR;    /**< Tamper and alternate function configuration register. */
    uint32_t volatile RTC_ALRMASSR; /**< Alarm A sub-second register. */
    uint32_t volatile RTC_ALRMBSSR; /**< Alarm B sub-second register. */
    uint32_t RESERVED2;
    uint32_t volatile RTC_BKPxR[20]; /**< Backup registers BKP0R–BKP19R: retained across resets if backup domain powered. */
} RTC_REGDEF_ts;

/**
 * @brief PWR (Power Control) peripheral register map.
 */
typedef struct {
    uint32_t volatile PWR_CR;  /**< Power control register: voltage regulator, backup domain access (DBP). */
    uint32_t volatile PWR_CSR; /**< Power control/status register: wakeup flags, voltage level indicator. */
} PWR_REGDEF_ts;

/**
 * @brief SPI peripheral register map.
 *
 * @details
 * Shared by SPI1–SPI4. Also used for the I2S interface (I2SCFGR, I2SPR).
 * Instantiated via the @ref SPI1 … @ref SPI4 accessor macros.
 */
typedef struct {
    uint32_t volatile SPI_CR1;     /**< Control register 1: mode, clock, frame format, enable (SPE). */
    uint32_t volatile SPI_CR2;     /**< Control register 2: DMA, SSOE, interrupt enables. */
    uint32_t volatile SPI_SR;      /**< Status register: TXE, RXNE, BSY, error flags. */
    uint32_t volatile SPI_DR;      /**< Data register: read to receive, write to transmit. */
    uint32_t volatile SPI_CRCPR;   /**< CRC polynomial register. */
    uint32_t volatile SPI_TXCRCR;  /**< TX CRC register. */
    uint32_t volatile SPI_RXCRCR;  /**< RX CRC register. */
    uint32_t volatile SPI_I2SCFGR; /**< I2S configuration register. */
    uint32_t volatile SPI_I2SPR;   /**< I2S prescaler register. */
} SPI_REGDEF_ts;

/**
 * @brief I2C peripheral register map.
 *
 * @details
 * Instantiated via the @ref I2C1, @ref I2C2, @ref I2C3 accessor macros.
 */
typedef struct {
    uint32_t volatile I2C_CR1;   /**< Control register 1: PE, START, STOP, ACK, clock stretch. */
    uint32_t volatile I2C_CR2;   /**< Control register 2: FREQ field (APB1 MHz), interrupt and DMA enables. */
    uint32_t volatile I2C_OAR1;  /**< Own address register 1: 7-bit or 10-bit own address. */
    uint32_t volatile I2C_OAR2;  /**< Own address register 2: dual-address mode. */
    uint32_t volatile I2C_DR;    /**< Data register: write to transmit, read to receive. */
    uint32_t volatile I2C_SR1;   /**< Status register 1: SB, ADDR, TxE, RxNE, BTF, error flags. */
    uint32_t volatile I2C_SR2;   /**< Status register 2: MSL, BUSY, TRA flags; reading clears ADDR. */
    uint32_t volatile I2C_CCR;   /**< Clock control register: CCR value, DUTY, FS (standard/fast mode). */
    uint32_t volatile I2C_TRISE; /**< TRISE register: maximum rise time in units of 1/APB1 clock. */
    uint32_t volatile I2C_FLTR;  /**< Filter register: analog and digital noise filters. */
} I2C_REGDEF_ts;

/**
 * @brief USART peripheral register map.
 *
 * @details
 * Instantiated via the @ref USART1, @ref USART2, @ref USART6 accessor macros.
 */
typedef struct {
    uint32_t volatile USART_SR;   /**< Status register: TXE, TC, RXNE, IDLE, error flags. */
    uint32_t volatile USART_DR;   /**< Data register: write to transmit, read to receive (clears RXNE). */
    uint32_t volatile USART_BRR;  /**< Baud rate register: mantissa and fraction fields. */
    uint32_t volatile USART_CR1;  /**< Control register 1: UE, M (word length), PCE, PS, TE, RE, interrupt enables. */
    uint32_t volatile USART_CR2;  /**< Control register 2: STOP bits, clock (sync mode), LIN. */
    uint32_t volatile USART_CR3;  /**< Control register 3: hardware flow control, DMA, IrDA, smartcard, ONEBIT. */
    uint32_t volatile USART_GTPR; /**< Guard time and prescaler register (smartcard / IrDA). */
} USART_REGDEF_ts;

/** @} */
    
/**
 * @defgroup stm32f401re_bitpos Register Bit Positions
 * @ingroup stm32f401re_hal
 * @brief Bit position enumerations for STM32F401RE peripheral registers.
 *
 * @details
 * Each enumerator value is a bit position (not a mask) for use with the
 * shift-and-mask pattern: `(reg >> BIT_POS) & mask`.
 * @{
 */

/**
 * @brief APB1ENR register bit positions (legacy, prefer @ref RCC_APB1ENR_te).
 */
typedef enum
{
    APB1ENR_PWREN = 28 /**< Bit 28: Power interface clock enable. */
} APB1ENR_te;

/**
 * @brief PWR_CR register bit positions.
 */
typedef enum
{
    PWR_CR_DBP = 8 /**< Bit 8: Disable backup domain write protection. Must be set before writing RTC/backup registers. */
} PWR_CR_te;

/**
 * @brief RCC_CR register bit positions.
 */
typedef enum
{
    RCC_CR_HSEON  = 16, /**< Bit 16: HSE oscillator enable. */
    RCC_CR_HSERDY = 17  /**< Bit 17: HSE oscillator ready flag (read-only). */
} RCC_CR_te;

/**
 * @brief RCC_BDCR register bit positions.
 */
typedef enum
{
    RCC_BDCR_LSEON  = 0,  /**< Bit 0:  LSE oscillator enable. */
    RCC_BDCR_LSERDY = 1,  /**< Bit 1:  LSE oscillator ready flag (read-only). */
    RCC_BDCR_LSEBYP = 2,  /**< Bit 2:  LSE oscillator bypass (for external clock input). */
    RCC_BDCR_RTCSEL = 8,  /**< Bits 9:8: RTC clock source selection (1 = LSE, 2 = LSI, 3 = HSE/div). */
    RCC_BDCR_RTCEN  = 15, /**< Bit 15: RTC clock enable. */
    RCC_BDCR_BDRST  = 16  /**< Bit 16: Backup domain software reset. Clears RTC and backup registers. */
} RCC_BDCR_te;

/**
 * @brief RCC_CSR register bit positions.
 */
typedef enum
{
    RCC_CSR_LSION  = 0, /**< Bit 0: LSI oscillator enable. */
    RCC_CSR_LSIRDY = 1  /**< Bit 1: LSI oscillator ready flag (read-only). */
} RCC_CSR_te;

/**
 * @brief RTC_CR register bit positions.
 */
typedef enum
{
    RTC_CR_FMT = 6 /**< Bit 6: Time format (0 = 24-hour, 1 = AM/PM). */
} RTC_CR_te;

/**
 * @brief RTC_ISR register bit positions.
 */
typedef enum
{
    RTC_ISR_RSF   = 5, /**< Bit 5: Registers synchronization flag. Set after TR/DR shadow regs are updated. */
    RTC_ISR_INITF = 6, /**< Bit 6: Initialization mode flag. Set when RTC is ready for initialization writes. */
    RTC_ISR_INIT  = 7  /**< Bit 7: Initialization mode enable. Write 1 to enter, 0 to exit. */
} RTC_ISR_te;

/**
 * @brief RTC_TR register bit positions (all fields BCD-encoded).
 */
typedef enum
{
    RTC_TR_SU  = 0,  /**< Bits 3:0:  Seconds units (0–9). */
    RTC_TR_ST  = 4,  /**< Bits 6:4:  Seconds tens (0–5). */
    RTC_TR_MNU = 8,  /**< Bits 11:8: Minutes units (0–9). */
    RTC_TR_MNT = 12, /**< Bits 14:12: Minutes tens (0–5). */
    RTC_TR_HU  = 16, /**< Bits 19:16: Hours units (0–9). */
    RTC_TR_HT  = 20, /**< Bits 21:20: Hours tens (0–2). */
    RTC_TR_PM  = 22  /**< Bit 22: AM/PM notation (0 = AM, 1 = PM). Only relevant in 12-hour mode. */
} RTC_TR_te;

/**
 * @brief RTC_DR register bit positions (all fields BCD-encoded).
 */
typedef enum
{
    RTC_DR_DU  = 0,  /**< Bits 3:0:  Date units (1–9). */
    RTC_DR_DT  = 4,  /**< Bits 5:4:  Date tens (0–3). */
    RTC_DR_MU  = 8,  /**< Bits 11:8: Month units (1–9). */
    RTC_DR_MT  = 12, /**< Bit 12:    Month tens (0 or 1). */
    RTC_DR_WDU = 13, /**< Bits 15:13: Weekday units (1 = Monday … 7 = Sunday). */
    RTC_DR_YU  = 16, /**< Bits 19:16: Year units (0–9). */
    RTC_DR_YT  = 20  /**< Bits 23:20: Year tens (0–9). Year is 2000 + (YT×10 + YU). */
} RTC_DR_te;

/**
 * @brief RTC_PRER register bit positions.
 */
typedef enum
{
    RTC_PRER_PREDIV_S = 0,  /**< Bits 14:0:  Synchronous prescaler factor. f_ck_spre = f_ck_apre / (PREDIV_S + 1). */
    RTC_PRER_PREDIV_A = 16  /**< Bits 22:16: Asynchronous prescaler factor. f_ck_apre = LSE / (PREDIV_A + 1). */
} RTC_PRER_te;

/**
 * @brief SPI_CR1 register bit positions.
 */
typedef enum
{
    SPI_CR1_CPHA     = 0,  /**< Bit 0:     Clock phase (0 = first edge, 1 = second edge). */
    SPI_CR1_CPOL     = 1,  /**< Bit 1:     Clock polarity (0 = idle low, 1 = idle high). */
    SPI_CR1_MSTR     = 2,  /**< Bit 2:     Master selection (0 = slave, 1 = master). */
    SPI_CR1_BR       = 3,  /**< Bits 5:3:  Baud rate control (SCK = PCLK / 2^(BR+1)). */
    SPI_CR1_SPE      = 6,  /**< Bit 6:     SPI enable. */
    SPI_CR1_LSBFIRST = 7,  /**< Bit 7:     Frame format (0 = MSB first, 1 = LSB first). */
    SPI_CR1_SSI      = 8,  /**< Bit 8:     Internal slave select (must be 1 in master SW-NSS mode). */
    SPI_CR1_SSM      = 9,  /**< Bit 9:     Software slave management (0 = HW NSS, 1 = SW NSS). */
    SPI_CR1_RXONLY   = 10, /**< Bit 10:    Receive-only mode. */
    SPI_CR1_DFF      = 11, /**< Bit 11:    Data frame format (0 = 8-bit, 1 = 16-bit). */
    SPI_CR1_CRCNEXT  = 12, /**< Bit 12:    Transmit CRC next. */
    SPI_CR1_CRCEN    = 13, /**< Bit 13:    Hardware CRC calculation enable. */
    SPI_CR1_BIDIOE   = 14, /**< Bit 14:    Bidirectional data mode output enable. */
    SPI_CR1_BIDIMODE = 15  /**< Bit 15:    Bidirectional data mode enable. */
} SPI_CR1_te;
/**
 * @brief SPI_CR2 register bit positions.
 */
typedef enum
{
    SPI_CR2_RXDMAEN = 0, /**< Bit 0: Rx buffer DMA enable. */
    SPI_CR2_TXDMAEN = 1, /**< Bit 1: Tx buffer DMA enable. */
    SPI_CR2_SSOE    = 2, /**< Bit 2: SS output enable (master HW-NSS mode). */
    SPI_CR2_FRF     = 4, /**< Bit 4: Frame format (0 = Motorola, 1 = TI). */
    SPI_CR2_ERRIE   = 5, /**< Bit 5: Error interrupt enable. */
    SPI_CR2_RXNEIE  = 6, /**< Bit 6: RXNE interrupt enable. */
    SPI_CR2_TXEIE   = 7  /**< Bit 7: TXE interrupt enable. */
} SPI_CR2_te;

/**
 * @brief SPI_SR register bit positions.
 */
typedef enum
{
    SPI_SR_RXNE   = 0, /**< Bit 0: Receive buffer not empty. */
    SPI_SR_TXE    = 1, /**< Bit 1: Transmit buffer empty. */
    SPI_SR_CHSIDE = 2, /**< Bit 2: Channel side (I2S). */
    SPI_SR_UDR    = 3, /**< Bit 3: Underrun flag (I2S). */
    SPI_SR_CRCERR = 4, /**< Bit 4: CRC error flag. */
    SPI_SR_MODF   = 5, /**< Bit 5: Mode fault (NSS low in master mode). */
    SPI_SR_OVR    = 6, /**< Bit 6: Overrun flag. */
    SPI_SR_BSY    = 7, /**< Bit 7: Busy flag (transfer in progress). */
    SPI_SR_FRE    = 8  /**< Bit 8: Frame format error (TI mode). */
} SPI_SR_te;

/**
 * @brief RCC_CFGR register bit positions.
 */
typedef enum
{
    RCC_CFGR_SWS   = 2,  /**< Bits 3:2:  System clock switch status (read-only). 0=HSI, 1=HSE, 2=PLL. */
    RCC_CFGR_HPRE  = 4,  /**< Bits 7:4:  AHB prescaler. Values ≤ 7 = /1; 8–11 = /2,4,8,16; 12–15 = /64,128,256,512. */
    RCC_CFGR_PPRE1 = 10, /**< Bits 12:10: APB1 (low-speed) prescaler. < 4 = /1; ≥ 4 = /2,4,8,16. */
    RCC_CFGR_PPRE2 = 13  /**< Bits 15:13: APB2 (high-speed) prescaler. Same encoding as PPRE1. */
} RCC_CFGR_te;

/**
 * @brief I2C_CR1 register bit positions.
 */
typedef enum
{
    I2C_CR1_PE        = 0,  /**< Bit 0:  Peripheral enable. */
    I2C_CR1_SMBUS     = 1,  /**< Bit 1:  SMBus mode. */
    I2C_CR1_SMBTYPE   = 3,  /**< Bit 3:  SMBus type (0 = device, 1 = host). */
    I2C_CR1_ENARP     = 4,  /**< Bit 4:  ARP enable. */
    I2C_CR1_ENPEC     = 5,  /**< Bit 5:  PEC enable. */
    I2C_CR1_ENGC      = 6,  /**< Bit 6:  General call enable. */
    I2C_CR1_NOSTRETCH = 7,  /**< Bit 7:  Clock stretching disable (slave mode). */
    I2C_CR1_START     = 8,  /**< Bit 8:  Start generation. */
    I2C_CR1_STOP      = 9,  /**< Bit 9:  Stop generation. */
    I2C_CR1_ACK       = 10, /**< Bit 10: Acknowledge enable. */
    I2C_CR1_POS       = 11, /**< Bit 11: Acknowledge/PEC position (for 2-byte reception). */
    I2C_CR1_PEC       = 12, /**< Bit 12: Packet error checking. */
    I2C_CR1_ALERT     = 13, /**< Bit 13: SMBus alert. */
    I2C_CR1_SWRST     = 15  /**< Bit 15: Software reset. */
} I2C_CR1_te;

/**
 * @brief I2C_CR2 register bit positions.
 */
typedef enum
{
    I2C_CR2_FREQ    = 0,  /**< Bits 5:0:  APB1 clock frequency in MHz (e.g. 16 for 16 MHz). */
    I2C_CR2_ITERREN = 8,  /**< Bit 8:     Error interrupt enable. */
    I2C_CR2_ITEVTEN = 9,  /**< Bit 9:     Event interrupt enable. */
    I2C_CR2_ITBUFEN = 10, /**< Bit 10:    Buffer interrupt enable. */
    I2C_CR2_DMAEN   = 11, /**< Bit 11:    DMA request enable. */
    I2C_CR2_LAST    = 12  /**< Bit 12:    DMA last transfer (for generating NACK on final byte). */
} I2C_CR2_te;

/**
 * @brief I2C_SR1 register bit positions.
 */
typedef enum
{
    I2C_SR1_SB       = 0,  /**< Bit 0:  Start bit generated. Read SR1 to clear. */
    I2C_SR1_ADDR     = 1,  /**< Bit 1:  Address sent/matched. Read SR1 then SR2 to clear. */
    I2C_SR1_BTF      = 2,  /**< Bit 2:  Byte transfer finished. */
    I2C_SR1_ADD10    = 3,  /**< Bit 3:  10-bit header sent. */
    I2C_SR1_STOPF    = 4,  /**< Bit 4:  Stop detection (slave mode). */
    I2C_SR1_RxNE     = 6,  /**< Bit 6:  Data register not empty (receiver). */
    I2C_SR1_TxE      = 7,  /**< Bit 7:  Data register empty (transmitter). */
    I2C_SR1_BERR     = 8,  /**< Bit 8:  Bus error. */
    I2C_SR1_ARLO     = 9,  /**< Bit 9:  Arbitration lost. */
    I2C_SR1_AF       = 10, /**< Bit 10: Acknowledge failure. */
    I2C_SR1_OVR      = 11, /**< Bit 11: Overrun/underrun. */
    I2C_SR1_PECERR   = 12, /**< Bit 12: PEC error in reception. */
    I2C_SR1_TIMEOUT  = 14, /**< Bit 14: Timeout or Tlow error. */
    I2C_SR1_SMBALERT = 15  /**< Bit 15: SMBus alert. */
} I2C_SR1_te;

/**
 * @brief I2C_SR2 register bit positions.
 */
typedef enum
{
    I2C_SR2_MSL        = 0, /**< Bit 0:    Master/slave (1 = master mode). */
    I2C_SR2_BUSY       = 1, /**< Bit 1:    Bus busy. */
    I2C_SR2_TRA        = 2, /**< Bit 2:    Transmitter/receiver (1 = transmitter). */
    I2C_SR2_GENCALL    = 4, /**< Bit 4:    General call address received. */
    I2C_SR2_SMBDEFAULT = 5, /**< Bit 5:    SMBus device default address. */
    I2C_SR2_SMBHOST    = 6, /**< Bit 6:    SMBus host header. */
    I2C_SR2_DUALF      = 7, /**< Bit 7:    Dual flag (address matched OAR2). */
    I2C_SR2_PEC        = 8  /**< Bits 15:8: Packet error checking register. */
} I2C_SR2_te;

/**
 * @brief I2C_CCR register bit positions.
 */
typedef enum
{
    I2C_CCR_CCR  = 0,  /**< Bits 11:0: Clock control value. Standard: CCR = f_PCLK1/(2×f_SCL). Fast: CCR = f_PCLK1/(25×f_SCL) with DUTY=1. */
    I2C_CCR_DUTY = 14, /**< Bit 14:    Fast mode duty cycle (0 = t_low/t_high = 2, 1 = 16/9). */
    I2C_CCR_FS   = 15  /**< Bit 15:    I2C master mode selection (0 = standard, 1 = fast). */
} I2C_CCR_te;

/**
 * @brief I2C_OAR1 register bit positions.
 */
typedef enum
{
    I2C_OAR1_ADD0    = 0,  /**< Bit 0:     LSB of 10-bit address. */
    I2C_OAR1_ADD7_1  = 1,  /**< Bits 7:1:  7-bit address or bits 7:1 of 10-bit address. */
    I2C_OAR1_ADD9_8  = 8,  /**< Bits 9:8:  Bits 9:8 of 10-bit address. */
    I2C_OAR1_ADDMODE = 15  /**< Bit 15:    Addressing mode (0 = 7-bit, 1 = 10-bit). */
} I2C_OAR1_te;

/**
 * @brief USART_SR register bit positions.
 */
typedef enum
{
    USART_SR_PE   = 0, /**< Bit 0: Parity error. */
    USART_SR_FE   = 1, /**< Bit 1: Framing error. */
    USART_SR_NF   = 2, /**< Bit 2: Noise detected. */
    USART_SR_ORE  = 3, /**< Bit 3: Overrun error. */
    USART_SR_IDLE = 4, /**< Bit 4: IDLE line detected. */
    USART_SR_RXNE = 5, /**< Bit 5: Read data register not empty (data received). */
    USART_SR_TC   = 6, /**< Bit 6: Transmission complete (all bits shifted out). */
    USART_SR_TXE  = 7, /**< Bit 7: Transmit data register empty (ready for next byte). */
    USART_SR_LBD  = 8, /**< Bit 8: LIN break detection flag. */
    USART_SR_CTS  = 9  /**< Bit 9: CTS flag. */
} USART_SR_te;

/**
 * @brief USART_BRR register bit positions.
 */
typedef enum
{
    USART_BRR_DIV_FRACTION = 0, /**< Bits 3:0:  Fractional part of USARTDIV. */
    USART_BRR_DIV_MANTISSA = 4  /**< Bits 15:4: Mantissa of USARTDIV. */
} USART_BRR_te;

/**
 * @brief USART_CR1 register bit positions.
 */
typedef enum
{
    USART_CR1_SBK    = 0,  /**< Bit 0:  Send break. */
    USART_CR1_RWU    = 1,  /**< Bit 1:  Receiver wakeup. */
    USART_CR1_RE     = 2,  /**< Bit 2:  Receiver enable. */
    USART_CR1_TE     = 3,  /**< Bit 3:  Transmitter enable. */
    USART_CR1_IDLEIE = 4,  /**< Bit 4:  IDLE interrupt enable. */
    USART_CR1_RXNEIE = 5,  /**< Bit 5:  RXNE interrupt enable. */
    USART_CR1_TCIE   = 6,  /**< Bit 6:  Transmission complete interrupt enable. */
    USART_CR1_TXEIE  = 7,  /**< Bit 7:  TXE interrupt enable. */
    USART_CR1_PEIE   = 8,  /**< Bit 8:  PE interrupt enable. */
    USART_CR1_PS     = 9,  /**< Bit 9:  Parity selection (0 = even, 1 = odd). */
    USART_CR1_PCE    = 10, /**< Bit 10: Parity control enable. */
    USART_CR1_WAKE   = 11, /**< Bit 11: Wakeup method. */
    USART_CR1_M      = 12, /**< Bit 12: Word length (0 = 8 bits, 1 = 9 bits). */
    USART_CR1_UE     = 13, /**< Bit 13: USART enable. */
    USART_CR1_OVER8  = 15  /**< Bit 15: Oversampling mode (0 = ×16, 1 = ×8). */
} USART_CR1_te;

/**
 * @brief USART_CR2 register bit positions.
 */
typedef enum
{
    USART_CR2_ADD   = 0,  /**< Bits 3:0:  Address of the USART node. */
    USART_CR2_LBDL  = 5,  /**< Bit 5:     LIN break detection length. */
    USART_CR2_LBDIE = 6,  /**< Bit 6:     LIN break detection interrupt enable. */
    USART_CR2_LBCL  = 8,  /**< Bit 8:     Last bit clock pulse (sync mode). */
    USART_CR2_CPHA  = 9,  /**< Bit 9:     Clock phase (sync mode). */
    USART_CR2_CPOL  = 10, /**< Bit 10:    Clock polarity (sync mode). */
    USART_CR2_CLKEN = 11, /**< Bit 11:    Clock enable (sync mode). */
    USART_CR2_STOP  = 12, /**< Bits 13:12: Stop bits (0=1, 1=0.5, 2=2, 3=1.5). */
    USART_CR2_LINEN = 14  /**< Bit 14:    LIN mode enable. */
} USART_CR2_te;

/**
 * @brief USART_CR3 register bit positions.
 */
typedef enum
{
    USART_CR3_EIE    = 0,  /**< Bit 0:  Error interrupt enable. */
    USART_CR3_IREN   = 1,  /**< Bit 1:  IrDA mode enable. */
    USART_CR3_IRLP   = 2,  /**< Bit 2:  IrDA low-power. */
    USART_CR3_HDSEL  = 3,  /**< Bit 3:  Half-duplex selection. */
    USART_CR3_NACK   = 4,  /**< Bit 4:  Smartcard NACK enable. */
    USART_CR3_SCEN   = 5,  /**< Bit 5:  Smartcard mode enable. */
    USART_CR3_DMAR   = 6,  /**< Bit 6:  DMA enable receiver. */
    USART_CR3_DMAT   = 7,  /**< Bit 7:  DMA enable transmitter. */
    USART_CR3_RTSE   = 8,  /**< Bit 8:  RTS enable. */
    USART_CR3_CTSE   = 9,  /**< Bit 9:  CTS enable. */
    USART_CR3_CTSIE  = 10, /**< Bit 10: CTS interrupt enable. */
    USART_CR3_ONEBIT = 11  /**< Bit 11: One sample bit method enable (0 = 3-sample, 1 = 1-sample). */
} USART_CR3_te;

/**
 * @brief RCC_AHB1ENR register bit positions.
 */
typedef enum
{
    RCC_AHB1ENR_GPIOAEN = 0,  /**< Bit 0:  GPIOA clock enable. */
    RCC_AHB1ENR_GPIOBEN = 1,  /**< Bit 1:  GPIOB clock enable. */
    RCC_AHB1ENR_GPIOCEN = 2,  /**< Bit 2:  GPIOC clock enable. */
    RCC_AHB1ENR_GPIODEN = 3,  /**< Bit 3:  GPIOD clock enable. */
    RCC_AHB1ENR_GPIOEEN = 4,  /**< Bit 4:  GPIOE clock enable. */
    RCC_AHB1ENR_GPIOHEN = 7,  /**< Bit 7:  GPIOH clock enable. */
    RCC_AHB1ENR_CRCEN   = 12, /**< Bit 12: CRC clock enable. */
    RCC_AHB1ENR_DMA1EN  = 21, /**< Bit 21: DMA1 clock enable. */
    RCC_AHB1ENR_DMA2EN  = 22  /**< Bit 22: DMA2 clock enable. */
} RCC_AHB1ENR_te;

/**
 * @brief RCC_AHB1RSTR register bit positions.
 */
typedef enum
{
    RCC_AHB1RSTR_GPIOARST = 0,  /**< Bit 0:  GPIOA reset. */
    RCC_AHB1RSTR_GPIOBRST = 1,  /**< Bit 1:  GPIOB reset. */
    RCC_AHB1RSTR_GPIOCRST = 2,  /**< Bit 2:  GPIOC reset. */
    RCC_AHB1RSTR_GPIODRST = 3,  /**< Bit 3:  GPIOD reset. */
    RCC_AHB1RSTR_GPIOERST = 4,  /**< Bit 4:  GPIOE reset. */
    RCC_AHB1RSTR_GPIOHRST = 7,  /**< Bit 7:  GPIOH reset. */
    RCC_AHB1RSTR_CRCRST   = 12, /**< Bit 12: CRC reset. */
    RCC_AHB1RSTR_DMA1RST  = 21, /**< Bit 21: DMA1 reset. */
    RCC_AHB1RSTR_DMA2RST  = 22  /**< Bit 22: DMA2 reset. */
} RCC_AHB1RSTR_te;

/**
 * @brief RCC_APB1ENR register bit positions.
 */
typedef enum
{
    RCC_APB1ENR_TIM2EN   = 0,  /**< Bit 0:  TIM2 clock enable. */
    RCC_APB1ENR_TIM3EN   = 1,  /**< Bit 1:  TIM3 clock enable. */
    RCC_APB1ENR_TIM4EN   = 2,  /**< Bit 2:  TIM4 clock enable. */
    RCC_APB1ENR_TIM5EN   = 3,  /**< Bit 3:  TIM5 clock enable. */
    RCC_APB1ENR_WWDGEN   = 11, /**< Bit 11: WWDG clock enable. */
    RCC_APB1ENR_SPI2EN   = 14, /**< Bit 14: SPI2 clock enable. */
    RCC_APB1ENR_SPI3EN   = 15, /**< Bit 15: SPI3 clock enable. */
    RCC_APB1ENR_USART2EN = 17, /**< Bit 17: USART2 clock enable. */
    RCC_APB1ENR_I2C1EN   = 21, /**< Bit 21: I2C1 clock enable. */
    RCC_APB1ENR_I2C2EN   = 22, /**< Bit 22: I2C2 clock enable. */
    RCC_APB1ENR_I2C3EN   = 23, /**< Bit 23: I2C3 clock enable. */
    RCC_APB1ENR_PWREN    = 28  /**< Bit 28: Power interface clock enable. */
} RCC_APB1ENR_te;

/**
 * @brief RCC_APB1RSTR register bit positions.
 */
typedef enum
{
    RCC_APB1RSTR_TIM2RST   = 0,  /**< Bit 0:  TIM2 reset. */
    RCC_APB1RSTR_TIM3RST   = 1,  /**< Bit 1:  TIM3 reset. */
    RCC_APB1RSTR_TIM4RST   = 2,  /**< Bit 2:  TIM4 reset. */
    RCC_APB1RSTR_TIM5RST   = 3,  /**< Bit 3:  TIM5 reset. */
    RCC_APB1RSTR_WWDGRST   = 11, /**< Bit 11: WWDG reset. */
    RCC_APB1RSTR_SPI2RST   = 14, /**< Bit 14: SPI2 reset. */
    RCC_APB1RSTR_SPI3RST   = 15, /**< Bit 15: SPI3 reset. */
    RCC_APB1RSTR_USART2RST = 17, /**< Bit 17: USART2 reset. */
    RCC_APB1RSTR_I2C1RST   = 21, /**< Bit 21: I2C1 reset. */
    RCC_APB1RSTR_I2C2RST   = 22, /**< Bit 22: I2C2 reset. */
    RCC_APB1RSTR_I2C3RST   = 23, /**< Bit 23: I2C3 reset. */
    RCC_APB1RSTR_PWRRST    = 28  /**< Bit 28: Power interface reset. */
} RCC_APB1RSTR_te;

/**
 * @brief RCC_APB2ENR register bit positions.
 */
typedef enum
{
    RCC_APB2ENR_TIM1EN   = 0,  /**< Bit 0:  TIM1 clock enable. */
    RCC_APB2ENR_USART1EN = 4,  /**< Bit 4:  USART1 clock enable. */
    RCC_APB2ENR_USART6EN = 5,  /**< Bit 5:  USART6 clock enable. */
    RCC_APB2ENR_ADC1EN   = 8,  /**< Bit 8:  ADC1 clock enable. */
    RCC_APB2ENR_SDIOEN   = 11, /**< Bit 11: SDIO clock enable. */
    RCC_APB2ENR_SPI1EN   = 12, /**< Bit 12: SPI1 clock enable. */
    RCC_APB2ENR_SPI4EN   = 13, /**< Bit 13: SPI4 clock enable. */
    RCC_APB2ENR_SYSCFGEN = 14, /**< Bit 14: SYSCFG clock enable. */
    RCC_APB2ENR_TIM9EN   = 16, /**< Bit 16: TIM9 clock enable. */
    RCC_APB2ENR_TIM10EN  = 17, /**< Bit 17: TIM10 clock enable. */
    RCC_APB2ENR_TIM11EN  = 18  /**< Bit 18: TIM11 clock enable. */
} RCC_APB2ENR_te;

/**
 * @brief RCC_APB2RSTR register bit positions.
 */
typedef enum
{
    RCC_APB2RSTR_TIM1RST   = 0,  /**< Bit 0:  TIM1 reset. */
    RCC_APB2RSTR_USART1RST = 4,  /**< Bit 4:  USART1 reset. */
    RCC_APB2RSTR_USART6RST = 5,  /**< Bit 5:  USART6 reset. */
    RCC_APB2RSTR_ADC1RST   = 8,  /**< Bit 8:  ADC1 reset. */
    RCC_APB2RSTR_SDIORST   = 11, /**< Bit 11: SDIO reset. */
    RCC_APB2RSTR_SPI1RST   = 12, /**< Bit 12: SPI1 reset. */
    RCC_APB2RSTR_SPI4RST   = 13, /**< Bit 13: SPI4 reset. */
    RCC_APB2RSTR_SYSCFGRST = 14, /**< Bit 14: SYSCFG reset. */
    RCC_APB2RSTR_TIM9RST   = 16, /**< Bit 16: TIM9 reset. */
    RCC_APB2RSTR_TIM10RST  = 17, /**< Bit 17: TIM10 reset. */
    RCC_APB2RSTR_TIM11RST  = 18  /**< Bit 18: TIM11 reset. */
} RCC_APB2RSTR_te;

/** @} */

/**
 * @defgroup stm32f401re_accessors Peripheral Accessor Macros
 * @ingroup stm32f401re_hal
 * @brief Pointer-cast macros for accessing peripheral register maps by name.
 * @{
 */
#define RCC    ((RCC_REGDEF_ts*)ADDR_RCC)       /**< RCC register map accessor.    */
#define EXTI   ((EXTI_REGDEF_ts*)ADDR_EXTI)     /**< EXTI register map accessor.   */
#define SYSCFG ((SYSCFG_REGDEF_ts*)ADDR_SYSCFG) /**< SYSCFG register map accessor. */
#define RTC    ((RTC_REGDEF_ts*)ADDR_RTC)        /**< RTC register map accessor.    */
#define PWR    ((PWR_REGDEF_ts*)ADDR_PWR)        /**< PWR register map accessor.    */
#define GPIOA  ((GPIO_REGDEF_ts*)ADDR_GPIOA)     /**< GPIOA register map accessor.  */
#define GPIOB  ((GPIO_REGDEF_ts*)ADDR_GPIOB)     /**< GPIOB register map accessor.  */
#define GPIOC  ((GPIO_REGDEF_ts*)ADDR_GPIOC)     /**< GPIOC register map accessor.  */
#define GPIOD  ((GPIO_REGDEF_ts*)ADDR_GPIOD)     /**< GPIOD register map accessor.  */
#define GPIOE  ((GPIO_REGDEF_ts*)ADDR_GPIOE)     /**< GPIOE register map accessor.  */
#define GPIOH  ((GPIO_REGDEF_ts*)ADDR_GPIOH)     /**< GPIOH register map accessor.  */
#define I2C1   ((I2C_REGDEF_ts*)ADDR_I2C1)       /**< I2C1 register map accessor.   */
#define I2C2   ((I2C_REGDEF_ts*)ADDR_I2C2)       /**< I2C2 register map accessor.   */
#define I2C3   ((I2C_REGDEF_ts*)ADDR_I2C3)       /**< I2C3 register map accessor.   */
#define SPI1   ((SPI_REGDEF_ts*)ADDR_SPI1)        /**< SPI1 register map accessor.   */
#define SPI2   ((SPI_REGDEF_ts*)ADDR_SPI2)        /**< SPI2 register map accessor.   */
#define SPI3   ((SPI_REGDEF_ts*)ADDR_SPI3)        /**< SPI3 register map accessor.   */
#define SPI4   ((SPI_REGDEF_ts*)ADDR_SPI4)        /**< SPI4 register map accessor.   */
#define USART1 ((USART_REGDEF_ts*)ADDR_USART1)    /**< USART1 register map accessor. */
#define USART2 ((USART_REGDEF_ts*)ADDR_USART2)    /**< USART2 register map accessor. */
#define USART6 ((USART_REGDEF_ts*)ADDR_USART6)    /**< USART6 register map accessor. */
/** @} */

/**
 * @defgroup stm32f401re_irq IRQ Numbers and EXTI Lines
 * @ingroup stm32f401re_hal
 * @brief IRQ position enumeration and EXTI line definitions.
 * @{
 */

/**
 * @brief IRQ numbers in the STM32F401RE Cortex-M4 vector table.
 *
 * @details
 * Values correspond to the position offset from the first device IRQ (IRQ0),
 * used directly as the argument to @ref nvic_set_interrupt.
 */
typedef enum
{
    WWDG_IRQn               = 0,  /**< Window watchdog interrupt. */
    EXTI16_IRQn             = 1,  /**< PVD through EXTI line 16. */
    EXTI21_IRQn             = 2,  /**< Tamper and timestamp through EXTI line 21. */
    EXTI22_IRQn             = 3,  /**< RTC wakeup through EXTI line 22. */
    FLASH_IRQn              = 4,  /**< Flash global interrupt. */
    RCC_IRQn                = 5,  /**< RCC global interrupt. */
    EXTI0_IRQn              = 6,  /**< EXTI line 0 interrupt. */
    EXTI1_IRQn              = 7,  /**< EXTI line 1 interrupt. */
    EXTI2_IRQn              = 8,  /**< EXTI line 2 interrupt. */
    EXTI3_IRQn              = 9,  /**< EXTI line 3 interrupt. */
    EXTI4_IRQn              = 10, /**< EXTI line 4 interrupt. */
    DMA1_Stream0_IRQn       = 11, /**< DMA1 stream 0 global interrupt. */
    DMA1_Stream1_IRQn       = 12, /**< DMA1 stream 1 global interrupt. */
    DMA1_Stream2_IRQn       = 13, /**< DMA1 stream 2 global interrupt. */
    DMA1_Stream3_IRQn       = 14, /**< DMA1 stream 3 global interrupt. */
    DMA1_Stream4_IRQn       = 15, /**< DMA1 stream 4 global interrupt. */
    DMA1_Stream5_IRQn       = 16, /**< DMA1 stream 5 global interrupt. */
    DMA1_Stream6_IRQn       = 17, /**< DMA1 stream 6 global interrupt. */
    ADC_IRQn                = 18, /**< ADC1 global interrupt. */
    EXTI9_5_IRQn            = 23, /**< EXTI lines 5–9 shared interrupt. */
    TIM1_BRK_TIM9_IRQn     = 24, /**< TIM1 break / TIM9 global interrupt. */
    TIM1_UP_TIM10_IRQn     = 25, /**< TIM1 update / TIM10 global interrupt. */
    TIM1_TRG_COM_TIM11_IRQn = 26,/**< TIM1 trigger+commutation / TIM11 global interrupt. */
    TIM1_CC_IRQn            = 27, /**< TIM1 capture compare interrupt. */
    TIM2_IRQn               = 28, /**< TIM2 global interrupt. */
    TIM3_IRQn               = 29, /**< TIM3 global interrupt. */
    TIM4_IRQn               = 30, /**< TIM4 global interrupt. */
    I2C1_EV_IRQn            = 31, /**< I2C1 event interrupt. */
    I2C1_ER_IRQn            = 32, /**< I2C1 error interrupt. */
    I2C2_EV_IRQn            = 33, /**< I2C2 event interrupt. */
    I2C2_ER_IRQn            = 34, /**< I2C2 error interrupt. */
    SPI1_IRQn               = 35, /**< SPI1 global interrupt. */
    SPI2_IRQn               = 36, /**< SPI2 global interrupt. */
    USART1_IRQn             = 37, /**< USART1 global interrupt. */
    USART2_IRQn             = 38, /**< USART2 global interrupt. */
    EXTI15_10_IRQn          = 40, /**< EXTI lines 10–15 shared interrupt. */
    EXTI17_IRQn             = 41, /**< RTC alarm through EXTI line 17. */
    EXTI18_IRQn             = 42, /**< USB OTG FS wakeup through EXTI line 18. */
    DMA1_Stream7_IRQn       = 47, /**< DMA1 stream 7 global interrupt. */
    SDIO_IRQn               = 49, /**< SDIO global interrupt. */
    TIM5_IRQn               = 50, /**< TIM5 global interrupt. */
    SPI3_IRQn               = 51, /**< SPI3 global interrupt. */
    DMA2_Stream0_IRQn       = 56, /**< DMA2 stream 0 global interrupt. */
    DMA2_Stream1_IRQn       = 57, /**< DMA2 stream 1 global interrupt. */
    DMA2_Stream2_IRQn       = 58, /**< DMA2 stream 2 global interrupt. */
    DMA2_Stream3_IRQn       = 59, /**< DMA2 stream 3 global interrupt. */
    DMA2_Stream4_IRQn       = 60, /**< DMA2 stream 4 global interrupt. */
    OTG_FS_IRQn             = 67, /**< USB OTG FS global interrupt. */
    DMA2_Stream5_IRQn       = 68, /**< DMA2 stream 5 global interrupt. */
    DMA2_Stream6_IRQn       = 69, /**< DMA2 stream 6 global interrupt. */
    DMA2_Stream7_IRQn       = 70, /**< DMA2 stream 7 global interrupt. */
    USART6_IRQn             = 71, /**< USART6 global interrupt. */
    I2C3_EV_IRQn            = 72, /**< I2C3 event interrupt. */
    I2C3_ER_IRQn            = 73, /**< I2C3 error interrupt. */
    FPU_IRQn                = 81, /**< FPU global interrupt. */
    SPI4_IRQn               = 84  /**< SPI4 global interrupt. */
} IRQn_te;

/**
 * @brief EXTI line numbers.
 *
 * @details
 * Used with @ref gpio_clear_interrupt to identify which EXTI pending flag to clear.
 * Lines 0–15 correspond to GPIO pins 0–15. Lines 16–22 are connected to
 * internal peripherals (PVD, RTC, USB, etc.).
 */
typedef enum {
    EXTI_LINE_0,        /**< EXTI line 0  (GPIO pin 0). */
    EXTI_LINE_1,        /**< EXTI line 1  (GPIO pin 1). */
    EXTI_LINE_2,        /**< EXTI line 2  (GPIO pin 2). */
    EXTI_LINE_3,        /**< EXTI line 3  (GPIO pin 3). */
    EXTI_LINE_4,        /**< EXTI line 4  (GPIO pin 4). */
    EXTI_LINE_5,        /**< EXTI line 5  (GPIO pin 5). */
    EXTI_LINE_6,        /**< EXTI line 6  (GPIO pin 6). */
    EXTI_LINE_7,        /**< EXTI line 7  (GPIO pin 7). */
    EXTI_LINE_8,        /**< EXTI line 8  (GPIO pin 8). */
    EXTI_LINE_9,        /**< EXTI line 9  (GPIO pin 9). */
    EXTI_LINE_10,       /**< EXTI line 10 (GPIO pin 10). */
    EXTI_LINE_11,       /**< EXTI line 11 (GPIO pin 11). */
    EXTI_LINE_12,       /**< EXTI line 12 (GPIO pin 12). */
    EXTI_LINE_13,       /**< EXTI line 13 (GPIO pin 13). */
    EXTI_LINE_14,       /**< EXTI line 14 (GPIO pin 14). */
    EXTI_LINE_15,       /**< EXTI line 15 (GPIO pin 15). */
    EXTI_LINE_16,       /**< EXTI line 16 (PVD output). */
    EXTI_LINE_17,       /**< EXTI line 17 (RTC alarm). */
    EXTI_LINE_18,       /**< EXTI line 18 (USB OTG FS wakeup). */
    EXTI_LINE_21 = 21,  /**< EXTI line 21 (RTC tamper and timestamp). */
    EXTI_LINE_22        /**< EXTI line 22 (RTC wakeup timer). */
} EXTI_LINES_te;

/**
 * @brief SYSCFG port codes for EXTICRx routing.
 *
 * @details
 * Written into the SYSCFG_EXTICRx 4-bit fields to select which GPIO port
 * is routed to an EXTI line. Used internally by @ref gpio_init.
 * Note: PH = 7 to match the hardware encoding for GPIOH.
 */
typedef enum {
    PA = 0, /**< Port A. */
    PB = 1, /**< Port B. */
    PC = 2, /**< Port C. */
    PD = 3, /**< Port D. */
    PE = 4, /**< Port E. */
    PH = 7  /**< Port H (value 7 to match hardware encoding). */
} PORT_CODES_ts;

/** @} */

/**
 * @defgroup stm32f401re_legacy_macros Legacy GPIO Clock Macros
 * @ingroup stm32f401re_hal
 * @brief Convenience macros for GPIO peripheral clock enable/disable.
 *
 * @note These macros are superseded by @ref rcc_set_pclk_ahb1. Prefer the
 *       function-based API in new code.
 * @{
 */
#define GPIOA_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 0)  /**< Enable GPIOA clock. */
#define GPIOB_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 1)  /**< Enable GPIOB clock. */
#define GPIOC_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 2)  /**< Enable GPIOC clock. */
#define GPIOD_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 3)  /**< Enable GPIOD clock. */
#define GPIOE_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 4)  /**< Enable GPIOE clock. */
#define GPIOH_CLK_EN()  (RCC->RCC_AHB1ENR |= 0b1 << 7)  /**< Enable GPIOH clock. */

#define GPIOA_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 0)) /**< Disable GPIOA clock. */
#define GPIOB_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 1)) /**< Disable GPIOB clock. */
#define GPIOC_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 2)) /**< Disable GPIOC clock. */
#define GPIOD_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 3)) /**< Disable GPIOD clock. */
#define GPIOE_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 4)) /**< Disable GPIOE clock. */
#define GPIOH_CLK_DIS() (RCC->RCC_AHB1ENR |= ~(0b1 << 7)) /**< Disable GPIOH clock. */
/** @} */

#endif

/** @} */