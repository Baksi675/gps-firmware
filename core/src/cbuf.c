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
#include "log.h"
#include <stdint.h>

 /** 
 * @defgroup CB_Public_APIs CB Public APIs
 * @{
 */

/**
 * @brief Reads the content of the circular buffer into an output buffer.
 * 
 * @param cbuf_handle The circular buffer to read from.
 * @param output_buf_o The output buffer to read to.
 * @return ERR_te Error code produced during execution
 */
ERR_te cbuf_read(CBUF_HANDLE_ts *cbuf_handle, uint8_t* output_buf_o) {
	ERR_te err;
	uint8_t len = 0;

	do {
		err = cbuf_len(cbuf_handle, &len);
		if(err != ERR_OK) {
			return ERR_UNKNOWN;
		}

		*output_buf_o = cbuf_handle->ptr[cbuf_handle->read_position];
		output_buf_o++;
		cbuf_handle->read_position = (cbuf_handle->read_position + 1) & (cbuf_handle->size - 1);
	}while (len != 0);

	return ERR_OK;
}

/**
 * @brief Writes data from an input buffer into a circular buffer.
 * 
 * @param cbuf_handle The circular buffer to write data into.
 * @param input_buf The input buffer to write data from.
 * @param input_len The length of the data to write from the input buffer.
 */
ERR_te cbuf_write(CBUF_HANDLE_ts *cbuf_handle, uint8_t* input_buf, uint32_t input_len) {
	ERR_te err;
	uint8_t len;
	
	while(input_len != 0) {
		err = cbuf_len(cbuf_handle, &len);
		if(err != ERR_OK) {
			return ERR_UNKNOWN;
		}

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

/**
 * @brief Returns the length of the data in the circular buffer. 0 ==> empty, size - 1 ==> full.
 * 
 * @param cbuf_handle A pointer to the circular buffer object.
 * @return uint8_t  The length of the data in the circular buffer.
 */
ERR_te cbuf_len(CBUF_HANDLE_ts *cbuf_handle, uint8_t *len) {	
	*len = (cbuf_handle->write_position - cbuf_handle->read_position) & (cbuf_handle->size - 1);
	
	return ERR_OK;
}

  /** @} */