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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <jansson.h>

#include "fir.h"
#include "find.h"
#include "log.h"

#ifndef ENABLE_FIR_VERBOSE_LOGGING
#undef log_verbose
#define log_verbose(...)
#endif

struct fir_stage {

    /* Properties */
    unsigned int decimation;
    float *taps;
    size_t num_taps;

    /* Current state */
    size_t count;

    struct complexf *state;   /* Current state buffer that's twice as long
                               * as needed so we can avoid reording elements
                               * when we "shift" samples in time. */

    struct complexf *ins1;    /* Insertion point 1 in state */
    struct complexf *ins2;    /* Insertion point 2 in state */

    struct complexf *output;  /* Output buffer */
    size_t output_len;        /* Output buffer length, in samples */
};

struct fir_filter {
    struct fir_stage *stages;
    size_t num_stages;

    size_t max_input;
    unsigned int total_decimation;
};

struct fir_filter * fir_init(const char *filter_name, size_t max_input)
{
    int status = -1;

    size_t i;
    json_error_t error;
    FILE *input;
    json_t *root = NULL;
    json_t *json_filt, *stages, *stage, *decimation, *taps, *tap;
    struct fir_filter *fir = NULL;

    unsigned int total_decimation = 1;

    input = find_filter_file(filter_name);
    if (!input) {
        log_error("Unable to find filter file: %s\n", filter_name);
        goto out;
    }

    root = json_loadf(input, JSON_REJECT_DUPLICATES, &error);
    if (!root) {
        log_error("Error in %s (line %d, column %d):\n  %s\n",
                 filter_name, error.line, error.column, error.text);
        goto out;
    }

    json_filt = json_object_get(root, "filter");
    if (!json_filt) {
        log_error("Error: Failed to find \"filter\" entry in filter file.\n");
        goto out;
    }

    log_verbose("Loading filter...\n");
    stages = json_object_get(json_filt, "stages");
    if (!stages) {
        log_error("Error: Failed to find \"stages\" entry in filter file.\n");
        goto out;
    }

    if (!json_is_array(stages)) {
        log_error("Error: \"filter\" entry in filter file is not an array.\n");
        goto out;
    }

    fir = calloc(1, sizeof(fir[0]));
    if (!fir) {
        log_error("Error: Failed to allocate FIR.\n");
        goto out;
    }

    fir->num_stages = json_array_size(stages);
    if (fir->num_stages == 0) {
        log_error("Error: Filter must have 1 or more stages.\n");
        goto out;
    }

    fir->stages = calloc(fir->num_stages, sizeof(fir->stages[0]));
    if (!fir->stages) {
        goto out;
    }

    for (i = 0; i < fir->num_stages; i++) {
        size_t len;
        size_t tap_idx;

        stage = json_array_get(stages, i);
        if (!stage) {
            log_error("Error: Failed to get filter stage %zd\n", i);
            goto out;
        }

        decimation = json_object_get(stage, "decimation");
        if (decimation) {
            json_int_t val;

            if (!json_is_integer(decimation)) {
                log_error("Error: Decimation must be an integer.\n");
                goto out;
            }

            val = json_integer_value(decimation);
            if (val <= 0 || val >= UINT_MAX) {
                log_error("Error: Decimation value is outside of allowed range.\n");
                goto out;
            }

            fir->stages[i].decimation = (unsigned int) val;
        } else {
            fir->stages[i].decimation = 1;
        }

        total_decimation *= fir->stages[i].decimation;

        taps = json_object_get(stage, "taps");
        if (!taps) {
            log_error("Error: Filter stage is missing \"taps\" entry.\n");
            goto out;
        }

        if (!json_is_array(taps)) {
            log_error("Error: Filter \"taps\" must be an array.\n");
            goto out;
        }

        fir->stages[i].num_taps = json_array_size(taps);
        if (fir->stages[i].num_taps <= 0) {
            log_error("Error: "
                      "Filter stage %zd must have 1 or more taps.\n", i + 1);
            goto out;
        }

        /* Allocate twice the num_taps we need so we can use two insertion
         * points instead of moving elements around for a "shift" operation */
        len = 2 * fir->stages[i].num_taps * sizeof(fir->stages[i].state[0]);
        fir->stages[i].state = malloc(len);

        if (!fir->stages[i].state) {
            log_error("Error: Failed to allocate filter %zd state.\n", i + 1);
            goto out;
        }

        len = fir->stages[i].num_taps * sizeof(fir->stages[i].taps[0]);
        fir->stages[i].taps = malloc(len);

        if (!fir->stages[i].taps) {
            log_error("Error: Failed to allocate filter %zd taps.\n", i + 1);
            goto out;
        }

        /* Output buffer needs to fit: integer_ceil(total_num_taps / decimation) */
        fir->stages[i].output_len =
            (max_input + total_decimation - 1) / total_decimation;

        log_verbose("Stage %zd output buffer length: %zd\n",
                    i + 1, fir->stages[i].output_len);

        len = fir->stages[i].output_len * sizeof(fir->stages[i].output[0]);

        if (len <= 0) {
            log_error("Bug: Invalid output buffer length encountered.\n");
            goto out;
        }

        fir->stages[i].output = malloc(len);
        if (!fir->stages[i].output) {
            log_error("Failed to allocate output buffer.\n");
            goto out;
        }

        json_array_foreach(taps, tap_idx, tap) {
            if (!json_is_number(tap)) {
                log_error("Error: tap %zd in stage %zd is an invalid value.\n",
                          tap_idx + 1, i + 1);
                goto out;
            }

            fir->stages[i].taps[tap_idx] = (float) json_number_value(tap);
        }
    }

    fir->max_input = max_input;
    fir->total_decimation = total_decimation;

    fir_reset(fir);
    status = 0;

out:
    if (input) {
        fclose(input);
    }

    if (root) {
        json_decref(root);
    }

    if (status != 0) {
        fir_deinit(fir);
        fir = NULL;
    }

    return fir;
}

void fir_deinit(struct fir_filter *fir)
{
    size_t i;

    if (!fir) {
        return;
    }

    if (fir->stages) {
        for (i = 0; i < fir->num_stages; i++) {
            free(fir->stages[i].state);
            free(fir->stages[i].taps);
            free(fir->stages[i].output);
        }

        free(fir->stages);
    }

    free(fir);
}

void fir_reset(struct fir_filter *filter)
{
    size_t i;
    size_t s;

    struct fir_stage *stage;

    for (s = 0; s < filter->num_stages; s++) {
        stage = &filter->stages[s];

        for (i = 0; i < (2 * stage->num_taps); i++) {
            stage->state[i].real = stage->state[i].imag = 0.0f;
        }

        for (i = 0; i < stage->output_len; i++) {
            stage->output[i].real = stage->output[i].imag = 0.0f;
        }

        stage->count = stage->decimation;

        stage->ins1 = &stage->state[0];
        stage->ins2 = &stage->state[stage->num_taps];
    }
}

unsigned int fir_get_total_decimation(struct fir_filter *f)
{
    return f->total_decimation;
}

static inline bool update(struct fir_stage *f, struct complexf *out)
{
    bool updated_output = false;

    f->count--;

    /* Perform convolution when decimation countdown reaches 0 */
    if (f->count == 0) {
        size_t i;
        const struct complexf *x = f->ins2;

        out->real = out->imag = 0;

        for (i = 0; i < f->num_taps; i++, x--) {
            out->real += f->taps[i] * x->real;
            out->imag += f->taps[i] * x->imag;
        }


        updated_output = true;
        f->count = f->decimation;
    }

    f->ins1++;
    f->ins2++;

    if (f->ins2 == &f->state[2 * f->num_taps]) {
        f->ins1 = &f->state[0];
        f->ins2 = &f->state[f->num_taps];
    }

    return updated_output;
}

static size_t inline perform_stage(struct fir_stage *f,
                                   const struct complexf *in, size_t n,
                                   struct complexf *out)
{
    size_t i;
    size_t num_out = 0;

    for (i = 0; i < n; i++) {
        *f->ins1 = *f->ins2 = in[i];
        if (update(f, out)) {
            log_verbose("out[%zd] = %f + %fj\n", num_out, out->real, out->imag);
            out++;
            num_out++;
        }
    }

    return num_out;
}

size_t fir_filter_and_decimate(struct fir_filter *filter,
                               const struct complexf *fn_input, size_t count,
                               struct complexf *fn_output)
{
    const struct complexf *input;
    struct complexf *output;
    size_t s;
    size_t n_in;
    size_t n_out = count;

    for (s = 0; s < filter->num_stages; s++) {

        /* Select output from previous stage or fn input for first stage */
        if (s == 0) {
            input = fn_input;
            log_verbose("Stage %zd input is fn_input (%p)\n", s + 1, input);
        } else {
            input = filter->stages[s-1].output;
            log_verbose("Stage %zd input is Stage %zd output (%p) \n",
                        s + 1, s, input);
        }

        /* Write output to next stage, or function output on last stage */
        if (s == (filter->num_stages - 1)) {
            output = fn_output;
            log_verbose("Stage %zd output is fn output (%p)\n",
                        s + 1, output);
        } else {
            output = filter->stages[s].output;
            log_verbose("Stage %zd output is internal buffer (%p)\n",
                        s + 1, output);
        }

        n_in  = n_out;
        n_out = perform_stage(&filter->stages[s], input, n_in, output);

        log_verbose("Stage %zd: %zd in, %zd out\n", s + 1, n_in, n_out);
    }

    return n_out;
}
