/*
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
#ifndef FORMATTER_H_
#define FORMATTER_H_

#include <stdint.h>
#include <stdbool.h>

#include "keyval_list.h"
#include "spt.h"

/**
 * Formatter field formats.
 *
 * This describes how bits in a field are interpreted and displayed.
 */
enum formatter_fmt
{
    FORMATTER_FMT_INVALID = 0,      /**< Invalid format selection */
    FORMATTER_FMT_HEX,              /**< Unsigned value presented in hex */
    FORMATTER_FMT_UNSIGNED_DEC,     /**< Unsigned value presented in decimal */
    FORMATTER_FMT_SIGN_MAGNITUDE,   /**< Sign-magnitude value, decimal */
    FORMATTER_FMT_TWOS_COMPLEMENT,  /**< Two's complement value, decimal */
    FORMATTER_FMT_FLOAT,            /**< Floating point value */
    FORMATTER_FMT_ENUM,             /**< Enumerated hex values */
};

/**
 * Formatter field endianness.
 *
 * This describes the BIT endianness to use when inserting data into or
 * extracting data from a field.
 */
enum formatter_endianness
{
    FORMATTER_ENDIAN_INVALD = 0,    /**< Invalid endianness selection */
    FORMATTER_ENDIAN_BIG,           /**< Big endian bit ordering */
    FORMATTER_ENDIAN_LITTLE,        /**< Little endian bit ordering */
};

/**
 * Reception timestamping mode.
 *
 * Note that the timestamps reflect when the host has parsed them message - not
 * when the signal has entered the HW RFFE.
 */
enum formatter_ts_mode
{
    FORMATTER_TS_INVALID = 0,   /**< Invalid mode selection */
    FORMATTER_TS_NONE,          /**< Do not timestamp received messages */
    FORMATTER_TS_UNIX_INT,      /**< Integer seconds since Unix Epoch */
    FORMATTER_TS_UNIX_FRAC,     /**< Fractional seconds since Unix Epoch */
    FORMATTER_TS_DATETIME_24,   /**< Date and 24-hour time */
    FORMATTER_TS_DATETIME_AMPM, /**< Data and 12-hour time with am/pm */
};

/**
 * Formatter field parameter name/value pair
 */
struct formatter_param {
    const char *name;       /**< Parameter name */
    const char value[80];   /**< Parameter value, as a string */
};

/**
 * Formatter field parameter list
 */
struct formatter_params {
    struct formatter_param *params;     /**< Array of parameters */
    unsigned int count;                 /**< Array size, in elements */
};

/**
 * Opaque handle to a formatter object
 */
struct formatter;

/**
 * Create a formatter object
 *
 * @param   num_fields      Total number of fields that will be added via calls
 *                          to formatter_add_field().
 *
 * @param   max_bits        Total length of all fields fields, in bits.
 *
 * @param   ts_mode         Timestamping mode
 *
 * @return Formatter handle on success, or NULL on failure
 */
struct formatter * formatter_init(unsigned int num_fields, unsigned int max_bit,
                                  enum formatter_ts_mode ts_mode);

/**
 * Add a filed description to the specified formatter object
 *
 * @param   f               Formatter object to add field description to
 *
 * @param   name            Name of the field to add
 *
 * @param   start_bit       Position of first bit of the field, with respect
 *                          to the message it is found in. 0 is the left-most
 *                          bit.
 *
 * @param   end_bit         Position of last bit of the field, with respect
 *                          to the message it is found in. 0 is the left-most
 *                          bit. This must be > start_bit.
 *
 * @param   format          How this field is formatter and should be
 *                          interpreted.
 *
 * @param   enum_count      When format=FORMATTER_FMT_ENUM, this should
 *                          list the number of enumeraged values. Otherwise
 *                          it should be 0. When this is non-zero,
 *                          formatter_add_field_enum() must be called to add
 *                          each enum value.
 *
 * @param   endianness      Bit endianness of a value within its field.
 *
 * @param   scaling         How this a should be scaled when READING
 *                          it from the field. The inverse of this scale
 *                          factor will be applied when WRITING a value into the
 *                          field.
 *
 * @param   offset          And additional offset to add after scaling the
 *                          value when READING from a field. When WRITING to
 *                          the field, this value is subtracted prior to
 *                          scaling.
 *
 * @return true on success, for false on failure
 */
bool formatter_add_field(struct formatter *f,
                         const char *name,
                         unsigned int start_bit, unsigned int end_bit,
                         enum formatter_fmt format,
                         size_t enum_count,
                         enum formatter_endianness endianness,
                         float scaling, float offset);

/**
 * Add an enumeration string-value pair to the specified field
 *
 * @param   f               Formatter object to update
 * @param   field_name      Name of the field to update
 * @param   enum_name       Enumeration name string
 * @param   value           Value to associated with `enum_name`
 *
 * @return true on success, for false on failure
 */
bool formatter_add_field_enum(struct formatter *f, const char *field_name,
                              const char *enum_name, spt value);

/**
 * Set the default value of a field. This should be called afert
 * formatter_add_field() and formatter_add_field_enum(), if relevant.
 *
 * @param   f               Formatter object to update
 * @param   field_name      Name of the field to update
 * @param   default_value   Default field value, as a string.
 *
 * @return true on success, for false on failure
 */

bool formatter_set_field_default(struct formatter *f, const char *field_name,
                                 const char *default_value);

/**
 * Test if all fields in the formatter have been properly initialized
 *
 * @param   f       Formatter object to test
 *
 * @return true if initialized and ready for use, false otherwise
 */
bool formatter_initialized(struct formatter *f);

/**
 * Given binary data, this function extracts fields and provides a key-value
 * list of field and their values (as strings) using the provided formatter
 * to interpret the data.
 *
 * @param[in]   f           Formatter object to to extract fields from data
 *
 * @param[in]   data        Binary data to parse. It is assumed that this is
 *                          AT LEAST ceil(max_bits / 8) bytes in length, where
 *                          `max_bits` is the value supplied to
 *                          formatter_init().
 *
 * @parma[out]  kv_list     For each extracted data field, keyval_list_append()
 *                          is called on this list. It is assumed that kv_list
 *                          has been previously initialized.
 *
 * @return true on success, or false on failure
 */
bool formatter_data_to_keyval(const struct formatter *f,
                              const uint8_t *data,
                              struct keyval_list *kv_list);

/**
 * Use the provided formatter to convert a key-value list of fields to
 * their binary format. If a field is not specified in the input list,
 * its default value will be used.
 *
 * @param[in]   f           Formatter to use
 * @param[in]   kv_list     Key-value list of input parameters
 * @param[out]  data        This will be filled with the binary representation
 *                          of all the fields, populated in their respective
 *                          positions. It is assumed that this is AT LEAST
 *                          ceil(max_bits / 8) bytes in length, where `max_bits`
 *                          is the value supplied to formatter_init().
 *
 * @return true on success, false otherwise.
 */
bool formatter_keyval_to_data(const struct formatter *f,
                              const struct keyval_list *kv_list,
                              uint8_t *data);

/**
 * Get the binary representation of all fields when they are set to their
 * default values
 *
 * @param[in]       f       Formatter to use. It is assumed that this has
 *                          previously been initialized and had all of its
 *                          fields definitions configured.
 *
 * @param[out]      data    Binary representation of all fields in their default
 *                          values, located in their respective positions.
 *                          It is assumed that this is AT LEAST
 *                          ceil(max_bits / 8) bytes in length, where `max_bits`
 *                          is the value supplied to formatter_init().
 */
void formatter_default_data(const struct formatter *f, uint8_t *data);

/**
 * Convert a string to a FORMATTER_ENDIAN_* value, as listed below. This
 * function is case-insensitive.
 *
 * <pre>
 *      "big"           ->      FORMATTER_ENDIAN_BIG
 *      "little"        ->      FORMATTER_ENDIAN_LITTLE
 *      <other input>   ->      FORMATTER_ENDIAN_INVALD
 * </pre>
 *
 * @return  A FORMATTER_ENDIAN_* value as specified above
 */
enum formatter_endianness formatter_endianess_value(const char *str);

/**
 * Convert a string to a FORMATTER_FMT_* value, as listed below. This function
 * is case-insensitive.
 *
 * <pre>
 *      "hex"                   ->      FORMATTER_FMT_HEX
 *      "unsigned decimal"      ->      FORMATTER_FMT_UNSIGNED_DEC
 *      "sign-magnitude"        ->      FORMATTER_FMT_SIGN_MAGNITUDE
 *      "two's complement"      ->      FORMATTER_FMT_TWOS_COMPLEMENT
 *      "float"                 ->      FORMATTER_FMT_FLOAT
 *      <other input>           ->      FORMATTER_FMT_INVALID
 * </pre>
 *
 * @param   str     A string describing the desired format enum value.
 *
 * @return A FORMATTER_FMT_* value as specified above
 */
enum formatter_fmt formatter_fmt_value(const char *str);

/**
 * Convert a string to a FORMATTER_TS_* value, as listed below. This function
 * is case-insensitve.
 *
 * <pre>
 *      "none"                  ->      FORMATTER_TS_NONE (default)
 *      "unix"                  ->      FORMATTER_TS_UNIX_INT
 *      "unix-frac"             ->      FORMATTER_TS_UNIX_FRAC
 * </pre>
 *
 */
enum formatter_ts_mode formatter_ts_mode_value(const char *str);

/**
 * Deinitialize and deallocate the provided formatter.
 *
 * @param   f       Formatter to deinitialize
 */
void formatter_deinit(struct formatter *f);

#endif

