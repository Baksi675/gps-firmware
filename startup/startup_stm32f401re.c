#include <stdint.h>

#define SRAM_START  0x20000000U
#define SRAM_SIZE   ((96) * (1024))
#define SRAM_END    ((SRAM_START) + (SRAM_SIZE))

#define STACK_START SRAM_END	

extern uint32_t __bss_start;
extern uint32_t __bss_end;
extern uint32_t __data_lma;
extern uint32_t __data_start;
extern uint32_t __data_end;

extern int main();

void Reset_Handler                  (void);
void NMI_Handler                    (void) __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler              (void) __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler              (void) __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler               (void) __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler             (void) __attribute__ ((weak, alias("Default_Handler")));
void SVCall_Handler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void DebugMonitor_Handler           (void) __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler                 (void) __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler                (void) __attribute__ ((weak, alias("Default_Handler")));
void WWDG_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));
void PVD_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));             
void TAMP_STAMP_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));      
void RTC_WKUP_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler"))); 
void Flash_IRQHandler 			    (void) __attribute__ ((weak, alias("Default_Handler"))); 
void RCC_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));             
void EXTI0_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));           
void EXTI1_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));           
void EXTI2_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));           
void EXTI3_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));           
void EXTI4_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));           
void DMA1_Stream0_IRQHandler 		(void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream1_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream2_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream3_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream4_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream5_IRQHandler 	    (void) __attribute__ ((weak, alias("Default_Handler")));    
void DMA1_Stream6_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void ADC_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));   
void EXTI9_5_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));               
void TIM1_BRK_TIM9_IRQHandler 	    (void) __attribute__ ((weak, alias("Default_Handler")));   
void TIM1_UP_TIM10_IRQHandler 	    (void) __attribute__ ((weak, alias("Default_Handler")));   
void TIM1_TRG_COM_TIM11_IRQHandler  (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));         
void TIM2_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));            
void TIM3_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));            
void TIM4_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));            
void I2C1_EV_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));         
void I2C1_ER_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));         
void I2C2_EV_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));         
void I2C2_ER_IRQHandler 			(void) __attribute__ ((weak, alias("Default_Handler")));         
void SPI1_IRQHandler  				(void) __attribute__ ((weak, alias("Default_Handler")));           
void SPI2_IRQHandler 				(void) __attribute__ ((weak, alias("Default_Handler")));            
void USART1_IRQHandler  			(void) __attribute__ ((weak, alias("Default_Handler")));         
void USART2_IRQHandler  			(void) __attribute__ ((weak, alias("Default_Handler")));        
void EXTI15_10_IRQHandler           (void) __attribute__ ((weak, alias("Default_Handler")));     
void RTC_Alarm_IRQHandler           (void) __attribute__ ((weak, alias("Default_Handler")));    
void OTG_FS_WKUP_IRQHandler         (void) __attribute__ ((weak, alias("Default_Handler"))); 
void DMA1_Stream7_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler"))); 
void SDIO_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler")));
void TIM5_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler")));
void SPI3_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream0_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream1_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream2_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream3_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream4_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void OTG_FS_IRQHandler              (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream5_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream6_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void DMA2_Stream7_IRQHandler        (void) __attribute__ ((weak, alias("Default_Handler")));
void USART6_IRQHandler              (void) __attribute__ ((weak, alias("Default_Handler")));
void I2C3_EV_IRQHandler             (void) __attribute__ ((weak, alias("Default_Handler")));
void I2C3_ER_IRQHandler             (void) __attribute__ ((weak, alias("Default_Handler")));
void FPU_IRQHandler                 (void) __attribute__ ((weak, alias("Default_Handler"))); 
void SPI4_IRQHandler                (void) __attribute__ ((weak, alias("Default_Handler"))); 

uint32_t const vectors[] __attribute__ ((section(".vector"))) = {
    /* Cortex-M4 Core Handlers */
    (uint32_t)STACK_START,                        /* Initial Stack Pointer */
    (uint32_t)Reset_Handler,                   /* Reset Handler */
    (uint32_t)NMI_Handler,                     /* NMI Handler */
    (uint32_t)HardFault_Handler,               /* Hard Fault Handler */
    (uint32_t)MemManage_Handler,               /* MPU Fault Handler */
    (uint32_t)BusFault_Handler,                /* Bus Fault Handler */
    (uint32_t)UsageFault_Handler,              /* Usage Fault Handler */
    0, 0, 0, 0,                      /* Reserved */
    (uint32_t)SVCall_Handler,                     /* SVCall Handler */
    (uint32_t)DebugMonitor_Handler,                /* Debug Monitor Handler */
    0,                               /* Reserved */
    (uint32_t)PendSV_Handler,                  /* PendSV Handler */
    (uint32_t)SysTick_Handler,                 /* SysTick Handler */

    /* External Interrupts */
    0,                               /* Reserved */
    (uint32_t)PVD_IRQHandler,           /* PVD through EXTI Line 16 */
    (uint32_t)TAMP_STAMP_IRQHandler,           /* Tamper and TimeStamp */
    (uint32_t)RTC_WKUP_IRQHandler,      /* RTC Wakeup through EXTI Line 22 */
    (uint32_t)Flash_IRQHandler,                /* Flash */
    (uint32_t)RCC_IRQHandler,                  /* RCC */
    (uint32_t)EXTI0_IRQHandler,                /* EXTI Line 0 */
    (uint32_t)EXTI1_IRQHandler,                /* EXTI Line 1 */
    (uint32_t)EXTI2_IRQHandler,                /* EXTI Line 2 */
    (uint32_t)EXTI3_IRQHandler,                /* EXTI Line 3 */
    (uint32_t)EXTI4_IRQHandler,                /* EXTI Line 4 */
    (uint32_t)DMA1_Stream0_IRQHandler,         /* DMA1 Stream 0 */
    (uint32_t)DMA1_Stream1_IRQHandler,         /* DMA1 Stream 1 */
    (uint32_t)DMA1_Stream2_IRQHandler,         /* DMA1 Stream 2 */
    (uint32_t)DMA1_Stream3_IRQHandler,         /* DMA1 Stream 3 */
    (uint32_t)DMA1_Stream4_IRQHandler,         /* DMA1 Stream 4 */
    (uint32_t)DMA1_Stream5_IRQHandler,         /* DMA1 Stream 5 */
    (uint32_t)DMA1_Stream6_IRQHandler,         /* DMA1 Stream 6 */
    (uint32_t)ADC_IRQHandler,                  /* ADC1 */
    0, 0, 0, 0,                      /* Reserved */
    (uint32_t)EXTI9_5_IRQHandler,              /* EXTI Line [9:5] */
    (uint32_t)TIM1_BRK_TIM9_IRQHandler,        /* TIM1 Break and TIM9 */
    (uint32_t)TIM1_UP_TIM10_IRQHandler,        /* TIM1 Update and TIM10 */
    (uint32_t)TIM1_TRG_COM_TIM11_IRQHandler,   /* TIM1 Trigger/Com and TIM11 */
    (uint32_t)TIM1_CC_IRQHandler,              /* TIM1 Capture Compare */
    (uint32_t)TIM2_IRQHandler,                 /* TIM2 */
    (uint32_t)TIM3_IRQHandler,                 /* TIM3 */
    (uint32_t)TIM4_IRQHandler,                 /* TIM4 */
    (uint32_t)I2C1_EV_IRQHandler,              /* I2C1 Event */
    (uint32_t)I2C1_ER_IRQHandler,              /* I2C1 Error */
    (uint32_t)I2C2_EV_IRQHandler,              /* I2C2 Event */
    (uint32_t)I2C2_ER_IRQHandler,              /* I2C2 Error */
    (uint32_t)SPI1_IRQHandler,                 /* SPI1 */
    (uint32_t)SPI2_IRQHandler,                 /* SPI2 */
    (uint32_t)USART1_IRQHandler,               /* USART1 */
    (uint32_t)USART2_IRQHandler,               /* USART2 */
    0,                               /* Reserved */
    (uint32_t)EXTI15_10_IRQHandler,            /* EXTI Line [15:10] */
    (uint32_t)RTC_Alarm_IRQHandler,     /* RTC Alarm (A and B) through EXTI Line 17 */
    (uint32_t)OTG_FS_WKUP_IRQHandler,   /* USB OTG FS Wakeup through EXTI Line 18 */
    0, 0, 0, 0,                      /* Reserved */
    (uint32_t)DMA1_Stream7_IRQHandler,         /* DMA1 Stream 7 */
    0,                               /* Reserved */
    (uint32_t)SDIO_IRQHandler,                 /* SDIO */
    (uint32_t)TIM5_IRQHandler,                 /* TIM5 */
    (uint32_t)SPI3_IRQHandler,                 /* SPI3 */
    0, 0, 0, 0,                      /* Reserved */
    (uint32_t)DMA2_Stream0_IRQHandler,         /* DMA2 Stream 0 */
    (uint32_t)DMA2_Stream1_IRQHandler,         /* DMA2 Stream 1 */
    (uint32_t)DMA2_Stream2_IRQHandler,         /* DMA2 Stream 2 */
    (uint32_t)DMA2_Stream3_IRQHandler,         /* DMA2 Stream 3 */
    (uint32_t)DMA2_Stream4_IRQHandler,         /* DMA2 Stream 4 */
    0, 0, 0, 0, 0, 0,                /* Reserved */
    (uint32_t)OTG_FS_IRQHandler,               /* USB OTG FS */
    (uint32_t)DMA2_Stream5_IRQHandler,         /* DMA2 Stream 5 */
    (uint32_t)DMA2_Stream6_IRQHandler,         /* DMA2 Stream 6 */
    (uint32_t)DMA2_Stream7_IRQHandler,         /* DMA2 Stream 7 */
    (uint32_t)USART6_IRQHandler,               /* USART6 */
    (uint32_t)I2C3_EV_IRQHandler,              /* I2C3 Event */
    (uint32_t)I2C3_ER_IRQHandler,              /* I2C3 Error */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* Reserved */
    (uint32_t)SPI4_IRQHandler                  /* SPI4 */
};

void Reset_Handler() {
	uint8_t *ram_current_addr;
	uint8_t *rom_current_addr;

	// Assign 0s to variables in .bss section
	uint32_t bss_size = (uint32_t)&__bss_end - (uint32_t)&__bss_start;
	ram_current_addr = (uint8_t*)&__bss_start;
	for(uint32_t i = 0; i < bss_size; i++) {
		*ram_current_addr++ = 0;
	}

	// Copy .data from ROM to RAM
	uint32_t data_size = (uint32_t)&__data_end - (uint32_t)&__data_start;
	ram_current_addr = (uint8_t*)&__data_start;
	rom_current_addr = (uint8_t*)&__data_lma;
	for(uint32_t i = 0; i < data_size; i++) {
		*ram_current_addr++ = *rom_current_addr++;
	}

	main();
}

void Default_Handler(void) {
  while(1);
}
