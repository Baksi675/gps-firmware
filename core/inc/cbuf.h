/**
 * @file cbuf.h
 * @author github.com/Baksi675
 * @brief Circular buffer header file
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef CBUF_H__
#define CBUF_H__

#include <stdint.h>

typedef struct {
  uint8_t *ptr;
  uint8_t size;
  uint8_t read_position;
  uint8_t write_position;
}CBUF_HANDLE_ts;

void cbuf_read(CBUF_HANDLE_ts *cbuf_handle, uint8_t *output_buf);
void cbuf_write(CBUF_HANDLE_ts *cbuf_handle, uint8_t *input_buf, uint32_t input_len);
uint8_t cbuf_len(CBUF_HANDLE_ts *cbuf_handle);

#endif