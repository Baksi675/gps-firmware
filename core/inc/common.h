/**
 * @file common.h
 * @author github.com/Baksi675
 * @brief Common utility module public API.
 *
 * @details
 * This module provides general-purpose utility functions and type definitions
 * shared across all other modules, including:
 * - String manipulation (copy, compare, tokenize, convert)
 * - Numeric conversion (integer, double, hex to/from string)
 * - Bit extraction from byte arrays
 * - Common type definitions (pin state, enable state, direction)
 * - Convenience macros (BCD conversion, blocking delay)
 *
 * @version 0.1
 * @date 2026-01-22
 *
 * @copyright Copyright (c) 2026
 *
 */

/**
 * @defgroup common_module Common Utility Module
 * @brief General-purpose utilities shared across all modules.
 * @{
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup common_macros Common Macros
 * @ingroup common_module
 * @brief Convenience macros used across the project.
 * @{
 */

/**
 * @brief Converts a BCD-encoded byte to its decimal equivalent.
 *
 * @param BCD A byte where the upper nibble holds the tens digit
 *            and the lower nibble holds the units digit.
 */
#define BCD_TO_DEC(BCD)     (((BCD) >> 4) * 10 + ((BCD) & 0x0F))

/**
 * @brief Converts a decimal value to its BCD-encoded byte equivalent.
 *
 * @param DEC A decimal integer value (0–99).
 */
#define DEC_TO_BCD(DEC)     (((DEC / 10) << 4) | (DEC % 10))

/**
 * @brief Blocking delay using the system tick counter.
 *
 * @details
 * Spins until @p ms milliseconds have elapsed since the macro was entered.
 * Requires `systick_get_ms()` to be available in scope.
 *
 * @param ms Number of milliseconds to wait.
 */
#define DELAY(ms)                               \
    do {                                        \
        uint32_t delay_start_time = systick_get_ms(); \
        while ((systick_get_ms() - delay_start_time) < (ms)) { \
        }                                       \
    } while (0)

/** @} */

/**
 * @defgroup common_types Common Types
 * @ingroup common_module
 * @brief Common enumeration types used across modules.
 * @{
 */

/**
 * @brief Represents the logical level of a GPIO pin.
 */
typedef enum {
    /** Pin is at logic low (0 V). */
    LOW,

    /** Pin is at logic high (VCC). */
    HIGH
} PIN_STATUS_te;

/**
 * @brief Represents an enabled or disabled state.
 */
typedef enum {
    /** Feature or peripheral is disabled. */
    DISABLE,

    /** Feature or peripheral is enabled. */
    ENABLE
} EN_STATUS_te;

/**
 * @brief Represents a vertical movement direction.
 */
typedef enum {
    /** Downward direction. */
    DOWN,

    /** Upward direction. */
    UP
} VERTICAL_DIR_te;

/** @} */

/**
 * @defgroup common_api Common Public API
 * @ingroup common_module
 * @brief Public utility functions available to all modules.
 * @{
 */

/**
 * @brief Returns the length of a string, excluding the null terminator.
 *
 * @param[in] str Pointer to a null-terminated string.
 *
 * @return Number of characters before the null terminator.
 */
uint32_t get_str_len(char const *str);

/**
 * @brief Converts an integer to its decimal string representation.
 *
 * @details
 * Handles negative values. The result is written into @p str, which
 * must be large enough to hold the digits, an optional leading '-',
 * and the null terminator.
 *
 * @param[in]  num The integer to convert.
 * @param[out] str Pointer to the destination buffer.
 */
void int_to_str(int num, char *str);

/**
 * @brief Converts a decimal string to an integer.
 *
 * @details
 * Handles an optional leading '-' for negative values.
 * Non-digit characters after the sign are not validated.
 *
 * @param[in] str Pointer to a null-terminated decimal string.
 *
 * @return The parsed integer value.
 */
int str_to_int(const char *str);

/**
 * @brief Converts a double to a decimal string with a fixed number of fractional digits.
 *
 * @details
 * Handles negative values, including values in the range (-1, 0).
 * Leading zeros in the fractional part are preserved.
 *
 * @param[in]  num        The double value to convert.
 * @param[out] str        Pointer to the destination buffer. Must be large enough
 *                        for the integer part, '.', fractional digits, and null terminator.
 * @param[in]  frac_digits Number of digits to include after the decimal point.
 */
void double_to_str(double num, char *str, int8_t frac_digits);

/**
 * @brief Converts a single byte to a two-character uppercase hexadecimal string.
 *
 * @details
 * Does not append a null terminator. The caller must ensure @p str
 * points to a buffer of at least 2 bytes.
 *
 * @param[in]  byte The byte to convert.
 * @param[out] str  Pointer to a buffer that will receive the two hex characters.
 */
void hex_byte_to_str(uint8_t byte, char *str);

/**
 * @brief Overwrites a region of a target string with the contents of a source string.
 *
 * @details
 * Copies @p host_str_len characters from @p host_str into @p target_str
 * starting at byte offset @p pos. No null terminator is written.
 *
 * @param[in,out] target_str  Pointer to the string to modify.
 * @param[in]     host_str    Pointer to the source string.
 * @param[in]     host_str_len Number of characters to copy from @p host_str.
 * @param[in]     pos         Byte offset in @p target_str at which to begin writing.
 */
void str_set(char *target_str, char const *host_str, uint32_t host_str_len, uint32_t pos);

/**
 * @brief Computes an integer power.
 *
 * @param[in] base     The base value.
 * @param[in] exponent The exponent value. Must be non-negative.
 *
 * @return @p base raised to the power of @p exponent.
 */
int32_t get_pow(int32_t base, int32_t exponent);

/**
 * @brief Compresses an array by removing null bytes and shifting remaining elements left.
 *
 * @details
 * All non-null characters are packed to the front of the array.
 * Trailing positions are filled with null bytes.
 *
 * @param[in,out] arr Pointer to the character array to compress.
 * @param[in]     len Total length of the array in bytes.
 */
void arr_cmprs(char *arr, uint8_t len);

/**
 * @brief Compares two null-terminated strings for equality.
 *
 * @param[in] str1 Pointer to the first string.
 * @param[in] str2 Pointer to the second string.
 *
 * @return true if both strings are identical, false otherwise.
 */
bool str_cmp(const char *str1, const char *str2);

/**
 * @brief Converts two ASCII hex characters into a single byte value.
 *
 * @details
 * Accepts uppercase hex digits (0–9, A–F). Lowercase is not handled.
 *
 * @param[in] high The most significant nibble as an ASCII character (e.g. 'A').
 * @param[in] low  The least significant nibble as an ASCII character (e.g. 'F').
 *
 * @return The combined byte value (e.g. 'A', 'F' → 0xAF).
 */
uint8_t ascii_hex_to_byte(char high, char low);

/**
 * @brief Splits a string into tokens separated by a given delimiter.
 *
 * @details
 * Modifies @p str in place by replacing delimiter characters with null bytes.
 * Leading spaces before each token are skipped.
 * Pointers to the start of each token are stored in @p tokens.
 *
 * @param[in,out] str        Pointer to the null-terminated input string. Modified in place.
 * @param[in]     separator  Null-terminated string containing the delimiter character(s).
 * @param[in]     max_tokens Maximum number of tokens to extract.
 * @param[out]    tokens     Array of pointers that will be set to the start of each token.
 * @param[out]    num_tokens Pointer to a variable that will receive the number of tokens found.
 *
 * @return 0 on success, -1 if the token limit is exceeded or the input is empty.
 */
int str_tokenize(char *str, const char *separator, uint16_t max_tokens, char **tokens, uint16_t *num_tokens);

/**
 * @brief Converts a string representation of a boolean to a bool value.
 *
 * @details
 * Recognizes the strings `"true"` and `"false"` (case-sensitive).
 * Any other input returns false.
 *
 * @param[in] str Pointer to a null-terminated string.
 *
 * @return true if @p str equals `"true"`, false otherwise.
 */
bool str_to_bool(char const *str);

/**
 * @brief Copies a null-terminated string into a destination buffer.
 *
 * @details
 * Copies at most @p len - 1 characters from @p str_from into @p str_to
 * and always null-terminates the result. Stops early if the source
 * string ends before @p len - 1 characters are copied.
 *
 * @param[out] str_to   Pointer to the destination buffer.
 * @param[in]  str_from Pointer to the null-terminated source string.
 * @param[in]  len      Total size of the destination buffer in bytes,
 *                      including space for the null terminator.
 *
 * @return 0 on success, -1 if any argument is NULL or @p len is 0.
 */
int str_cpy(char *str_to, const char *str_from, uint32_t len);

/**
 * @brief Copies a fixed-length block of text into a destination buffer.
 *
 * @details
 * Unlike @ref str_cpy, this function copies exactly @p len bytes and does
 * not append a null terminator. It stops early if the source ends.
 *
 * @param[out] txt_to   Pointer to the destination buffer.
 * @param[in]  txt_from Pointer to the source text.
 * @param[in]  len      Number of bytes to copy.
 *
 * @return 0 on success, -1 if any argument is NULL or @p len is 0.
 */
int txt_cpy(char *txt_to, const char *txt_from, uint32_t len);

/**
 * @brief Checks whether a number is a power of two.
 *
 * @param[in] num The value to test.
 *
 * @return true if @p num is a non-zero power of two, false otherwise.
 */
bool is_pow(uint32_t num);

/**
 * @brief Extracts a range of bits from a big-endian byte array.
 *
 * @details
 * Bit numbering follows big-endian convention: bit 0 of the array is the
 * most significant bit of the first byte. Extracted bits are assembled
 * into the result with the first extracted bit as the MSB.
 *
 * @param[in] data      Pointer to the byte array to extract from.
 * @param[in] start_bit Index of the first bit to extract (0 = MSB of data[0]).
 * @param[in] num_bits  Number of consecutive bits to extract.
 *
 * @return The extracted bits packed into a uint32_t, MSB-first.
 */
uint32_t extract_bits(const uint8_t *data, uint16_t start_bit, uint8_t num_bits);

/** @} */

#endif

/** @} */