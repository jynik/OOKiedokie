/*
 * This file is a modified version of the conversions.c included in
 * the bladeRF project: http://www.github.com/nuand/bladeRF
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "conversions.h"
#include "log.h"

enum str2args_parse_state {
    PARSE_STATE_IN_SPACE,
    PARSE_STATE_START_ARG,
    PARSE_STATE_IN_ARG,
    PARSE_STATE_IN_QUOTE,
    PARSE_STATE_ERROR
};


int str2int(const char *str, int min, int max, bool *ok)
{
    long value;
    char *endptr;

    errno = 0;
    value = strtol(str, &endptr, 0);

    if (errno != 0 || value < (long)min || value > (long)max ||
        endptr == str || *endptr != '\0') {

        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }
    return (int)value;
}

unsigned int str2uint(const char *str, unsigned int min, unsigned int max, bool *ok)
{
    unsigned long value;
    char *endptr;

    errno = 0;
    value = strtoul(str, &endptr, 0);

    if (errno != 0 ||
        value < (unsigned long)min || value > (unsigned long)max ||
        endptr == str || *endptr != '\0') {

        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }
    return (unsigned int)value;
}

int64_t str2int64(const char *str, int64_t min, int64_t max, bool *ok)
{
    long long value;
    char *endptr;

    errno = 0;
    value = strtol(str, &endptr, 0);

    if (errno != 0 || value < (long long)min || value > (long long)max ||
        endptr == str || *endptr != '\0') {

        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }

    return (int64_t) value;
}


uint64_t str2uint64(const char *str, uint64_t min, uint64_t max, bool *ok)
{
    unsigned long long value;
    char *endptr;

    errno = 0;
    value = strtoull(str, &endptr, 0);

    if (errno != 0 || endptr == str || *endptr != '\0' ||
        value < (unsigned long long)min || value > (unsigned long long)max) {

        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }

    return (uint64_t)value;
}

double str2double(const char *str, double min, double max, bool *ok)
{
    double value;
    char *endptr;

    errno = 0;
    value = strtod(str, &endptr);

    if (errno != 0 || value < min || value > max ||
        endptr == str || *endptr != '\0') {

        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }

    return value;
}

unsigned int str2uint_suffix(const char *str,
                             unsigned int min, unsigned int max,
                             const struct numeric_suffix suffixes[],
                             size_t num_suffixes, bool *ok)
{
    double value;
    char *endptr;
    size_t i;

    errno = 0;
    value = strtod(str, &endptr);

    /* If a number could not be parsed at the beginning of the string */
    if (errno != 0 || endptr == str) {
        if (ok) {
            *ok = false;
        }

        return 0;
    }

    /* Loop through each available suffix */
    for (i = 0; i < num_suffixes; i++) {
        /* If the suffix appears at the end of the number */
        if (!strcasecmp(endptr, suffixes[i].suffix)) {
            /* Apply the multiplier */
            value *= suffixes[i].multiplier;
            break;
        }
    }

    /* Check that the resulting value is in bounds */
    if (value > max || value < min) {
        if (ok) {
            *ok = false;
        }

        return 0;
    }

    if (ok) {
        *ok = true;
    }

    /* Truncate the floating point value to an integer and return it */
    return (unsigned int)value;
}

enum log_level str2loglevel(const char *str, bool *ok)
{
    enum log_level level = LOG_LEVEL_ERROR;
    bool valid = true;

    if (!strcasecmp(str, "critical")) {
        level = LOG_LEVEL_CRITICAL;
    } else if (!strcasecmp(str, "error")) {
        level = LOG_LEVEL_ERROR;
    } else if (!strcasecmp(str, "warning")) {
        level = LOG_LEVEL_WARNING;
    } else if (!strcasecmp(str, "info")) {
        level = LOG_LEVEL_INFO;
    } else if (!strcasecmp(str, "debug")) {
        level = LOG_LEVEL_DEBUG;
    } else if (!strcasecmp(str, "verbose")) {
        level = LOG_LEVEL_VERBOSE;
    } else {
        valid = false;
    }

    *ok = valid;
    return level;
}

