#include <stdint.h>

#define LED_PIN_NO 5

#define RCC_AHB1ENR 	(*(volatile uint32_t*)0x40023830)
#define GPIOA_MODER		(*(volatile uint32_t*)0x40020000)
#define GPIOA_ODR		(*(volatile uint32_t*)0x40020014)

void led_init(void) {
	// Enable GPIOA peripheral
	RCC_AHB1ENR |= 0b1 << 0;

	// Clear and set to output mode
	GPIOA_MODER &= ~(0b11 << LED_PIN_NO);
	GPIOA_MODER |= (0b1 << (LED_PIN_NO * 2));
}

void led_toggle(void) {
	// XOR the output
	GPIOA_ODR ^= (0b1 << LED_PIN_NO);
}

uint8_t count = 0;

int main(void) {
	led_init();

	while(1) {
		if(count < 10) {
			led_toggle();
			for(uint32_t i = 0; i < 200000; i++);
			count++;
		}
	
	}

	return 0;
}