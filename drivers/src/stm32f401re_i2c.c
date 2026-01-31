/**
 * @file stm32f401re_i2c_driver.c
 * @author github.com/Baksi675
 * @brief I2C driver implementation for STM32F401RE
 * @version 0.1
 * @date 2025-08-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "stm32f401re_i2c.h"
#include "stm32f401re_rcc.h"

static void i2c_set_pclk(I2C_REGDEF_ts const *i2c_instance, EN_STATUS_te en_status);

/** 
 * @defgroup I2C_Public_APIs I2C Public APIs
 * @{
 */

 /**
 * @brief Initializes the I2C peripheral. If no configuration is given in the application layer, then the I2C is initialized with the following defaults:
 * - Slave clock stretching is ENABLE
 * - Own address is 0
 * - SCLK speed is 100 kHz
 *
 * Comments about interface:
 * - A configuration object must be created before calling this function.
 * - In a local scoped environment it's recommended to initialize the configuration object to zero before passing it as a parameter to avoid gargabe values.
 *
 * @param[in] i2c_handle I2C configuration object.
 */
 void i2c_init(I2C_HANDLE_ts *i2c_handle) {
	// Enable peripheral clock
	i2c_set_pclk(i2c_handle->i2c_instance, ENABLE);

	// Set clock stretching from slave
	i2c_handle->i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_NOSTRETCH);
	i2c_handle->i2c_instance->I2C_CR1 |= (i2c_handle->i2c_clock_strech << I2C_CR1_NOSTRETCH);

	// Set address
	i2c_handle->i2c_instance->I2C_OAR1 &= ~(0x7F << I2C_OAR1_ADD7_1);
	i2c_handle->i2c_instance->I2C_OAR1 |= ((i2c_handle->i2c_address & 0x7F) << I2C_OAR1_ADD7_1);
	i2c_handle->i2c_instance->I2C_OAR1 |= (0x1 << 14);

	// Set peripheral clock frequency
	uint32_t i2c_clock_hz = rcc_get_apb1_clk();
	i2c_handle->i2c_instance->I2C_CR2 &= ~(0x3F << I2C_CR2_FREQ);
	i2c_handle->i2c_instance->I2C_CR2 |= (((i2c_clock_hz / 1000000) & 0x3F) << I2C_CR2_FREQ);

	// Set speed
	uint16_t ccr_value = 0;
	uint8_t t_rise = 0;
	if(i2c_handle->i2c_speed == I2C_SPEED_100kHz) {
		// Set standard mode, 100 kHz (duty cycle = 50%)
		i2c_handle->i2c_instance->I2C_CCR &= ~(0x1 << I2C_CCR_FS);
		
		ccr_value = i2c_clock_hz / (2 * 100000);

		t_rise = (i2c_clock_hz / 1000000) + 1;
	}
	else if(i2c_handle->i2c_speed == I2C_SPEED_400kHz) {
		// Set fast mode, 400 kHz (duty cycle = 9/16 (thigh / tlow))
		i2c_handle->i2c_instance->I2C_CCR &= ~(0x1 << I2C_CCR_FS);
		i2c_handle->i2c_instance->I2C_CCR |= (0x1 << I2C_CCR_FS);

		i2c_handle->i2c_instance->I2C_CCR &= ~(0x1 << I2C_CCR_DUTY);
		i2c_handle->i2c_instance->I2C_CCR |= (0x1 << I2C_CCR_DUTY);

		ccr_value = i2c_clock_hz / (25 * 400000);

		t_rise = ((i2c_clock_hz * 300) / 1000000000) + 1;
	}
	i2c_handle->i2c_instance->I2C_CCR &= ~(0xFFF);
	i2c_handle->i2c_instance->I2C_CCR |= (ccr_value << I2C_CCR_CCR);

	i2c_handle->i2c_instance->I2C_TRISE &= ~(0x3F);
	i2c_handle->i2c_instance->I2C_TRISE |= (t_rise);
 }

/**
 * @brief Deinitializes the given I2C peripheral by setting its registers back to their reset values. It also turns of the peripherals clock.
 * 
 * @param[in] i2c_instance The I2C instance to deinitialize.
 */
void i2c_deinit(I2C_REGDEF_ts const *i2c_instance) {
	if(i2c_instance == I2C1) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_I2C1RST);
	}
	else if(i2c_instance == I2C2) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_I2C2RST);
	}
	else if(i2c_instance == I2C3) {
		rcc_reset_periph_apb1(RCC_APB1RSTR_I2C3RST);
	}

	i2c_set_pclk(i2c_instance, DISABLE);
}

/**
 * @brief A blocking I2C master send function. Sends data, blocks until it's completed.
 * 
 * Comments about interface:
 * - Before calling this function the peripheral must be ENABLE with \ref i2c_master_set_comm if it hasn't been done so.
 *
 * @param[in] i2c_instance The I2C instance on which to send data.
 * @param[in] slave_addr The address of the slave to send data.
 * @param[in] tx_buffer A pointer to the buffer containing the data to be sent.
 * @param[in] len The length of the buffer to be sent.
 */
void i2c_master_send(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *tx_buffer, uint32_t len) {
	uint16_t volatile dummy_read = 0; 
	(void)dummy_read;

	// Generate start condition
	i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_START);

	// Check if start condition has been generated via SB bit in SR1. Clear SB by reading SR1.
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_SB) & 0x1));
	dummy_read = i2c_instance->I2C_SR1;

	// Write slave address to DR register
	i2c_instance->I2C_DR = (slave_addr << 1);

	// Wait until slave address has been sent by checking ADDR bit. Clear ADDR bit by reading SR1 and SR2.
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
	dummy_read = i2c_instance->I2C_SR1;
	dummy_read = i2c_instance->I2C_SR2;

	// Send data until len becomes 0
	while(len != 0) {
		while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
		i2c_instance->I2C_DR = *tx_buffer;
		tx_buffer++;
		len--;
	}
	// Check if last byte has been sent 
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_BTF) & 0x1));
}

/**
 * @brief Sends data with repeated start.
 * 
 * @param[in] i2c_instance The I2C instance used to send data.
 * @param[in] tx_buffer Pointer to a buffer of data to be sent.
 * @param[in] len Lenght of the data to be sent.
 */
void i2c_master_send_continue(I2C_REGDEF_ts *i2c_instance, uint8_t *tx_buffer, uint32_t len) {
	uint16_t volatile dummy_read = 0; 
	(void)dummy_read;

	// Send data until len becomes 0
	while(len != 0) {
		while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
		i2c_instance->I2C_DR = *tx_buffer;
		tx_buffer++;
		len--;
	}
	// Check if last byte has been sent 
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_TxE) & 0x1));
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_BTF) & 0x1));
}

/**
 * @brief A blocking I2C master receive function. Receives data, blocks until completed.
 *
 * Comments about interface:
 * - Before calling this function the peripheral must be ENABLE with \ref i2c_master_set_comm if it hasn't been done so.
 * 
 * @param[in] i2c_instance The I2C instance on which to receive data.
 * @param[in] slave_addr The address of the slave from which to receive data.
 * @param[out] rx_buffer A pointer to the buffer that will store the received data.
 * @param[in] len The length of the data to be received.
 */
void i2c_master_receive(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *rx_buffer, uint32_t len) {
	uint16_t volatile dummy_read = 0; 
	(void)dummy_read;
	
	// Generate start condition
	i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_START);

	// Check if start condition has been generated via SB bit in SR1. Clear SB by reading SR1.
	while(!((i2c_instance->I2C_SR1 >> I2C_SR1_SB) & 0x1));
	dummy_read = i2c_instance->I2C_SR1;

	// Write slave address to DR register (LSB is 1 ==> read sequence)
	i2c_instance->I2C_DR = ((slave_addr << 1) | 0x1);

	// Case when message is only 1 byte in lenght
	if(len == 1) {
		// Clear ACK bit in order to send NACK (to terminate the reception after 1 byte is received)
		i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_ACK);

		// Wait until slave address has been sent by checking ADDR bit. Clear ADDR bit by reading SR1 and SR2.
		while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
		dummy_read = i2c_instance->I2C_SR1;
		dummy_read = i2c_instance->I2C_SR2;

		// Receive a byte
		while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
		*rx_buffer = i2c_instance->I2C_DR;
	}
	// Case when message is more than 1 byte in length
	else if(len > 1) {
		// Wait until slave address has been sent by checking ADDR bit. Clear ADDR bit by reading SR1 and SR2.
		while(!((i2c_instance->I2C_SR1 >> I2C_SR1_ADDR) & 0x1));
		dummy_read = i2c_instance->I2C_SR1;
		dummy_read = i2c_instance->I2C_SR2;

		while(len != 0) {
			// When length is 2 disable the ACK bit to signal to the slave to stop transmission after the last byte has been sent
			if(len == 2) {
				while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
				i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_ACK);
				
				*rx_buffer = i2c_instance->I2C_DR;
				rx_buffer++;
				len--;
			}
			else {
				while(!((i2c_instance->I2C_SR1 >> I2C_SR1_RxNE) & 0x1));
				*rx_buffer = i2c_instance->I2C_DR;
				rx_buffer++;
				len--;
			}
		}
	}

	// Reenable ACK 
	i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_ACK);
}

/**
 * @brief Enables or disables the peripheral communication.
 *
 * Comments about interface:
 * - This function must be called before a communication sequence is initiated in order to enable the peripheral for communication and take control of the bus.
 * - After a communication sequence is completed this function must be called in order to terminate the communication and release the bus.
 * - Between the ENABLE and DISABLE calls, the peripheral can't be taken control of via another master.
 * 
 * @param[in] i2c_instance The I2C instance to be ENABLE or DISABLE.
 * @param[in] en_status Whether to enable or disable the communication.
 */
void i2c_master_set_comm(I2C_REGDEF_ts *i2c_instance, EN_STATUS_te en_status) {
	if(en_status == ENABLE) {
		i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_PE);

		// Set acknowledge --> must be done after PE has been ENABLE
		i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_ACK);
	}
	else if(en_status == DISABLE) {
		i2c_instance->I2C_CR1 |= (0x1 << I2C_CR1_STOP);
		while((i2c_instance->I2C_SR2 >> I2C_SR2_BUSY) & 0x01);
		i2c_instance->I2C_CR1 &= ~(0x1 << I2C_CR1_PE);
	}
}

/**
 * @brief Returns the name in strings of the I2C peripheral.
 * 
 * @param[in] i2c_instance The I2C peripheral.
 * @param[in] name A pointer to a buffer which will contain the name.
 */
void i2c_get_name(I2C_REGDEF_ts const *i2c_instance, char *name) {
	const char i2c[] = "I2C";
	uint8_t i2c_len = get_str_len(i2c);
	uint8_t pos_counter = 0;

	while(pos_counter != i2c_len) {
		name[pos_counter] = i2c[pos_counter];
		pos_counter++;
	}
	
	if(i2c_instance == I2C1) {
		name[pos_counter] = '1';
	}
	else if(i2c_instance == I2C2) {
		name[pos_counter] = '2';
	}
	else if(i2c_instance == I2C3) {
		name[pos_counter] = '6';
	}
	pos_counter++;

	name[pos_counter] = '\0';
}

/** @} */

/** 
 * @defgroup I2C_Internal_Helper I2C Internal Helpers
 * @{
 */

 /**
  * @brief Enable or disable the peripheral clock of the given I2C instance.
  * 
  * @param[in] i2c_instance The I2C instance.
  * @param[in] en_status Whether to enable or disable the peripheral clock.
  */
static void i2c_set_pclk(I2C_REGDEF_ts const *i2c_instance, EN_STATUS_te en_status) {
	if(i2c_instance == I2C1) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C1EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C1EN, DISABLE);
		}
	}
	else if(i2c_instance == I2C2) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C2EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C2EN, DISABLE);
		}
	}
	else if(i2c_instance == I2C3) {
		if(en_status == ENABLE) {
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C3EN, ENABLE);
		}
		else if(en_status == DISABLE){
			rcc_set_pclk_apb1(RCC_APB1ENR_I2C3EN, DISABLE);		
		}
	}
}

/** @} */