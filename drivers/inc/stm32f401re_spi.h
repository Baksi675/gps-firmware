/**
 * @file stm32f401re_spi.h
 * @author github.com/Baksi675
 * @brief SPI driver header file.
 * @version 0.1
 * @date 2026-01-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef STM32F401RE_SPI_DRIVER_H__
#define STM32F401RE_SPI_DRIVER_H__

#include "stm32f401re.h"
#include "common.h"

typedef enum {
	SPI_MODE_SLAVE,
	SPI_MODE_MASTER
}SPI_MODE_te;

typedef enum {
	SPI_DATA_FRAME_FORMATE_8_BIT,
	SPI_DATA_FRAME_FORMAT_16_BIT
}SPI_DATA_FRAME_FORMAT_te;

typedef enum {
	SPI_CLOCK_POLARITY_0_IDLE,
	SPI_CLOCK_POLARITY_1_IDLE
}SPI_CLOCK_POLARITY_te;

typedef enum {
	SPI_CLOCK_PHASE_FIRST_TRANSITION,
	SPI_CLOCK_PHASE_SECOND_TRANSITION
}SPI_CLOCK_PHASE_te;

typedef enum {
	SPI_BIT_FIRST_MSB,
	SPI_BIT_FIRST_LSB
}SPI_BIT_FIRST_te;

typedef enum {
	SPI_SLAVE_SELECT_MODE_HW,
	SPI_SLAVE_SELECT_MODE_SW
}SPI_SLAVE_SELECT_MODE_te;

typedef enum {
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_2,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_4,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_8,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_16,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_32,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_64,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_128,
	SPI_MASTER_SCLK_SPEED_PCLK_DIV_256
}SPI_MASTER_SCLK_SPEED_te;
	
typedef struct {
	SPI_REGDEF_ts *instance;			
	SPI_MODE_te mode;
	SPI_DATA_FRAME_FORMAT_te data_frame_format;
	SPI_CLOCK_POLARITY_te clock_polarity;
	SPI_CLOCK_PHASE_te clock_phase;
	SPI_BIT_FIRST_te bit_first;
	SPI_SLAVE_SELECT_MODE_te slave_select_mode;
	SPI_MASTER_SCLK_SPEED_te master_sclk_speed;
}SPI_HANDLE_ts;

void spi_init(SPI_HANDLE_ts *spi_handle);
void spi_deinit(SPI_REGDEF_ts const *spi_instance);
void spi_send(SPI_REGDEF_ts *spi_instance, uint8_t *tx_buffer, uint32_t len);
void spi_receive(SPI_REGDEF_ts *spi_instance, uint8_t *rx_buffer, uint32_t len);
void spi_set_comm(SPI_REGDEF_ts *spi_instance, EN_STATUS_te en_status);

#endif