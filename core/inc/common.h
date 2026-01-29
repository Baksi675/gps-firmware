/**
 * @file common.h
 * @author github.com/Baksi675
 * @brief A module that contains code common to all other modules
 * @version 0.1
 * @date 2026-01-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdbool.h>

#define BCD_TO_DEC(BCD)  		(((BCD) >> 4) * 10 + ((BCD) & 0x0F))
#define DEC_TO_BCD(DEC)			(((DEC / 10) << 4) | (DEC % 10))
#define NULL					((void*)0)

/**
 * @brief Pin status definition
 * 
 */
typedef enum {
    LOW,
    HIGH
}PIN_STATUS_te;

/**
 * @brief Enable / disable status definition
 * 
 */
typedef enum {
    DISABLE,
    ENABLE
}EN_STATUS_te;

/**
 * @brief Vertical direction definition
 * 
 */
typedef enum {
	DOWN,
	UP
}VERTICAL_DIR_te;

uint32_t get_str_len(char const *str);
void int_to_str(int num, char *str);
int str_to_int(const char* str);
void str_set(char *target_str, char const *host_str, uint32_t host_str_len, uint32_t pos);
void double_to_str(double num, char *str, int8_t frac_digits);
void hex_byte_to_str(uint8_t byte, char *str);
int32_t get_pow(int32_t base, int32_t exponent);
void arr_cmprs(char *arr, uint8_t len);
bool str_cmp(const char *str1, const char *str2);
uint8_t ascii_hex_to_byte(char high, char low);
int str_tokenize(char *str, const char *separator, uint16_t max_tokens, char **tokens, uint16_t *num_tokens);
bool str_to_bool(char const *str);
int str_cpy(char *str_to, const char *str_from, uint32_t len);
int txt_cpy(char *txt_to, const char *txt_from, uint32_t len);
bool is_pow(uint32_t num);

#endif 