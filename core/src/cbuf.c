/**
 * @file cbuf.c
 * @author github.com/Baksi675
 * @brief Circular buffer implementation
 * @version 0.1
 * @date 2025-10-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "cbuf.h"
#include "err.h"
#include <stdint.h>

/** 
 * @defgroup cbuf_public_apis Circular Buffer Public APIs
 * @{
 */

/** @brief Reads all available data from the circular buffer. @see cbuf_read */
ERR_te cbuf_read(CBUF_HANDLE_ts *cbuf_handle, uint8_t* output_buf_o) {
	uint8_t len = 0;

	while(1) {
		cbuf_len(cbuf_handle, &len);

		if(len == 0) {
			break;
		}

		*output_buf_o = cbuf_handle->ptr[cbuf_handle->read_position];
		output_buf_o++;
		cbuf_handle->read_position = (cbuf_handle->read_position + 1) & (cbuf_handle->size - 1);
	}

	return ERR_OK;
}

/** @brief Writes data from an input buffer into the circular buffer. @see cbuf_write */
ERR_te cbuf_write(CBUF_HANDLE_ts *cbuf_handle, uint8_t* input_buf, uint32_t input_len) {
	uint8_t len;
	
	while(input_len != 0) {
		cbuf_len(cbuf_handle, &len);

		if(len == cbuf_handle->size - 1) {
			return ERR_BUFFER_FULL;
		}

		cbuf_handle->ptr[cbuf_handle->write_position] = *input_buf;
		input_len--;
		input_buf++;
		cbuf_handle->write_position = (cbuf_handle->write_position + 1) & (cbuf_handle->size - 1);
	}

	return ERR_OK;
}

/** @brief Returns the number of bytes currently stored in the circular buffer. @see cbuf_len */
ERR_te cbuf_len(CBUF_HANDLE_ts const *cbuf_handle, uint8_t *len_o) {	
	*len_o = (cbuf_handle->write_position - cbuf_handle->read_position) & (cbuf_handle->size - 1);
	
	return ERR_OK;
}

/** @} */