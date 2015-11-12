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
#ifndef OOKIEDOKIE_STATE_MACHINE_H_
#define OOKIEDOKIE_STATE_MACHINE_H_

#include <stdbool.h>
#include <stdint.h>

#include "complexf.h"

enum sm_trigger_cond {
    SM_TRIGGER_COND_INVALID = 0,
    SM_TRIGGER_COND_ALWAYS,
    SM_TRIGGER_COND_PULSE_START,
    SM_TRIGGER_COND_PULSE_END,
    SM_TRIGGER_COND_TIMEOUT,
    SM_TRIGGER_COND_MSG_COMPLETE,   /** When # bits == max_bits */
};

enum sm_trigger_action {
    SM_TRIGGER_ACTION_INVALID = 0,
    SM_TRIGGER_ACTION_NONE,
    SM_TRIGGER_ACTION_APPEND_0,
    SM_TRIGGER_ACTION_APPEND_1,
    SM_TRIGGER_ACTION_OUTPUT_DATA,
};

enum sm_process_result {
    SM_PROCESS_RESULT_ERROR = -1,
    SM_PROCESS_RESULT_NO_OUTPUT,
    SM_PROCESS_RESULT_OUTPUT_READY,
};

struct state_machine;


/**
 * @param   data    Data buffer to use when aggregating received data, or
 *                  when generating an output signal to transmit. This is
 *                  expected to contain at least max_bits, rouneded up to
 *                  the nearest byte.
 */
struct state_machine * sm_init(unsigned int num_states,
                               uint8_t *data, unsigned int max_bits,
                               unsigned int sample_rate);

bool sm_add_state(struct state_machine *sm, const char *state_name,
                  uint64_t duration_us, uint64_t timeout_us,
                  unsigned int num_triggers);

bool sm_add_state_trigger(struct state_machine *sm,
                          const char *state_name,
                          enum sm_trigger_cond cond,
                          uint64_t duration_us,
                          const char *next_state,
                          enum sm_trigger_action action);

enum sm_trigger_cond sm_trigger_cond_value(const char *string);
enum sm_trigger_action sm_trigger_action_value(const char *string);

bool sm_initialized(struct state_machine *sm);

/**
 * Process received samples.
 *
 * @param[in]   sm          State machine to pass samples through
 * @param[in]   data        Data to process
 * @param[in]   count       Number of values in data
 * @param[out]  num_proc    Actual number of items processed (<= count).
 *
 * @return An sm_process_result value denoting the result of the operation.
 *
 * @note When SM_PROCESS_RESULT_OUTPUT_READY is returned, the data buffer
 *       passed to sm_init *must* be read before another call to sm_process().
 *
 * @note When SM_PROCESS_RESULT_OUTPUT_READY is returned, num_processed
 *       will be set to <= count; you many need to call this function again
 *       with &data[num_processed], (count - *num_processed).
 */
enum sm_process_result sm_process(struct state_machine *sm, bool *data,
                                  unsigned int count, unsigned int *num_proc);

/**
 * Generate samples for the provided data. This function expects to
 * receive all data in a single call.
 *
 * @param[in]   sm          State machine to use to generate samples
 * @param[in]   data        Input data to generate samples for
 * @param[in]   count       Number of bits in the provided data
 * @param[in]   on_val      Value of an "on" bit
 * @param[out]  num_samples Number of samples in the returned array
 *
 * @return Heap-allocated samples on success. NULL on failure. The caller is
 *         responsible for freeing samples.
 */
struct complexf * sm_generate(struct state_machine *sm,
                              uint8_t *data, unsigned int count,
                              float on_val, unsigned int *num_samples);


void sm_deinit(struct state_machine *sm);

#endif
