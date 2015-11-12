/**
 * @file conversions.h
 *
 * @brief Miscellaneous conversion routines
 *
 * This file is a modified version of the conversions.h included in the bladeRF
 * project: http://www.github.com/nuand/bladeRF
 *
 * Copyright (c) 2013 Nuand LLC
 * Copyright (c) 2015 Jon Szymaniak <jon.szymaniak@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef CONVERSIONS_H__
#define CONVERSIONS_H__

#include <stdint.h>

/**
 * Represents an association between a string suffix for a numeric value and
 * its multiplier. For example, "k" might correspond to 1000.
 */
typedef struct numeric_suffix {
    const char *suffix;
    int multiplier;
} numeric_suffix;

/**
 * String to integer conversion with range and error checking
 *
 *  @param  str     String to convert
 *  @param  min     Inclusive minimum allowed value
 *  @param  max     Inclusive maximum allowed value
 *  @param  ok      If non-NULL, this will be set to true to denote that
 *                  the conversion succeeded. If this value is not true,
 *                  then the return value should not be used.
 *
 * @return 0 on success, undefined on failure
 */
int str2int(const char *str, int min, int max, bool *ok);

/**
 * Convert a string to an unsigned inteager with range and error checking
 *
 * @param[in]   str     String to conver
 * @param[in]   min     Minimum allowed value (inclusive)
 * @param[in]   max     Maximum allowed value (inclusive)
 * @param[out]  ok      If non-NULL, this is set to true if the conversion was
 *                      successful, and false for an invalid or out of range
 *                      value.
 *
 * @return  Converted value on success, 0 on failure
 */
unsigned int str2uint(const char *str,
                      unsigned int min, unsigned int max, bool *ok);

/**
 * Convert a string to a signed 64-bit integer with range and error checking
 *
 * @param[in]   str     String to conver
 * @param[in]   min     Minimum allowed value (inclusive)
 * @param[in]   max     Maximum allowed value (inclusive)
 * @param[out]  ok      If non-NULL, this is set to true if the conversion was
 *                      successful, and false for an invalid or out of range
 *                      value.
 *
 * @return  Converted value on success, 0 on failure
 */
int64_t str2int64(const char *str, int64_t min, int64_t max, bool *ok);

/**
 * Convert a string to an unsigned 64-bit integer with range and error checking
 *
 * @param[in]   str     String to conver
 * @param[in]   min     Minimum allowed value (inclusive)
 * @param[in]   max     Maximum allowed value (inclusive)
 * @param[out]  ok      If non-NULL, this is set to true if the conversion was
 *                      successful, and false for an invalid or out of range
 *                      value.
 *
 * @return  Converted value on success, 0 on failure
 */
uint64_t str2uint64(const char *str, uint64_t min, uint64_t max, bool *ok);

/**
 * Convert a string to a double with range and error checking
 *
 * @param[in]   str     String to conver
 * @param[in]   min     Minimum allowed value (inclusive)
 * @param[in]   max     Maximum allowed value (inclusive)
 * @param[out]  ok      If non-NULL, this is set to true if the conversion was
 *                      successful, and false for an invalid or out of range
 *                      value.
 *
 * @return  Converted value on success, 0 on failure
 */
double str2double(const char *str, double min, double max, bool *ok);

/**
 * Convert a string to an unsigned integer with range and error checking.
 * Supports the use of decimal representations and suffixes in the string.
 * For example, a string "2.4G" might be converted to 2400000000.
 *
 * @param[in]   str     String to convert
 * @param[in]   min     Minimum allowed value (inclusive)
 * @param[in]   max     Maximum allowed value (inclusive)
 * @param[in]   suffixes    List of allowed suffixes and their multipliers
 * @param[in]   num_suffixes    Number of suffixes in the list
 * @param[out]  ok      If non-NULL, this is set to true if the conversion was
 *                      successful, and false for an invalid or out of range
 *                      value.
 *
 * @return  Converted value on success, 0 on failure
 */
unsigned int str2uint_suffix(const char *str,
                             unsigned int min, unsigned int max,
                             const struct numeric_suffix suffixes[],
                             size_t num_suffixes, bool *ok);

/**
 * Convert a string to a log level enumeration value
 *
 * @parma[in]   str   Input string
 * @param[out]  ok    The string was a valid value
 *
 * @return log level value if *ok == true, undefined otherwise
 */
enum log_level str2loglevel(const char *str, bool *ok);

#endif