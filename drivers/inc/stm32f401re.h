/**
 * @file stm32f401re.h
 * @author github.com/Baksi675
 * @brief MCU specific header file
 * @version 0.1
 * @date 2026-01-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef STM32F401RE_H_
#define STM32F401RE_H_

#include <stdint.h>

/**
 * @brief APB1 peripheral addresses
 * 
 */
#define ADDR_APB1               (0x40000000U)
#define ADDR_SPI3				(ADDR_APB1 + 0x3C00)
#define ADDR_SPI2				(ADDR_APB1 + 0x3800)
#define ADDR_RTC				(ADDR_APB1 + 0x2800)
#define ADDR_PWR				(ADDR_APB1 + 0x7000)
#define ADDR_I2C3				(ADDR_APB1 + 0x5C00)
#define ADDR_I2C2				(ADDR_APB1 + 0x5800)
#define ADDR_I2C1				(ADDR_APB1 + 0x5400)
#define ADDR_USART2				(ADDR_APB1 + 0x4400)

/**
 * @brief APB2 peripheral addresses
 * 
 */
#define ADDR_APB2               (0x40010000U)
#define ADDR_EXTI				(ADDR_APB2 + 0x3C00)
#define ADDR_SYSCFG				(ADDR_APB2 + 0x3800)
#define ADDR_SPI4				(ADDR_APB2 + 0x3400)
#define ADDR_SPI1				(ADDR_APB2 + 0x3000)
#define ADDR_USART6				(ADDR_APB2 + 0x1400)
#define ADDR_USART1				(ADDR_APB2 + 0x1000)

/**
 * @brief AHB1 peripheral addresses
 * 
 */
#define ADDR_AHB1               (0x40020000U)
#define ADDR_GPIOA              (ADDR_AHB1)
#define ADDR_GPIOB              (ADDR_AHB1 + 0x0400)
#define ADDR_GPIOC              (ADDR_AHB1 + 0x0800)
#define ADDR_GPIOD              (ADDR_AHB1 + 0x0C00)
#define ADDR_GPIOE              (ADDR_AHB1 + 0x1000)
#define ADDR_GPIOH              (ADDR_AHB1 + 0x1C00)
#define ADDR_RCC                (ADDR_AHB1 + 0x3800)

/**
 * @brief AHB2 peripheral addresses
 * 
 */
#define ADDR_AHB2               (0x50000000U)

/**
 * @brief GPIO peripheral register definition
 * 
 */
typedef struct {
    uint32_t volatile GPIO_MODER;
    uint32_t volatile GPIO_OTYPER;
    uint32_t volatile GPIO_OSPEEDR;
    uint32_t volatile GPIO_PUPDR;
    uint32_t volatile GPIO_IDR;
    uint32_t volatile GPIO_ODR;
    uint32_t volatile GPIO_BSRR;
    uint32_t volatile GPIO_LCKR;
    uint32_t volatile GPIO_AFR[2];
}GPIO_REGDEF_ts;

/**
 * @brief RCC peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile RCC_CR;
	uint32_t volatile RCC_PLLCFGR;
	uint32_t volatile RCC_CFGR;
	uint32_t volatile RCC_CIR;
	uint32_t volatile RCC_AHB1RSTR;
	uint32_t volatile RCC_AHB2RSTR;
	uint32_t RESERVED0[2];
	uint32_t volatile RCC_APB1RSTR;
	uint32_t volatile RCC_APB2RSTR;
	uint32_t RESERVED1[2];
	uint32_t volatile RCC_AHB1ENR;
	uint32_t volatile RCC_AHB2ENR;
	uint32_t RESERVED2[2];
	uint32_t volatile RCC_APB1ENR;
	uint32_t volatile RCC_APB2ENR;
	uint32_t RESERVED3[2];
	uint32_t volatile RCC_AHB1LPENR;
	uint32_t volatile RCC_AHB2LPENR;
	uint32_t RESERVED4[2];
	uint32_t volatile RCC_APB1LPENR;
	uint32_t volatile RCC_APB2LPENR;
	uint32_t RESERVED5[2];
	uint32_t volatile RCC_BDCR;
	uint32_t volatile RCC_CSR;
	uint32_t RESERVED6[2];
	uint32_t volatile RCC_SSCFGR;
	uint32_t volatile RCC_PLLI2SCFGR;
	uint32_t RESERVED7;
	uint32_t volatile RCC_DCKCFGR;
}RCC_REGDEF_ts;

/**
 * @brief EXTI peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile EXTI_IMR;
	uint32_t volatile EXTI_EMR;
	uint32_t volatile EXTI_RTSR;
	uint32_t volatile EXTI_FTSR;
	uint32_t volatile EXTI_SWIER;
	uint32_t volatile EXTI_PR;
}EXTI_REGDEF_ts;

/**
 * @brief SYSCFG peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile SYSCFG_MEMRMP;
	uint32_t volatile SYSCFG_PMC;
	uint32_t volatile SYSCFG_EXTICR1;
	uint32_t volatile SYSCFG_EXTICR2;
	uint32_t volatile SYSCFG_EXTICR3;
	uint32_t volatile SYSCFG_EXTICR4;
	uint32_t volatile SYSCFG_CMPCR;
}SYSCFG_REGDEF_ts;

/**
 * @brief RTC peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile RTC_TR;
	uint32_t volatile RTC_DR;
	uint32_t volatile RTC_CR;
	uint32_t volatile RTC_ISR;
	uint32_t volatile RTC_PRER;
	uint32_t volatile RTC_WUTR;
	uint32_t volatile RTC_CALIBR;
	uint32_t volatile RTC_ALRMAR;
	uint32_t volatile RTC_ALRMBR;
	uint32_t volatile RTC_WPR;
	uint32_t volatile RTC_SSR;
	uint32_t volatile RTC_TSTR;
	uint32_t volatile RTC_TSSSR;
	uint32_t volatile RTC_CALR;
	uint32_t volatile RTC_TAFCR;
	uint32_t volatile RTC_ALRMASSR;
	uint32_t volatile RTC_ALRMBSSR;
	uint32_t volatile RTC_BKP0R;
	uint32_t volatile RTC_BKP19R;
}RTC_REGDEF_ts;

/**
 * @brief PWR peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile PWR_CR;
	uint32_t volatile PWR_CSR;
}PWR_REGDEF_ts;

/**
 * @brief SPI peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile SPI_CR1;
	uint32_t volatile SPI_CR2;
	uint32_t volatile SPI_SR;
	uint32_t volatile SPI_DR;
	uint32_t volatile SPI_CRCPR;
	uint32_t volatile SPI_TXCRCR;
	uint32_t volatile SPI_RXCRCR;
	uint32_t volatile SPI_I2SCFGR;
	uint32_t volatile SPI_I2SPR;
}SPI_REGDEF_ts;

/**
 * @brief I2C peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile I2C_CR1;
	uint32_t volatile I2C_CR2;
	uint32_t volatile I2C_OAR1;
	uint32_t volatile I2C_OAR2;
	uint32_t volatile I2C_DR;
	uint32_t volatile I2C_SR1;
	uint32_t volatile I2C_SR2;
	uint32_t volatile I2C_CCR;
	uint32_t volatile I2C_TRISE;
	uint32_t volatile I2C_FLTR;
}I2C_REGDEF_ts;

/**
 * @brief USART peripheral register definition
 * 
 */
typedef struct {
	uint32_t volatile USART_SR;
	uint32_t volatile USART_DR;
	uint32_t volatile USART_BRR;
	uint32_t volatile USART_CR1;
	uint32_t volatile USART_CR2;
	uint32_t volatile USART_CR3;
	uint32_t volatile USART_GTPR;
}USART_REGDEF_ts;
    
/**
 * @brief APB1ENR register bit positions
 */
typedef enum
{
    APB1ENR_PWREN = 28
}APB1ENR_te;

/**
 * @brief PWR_CR register bit positions
 */
typedef enum
{
    PWR_CR_DBP = 8
}PWR_CR_te;

/**
 * @brief RCC_CR register bit positions
 */
typedef enum
{
    RCC_CR_HSEON  = 16,
    RCC_CR_HSERDY = 17
}RCC_CR_te;

/**
 * @brief RCC_BDCR register bit positions
 */
typedef enum
{
    RCC_BDCR_LSEON   = 0,
    RCC_BDCR_LSERDY  = 1,
    RCC_BDCR_RTCSEL  = 8,
    RCC_BDCR_RTCEN   = 15
}RCC_BDCR_te;

/**
 * @brief RCC_CSR register bit positions
 */
typedef enum
{
    RCC_CSR_LSION  = 0,
    RCC_CSR_LSIRDY = 1
}RCC_CSR_te;

/**
 * @brief RTC_CR register bit positions
 */
typedef enum
{
    RTC_CR_FMT = 6
}RTC_CR_te;

/**
 * @brief RTC_ISR register bit positions
 */
typedef enum
{
    RTC_ISR_RSF    = 5,
    RTC_ISR_INITF  = 6,
    RTC_ISR_INIT   = 7
}RTC_ISR_te;

/**
 * @brief RTC_TR register bit positions
 */
typedef enum
{
    RTC_TR_SU  = 0,
    RTC_TR_ST  = 4,
    RTC_TR_MNU = 8,
    RTC_TR_MNT = 12,
    RTC_TR_HU  = 16,
    RTC_TR_HT  = 20,
    RTC_TR_PM  = 22
}RTC_TR_te;

/**
 * @brief RTC_DR register bit positions
 */
typedef enum
{
    RTC_DR_DU   = 0,
    RTC_DR_DT   = 4,
    RTC_DR_MU   = 8,
    RTC_DR_MT   = 12,
    RTC_DR_WDU  = 13,
    RTC_DR_YU   = 16,
    RTC_DR_YT   = 20
}RTC_DR_te;

/**
 * @brief RTC_PRER register bit positions
 */
typedef enum
{
    RTC_PRER_PREDIV_S = 0,
    RTC_PRER_PREDIV_A = 16
}RTC_PRER_te;

/**
 * @brief SPI_CR1 register bit positions
 */
typedef enum
{
    SPI_CR1_CPHA       = 0,
    SPI_CR1_CPOL       = 1,
    SPI_CR1_MSTR       = 2,
    SPI_CR1_BR         = 3,
    SPI_CR1_SPE        = 6,
    SPI_CR1_LSBFIRST   = 7,
    SPI_CR1_SSI        = 8,
    SPI_CR1_SSM        = 9,
    SPI_CR1_RXONLY     = 10,
    SPI_CR1_DFF        = 11,
    SPI_CR1_CRCNEXT    = 12,
    SPI_CR1_CRCEN      = 13,
    SPI_CR1_BIDIOE     = 14,
    SPI_CR1_BIDIMODE   = 15
}SPI_CR1_te;

/**
 * @brief SPI_CR2 register bit positions
 */
typedef enum
{
    SPI_CR2_RXDMAEN = 0,
    SPI_CR2_TXDMAEN = 1,
    SPI_CR2_SSOE    = 2,
    SPI_CR2_FRF     = 4,
    SPI_CR2_ERRIE   = 5,
    SPI_CR2_RXNEIE  = 6,
    SPI_CR2_TXEIE   = 7
}SPI_CR2_te;

/**
 * @brief SPI_SR register bit positions
 */
typedef enum
{
    SPI_SR_RXNE   = 0,
    SPI_SR_TXE    = 1,
    SPI_SR_CHSIDE = 2,
    SPI_SR_UDR    = 3,
    SPI_SR_CRCERR = 4,
    SPI_SR_MODF   = 5,
    SPI_SR_OVR    = 6,
    SPI_SR_BSY    = 7,
    SPI_SR_FRE    = 8
}SPI_SR_te;

/**
 * @brief RCC_CFGR register bit positions
 */
typedef enum
{
    RCC_CFGR_SWS   = 2,
    RCC_CFGR_HPRE  = 4,
    RCC_CFGR_PPRE1 = 10,
    RCC_CFGR_PPRE2 = 13
}RCC_CFGR_te;

/**
 * @brief I2C_CR1 register bit positions
 */
typedef enum
{
    I2C_CR1_PE        = 0,
    I2C_CR1_SMBUS     = 1,
    I2C_CR1_SMBTYPE   = 3,
    I2C_CR1_ENARP     = 4,
    I2C_CR1_ENPEC     = 5,
    I2C_CR1_ENGC      = 6,
    I2C_CR1_NOSTRETCH = 7,
    I2C_CR1_START     = 8,
    I2C_CR1_STOP      = 9,
    I2C_CR1_ACK       = 10,
    I2C_CR1_POS       = 11,
    I2C_CR1_PEC       = 12,
    I2C_CR1_ALERT     = 13,
    I2C_CR1_SWRST     = 15
}I2C_CR1_te;

/**
 * @brief I2C_CR2 register bit positions
 */
typedef enum
{
    I2C_CR2_FREQ      = 0,
    I2C_CR2_ITERREN   = 8,
    I2C_CR2_ITEVTEN   = 9,
    I2C_CR2_ITBUFEN   = 10,
    I2C_CR2_DMAEN     = 11,
    I2C_CR2_LAST      = 12
}I2C_CR2_te;

/**
 * @brief I2C_SR1 register bit positions
 */
typedef enum
{
    I2C_SR1_SB        = 0,
    I2C_SR1_ADDR      = 1,
    I2C_SR1_BTF       = 2,
    I2C_SR1_ADD10     = 3,
    I2C_SR1_STOPF     = 4,
    I2C_SR1_RxNE      = 6,
    I2C_SR1_TxE       = 7,
    I2C_SR1_BERR      = 8,
    I2C_SR1_ARLO      = 9,
    I2C_SR1_AF        = 10,
    I2C_SR1_OVR       = 11,
    I2C_SR1_PECERR    = 12,
    I2C_SR1_TIMEOUT   = 14,
    I2C_SR1_SMBALERT  = 15
}I2C_SR1_te;

/**
 * @brief I2C_SR2 register bit positions
 */
typedef enum
{
    I2C_SR2_MSL        = 0,
    I2C_SR2_BUSY       = 1,
    I2C_SR2_TRA        = 2,
    I2C_SR2_GENCALL    = 4,
    I2C_SR2_SMBDEFAULT = 5,
    I2C_SR2_SMBHOST    = 6,
    I2C_SR2_DUALF      = 7,
    I2C_SR2_PEC        = 8
}I2C_SR2_te;

/**
 * @brief I2C_CCR register bit positions
 */
typedef enum
{
    I2C_CCR_CCR   = 0,
    I2C_CCR_DUTY  = 14,
    I2C_CCR_FS    = 15
}I2C_CCR_te;

/**
 * @brief I2C_OAR1 register bit positions
 */
typedef enum
{
    I2C_OAR1_ADD0      = 0,
    I2C_OAR1_ADD7_1    = 1,
    I2C_OAR1_ADD9_8    = 8,
    I2C_OAR1_ADDMODE   = 15
}I2C_OAR1_te;

/**
 * @brief USART_SR register bit positions
 */
typedef enum
{
    USART_SR_PE    = 0,
    USART_SR_FE    = 1,
    USART_SR_NF    = 2,
    USART_SR_ORE   = 3,
    USART_SR_IDLE  = 4,
    USART_SR_RXNE  = 5,
    USART_SR_TC    = 6,
    USART_SR_TXE   = 7,
    USART_SR_LBD   = 8,
    USART_SR_CTS   = 9
}USART_SR_te;

/**
 * @brief USART_BRR register bit positions
 */
typedef enum
{
    USART_BRR_DIV_FRACTION = 0,
    USART_BRR_DIV_MANTISSA = 4
}USART_BRR_te;

/**
 * @brief USART_CR1 regiser bit positions
 * 
 */
typedef enum
{
    USART_CR1_SBK       = 0,
    USART_CR1_RWU       = 1,
    USART_CR1_RE        = 2,
    USART_CR1_TE        = 3,
    USART_CR1_IDLEIE    = 4,
    USART_CR1_RXNEIE    = 5,
    USART_CR1_TCIE      = 6,
    USART_CR1_TXEIE     = 7,
    USART_CR1_PEIE      = 8,
    USART_CR1_PS        = 9,
    USART_CR1_PCE       = 10,
    USART_CR1_WAKE      = 11,
    USART_CR1_M         = 12,
    USART_CR1_UE        = 13,
    USART_CR1_OVER8     = 15
}USART_CR1_te;

/**
 * @brief USART_CR2 regiser bit positions
 * 
 */
typedef enum
{
    USART_CR2_ADD     = 0,
    USART_CR2_LBDL    = 5,
    USART_CR2_LBDIE   = 6,
    USART_CR2_LBCL    = 8,
    USART_CR2_CPHA    = 9,
    USART_CR2_CPOL    = 10,
    USART_CR2_CLKEN   = 11,
    USART_CR2_STOP    = 12,
    USART_CR2_LINEN   = 14
}USART_CR2_te;

/**
 * @brief USART_CR3 regiser bit positions
 * 
 */
typedef enum
{
    USART_CR3_EIE      = 0,
    USART_CR3_IREN     = 1,
    USART_CR3_IRLP     = 2,
    USART_CR3_HDSEL    = 3,
    USART_CR3_NACK     = 4,
    USART_CR3_SCEN     = 5,
    USART_CR3_DMAR     = 6,
    USART_CR3_DMAT     = 7,
    USART_CR3_RTSE     = 8,
    USART_CR3_CTSE     = 9,
    USART_CR3_CTSIE    = 10,
    USART_CR3_ONEBIT   = 11
}USART_CR3_te;

/**
 * @brief RCC_AHB1ENR register bit positions
 * 
 */
typedef enum
{
    RCC_AHB1ENR_GPIOAEN    = 0,
    RCC_AHB1ENR_GPIOBEN    = 1,
    RCC_AHB1ENR_GPIOCEN    = 2,
    RCC_AHB1ENR_GPIODEN    = 3,
    RCC_AHB1ENR_GPIOEEN    = 4,
    RCC_AHB1ENR_GPIOHEN    = 7,
    RCC_AHB1ENR_CRCEN      = 12,
    RCC_AHB1ENR_DMA1EN     = 21,
    RCC_AHB1ENR_DMA2EN     = 22
}RCC_AHB1ENR_te;

/**
 * @brief RCC_AHB1RSTR register bit positions
 * 
 */
typedef enum
{
    RCC_AHB1RSTR_GPIOARST  = 0,
    RCC_AHB1RSTR_GPIOBRST  = 1,
    RCC_AHB1RSTR_GPIOCRST  = 2,
    RCC_AHB1RSTR_GPIODRST  = 3,
    RCC_AHB1RSTR_GPIOERST  = 4,
    RCC_AHB1RSTR_GPIOHRST  = 7,
    RCC_AHB1RSTR_CRCRST    = 12,
    RCC_AHB1RSTR_DMA1RST   = 21,
    RCC_AHB1RSTR_DMA2RST   = 22
}RCC_AHB1RSTR_te;

/**
 * @brief RCC_APB1ENR register bit positions
 * 
 */
typedef enum
{
    RCC_APB1ENR_TIM2EN     = 0,
    RCC_APB1ENR_TIM3EN     = 1,
    RCC_APB1ENR_TIM4EN     = 2,
    RCC_APB1ENR_TIM5EN     = 3,
    RCC_APB1ENR_WWDGEN     = 11,
    RCC_APB1ENR_SPI2EN     = 14,
    RCC_APB1ENR_SPI3EN     = 15,
    RCC_APB1ENR_USART2EN   = 17,
    RCC_APB1ENR_I2C1EN     = 21,
    RCC_APB1ENR_I2C2EN     = 22,
    RCC_APB1ENR_I2C3EN     = 23,
    RCC_APB1ENR_PWREN      = 28
}RCC_APB1ENR_te;

/**
 * @brief RCC_APB1RSTR register bit positions
 */
typedef enum
{
    RCC_APB1RSTR_TIM2RST    = 0,
    RCC_APB1RSTR_TIM3RST    = 1,
    RCC_APB1RSTR_TIM4RST    = 2,
    RCC_APB1RSTR_TIM5RST    = 3,
    RCC_APB1RSTR_WWDGRST    = 11,
    RCC_APB1RSTR_SPI2RST    = 14,
    RCC_APB1RSTR_SPI3RST    = 15,
    RCC_APB1RSTR_USART2RST  = 17,
    RCC_APB1RSTR_I2C1RST    = 21,
    RCC_APB1RSTR_I2C2RST    = 22,
    RCC_APB1RSTR_I2C3RST    = 23,
    RCC_APB1RSTR_PWRRST     = 28
}RCC_APB1RSTR_te;

/**
 * @brief RCC_APB2ENR register bit positions.
 */
typedef enum
{
    RCC_APB2ENR_TIM1EN     = 0,
    RCC_APB2ENR_USART1EN   = 4,
    RCC_APB2ENR_USART6EN   = 5,
    RCC_APB2ENR_ADC1EN     = 8,
    RCC_APB2ENR_SDIOEN     = 11,
    RCC_APB2ENR_SPI1EN     = 12,
    RCC_APB2ENR_SPI4EN     = 13,
    RCC_APB2ENR_SYSCFGEN   = 14,
    RCC_APB2ENR_TIM9EN     = 16,
    RCC_APB2ENR_TIM10EN    = 17,
    RCC_APB2ENR_TIM11EN    = 18
}RCC_APB2ENR_te;

/**
 * @brief RCC_APB2RSTR register bit positions.
 */
typedef enum
{
    RCC_APB2RSTR_TIM1RST   = 0,
    RCC_APB2RSTR_USART1RST = 4,
    RCC_APB2RSTR_USART6RST = 5,
    RCC_APB2RSTR_ADC1RST   = 8,
    RCC_APB2RSTR_SDIORST   = 11,
    RCC_APB2RSTR_SPI1RST   = 12,
    RCC_APB2RSTR_SPI4RST   = 13,
    RCC_APB2RSTR_SYSCFGRST = 14,
    RCC_APB2RSTR_TIM9RST   = 16,
    RCC_APB2RSTR_TIM10RST  = 17,
    RCC_APB2RSTR_TIM11RST  = 18
}RCC_APB2RSTR_te;

/**
 * @brief Peripheral definitions
 * 
 */
#define RCC					((RCC_REGDEF_ts*)ADDR_RCC)
#define EXTI				((EXTI_REGDEF_ts*)ADDR_EXTI)
#define SYSCFG				((SYSCFG_REGDEF_ts*)ADDR_SYSCFG)
#define RTC					((RTC_REGDEF_ts*)ADDR_RTC)
#define PWR					((PWR_REGDEF_ts*)ADDR_PWR)
#define GPIOA               ((GPIO_REGDEF_ts*)ADDR_GPIOA)
#define GPIOB               ((GPIO_REGDEF_ts*)ADDR_GPIOB)
#define GPIOC               ((GPIO_REGDEF_ts*)ADDR_GPIOC)
#define GPIOD               ((GPIO_REGDEF_ts*)ADDR_GPIOD)
#define GPIOE               ((GPIO_REGDEF_ts*)ADDR_GPIOE)
#define GPIOH               ((GPIO_REGDEF_ts*)ADDR_GPIOH)
#define I2C1				((I2C_REGDEF_ts*)ADDR_I2C1)
#define I2C2				((I2C_REGDEF_ts*)ADDR_I2C2)
#define I2C3				((I2C_REGDEF_ts*)ADDR_I2C3)
#define SPI1				((SPI_REGDEF_ts*)ADDR_SPI1)
#define SPI2				((SPI_REGDEF_ts*)ADDR_SPI2)
#define SPI3				((SPI_REGDEF_ts*)ADDR_SPI3)
#define SPI4				((SPI_REGDEF_ts*)ADDR_SPI4)
#define USART1				((USART_REGDEF_ts*)ADDR_USART1)
#define USART2				((USART_REGDEF_ts*)ADDR_USART2)
#define USART6				((USART_REGDEF_ts*)ADDR_USART6)

/**
 * @brief IRQ positions in the vector table
 * 
 */
typedef enum
{
    WWDG_IRQn                   = 0,
    EXTI16_IRQn                 = 1,
    EXTI21_IRQn                 = 2,
    EXTI22_IRQn                 = 3,
    FLASH_IRQn                  = 4,
    RCC_IRQn                    = 5,
    EXTI0_IRQn                  = 6,
    EXTI1_IRQn                  = 7,
    EXTI2_IRQn                  = 8,
    EXTI3_IRQn                  = 9,
    EXTI4_IRQn                  = 10,
    DMA1_Stream0_IRQn           = 11,
    DMA1_Stream1_IRQn           = 12,
    DMA1_Stream2_IRQn           = 13,
    DMA1_Stream3_IRQn           = 14,
    DMA1_Stream4_IRQn           = 15,
    DMA1_Stream5_IRQn           = 16,
    DMA1_Stream6_IRQn           = 17,
    ADC_IRQn                    = 18,
    EXTI9_5_IRQn                = 23,
    TIM1_BRK_TIM9_IRQn          = 24,
    TIM1_UP_TIM10_IRQn          = 25,
    TIM1_TRG_COM_TIM11_IRQn     = 26,
    TIM1_CC_IRQn                = 27,
    TIM2_IRQn                   = 28,
    TIM3_IRQn                   = 29,
    TIM4_IRQn                   = 30,
    I2C1_EV_IRQn                = 31,
    I2C1_ER_IRQn                = 32,
    I2C2_EV_IRQn                = 33,
    I2C2_ER_IRQn                = 34,
    SPI1_IRQn                   = 35,
    SPI2_IRQn                   = 36,
    USART1_IRQn                 = 37,
    USART2_IRQn                 = 38,
    EXTI15_10_IRQn              = 40,
    EXTI17_IRQn                 = 41,
    EXTI18_IRQn                 = 42,
    DMA1_Stream7_IRQn           = 47,
    SDIO_IRQn                   = 49,
    TIM5_IRQn                   = 50,
    SPI3_IRQn                   = 51,
    DMA2_Stream0_IRQn           = 56,
    DMA2_Stream1_IRQn           = 57,
    DMA2_Stream2_IRQn           = 58,
    DMA2_Stream3_IRQn           = 59,
    DMA2_Stream4_IRQn           = 60,
    OTG_FS_IRQn                 = 67,
    DMA2_Stream5_IRQn           = 68,
    DMA2_Stream6_IRQn           = 69,
    DMA2_Stream7_IRQn           = 70,
    USART6_IRQn                 = 71,
    I2C3_EV_IRQn                = 72,
    I2C3_ER_IRQn                = 73,
    FPU_IRQn                    = 81,
    SPI4_IRQn                   = 84
}IRQn_te;

typedef enum {
	EXTI_LINE_0,
 	EXTI_LINE_1,
	EXTI_LINE_2,
	EXTI_LINE_3,
 	EXTI_LINE_4,
	EXTI_LINE_5,	
	EXTI_LINE_6,
	EXTI_LINE_7,
	EXTI_LINE_8,
	EXTI_LINE_9,
	EXTI_LINE_10,
	EXTI_LINE_11,
	EXTI_LINE_12,
	EXTI_LINE_13,
	EXTI_LINE_14,
	EXTI_LINE_15,
	EXTI_LINE_16,
	EXTI_LINE_17,
	EXTI_LINE_18,
	EXTI_LINE_21 = 21,
 	EXTI_LINE_22
}EXTI_LINES_te;

typedef enum {
	PA,
	PB,
	PC,
	PD,
	PE,
	PH = 7
}PORT_CODES_ts;

#define GPIOA_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 0)
#define GPIOB_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 1)
#define GPIOC_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 2)
#define GPIOD_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 3)
#define GPIOE_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 4)
#define GPIOH_CLK_EN()		(RCC->RCC_AHB1ENR |= 0b1 << 7)

#define GPIOA_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 0))
#define GPIOB_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 1))
#define GPIOC_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 2))
#define GPIOD_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 3))
#define GPIOE_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 4))
#define GPIOH_CLK_DIS()		(RCC->RCC_AHB1ENR |= ~(0b1 << 7))

#endif