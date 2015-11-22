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

/**
 * State machine trigger conditions
 */
enum sm_trigger_cond {
    SM_TRIGGER_COND_INVALID = 0,    /**< Denotes invalid condition value */
    SM_TRIGGER_COND_ALWAYS,         /**< Always fires */
    SM_TRIGGER_COND_PULSE_START,    /**< A bit has transitioned from 0 to 1 */
    SM_TRIGGER_COND_PULSE_END,      /**< A bit has transitioned from 1 to 0 */
    SM_TRIGGER_COND_TIMEOUT,        /**< A specified timeout has expired */
    SM_TRIGGER_COND_MSG_COMPLETE,   /**< All message bits have been processed */
};

/**
 * Actions to take when a trigger condition has been satisfied
 */
enum sm_trigger_action {
    SM_TRIGGER_ACTION_INVALID = 0,  /**< Denotes invalid action value */
    SM_TRIGGER_ACTION_NONE,         /**< Take no action */
    SM_TRIGGER_ACTION_APPEND_0,     /**< Append a 0 to the aggregated data */
    SM_TRIGGER_ACTION_APPEND_1,     /**< Append a 1 to the aggregated data */
    SM_TRIGGER_ACTION_OUTPUT_DATA,  /**< Output the aggregated data */
};

/**
 * Result status for processing a digital input stream
 */
enum sm_process_result {
    SM_PROCESS_RESULT_ERROR = -1,   /**< A failure occurred */
    SM_PROCESS_RESULT_NO_OUTPUT,    /**< More input required; no output ready */
    SM_PROCESS_RESULT_OUTPUT_READY, /**< Output is ready for consumption */
};

/** Opaque start machine handle */
struct state_machine;

/**
 * Initialize state machine handle. This is expected to be followed
 * by a series of sm_add_state() calls to add information about each state.
 *
 * @param   num_states      Total number of states that will be added via
 *                          sm_add_state().
 *
 * @param   data            Data buffer to use when aggregating received data,
 *                          or when generating an output signal to transmit.
 *                          This is expected to contain at least max_bits,
 *                          rounded up to the nearest byte.
 *
 * @param   max_bits        Number of aggregated bits to accumulate before
 *                          asserting that output data is available.
 *
 * @param   sample_rate     Sample rate of input. Used internally to convert
 *                          sample counts to microseconds.
 *
 * @return state_machine handle on success, or NULL if an error occurred.
 */
struct state_machine * sm_init(unsigned int num_states,
                               uint8_t *data, unsigned int max_bits,
                               unsigned int sample_rate);

/**
 * Add a state to the specified state machine. This is expected to be
 * followed by a series of sm_add_state_trigger() calls to add triggers
 *
 * @param   sm              State machine to update
 *
 * @param   state_name      String that uniquely identifies this state.
 *
 * @param   duration_us     Expected duration of this state in microseconds,
 *                          or 0 if indefinite.
 *
 * @param   timeout_us      Timeout of this state in microseconds, or 0 if
 *                          no timeout.
 *
 * @param   num_triggers    Total number of triggers that will be added via
 *                          sm_add_state_trigger().
 *
 * @return true on success or false on failure
 */
bool sm_add_state(struct state_machine *sm, const char *state_name,
                  uint64_t duration_us, uint64_t timeout_us,
                  unsigned int num_triggers);

/**
 * Add a trigger to the specified state
 *
 * @param   sm              State machine to update
 *
 * @param   state_name      Name of the state to update
 *
 * @param   cond            Condition under which to activate the trigger
 *
 * @param   duration_us     If non-zero, this duration is an additional
 *                          condition required to activate the trigger.
 *
 * @param   next_state      State to transition to upon trigger activation.
 *
 * @param   action          Action to take upon trigger activation.
 *
 * @return true on success or false on failure
 */
bool sm_add_state_trigger(struct state_machine *sm,
                          const char *state_name,
                          enum sm_trigger_cond cond,
                          uint64_t duration_us,
                          const char *next_state,
                          enum sm_trigger_action action);

/**
 * Get the trigger condition enumeration value associated with the provided
 * string.
 *
 * @return SM_TRIGGER_COND_* value on success. SM_TRIGGER_COND_INVALID will
 *         be returned for invalid input.
 */
enum sm_trigger_cond sm_trigger_cond_value(const char *string);
enum sm_trigger_action sm_trigger_action_value(const char *string);

/**
 * Verifies that state machine is fully initialized and ready for use. It
 * will be considered uninitialized if not all sm_add_state() and
 * sm_add_state_trigger() calls have been made.
 *
 * @return true if initialized, and false otherwise.
 */
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
enum sm_process_result sm_process(struct state_machine *sm, const bool *data,
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


/**
 * Deinitialize the provided state machine and free up its resources.
 *
 * @param   sm  State machine to deinit
 */
void sm_deinit(struct state_machine *sm);

#endif
