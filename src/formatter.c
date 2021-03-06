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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <inttypes.h>
#include <float.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "spt.h"
#include "formatter.h"
#include "conversions.h"
#include "log.h"

#define BIT_UNINITIALIZED UINT_MAX

struct formatter {
    struct formatter_field *fields;
    size_t num_fields;

    unsigned int max_bit;

    struct formatter_params output;
    uint8_t *input;

    enum formatter_ts_mode ts_mode;
};

struct enum_def {
    const char *str;
    spt value;
};

struct formatter_field {
    char *name;
    unsigned int start_bit;
    unsigned int end_bit;
    enum formatter_fmt format;
    enum formatter_endianness endianness;
    float scaling;
    float offset;
    spt default_value;
    struct enum_def *enums;
    size_t enum_count;
};

static inline unsigned int get_width(const struct formatter_field *field)
{
    return field->end_bit - field->start_bit + 1;
}

struct formatter * formatter_init(unsigned int num_fields, unsigned int max_bit,
                                  enum formatter_ts_mode ts_mode)
{
    int status = -1;
    struct formatter *f = NULL;
    unsigned int i;
    unsigned int max_bytes = (max_bit + 7) / 8;

    if (num_fields == 0) {
        log_error("Formatter must be initialized for one or more fields.\n");
        return NULL;
    }

    if (max_bit == 0) {
        log_error("Formatter cannot be initialized for 0 bits.\n");
        return NULL;
    }

    f = malloc(sizeof(f[0]));
    if (!f) {
        perror("malloc");
        return NULL;
    }

    f->num_fields = num_fields;
    f->max_bit = max_bit;

    f->output.count = num_fields;

    f->ts_mode = ts_mode;

    f->fields = calloc(num_fields, sizeof(f->fields[0]));
    if (!f->fields) {
        perror("calloc");
        goto out;
    }

    f->output.params = calloc(num_fields, sizeof(f->output.params[0]));
    if (!f->output.params) {
        perror("calloc");
        goto out;
    }

    f->input = calloc(max_bytes, 1);
    if (!f->input) {
        perror("calloc");
        goto out;
    }

    for (i = 0; i < num_fields; i++) {
        f->fields[i].start_bit = BIT_UNINITIALIZED;
        f->fields[i].end_bit   = BIT_UNINITIALIZED;
    }

    status = 0;

out:
    if (status != 0) {
        formatter_deinit(f);
    }

    return f;
}


static spt str_to_spt(const struct formatter_field *field,
                      const char *str,
                      bool *success)
{
    spt value;
    bool conv_ok = false;
    const unsigned int field_width = get_width(field);
    const uint64_t mask = (field_width < 64) ?
                            (1llu << field_width) - 1 :
                            0xffffffffffffffffllu;

    *success = false;

    switch (field->format) {
        case FORMATTER_FMT_HEX:
        case FORMATTER_FMT_UNSIGNED_DEC: {
            uint64_t tmp = str2uint64(str, 0, UINT64_MAX, &conv_ok);
            if (!conv_ok) {
                goto inval;
            }

            tmp = (uint64_t) (((float) tmp - field->offset) / field->scaling);
            value = spt_from_uint64(tmp);
            break;
        }

        case FORMATTER_FMT_TWOS_COMPLEMENT: {
            int64_t tmp = str2int64(str, INT64_MIN, INT64_MAX, &conv_ok);
            if (!conv_ok) {
                goto inval;
            }

            tmp = (int64_t) (((float) tmp - field->offset) / field->scaling);
            value = spt_from_int64(tmp);

            /* Mask off sign-extended bits to field with */
            value &= mask;
            break;
        }


        case FORMATTER_FMT_SIGN_MAGNITUDE: {
            int64_t tmp = str2int64(str, INT64_MIN, INT64_MAX, &conv_ok);
            bool negative = (tmp < 0);
            if (!conv_ok) {
                goto inval;
            }

            tmp = (int64_t) (((float) tmp - field->offset) / field->scaling);

            /* Field width includes sign bit (assumed to be MSB) */
            tmp &= (1 << (field_width - 1)) - 1;

            if (negative) {
                tmp |= (1 << (field_width - 1));
            }

            value = spt_from_uint64((uint64_t) tmp);
            break;
        }

        case FORMATTER_FMT_FLOAT: {
            float tmp = (float) str2double(str, -DBL_MAX, DBL_MAX, &conv_ok);
            if (!conv_ok) {
                goto inval;
            }

            value = spt_from_float((float) tmp, field->scaling, field->offset);

            /* Mask off sign-extended bits to field with */
            value &= mask;
            break;
        }

        case FORMATTER_FMT_ENUM: {
            size_t i;
            bool have_enum = false;

            for (i = 0; i < field->enum_count && !have_enum; i++) {
                if (!strcasecmp(str, field->enums[i].str)) {
                    value = field->enums[i].value;
                    have_enum = true;
                }
            }

            if (!have_enum) {
                uint64_t tmp = str2uint64(str, 0, UINT64_MAX, &conv_ok);
                if (!conv_ok) {
                    goto inval;
                }

                value = spt_from_uint64(tmp);
            }

            break;
        }

        default:
            log_critical("Bug: Invalid field format: %d\n", field->format);
            return spt_from_uint64(0);
    }

    if ((spt_to_uint64(value) & mask) != spt_to_uint64(value)) {
        log_error("Value is too large for field \"%s\": %s\n",
                    field->name, str);

        return spt_from_uint64(0);
    }

    *success = true;
    return value;

inval:
    log_error("Invalid value for field \"%s\": %s\n", field->name, str);
    return spt_from_uint64(0);
}

bool formatter_add_field(struct formatter *f,
                         const char *name,
                         unsigned int start_bit, unsigned int end_bit,
                         enum formatter_fmt format,
                         size_t enum_count,
                         enum formatter_endianness endianness,
                         float scaling, float offset)
{
    unsigned int i;
    bool conv_ok;

    if (end_bit < start_bit) {
        log_error("End bit must be >= start bit\n");
        return false;
    }

    if ((end_bit - start_bit + 1) > 64) {
        log_error("Fields larger than 64-bits are not currently supported.\n");
        return false;
    }

    for (i = 0; i < f->num_fields; i++) {
        if (f->fields[i].start_bit == BIT_UNINITIALIZED) {
            f->fields[i].name = strdup(name);
            if (!f->fields[i].name) {
                perror("strdup");
                return false;
            }

            f->fields[i].start_bit      = start_bit;
            f->fields[i].end_bit        = end_bit;
            f->fields[i].scaling        = (scaling == 0) ? 1.0f : scaling;
            f->fields[i].offset         = offset;

            f->output.params[i].name = (const char *) f->fields[i].name;

            switch (format) {
                case FORMATTER_FMT_HEX:
                case FORMATTER_FMT_UNSIGNED_DEC:
                case FORMATTER_FMT_SIGN_MAGNITUDE:
                case FORMATTER_FMT_TWOS_COMPLEMENT:
                case FORMATTER_FMT_FLOAT:
                    f->fields[i].format = format;
                    if (enum_count != 0) {
                        log_error("Non-zero enum count provided for non-enum.\n");
                        return false;
                    }

                    f->fields[i].enum_count = 0;
                    break;

                case FORMATTER_FMT_ENUM:
                    f->fields[i].format = format;
                    if (enum_count == 0) {
                        log_error("Enumeration format requires 1 or more values"
                                  " to be defined\n");
                        return false;
                    }

                    f->fields[i].enum_count = enum_count;
                    f->fields[i].enums = calloc(enum_count,
                                                sizeof(struct enum_def));

                    if (f->fields[i].enums == NULL) {
                        perror("calloc");
                        return false;
                    }
                    break;

                default:
                    log_error("Invalid format option: %d\n", format);
                    return false;
            }


            switch (endianness) {
                case FORMATTER_ENDIAN_BIG:
                case FORMATTER_ENDIAN_LITTLE:
                    f->fields[i].endianness = endianness;
                    break;

                default:
                    log_error("Invalid endianness option: %d\n", endianness);
                    return NULL;
            }


            /* Initialized via formatter_set_field_default() */
            return true;
        }
    }

    log_error("No room left in formatter for field: %s\n", name);
    return false;
}

bool formatter_add_field_enum(struct formatter *f, const char *field_name,
                              const char *enum_name, spt value)
{
    struct formatter_field *field = NULL;
    size_t i;

    for (i = 0; i < f->num_fields; i++) {
        if (!strcasecmp(f->fields[i].name, field_name)) {
            field = &f->fields[i];
            break;
        }
    }

    if (field == NULL) {
        log_error("Cannot add enumeration to unknown field: %s\n", field_name);
        return false;
    }

    for (i = 0; i < field->enum_count; i++) {
        if (field->enums[i].str == NULL) {
            struct enum_def *e = &field->enums[i];
            uint64_t tmp_val;
            bool ok;

            e->str = strdup(enum_name);
            if (!e->str) {
                perror("strdup");
                return false;
            }

            e->value = value;
            return true;

        } else if (!strcasecmp(field->enums[i].str, enum_name)) {
            log_error("Error: Duplicate enumeration name (%s)\n", enum_name);
            return false;
        }
    }

    log_error("Error: Enum list size exceeded.\n");
    return false;
}

bool formatter_set_field_default(struct formatter *f, const char *field_name,
                                 const char *default_value)
{
    struct formatter_field *field = NULL;
    size_t i;
    bool ok;

    for (i = 0; field == NULL && i < f->num_fields; i++) {
        if (f->fields[i].name && !strcasecmp(f->fields[i].name, field_name)) {
            field = &f->fields[i];
        }
    }

    if (field == NULL) {
        return false;
    }

    field->default_value = str_to_spt(field, default_value, &ok);

    if (!ok) {
        log_error("Invalid default value for field \"%s\": %s\n",
                  field->name, default_value);

        return false;
    } else {
        return true;
    }
}

static spt get_field_value(struct formatter_field *f, const uint8_t *data)
{
    uint64_t tmp = 0;
    unsigned int i;
    unsigned int dest_bit;

    if (f->endianness == FORMATTER_ENDIAN_BIG) {
        dest_bit = f->end_bit - f->start_bit;
    } else if (f->endianness == FORMATTER_ENDIAN_LITTLE) {
        dest_bit = 0;
    } else {
        log_critical ("Bug: Invalid endianness setting: %d\n", f->endianness);
        return (uint64_t) -1;
    }

    for (i = f->start_bit; i <= f->end_bit; i++) {
        const unsigned int byte     = i / 8;
        const unsigned int src_bit  = i % 8;

        const uint64_t val = (data[byte] >> src_bit) & 0x1;
        tmp |= val << dest_bit;

        if (f->endianness == FORMATTER_ENDIAN_BIG) {
            dest_bit--;
        } else if (f->endianness == FORMATTER_ENDIAN_LITTLE) {
            dest_bit++;
        }
    }

    return spt_from_uint64(tmp);
}

static void field_data_to_str(char *str, size_t max_chars, spt value,
                               struct formatter_field *field)
{
    const unsigned int field_width = get_width(field);
    const uint64_t mask = (field_width < 64) ?
                            (1llu << field_width) - 1 :
                            0xffffffffffffffffllu;

    switch (field->format) {

        case FORMATTER_FMT_HEX: {
            uint64_t tmp = spt_to_uint64(value);
            tmp = (tmp * field->scaling) + field->offset;

            if (field_width <= 8) {
                snprintf(str, max_chars, "0x%02x", (uint8_t) tmp);
            } else if (field_width <= 16) {
                snprintf(str, max_chars, "0x%02x", (uint16_t) tmp);
            } else if (field_width <= 24) {
                snprintf(str, max_chars, "0x%06x", (uint32_t) tmp);
            } else if (field_width <= 32) {
                snprintf(str, max_chars, "0x%08x", (uint32_t) tmp);
            } else if (field_width <= 40) {
                snprintf(str, max_chars, "0x%010" PRIu64, tmp);
            } else if (field_width <= 48) {
                snprintf(str, max_chars, "0x%012" PRIu64, tmp);
            } else if (field_width <= 56) {
                snprintf(str, max_chars, "0x%014" PRIu64, tmp);
            } else if (field_width <= 64) {
                snprintf(str, max_chars, "0x%016" PRIu64, tmp);
            }

            break;
        }

        case FORMATTER_FMT_UNSIGNED_DEC: {
            uint64_t tmp = spt_to_uint64(value);
            tmp = (tmp * field->scaling) + field->offset;
            snprintf(str, max_chars, "%"PRIu64, tmp);
            break;
        }

        case FORMATTER_FMT_TWOS_COMPLEMENT: {
            int64_t tmp;
            bool neg;

            neg = (value & (1 << (field_width - 1))) != 0;
            if (neg) {
                value = (~value + 1) & mask;
            }

            tmp = spt_to_int64(value);
            if (neg) {
                tmp = -tmp;
            }

            tmp = (tmp * field->scaling) + field->offset;
            snprintf(str, max_chars, "%"PRIi64, tmp);
            break;
        }

        case FORMATTER_FMT_SIGN_MAGNITUDE: {
            int64_t tmp_int;
            uint64_t tmp_uint = spt_to_uint64(value);
            bool is_negative = (tmp_uint & (1 << (field_width - 1))) != 0;

            tmp_int = tmp_uint & ((1 << (field_width - 1)) - 1);
            if (is_negative) {
                tmp_int = -tmp_int;
            }

            tmp_int = (tmp_int * field->scaling) + field->offset;
            snprintf(str, max_chars, "%"PRIi64, tmp_int);
            break;
        }

        case FORMATTER_FMT_FLOAT: {
            float tmp;
            float scaling;
            bool neg;

            neg = (value & (1 << (field_width - 1))) != 0;
            if (neg) {
                value = (~value + 1) & mask;
                scaling = -field->scaling;
            } else {
                scaling = field->scaling;
            }

            tmp = spt_to_float(value, scaling, field->offset);

            snprintf(str, max_chars, "%1.3f", tmp);
            break;
        }

        case FORMATTER_FMT_ENUM: {
            size_t i;
            bool have_enum = false;

            for (i = 0; i < field->enum_count && !have_enum; i++) {
                if (field->enums[i].value == value) {
                    snprintf(str, max_chars, "%s", field->enums[i].str);
                    have_enum = true;
                }
            }

            if (!have_enum) {
                snprintf(str, max_chars, "0x%"PRIx64, value);
            }

            break;
        }

        default:
            log_critical("Bug: invalid format %d\n", field->format);
    }
}

bool formatter_initialized(struct formatter *f)
{
    size_t i, j;

    if (f->ts_mode == FORMATTER_TS_INVALID) {
        log_error("Formatter timestamp mode is unset.\n");
        return false;
    }

    for (i = 0; i < f->num_fields; i++) {
        if (f->fields[i].format == FORMATTER_FMT_INVALID) {
            log_error("Field %zd has invalid format.\n", i);
            return false;
        }

        if (f->fields[i].start_bit == BIT_UNINITIALIZED) {
            log_error("Field %zd has uninitialized start bit value.\n");
            return false;
        }

        if (f->fields[i].end_bit == BIT_UNINITIALIZED) {
            log_error("Field %zd has uninitialized end bit value.\n");
            return false;
        }

        if (f->fields[i].endianness == FORMATTER_ENDIAN_INVALD) {
            log_error("Field %zd has uninitialized endianness value.\n");
            return false;
        }

        if (f->fields[i].format == FORMATTER_FMT_ENUM) {
            for (j = 0; j < f->fields[i].enum_count; j++) {
                if (f->fields[i].enums[j].str == NULL) {
                    log_error("Field %zd, enum %zd is unuinitialized.\n", i, j);
                    return false;
                }
            }
        }
    }

    return true;
}

static const char ts_key[] = "Decode Timestamp";

static bool timestamp_unix(struct keyval_list *kv_list, bool frac)
{
    int status;
    char buf[32];
    double ts;
    struct timeval tv;
    struct keyval kv;

    status = gettimeofday(&tv, NULL);
    if (status != 0) {
        log_error("Failed to get current time: %s\n", strerror(errno));
        return;
    }

    ts = tv.tv_sec + ((double) tv.tv_usec / 1000000.0);

    if (frac) {
        snprintf(buf, sizeof(buf), "%f", ts);
    } else {
        ts += 0.5;  /* Integer round when we cast to an unsigned */
    }

    kv.key = ts_key;
    kv.value = buf;
    return keyval_list_append(kv_list, &kv);
}

static bool timestamp_datetime(struct keyval_list *kv_list, bool ampm)
{
    int status;
    time_t t;
    struct tm *tmp;
    size_t len;
    struct keyval kv;
    char buf[80];

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        log_error("Failed to get local time.\n");
        return false;
    }

    if (ampm) {
        len = strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S %p", tmp);
    } else {
        len = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);
    }

    if (len != 0) {
        kv.key   = ts_key;
        kv.value = buf;
        return keyval_list_append(kv_list, &kv);
    } else {
        log_error("Failed to format timestamp.\n");
        return false;
    }
}

static void timestamp(const struct formatter *f, struct keyval_list *kv_list)
{
    bool success;
    char buf[64];

    switch (f->ts_mode) {
        case FORMATTER_TS_NONE:
            success = true;
            break;

        case FORMATTER_TS_UNIX_INT:
            success = timestamp_unix(kv_list, false);
            break;

        case FORMATTER_TS_UNIX_FRAC:
            success = timestamp_unix(kv_list, true);
            break;

        case FORMATTER_TS_DATETIME_24:
            success = timestamp_datetime(kv_list, false);
            break;

        case FORMATTER_TS_DATETIME_AMPM:
            success = timestamp_datetime(kv_list, true);
            break;

        default:
            log_error("Unexpected TS mode encountered: %d\n", f->ts_mode);
            return;
    }

    if (!success) {
        log_error("Failed to timestamp message.\n");
    }
}

bool formatter_data_to_keyval(const struct formatter *f,
                              const uint8_t *data, struct keyval_list *kv_list)

{
    unsigned int i;
    char buf[80];
    bool success = true;
    struct keyval kv;

    timestamp(f, kv_list);

    for (i = 0; i < f->num_fields && success; i++) {
        const spt value = get_field_value(&f->fields[i], data);

        memset(buf, 0, sizeof(buf));
        field_data_to_str(buf, sizeof(buf), value, &f->fields[i]);

        kv.key   = f->fields[i].name;
        kv.value = buf;

        success = keyval_list_append(kv_list, &kv);
    }

    return success;
}

/* TODO: This contains unchecked converisons.
 *       I don't expect to see devices with large ranges of values, but this
 *       would be a good place to check if things look wrong. ;) */
static bool str_to_field_bits(const struct formatter_field *field,
                              const char *str, uint64_t *bits)

{
    spt value;
    bool conv_ok;

    *bits = str_to_spt(field, str, &conv_ok);
    return conv_ok;
}

static void apply_field_bits(const struct formatter_field *f,
                             uint64_t input_bits, uint8_t *data)
{
    unsigned int i, b;
    uint64_t src_bit;

    if (f->endianness == FORMATTER_ENDIAN_BIG) {
        src_bit = f->end_bit - f->start_bit;
    } else if (f->endianness == FORMATTER_ENDIAN_LITTLE) {
        src_bit = 0;
    } else {
        log_critical ("Bug: Invalid endianness setting: %d\n", f->endianness);
        return;
    }

    for (i = f->start_bit, b = 0; i <= f->end_bit; i++) {
        const unsigned int byte = i / 8;
        const unsigned int bit  = i % 8;

        if (input_bits & (1 << src_bit)) {
            data[byte] |= (1 << bit);
        } else {
            data[byte] &= ~(1 << bit);
        }

        if (f->endianness == FORMATTER_ENDIAN_BIG) {
            src_bit--;
        } else if (f->endianness == FORMATTER_ENDIAN_LITTLE) {
            src_bit++;
        }
    }

}


bool formatter_keyval_to_data(const struct formatter *f,
                              const struct keyval_list *kv_list, uint8_t *data)
{
    unsigned int i, j;
    uint64_t field_bits;
    spt field_value;

    const struct keyval *kv = NULL;
    const struct formatter_field *field = NULL;

    bool success;

    for (i = 0; i < keyval_list_size(kv_list); i++) {
        kv = keyval_list_at(kv_list, i);

        /* Find the associated field information */
        field = NULL;
        for (j = 0; j < f->num_fields && field == NULL; j++) {
            if (!strcasecmp(f->fields[j].name, kv->key)) {
                field = &f->fields[j];
            }
        }

        if (!field) {
            log_error("Invalid parameter name: %s\n", kv->key);
            return false;
        }

        /* Convert string to bits, consdering the field's formatting settings */
        field_value = str_to_spt(field, kv->value, &success);
        if (!success) {
            return false;
        }

        field_bits = spt_to_uint64(field_value);

        /* Aggregate bits from this field */
        apply_field_bits(field, field_bits, data);
    }

    return true;
}


void formatter_default_data(const struct formatter *f, uint8_t *data)
{
    unsigned int i;
    struct formatter_field *field;
    uint64_t field_bits;

    for (i = 0; i < f->num_fields; i++) {
        field = &f->fields[i];
        field_bits = spt_to_uint64(field->default_value);
        apply_field_bits(field, field_bits, data);
    }

}

enum formatter_endianness formatter_endianess_value(const char *str)
{
    if (!strcasecmp("big", str)) {
        return FORMATTER_ENDIAN_BIG;
    } else if (!strcasecmp("little", str)) {
        return FORMATTER_ENDIAN_LITTLE;
    } else {
        return FORMATTER_ENDIAN_INVALD;
    }
}

enum formatter_fmt formatter_fmt_value(const char *str)
{
    if (!strcasecmp("hex", str)) {
        return FORMATTER_FMT_HEX;
    } else if (!strcasecmp("unsigned decimal", str)) {
        return FORMATTER_FMT_UNSIGNED_DEC;
    } else if (!strcasecmp("sign-magnitude", str)) {
        return FORMATTER_FMT_SIGN_MAGNITUDE;
    } else  if (!strcasecmp("two's complement", str)) {
        return FORMATTER_FMT_TWOS_COMPLEMENT;
    } else if (!strcasecmp("float", str)) {
        return FORMATTER_FMT_FLOAT;
    } else if (!strcasecmp("enumeration", str)) {
        return FORMATTER_FMT_ENUM;
    } else {
        return FORMATTER_FMT_INVALID;
    }
}

enum formatter_ts_mode formatter_ts_mode_value(const char *str)
{
    if (!strcasecmp("none", str)) {
        return FORMATTER_TS_NONE;
    } else if (!strcasecmp("unix", str)) {
        return FORMATTER_TS_UNIX_INT;
    } else if (!strcasecmp("unix-frac", str)) {
        return FORMATTER_TS_UNIX_FRAC;
    } else if (!strcasecmp("datetime-24", str)) {
        return FORMATTER_TS_DATETIME_24;
    } else if (!strcasecmp("datetime-ampm", str)) {
        return FORMATTER_TS_DATETIME_AMPM;
    } else {
        return FORMATTER_TS_INVALID;
    }
}

void formatter_deinit(struct formatter *f)
{
    if (f) {
        size_t i, j;

        for (i = 0; i < f->num_fields; i++) {
            for (j = 0; j < f->fields[i].enum_count; j++) {
                free((void*) f->fields[i].enums[j].str);
            }

            free(f->fields[i].enums);
            free(f->fields[i].name);
        }

        free(f->output.params);
        free(f->input);
        free(f->fields);
        free(f);
    }
}
