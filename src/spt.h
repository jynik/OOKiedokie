/*
 * This file defines simple polymorphic type implementation that's intended
 * for small values and floating point values of little precision. It is
 * designed to be packed/unpacked into message fields.
 *
 * Copyright (C) 2015 Jon Szymaniak <jon.szymaniak@gmail.com>
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

#ifndef SPT_INCLUDED_H_
#define SPT_INCLUDED_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Simple Polymorphic Type (SPT) values are stored as an int64_t
 */
typedef int64_t spt;

/* Minimum SPT value */
#define SPT_MAX INT64_MAX

/* Maximum SPT value */
#define SPT_MIN INT64_MIN

static inline spt spt_from_uint64(uint64_t value)   { return (spt) value; };

static inline spt spt_from_int64(int64_t value)     { return (spt) value; };

static inline spt spt_from_uint32(uint32_t value)   { return (spt) value; };

static inline spt spt_from_int32(int32_t value)     { return (spt) value; };

static inline spt spt_from_int16(int16_t value)     { return (spt) value; };

static inline spt spt_from_uint16(uint16_t value)   { return (spt) value; };

static inline spt spt_from_int8(int8_t value)       { return (spt) value; };

static inline spt spt_from_uint8(uint8_t value)     { return (spt) value; };

static inline spt spt_from_float(float value, float scaling, float offset)
{
    float tmp = (value - offset) / scaling;
    return (spt) tmp;
}

static inline int64_t spt_to_int64(spt value)       { return (int64_t) value; }

static inline uint64_t spt_to_uint64(spt value)     { return (uint64_t) value; }

static inline int32_t spt_to_int32(spt value)       { return (int32_t) value; }

static inline uint32_t spt_to_uint32(spt value)     { return (uint32_t) value; }

static inline int16_t spt_to_int16(spt value)       { return (int16_t) value; }

static inline uint16_t spt_to_uint16(spt value)     { return (uint16_t) value; }

static inline int16_t spt_to_int8(spt value)        { return (int8_t) value; }

static inline uint16_t spt_to_uint8(spt value)      { return (uint8_t) value; }

static inline float spt_to_float(spt value, float scaling, float offset)
{
    return (float) value * scaling + offset;
}

#endif
