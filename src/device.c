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
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <jansson.h>

#include "device.h"
#include "find.h"
#include "state_machine.h"
#include "formatter.h"
#include "log.h"

/* Actions to perform on state transition */

/** No action needed */
#define STATE_ACTION_NONE           0           /**< No action */

/** Append a 0 to aggregate bit string */
#define STATE_ACTION_APPEND_0       (1 << 0)

/** Append a 1 to aggregate bit string on the MSB side */
#define STATE_ACTION_APPEND_0_MSB   (1 << 1)

struct transition {
    uint8_t action;
    const struct state *next;
};

struct state {
    char *name;                     /**< State name */

    uint64_t delta_us;              /**< usec last state transition */
    uint64_t timeout_us;            /**< Timeout, in microseconds */

};

struct device
{
    json_t *root;

    char *name;
    char *description;

    uint8_t * data;
    size_t data_alloc_len;          /** Allocated size of data, in bytes */
    int num_bits;                   /** Number of bits in device's msg format */

    struct keyval_list *values;
    struct state_machine *sm;
    struct formatter *fmt;
};

static inline bool add_state(struct state_machine *sm, json_t *state)
{
    const char *name = NULL;
    uint64_t timeout_us = 0;
    uint64_t duration_us = 0;
    enum sm_trigger_cond condition = SM_TRIGGER_COND_ALWAYS;
    size_t i, num_triggers;
    json_t *triggers, *trigger, *tmp;

    tmp = json_object_get(state, "name");
    if (!json_is_string(tmp)) {
        log_error("Failed to get state name.\n");
        return false;
    } else {
        name = json_string_value(tmp);
        log_verbose("Loading state: %s\n", name);
    }

    tmp = json_object_get(state, "timeout_us");
    if (!json_is_integer(tmp)) {
    } else {
        int tmpval = json_integer_value(tmp);
        if (tmpval < 0) {
            log_error("Invalid timeout value: %d\n", tmpval);
            return false;
        } else {
            timeout_us = tmpval;
            log_verbose("%s timeout (us): %"PRIu64"\n", name, timeout_us);
        }
    }

    tmp = json_object_get(state, "duration_us");
    if (json_is_integer(tmp)) {
        int val = json_integer_value(tmp);
        if (val < 0) {
            log_error("Invalid trigger duration.\n");
        } else {
            duration_us = (uint64_t) val;
        }
    }

    triggers = json_object_get(state, "triggers");
    if (!json_is_array(triggers)) {
        log_error("Failed to get triggers for state \"%s\"\n", state);
        return false;
    } else if (json_array_size(triggers) == 0) {
        log_error("Triggers array is empty for state \"%s\n", state);
        return false;
    }

    num_triggers = json_array_size(triggers);
    if (!sm_add_state(sm, name, duration_us, timeout_us, num_triggers)) {
        log_error("Failed to add \"%s\" to state machine.\n", name);
        return false;
    }

    json_array_foreach(triggers, i, trigger) {
        enum sm_trigger_cond condition;
        enum sm_trigger_action action;
        uint64_t duration_us;
        const char *condition_str;
        const char *next_state;
        const char *action_str;

        /* Required trigger condition */
        tmp = json_object_get(trigger, "condition");
        if (!json_is_string(tmp)) {
            log_error("Failed to get trigger condition.\n");
            return false;
        }

        condition_str = json_string_value(tmp);
        condition = sm_trigger_cond_value(condition_str);
        if (condition == SM_TRIGGER_COND_INVALID) {
            log_error("Got invalid trigger condition: %s\n", condition_str);
            return false;
        } else {
            log_verbose("%s trigger condition: %s\n", name, condition_str);
        }

        /* Optional duration to consider, in addition to the above condition */
        tmp = json_object_get(trigger, "duration_us");
        if (json_is_integer(tmp)) {
            duration_us = json_number_value(tmp);
        } else {
            duration_us = 0;
        }


        /* Required next state */
        tmp = json_object_get(trigger, "state");
        if (!json_is_string(tmp)) {
            log_error("Failed to get trigger's next state.\n");
            return false;
        }

        next_state = json_string_value(tmp);
        log_verbose("Trigger's next state: %s\n", next_state);

        /* Optional trigger action */
        tmp = json_object_get(trigger,"action");
        if (json_is_string(tmp)) {
            action_str = json_string_value(tmp);
            action = sm_trigger_action_value(action_str);
            if (action == SM_TRIGGER_ACTION_INVALID) {
                log_error("Got invalid trigger action: %s\n", action_str);
                return false;
            } else {
                log_verbose("Trigger's action: %s\n", action_str);
            }
        } else {
            log_verbose("No action string. Setting action to \"none\".\n");
            action = SM_TRIGGER_ACTION_NONE;
        }

        sm_add_state_trigger(sm, name, condition, duration_us,
                             next_state, action);
    }

    return true;
}

static inline struct state_machine *
create_state_machine(json_t *device,
                     uint8_t *data, unsigned int num_bits,
                     unsigned int sample_rate)
{
    int status = -1;
    struct state_machine *sm = NULL;
    json_t *states, *state;
    size_t num_states;
    size_t i;

    states = json_object_get(device, "states");
    if (!json_is_array(states)) {
        log_error("Failed to get states array.\n");
        return NULL;
    }

    num_states = json_array_size(states);
    if (num_states == 0) {
        log_error("States array is empty.\n");
        return NULL;
    } else if (num_states > UINT_MAX) {
        log_error("States array is too large.\n");
        return NULL;
    }

    log_verbose("State machine has %u states\n", num_states);

    sm = sm_init((unsigned int) num_states, data, num_bits, sample_rate);
    if (!sm) {
        return NULL;
    }

    json_array_foreach(states, i, state) {
        bool success = add_state(sm, state);
        if (!success) {
            goto out;
        }
    }

    if (sm_initialized(sm)) {
        log_verbose("State machine initialized.\n");
        status = 0;
    } else {
        log_error("State machine is missing states or triggers.\n");
        status = -1;
    }


out:
    if (status != 0) {
        sm_deinit(sm);
        sm = NULL;
    }

    return sm;
}

static inline bool add_field(struct formatter *f, json_t *field)
{
    const char *name;
    const char *description;
    const char *default_value;
    int start_bit, end_bit;
    enum formatter_endianness endianness;
    enum formatter_fmt format;
    float offset = 0;
    float scaling = 0;

    json_t *tmp;

    size_t enum_count = 0;
    json_t *enums = NULL;

    tmp = json_object_get(field, "name");
    if (!json_is_string(tmp)) {
        log_error("Failed to get field name.\n");
        return false;
    } else {
        name = json_string_value(tmp);
        log_verbose("Field name: %s\n", name);
    }

    tmp = json_object_get(field, "default");
    if (!json_is_string(tmp)) {
        log_error("Failed to get default for \"%s\" field.\n", name);
        return false;
    } else {
        default_value = json_string_value(tmp);
        log_verbose("Default value: %s\n", default_value);
    }


    tmp = json_object_get(field, "start_bit");
    if (!json_is_integer(tmp)) {
        log_error("Failed to get start bit for \"%s\" field.\n", name);
        return false;
    } else {
        start_bit = json_integer_value(tmp);
        if (start_bit < 0) {
            log_error("Invalid start bit: %d\n", start_bit);
        } else {
            log_verbose("Start bit: %d\n", start_bit);
        }
    }

    tmp = json_object_get(field, "end_bit");
    if (!json_is_integer(tmp)) {
        log_error("Failed to get end bit for \"%s\" field.\n", name);
        return NULL;
    } else {
        end_bit = json_integer_value(tmp);
        if (end_bit < 0) {
            log_error("Invalid end  bit: %d\n", end_bit);
        } else {
            log_verbose("End bit: %d\n", end_bit);
        }
    }

    tmp = json_object_get(field, "endianness");
    if (!json_is_string(tmp)) {
        log_error("Failed to get endianness for \"%s\" field.\n", name);
        return false;
    } else {
        const char *endianness_str = json_string_value(tmp);
        endianness = formatter_endianess_value(endianness_str);
        if (endianness == FORMATTER_ENDIAN_INVALD) {
            log_error("Invalid endianness specified: %s\n", endianness_str);
            return false;
        }
    }

    tmp = json_object_get(field, "format");
    if (!json_is_string(tmp)) {
        log_error("Failed to get format for \"%s\" field.\n", name);
        return false;
    } else {
        const char *format_str = json_string_value(tmp);
        format = formatter_fmt_value(format_str);
        if (format == FORMATTER_FMT_INVALID) {
            log_error("Invalid format: %s\n", format_str);
            return false;
        } else {
            log_verbose("Format: %s\n", format_str);
        }
    }

    if (format == FORMATTER_FMT_ENUM) {
        enums = json_object_get(field, "enum_values");
        if (!json_is_array(enums)) {
            log_error("No \"enum_values\" array found for enumeration: %s\n", name);
            return false;
        }

        enum_count = json_array_size(enums);
        if (enum_count == 0) {
            log_error("Error: \"enum_values\" in field \"%s\" is empty.\n", name);
        }

    }

    tmp = json_object_get(field, "offset");
    if (!json_is_number(tmp)) {
        log_verbose("No offset value. Defaulting to %f.\n", offset);
    } else {
        offset = json_number_value(tmp);
        log_verbose("Offset value: %f\n", offset);
    }

    tmp = json_object_get(field, "scaling");
    if (!json_is_number(tmp)) {
        log_verbose("No scaling value. Defaulting to %f.\n", scaling);
    } else {
        scaling = json_number_value(tmp);
        log_verbose("Scaling value: %f\n", scaling);
    }

    if (!formatter_add_field(f, name,
                             start_bit, end_bit,
                             format, enum_count,
                             endianness, scaling, offset)) {

        return false;
    }

    if (format == FORMATTER_FMT_ENUM) {
        size_t i;
        json_t *e;
        uint64_t value;
        bool conv_ok;
        const char *enum_name;

        json_array_foreach(enums, i, e) {
            tmp = json_object_get(e, "string");
            if (!json_is_string(tmp)) {
                log_error("Enumeration value %zd is missing \"string.\"\n", i);
                return false;
            }

            enum_name = json_string_value(tmp);

            tmp = json_object_get(e, "value");
            if (!json_is_string(tmp)) {
                log_error("Enumeration item \"%s\" is missing \"value.\"\n", enum_name);
                return false;
            }

            value = str2uint64(json_string_value(tmp), 0, UINT64_MAX, &conv_ok);
            if (!conv_ok) {
                log_error("Invalid enumeration value: %s\n",
                          json_string_value(tmp));
                return false;
            }

            if (!formatter_add_field_enum(f, name, enum_name, spt_from_uint64(value))) {
                return false;
            } else {
                log_verbose("Added enumeration entry: %s=0x%"PRIx64"\n",
                            enum_name, value);
            }
        }
    }

    return formatter_set_field_default(f, name, default_value);
}

static inline struct formatter *
create_formatter(json_t *device, unsigned int num_bits)
{
    int status = -1;
    struct formatter *f = NULL;
    json_t *fields, *field;
    size_t num_fields;
    size_t i;

    fields = json_object_get(device, "fields");
    if (!json_is_array(fields)) {
        log_error("Failed to get fields array.\n");
        return NULL;
    }

    num_fields = json_array_size(fields);
    if (num_fields == 0) {
        log_error("Fields array is empty.\n");
        return NULL;
    } else if (num_fields > UINT_MAX) {
        log_error("Fields  array is too large.\n");
        return NULL;
    }

    f = formatter_init((unsigned int) num_fields, num_bits);
    if (!f) {
        return NULL;
    }

    json_array_foreach(fields, i, field) {
        bool success = add_field(f, field);
        if (!success) {
            goto out;
        }
    }

    if (!formatter_initialized(f)) {
        goto out;
    }

    status = 0;

out:
    if (status != 0) {
        formatter_deinit(f);
        f = NULL;
    }

    return f;
}

static inline int populate_device(struct device *d, json_t *device,
                                  unsigned int sample_rate)
{
    int status = -1;
    json_t *tmp;

    log_verbose("Loading device...\n");

    tmp = json_object_get(device, "name");
    if (!json_is_string(tmp)) {
        log_error("Failed to read device name string.\n");
        goto out;
    } else {
        d->name = strdup(json_string_value(tmp));
        if (!d->name) {
            log_error("Failed to set device name: %s\n", strerror(errno));
            goto out;
        }

        log_verbose("Device name: %s\n", d->name);
    }

    tmp = json_object_get(device, "description");
    if (!json_is_string(tmp)) {
        log_error("Failed to read device description string.\n");
        goto out;
    } else {
        d->description = strdup(json_string_value(tmp));
        if (!d->description) {
            log_error("Failed to set device description: %s\n",
                      strerror(errno));
            goto  out;
        }

        log_verbose("Device description: %s\n", d->description);
    }

    tmp = json_object_get(device, "num_bits");
    if (!json_is_integer(tmp)) {
        log_error("Failed to read \"num_bits\" property.\n");
        goto out;
    } else {
        d->num_bits = json_integer_value(tmp);
        if (d->num_bits > 0) {
            log_verbose("Maximum # of bits: %d\n", d->num_bits);
        } else {
            log_error("Invalid \"num_bits\" value: %d\n", d->num_bits);
            goto out;
        }
    }

    d->data_alloc_len = (d->num_bits + 7) / 8;
    d->data = calloc(d->data_alloc_len, 1);
    if (!d->data) {
        goto out;
    }


    d->sm = create_state_machine(device, d->data, d->num_bits, sample_rate);
    if (!d->sm) {
        goto out;
    }

    d->fmt = create_formatter(device, d->num_bits);
    if (!d->fmt) {
        goto out;
    }

    status = 0;

out:
    return status;
}

struct device * device_init(const char *device_name, unsigned int sample_rate)
{
    int status = -1;
    json_t *root = NULL;
    struct device *dev = NULL;
    json_t *json_dev = NULL;
    json_error_t error;
    FILE *input = find_device_file(device_name);

    if (!input) {
        log_error("Unable to find device file for: %s\n", device_name);
        return NULL;
    }

    dev = calloc(1, sizeof(dev[0]));
    if (dev == NULL) {
        perror("calloc");
        goto out;
    }

    root = json_loadf(input, JSON_REJECT_DUPLICATES, &error);
    if (!root) {
        log_error("Error in %s.json (line %d, column %d):\n"
                  "  %s\n", device_name, error.line, error.column, error.text);
        goto out;
    }

    json_dev = json_object_get(root, "device");
    if (!json_dev) {
        log_error("Failed to find \"device\" entry in device file\n");
        goto out;
    }

    status = populate_device(dev, json_dev, sample_rate);
    if (status != 0) {
        goto out;
    }

    dev->values = keyval_list_init();
    if (!dev->values) {
        goto out;
    }

    status = 0;

out:
    if (root) {
        json_decref(root);
    }

    if (status != 0) {
        device_deinit(dev);
        dev = NULL;
    }

    fclose(input);

    return dev;
}

const struct keyval_list * device_process(struct device *d,
                                          const bool *data, unsigned int count)
{
    unsigned int total_proc, num_proc;
    enum sm_process_result proc = SM_PROCESS_RESULT_NO_OUTPUT;

    keyval_list_clear(d->values);
    total_proc = 0;

    /* TODO: Check error paths - does it make more sense to keep trying or
     *       to just bail out like this?
     */
    while (total_proc < count && proc != SM_PROCESS_RESULT_ERROR) {
        proc = sm_process(d->sm, &data[total_proc],
                          count - total_proc, &num_proc);

        if (proc == SM_PROCESS_RESULT_OUTPUT_READY) {
            formatter_data_to_keyval(d->fmt, d->data, d->values);
        }

        total_proc += num_proc;
    }

    return d->values;
}

bool device_generate(struct device *d, const struct keyval_list *params,
                    struct complexf **samples, unsigned int *num_samples)
{
    bool success;

    /* Set device data to default values */
    formatter_default_data(d->fmt, d->data);

    /* Update values with those provided by caller */
    success = formatter_keyval_to_data(d->fmt, params, d->data);
    if (!success) {
        return false;
    }

    /* TODO define soft gain */
    *samples = sm_generate(d->sm, d->data, d->num_bits, 0.95, num_samples);

    return (*samples != NULL);
}

void device_deinit(struct device *dev)
{
    if (dev) {
        log_verbose("Deinitializing device: %s\n", dev->name);
        keyval_list_deinit(dev->values);
        sm_deinit(dev->sm);
        formatter_deinit(dev->fmt);
        free(dev->data);
        free(dev->name);
        free(dev->description);
        free(dev);
    }
}
