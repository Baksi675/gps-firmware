/**
 * @file stm32f401re_spi_driver.c
 * @author github.com/Baksi675
 * @brief SPI driver implementation for STM32F401RE
 * @version 0.1
 * @date 2025-07-29
 * 
 * @copyright Copyright (c) 2025
 */

#include "stm32f401re_spi.h"
#include "stm32f401re_rcc.h"

static void spi_set_pclk(SPI_REGDEF_ts const *spi_instance, EN_STATUS_te en_status);

/** 
 * @defgroup SPI_Public_APIs SPI Public APIs
 * @{
 */

/**
 * @brief Initializes the SPI peripheral to master or slave (default). If no configuration is given in the application layer, then the SPI is initialized with the following defaults:
 * - Slave mode
 * - fPCLK / 2 baud rate
 * - Clock polarity is 0 when idle
 * - Clock phase is on the first transition
 * - HW slave selection 
 * - 8 bit data frame format
 * - Frame format is MSB 
 *
 * Comments about interface:
 * - A configuration object must be created before calling this function
 * - In a local scoped environment it's recommended to initialize the configuration object to zero before passing it as a parameter to avoid gargabe values
 *
 * @param[in] spi_handale SPI configuration object
 */
void spi_init(SPI_HANDLE_ts *spi_handale) {
	spi_set_pclk(spi_handale->spi_instance, ENABLE);

	// Set data frame format to 8 or 16 bit
	spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_DFF);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_data_frame_format << SPI_CR1_DFF);

	// Select clock polarity
	spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_CPOL);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_clock_polarity << SPI_CR1_CPOL);

	// Select clock phase
	spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_CPHA);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_clock_phase << SPI_CR1_CPHA);

	// Select frame format (LSB first or MSB first)
	spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_LSBFIRST);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_bit_first << SPI_CR1_LSBFIRST);

	// Select SW or HW slave select
	spi_handale->spi_instance->SPI_CR1 &= ~(0b1 << SPI_CR1_SSM);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_slave_select_mode << SPI_CR1_SSM);

	if(spi_handale->spi_mode == SPI_MODE_MASTER) {
		// Set SSI to avoid MODF fault
		spi_handale->spi_instance->SPI_CR1 |= (0x1 << SPI_CR1_SSI);

		//  Enable SS output (set SSOE bit)
		if(spi_handale->spi_slave_select_mode == SPI_SLAVE_SELECT_MODE_HW) {
			spi_handale->spi_instance->SPI_CR2 &= ~(0x1 << SPI_CR2_SSOE);
			spi_handale->spi_instance->SPI_CR2 |= (0x1 << SPI_CR2_SSOE);
		}

		// Select serial clock baud rate
		spi_handale->spi_instance->SPI_CR1 &= ~(0x07 << SPI_CR1_BR);
		spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_master_sclk_speed << SPI_CR1_BR);
	}
	else if(spi_handale->spi_mode == SPI_MODE_SLAVE) {
		// Set slave selection internally (pull it low)
		spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SSI);
	}

	// Set SPI as slave or master
	spi_handale->spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_MSTR);
	spi_handale->spi_instance->SPI_CR1 |= (spi_handale->spi_mode << SPI_CR1_MSTR);
}

/**
 * @brief Deinitializes the given SPI peripheral by setting its registers back to their reset values. It also turns of the peripherals clock.
 * 
 * @param[in] spi_instance The SPI instance to deinitialize.
 */
void spi_deinit(SPI_REGDEF_ts const *spi_instance) {
	if(spi_instance == SPI1) {
		rcc_reset_periph_apb2(RCC_APB2RSTR_SPI1RST);
	}
	else if(spi_instance == SPI2) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_SPI2RST);
	}
	else if(spi_instance == SPI3) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_SPI3RST);
	}
	else if(spi_instance == SPI4) {
		rcc_reset_periph_apb2(RCC_APB2RSTR_SPI4RST);
	}

	spi_set_pclk(spi_instance, DISABLE);
}

/**
 * @brief A blocking SPI send function. Sends data, blocks until it's completed.
 * 
 * @param[in] spi_instance The SPI instance on which to send data.
 * @param[in] tx_buffer A pointer to the buffer containing the data to be sent.
 * @param[in] len The length of the buffer to be sent.
 */
void spi_send(SPI_REGDEF_ts *spi_instance, uint8_t *tx_buffer, uint32_t len) {
	while(len != 0) {
		// 8 bit data frame format
		if(((spi_instance->SPI_CR1 >> SPI_CR1_DFF) & 0x1) == 0) {
			while(!((spi_instance->SPI_SR >> SPI_SR_TXE) & 0x1));
			spi_instance->SPI_DR = *tx_buffer;
			tx_buffer++;
			len--; 
		}
		// 16 bit data frame format
		else {
			while(!((spi_instance->SPI_SR >> SPI_SR_TXE) & 0x1));
			spi_instance->SPI_DR = *((uint16_t*)tx_buffer);
			tx_buffer += 2;
			len -= 2; 
		}

		while(!((spi_instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
		uint16_t dummy_rx = SPI1->SPI_DR;
		(void)dummy_rx;
	}
}

/**
 * @brief A blocking SPI receive function. Receives data, blocks until completed.
 * 
 * @param[in] spi_instance The SPI instance on which to receive data.
 * @param[out] rx_buffer A pointer to the buffer that will store the received data.
 * @param[in] len The length of the data to be received.
 */
void spi_receive(SPI_REGDEF_ts *spi_instance, uint8_t *rx_buffer, uint32_t len) {
	uint16_t dummy_tx = 0xFFFF;
	
	while(len != 0) {
		// 8 bit data frame format
		if(((spi_instance->SPI_CR1 >> SPI_CR1_DFF) & 0x1) == 0) {
			spi_instance->SPI_DR = (uint8_t)dummy_tx;
			while(!((spi_instance->SPI_SR >> SPI_SR_TXE) & 0x1));

			while(!((spi_instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
			*rx_buffer = spi_instance->SPI_DR;
			len--;
			rx_buffer++;
		}
		// 16 bit data frame format
		else {
			spi_instance->SPI_DR = dummy_tx;
			while(!((spi_instance->SPI_SR >> SPI_SR_TXE) & 0x1));

			while(!((spi_instance->SPI_SR >> SPI_SR_RXNE) & 0x1));
			*((uint16_t*)rx_buffer) = spi_instance->SPI_DR;
			len -= 2;
			rx_buffer += 2;
		}
	}
}

/**
 * @brief Starts or terminates the communication by enabling / disabling the peripheral. 
 *
 * Comments:
 * - If set to ENABLE, this call must preceed the first send or receive call.
 * - If set to DISABLE, this call must follow the last send or receive call.
 *
 * @param[in] spi_instance The SPI instance to be ENABLE.
 * @param[out] en_status Whether to enable or disable the communication.
 */
void spi_set_comm(SPI_REGDEF_ts *spi_instance, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SPE);
		spi_instance->SPI_CR1 |= (0x1 << SPI_CR1_SPE);
	}
	else if(en_status == DISABLE) {
		while((spi_instance->SPI_SR >> SPI_SR_BSY) & 0x01);
		spi_instance->SPI_CR1 &= ~(0x1 << SPI_CR1_SPE);
	}
}

/** @} */

/** 
 * @defgroup SPI_Internal_Helper SPI Internal Helpers
 * @{
 */

/**
 * @brief Enable or disable the peripheral clock of the given SPI instance.
 * 
 * @param spi_instance The SPI instance.
 * @param en_status Whether to enable or disable the peripheral clock.
 */
static void spi_set_pclk(SPI_REGDEF_ts const *spi_instance, EN_STATUS_te en_status) {
	if(spi_instance == SPI1) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb2(RCC_APB2ENR_SPI1EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb2(RCC_APB2ENR_SPI1EN, DISABLE);
		}
	}
	else if(spi_instance == SPI2) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb1(RCC_APB1ENR_SPI2EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb1(RCC_APB1ENR_SPI2EN, DISABLE);

		}
	}
	else if(spi_instance == SPI3) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb1(RCC_APB1ENR_SPI3EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb1(RCC_APB1ENR_SPI3EN, DISABLE);
		}
	}
	else if(spi_instance == SPI4) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb2(RCC_APB2ENR_SPI4EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb2(RCC_APB2ENR_SPI4EN, DISABLE);
		}
	}
}

/** @} */