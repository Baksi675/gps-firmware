/**
 * @file cbuf.h
 * @author github.com/Baksi675
 * @brief Circular buffer module public API.
 *
 * @details
 * This module provides a lightweight circular (ring) buffer implementation
 * for byte-oriented data, including:
 * - Writing data from an input buffer
 * - Reading all available data into an output buffer
 * - Querying the number of bytes currently stored
 *
 * The buffer size must be a power of two, as the implementation uses
 * bitwise masking for wrap-around.
 *
 * Typical usage:
 * - Declare and populate a @ref CBUF_HANDLE_ts with a backing array and its size
 * - Write data using @ref cbuf_write
 * - Read data using @ref cbuf_read
 * - Query occupancy using @ref cbuf_len
 *
 * @version 0.1
 * @date 2026-01-23
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup cbuf_module Circular Buffer Module
 * @brief Lightweight circular buffer for byte-oriented data.
 * @{
 */

#ifndef CBUF_H__
#define CBUF_H__

#include "err.h"
#include <stdint.h>

/**
 * @defgroup cbuf_types Circular Buffer Types
 * @ingroup cbuf_module
 * @brief Data types used by the circular buffer module.
 * @{
 */

/**
 * @brief Handle representing a circular buffer instance.
 *
 * @details
 * This structure holds all state for a single circular buffer.
 * The caller is responsible for allocating the backing array and
 * setting @ref ptr and @ref size before first use.
 *
 * @note @ref size must be a power of two. The maximum usable capacity
 *       is @ref size - 1 bytes; one slot is reserved to distinguish
 *       full from empty.
 */
typedef struct {
    /** Pointer to the backing byte array. Must be @ref size bytes long. */
    uint8_t *ptr;

    /** Total size of the backing array in bytes. Must be a power of two. */
    uint8_t size;

    /** Index of the next byte to be read. */
    uint8_t read_position;

    /** Index of the next byte to be written. */
    uint8_t write_position;
} CBUF_HANDLE_ts;

/** @} */

/**
 * @defgroup cbuf_api Circular Buffer Public API
 * @ingroup cbuf_module
 * @brief Public functions to interact with a circular buffer instance.
 * @{
 */

/**
 * @brief Reads all available data from the circular buffer into an output buffer.
 *
 * @details
 * Drains the circular buffer completely, copying each byte in order into
 * @p output_buf_o and advancing the read position after each byte.
 * Stops when the buffer is empty.
 *
 * @param[in,out] cbuf_handle Pointer to the circular buffer to read from.
 * @param[out]    output_buf_o Pointer to the destination buffer. Must be large
 *                enough to hold all available bytes (up to @ref size - 1).
 *
 * @return
 * - ERR_OK on success
 */
ERR_te cbuf_read(CBUF_HANDLE_ts *cbuf_handle, uint8_t *output_buf_o);

/**
 * @brief Writes data from an input buffer into the circular buffer.
 *
 * @details
 * Copies up to @p input_len bytes from @p input_buf into the circular buffer,
 * advancing the write position after each byte.
 *
 * Writing stops early and returns @ref ERR_BUFFER_FULL if the buffer becomes
 * full before all @p input_len bytes have been written.
 *
 * @param[in,out] cbuf_handle Pointer to the circular buffer to write into.
 * @param[in]     input_buf   Pointer to the source data buffer.
 * @param[in]     input_len   Number of bytes to write.
 *
 * @return
 * - ERR_OK on success
 * - ERR_BUFFER_FULL if the buffer is full before all bytes could be written
 */
ERR_te cbuf_write(CBUF_HANDLE_ts *cbuf_handle, uint8_t *input_buf, uint32_t input_len);

/**
 * @brief Returns the number of bytes currently stored in the circular buffer.
 *
 * @details
 * The returned length ranges from 0 (empty) to @ref size - 1 (full).
 *
 * @param[in]  cbuf_handle Pointer to the circular buffer to query.
 * @param[out] len_o       Pointer to a variable that will receive the byte count.
 *
 * @return
 * - ERR_OK on success
 */
ERR_te cbuf_len(CBUF_HANDLE_ts const *cbuf_handle, uint8_t *len_o);

/** @} */

#endif

/** @} */