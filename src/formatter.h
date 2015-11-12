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

enum formatter_fmt
{
    FORMATTER_FMT_INVALID = 0,
    FORMATTER_FMT_HEX,
    FORMATTER_FMT_UNSIGNED_DEC,
    FORMATTER_FMT_SIGN_MAGNITUDE,
    FORMATTER_FMT_TWOS_COMPLEMENT,
    FORMATTER_FMT_FLOAT,
};

enum formatter_endianness
{
    FORMATTER_ENDIAN_INVALD = 0,
    FORMATTER_ENDIAN_BIG,
    FORMATTER_ENDIAN_LITTLE,
};

struct formatter_param {
    const char *name;
    const char value[80];
};

struct formatter_params {
    struct formatter_param *params;
    unsigned int count;
};

struct formatter;

struct formatter * formatter_init(unsigned int num_fields, unsigned int max_bit);

bool formatter_add_field(struct formatter *f,
                         const char *name,
                         const char *default_value,
                         unsigned int start_bit, unsigned int end_bit,
                         enum formatter_fmt format,
                         enum formatter_endianness endianness,
                         float scaling, float offset);

bool formatter_initialized(struct formatter *f);

/* Assumes data is large enough for number of bits (max_bit) provided to
 * formatter_init() */
bool formatter_data_to_keyval(const struct formatter *f,
                              const uint8_t *data,
                              struct keyval_list *kv_list);

/* Assumes data is large enough for number of bits (max_bit) provided to
 * formatter_init() */
bool formatter_keyval_to_data(const struct formatter *f,
                              const struct keyval_list *kv_list,
                              uint8_t *data);

/* Assumes data is large enough for number of bits (max_bit) provided to
 * formatter_init() */
void formatter_default_data(const struct formatter *f, uint8_t *data);

enum formatter_endianness formatter_endianess_value(const char *str);

enum formatter_fmt formatter_fmt_value(const char *str);

void formatter_deinit(struct formatter *f);

#endif

