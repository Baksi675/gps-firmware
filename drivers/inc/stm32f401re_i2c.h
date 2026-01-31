/**
 * @file stm32f401re_i2c.h
 * @author github.com/Baksi675
 * @brief Header file for STM32F401RE I2C driver.
 * @version 0.1
 * @date 2026-01-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 
#ifndef STM32F401RE_I2C_H__
#define STM32F401RE_I2C_H__

#include "stm32f401re.h"
#include "common.h"

#define I2C_NAME_LEN		4

typedef enum {
	I2C_CLOCK_STRETCH_ON,
	I2C_CLOCK_STRETCH_OFF
}I2C_CLOCK_STRECH_te;

typedef enum {
	I2C_ACK_OFF,
	I2C_ACK_ON
}I2C_ACK_te;

typedef enum {
	I2C_SPEED_100kHz = 100000,
	I2C_SPEED_400kHz = 400000
}I2C_SPEED_te;

typedef struct {
	I2C_REGDEF_ts *i2c_instance;
	I2C_CLOCK_STRECH_te i2c_clock_strech;
	uint8_t i2c_address;
	I2C_SPEED_te i2c_speed;
}I2C_HANDLE_ts;

void i2c_init(I2C_HANDLE_ts *i2c_object);
void i2c_deinit(I2C_REGDEF_ts const *i2c_instance);
void i2c_master_send(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *tx_buffer, uint32_t len);
void i2c_master_send_continue(I2C_REGDEF_ts *i2c_instance, uint8_t *tx_buffer, uint32_t len);
void i2c_master_receive(I2C_REGDEF_ts *i2c_instance, uint8_t slave_addr, uint8_t *rx_buffer, uint32_t len);
void i2c_master_set_comm(I2C_REGDEF_ts *i2c_instance, EN_STATUS_te en_status);
void i2c_get_name(I2C_REGDEF_ts const *i2c_instance, char *name);

#endif