/**
 * @file common.c
 * @author github.com/Baksi675
 * @brief A module that contains code common to all other modules
 * @version 0.1
 * @date 2025-09-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>

#include "common.h"

/** 
 * @defgroup COMMON_Public_APIs COMMON Public APIs
 * @{
 */

 /**
  * @brief Returns the length of the string given in as argument (without null terminator).
  * 
  * @param str The string to return the length of.
  * @return uint32_t The length of the string without a null terminator.
  */
uint32_t get_str_len(char const *str) {
	uint32_t len = 0;

	while(*str++) {
		len++;
	}

	return len;
}

/**
 * @brief Converts an integer to a string.
 * 
 * @param num The integer to convert to a string to.
 * @param str A preinitialized string which will contain the integer as a string.
 */
void int_to_str(int num, char *str) {
    char *start = str;
    bool is_negative = false;

    if (num < 0) {
        is_negative = true;
        num = -num;  
    }
    if (num == 0) {
        *str++ = '0';
    }

    while (num > 0) {
       	uint8_t digit = num % 10;
        num = num / 10;

        *str++ = (char)(digit + '0');
    }

    if (is_negative) {
        *str++ = '-';
    }

    *str = '\0';

    char *end = str - 1;
    while (start < end) {
        char tmp = *start;
        *start = *end;
        *end = tmp;
        start++;
        end--;
    }
}

/**
 * @brief Converts a string containing numerical characters to an integer type.
 * 
 * @param str The string to convert to integer.
 * @return int The converted integer type.
 */
int str_to_int(const char* str) {
	bool negative = false;
	uint8_t num_array[12] = { 0 };
	uint8_t digit_counter = 0;
	int32_t num = 0;

	if(*str == '-') {
		negative = true;
		str++;
	}

	while(*str) {
		num_array[digit_counter] = *str - 48;

		digit_counter++;
		str++;
	}

	uint32_t multiplier = 1;
	for(int8_t i = digit_counter - 1; i >= 0 ; i--) {
		num += num_array[i] * multiplier;

		multiplier *= 10;
	}

	if(negative) {
		num *= -1;
	}

	return num;
}

/**
 * @brief Converts a double type to string type.
 * 
 * @param num The double type to convert.
 * @param str Pointer to a string buffer that will contain the converted number.
 * @param frac_digits Number of digits in the fraction part.
 */
void double_to_str(double num, char *str, int8_t frac_digits) {
	int32_t int_part;
	double frac_part;
	uint8_t int_part_len;
	uint8_t frac_part_len;
	uint8_t total_len = 0;
	double addend;
	uint8_t missing_zeroes = 0;
	bool negative_0_to_1 = false;

	if(num > -1 && num < 0) {					
		negative_0_to_1 = true;
	}

	int_part = (int32_t)num;
	frac_part = ((num - (double)int_part) * get_pow(10, frac_digits));

	addend = 1.0 / (20 * get_pow(10, frac_digits));
	
	if(frac_part < 0) {
		frac_part -= addend;
		frac_part *= -1;
	}
	else {
		frac_part += addend;
	}

	for(uint8_t i = 1; i <= frac_digits; i++) {
		int32_t comp_to_frac = (get_pow(10, frac_digits - i));

		if(frac_part < comp_to_frac) {
			missing_zeroes++;
		}
		else {
			break;
		}
	}
	total_len += missing_zeroes;

	char int_buf[20] = { 0 };
	if(negative_0_to_1) {
		int_buf[0] = '-';
		int_to_str(int_part, int_buf + 1);
	}
	else {
		int_to_str(int_part, int_buf);
	}
	int_part_len = get_str_len(int_buf);
	total_len += int_part_len;

	char double_buf[12] = { 0 };
	int_to_str((int32_t)frac_part, double_buf);
	frac_part_len = get_str_len(double_buf);
	total_len += frac_part_len;

	int_buf[int_part_len] = '.';
	total_len++;

	for(uint8_t i = 0; i < missing_zeroes; i++) {
		int_buf[int_part_len + 1 + i] = '0';
	}

	str_set(int_buf, double_buf, frac_part_len, total_len - frac_part_len);

	int_buf[total_len] = '\0';
	total_len++;

	for(uint8_t i = 0; i < total_len; i++) {
		str[i] = int_buf[i];
	} 
}

/**
 * @brief Converts a byte in hexadecimal format to a string.
 * 
 * @param byte The byte to convert.
 * @param str A pointer to a preinitialized string which will contain the byte as a string in hexadecimal format.
 */
void hex_byte_to_str(uint8_t byte, char *str) {
	uint8_t first_part = byte / 16;
	uint8_t second_part = byte % 16;

	if(first_part < 10) {
		str[0] = (byte / 16) + 48;
	}
	else {
		str[0] = (byte / 16) + 55;
	}

	if(second_part < 10) {
		str[1] = (byte % 16) + 48;
	}
	else {
		str[1] = (byte % 16) + 55;
	}
}

/**
 * @brief Replaces characters in the target string with that of the host string on the given position.
 * 
 * @param target_str The string to modify.
 * @param host_str The string used to modify.
 * @param host_str_len The length of the string used to modify.
 * @param pos The position in the target string where the modification should take place from.
 */
void str_set(char *target_str, char const *host_str, uint32_t host_str_len, uint32_t pos) {
	uint32_t pos_counter = 0;
	bool pos_reached = false;
	uint32_t bytes_replaced = 0;

	while(bytes_replaced != host_str_len) {
		if(pos_counter == pos) {
			pos_reached = true;
		}

		if(pos_reached) {
			*target_str = host_str[bytes_replaced];
			bytes_replaced++;

			if(bytes_replaced == host_str_len) {
				break;
			}
		}

		target_str++;
		pos_counter++;
	}
}

/**
 * @brief Power of function implementation.
 * 
 * @param base The base number.
 * @param exponent The base number raised to the power of this number.
 * @return int32_t The result of the computation.
 */
int32_t get_pow(int32_t base, int32_t exponent) {
	int32_t result = 1;

	for(int32_t i = 0; i < exponent; i++) {
		result *= base;
	}

	return result;
}

/**
 * @brief Compresses the array given in the parameter.
 * 
 * @param arr A pointer to the array to be compressed.
 * @param len The length of the array to be compressed.
 */
void arr_cmprs(char *arr, uint8_t len) {
	uint8_t write = 0;

	for(uint8_t read = 0; read < len; read++) {
		if(arr[read] != '\0') {
			arr[write] = arr[read];
			write++;
		}
	}

	while(write < len) {
		arr[write] = '\0';
		write++;
	}
}

/**
 * @brief Compares to strings, returns true if they match.
 * 
 * @param str1 First string to compare. 
 * @param str2 Second string to compare.
 * @return bool true if they match, false if they don't.
 */
bool str_cmp(const char *str1, const char *str2) {
	while(*str1 != '\0' && *str2 != '\0') {
		if(*str1 != *str2) {
			return false;
		}
		str1++;
		str2++;
	}

	if(*str1 == '\0' && *str2 == '\0') {
		return true;
	}

	return false;
}

/**
 * @brief Converts two ASCII characters into a single HEX ASCII character.
 * 
 * @param high Most significant half.
 * @param low Least significant half.
 * @return uint8_t The single HEX byte.
 */
uint8_t ascii_hex_to_byte(char high, char low) {
    uint8_t value = 0;
    uint8_t nibble;

    if (high >= '0' && high <= '9')
        nibble = (uint8_t)(high - '0');
    else
        nibble = (uint8_t)(high - 'A' + 10);

    value = (uint8_t)(nibble << 4);

    if (low >= '0' && low <= '9')
        nibble = (uint8_t)(low - '0');
    else
        nibble = (uint8_t)(low - 'A' + 10);

    value |= nibble;

    return value;
}


/**
 * @brief Converts the input text to a set of tokens.
 * 
 * @param str The input text to convert.
 * @param separator The character that separates the tokens in the original text.
 * @param max_tokens The max number of tokens allowed.
 * @param tokens The output array which contains the addresses of the tokens.
 * @param num_tokens The number of tokens (output).
 * @return int8_t Returns 0 if everything went successfully, -1 if some errors occured.
 */
int str_tokenize(char *str, const char *separator, uint16_t max_tokens, char **tokens, uint16_t *num_tokens) {
	while(true) {
		while(*str == ' ') {
			str++;
		}

		if(*str == '\0') {
			return -1;
		}

		if(*num_tokens <= max_tokens) {			
			tokens[*num_tokens] = str;
			*num_tokens = *num_tokens + 1;
		}
		else {
			return -1;
		}

		while(*str && *str != *separator) {
			str++;
		}

		if(*str == '\0') {
			break;
		}

		*str++ = '\0';
	}

	return 0;
}

/**
 * @brief Converts a bool value in string format to bool type.
 * 
 * @param str The bool in string format.
 * @return bool The converted bool.
 */
bool str_to_bool(char const *str) {
	if(str_cmp(str, "true") == true) {
		return true;
	}
	else if(str_cmp(str, "false") == true) {
		return false;
	}

	// Error
	return false;
}

/**
 * @brief Copies a string from source to destination.
 * 
 * @param txt_to Where to copy the string.
 * @param txt_from Where to copy the string from.
 * @param len The length of the string to copy.
 * @return int 0 if succeeded.
 */
int str_cpy(char *str_to, const char *str_from, uint32_t len)
{
    if (len == 0 || str_to == NULL || str_from == NULL) {
        return -1;
    }

    while (*str_from && len > 1) {
        *str_to++ = *str_from++;
        len--;
    }

    *str_to = '\0';   // always null-terminate

    return 0;
}

/**
 * @brief Copies a text from source to destination.
 * 
 * @param txt_to Where to copy the text.
 * @param txt_from Where to copy the text from.
 * @param len The length of the text to copy.
 * @return int 0 if succeeded.
 */
int txt_cpy(char *txt_to, const char *txt_from, uint32_t len) {
    if (len == 0 || txt_to == NULL || txt_from == NULL) {
        return -1;
    }

    while (*txt_from && len != 0) {
        *txt_to++ = *txt_from++;
        len--;
    }

    return 0;	
}

/**
 * @brief Determines whether a number is power of two.
 * 
 * @param num The input number
 * @return int Returns true if a number is 2^n, if not it returns false.
 */
bool is_pow(uint32_t num) {
    return num && !(num & (num - 1));
}

/**
 * @brief Extracts a bit range from a given data structure.
 * 
 * @param[in] data Pointer to the data structure. 
 * @param[in] start_bit Starting bit position.
 * @param[in] num_bits Number of bits to extract from starting bit position. 
 * @return uint32_t The extracted bits.
 */
uint32_t extract_bits(const uint8_t *data, uint16_t start_bit, uint8_t num_bits) {
    uint32_t result = 0;
    uint16_t byte_index = start_bit / 8;
    uint8_t bit_offset = start_bit % 8;
    
    for (int i = 0; i < num_bits; i++) {
        // SD card registers are big-endian, bit 127 is MSB of first byte
        if (data[byte_index] & (1 << (7 - bit_offset))) {
            result |= (1 << (num_bits - 1 - i));
        }
        
        bit_offset++;
        if (bit_offset >= 8) {
            bit_offset = 0;
            byte_index++;
        }
    }
    
    return result;
}



/** @} */