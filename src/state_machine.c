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
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "state_machine.h"
#include "log.h"

struct trigger {
    enum sm_trigger_cond condition;
    uint64_t duration_us;               // TODO document this

    enum sm_trigger_action action;
    struct state *next_state;
};

struct state {
    char *name;

    uint64_t duration_us;       // TODO make this doubles for sub-us values
    uint64_t timeout_us;

    struct trigger *triggers;
    size_t num_triggers;
};

/* The reset state is always states[0] */
#define STATE_RESET 0

/* TODO: Make this run-time or device-configurable */
#define TOLERANCE 0.15

struct state_machine
{
    struct state *states;
    struct state *curr_state;

    unsigned int num_states;

    uint8_t *data;

    unsigned int max_bits;      /* Max number of bits to process */
    unsigned int num_bits;      /* Current count of # of bits processed */

    bool prev_bit;

    double elapsed_us;          /* us elapsed since last transition */
    uint64_t count_monotonic;   /* Monotonic sample counter for debug */

    unsigned int sample_rate;
};

/* Convert sample count to duration in us */
static inline double to_duration_us(struct state_machine *sm,
                                    unsigned int samples)
{
    return ((double) samples / (double) sm->sample_rate) * 1e6;
}

/* Convert duration to sample count
 *
 * TODO overflow check
 */
static inline unsigned int to_sample_count(struct state_machine *sm,
                                           uint64_t duration_us)
{
    return (unsigned int)(duration_us * ((double) sm->sample_rate / 1e6) + 0.5);
}



/*----------------------------------------------------------------------------
 * Receive processing
 *---------------------------------------------------------------------------*/

static bool matches_state_duration(const struct state_machine *sm, bool check)
{
    bool match = true;

    if (check) {
        const struct state *s = sm->curr_state;

        /* 0 implies "any duration" */
        if (s->duration_us != 0.0f) {
            const float min = s->duration_us - (TOLERANCE * s->duration_us);
            const float max = s->duration_us + (TOLERANCE * s->duration_us);

            match = (sm->elapsed_us >= min) && (sm->elapsed_us <= max);
        }
    }

    return match;
}

static bool matches_trigger_duration(const struct state_machine *sm,
                                     const struct trigger *trig)
{
    bool match = true;

    /* 0 Implies "any duration" */
    if (trig->duration_us != 0) {
        const float min = trig->duration_us - (TOLERANCE * trig->duration_us);
        const float max = trig->duration_us + (TOLERANCE * trig->duration_us);

        match = (sm->elapsed_us >= min) && (sm->elapsed_us <= max);
    }

    return match;
}

struct state_machine * sm_init(unsigned int num_states,
                               uint8_t *data, unsigned int max_bits,
                               unsigned int sample_rate)
{
    int status = -1;
    unsigned int num_bytes, i;
    struct state_machine *sm;

    if (max_bits == 0) {
        return NULL;
    }

    sm = calloc(1, sizeof(sm[0]));
    if (!sm) {
        return NULL;
    }

    sm->sample_rate = sample_rate;

    sm->num_states = num_states;
    sm->states = calloc(num_states, sizeof(sm->states[0]));
    if (!sm->states) {
        perror("calloc");
        goto out;
    }

    for (i = 0; i < num_states; i++) {
        sm->states[i].name = NULL;
    }

    sm->curr_state = &sm->states[STATE_RESET];

    sm->max_bits = max_bits;
    sm->data = data;

    status = 0;

out:
    if (status != 0) {
        sm_deinit(sm);
        sm = NULL;
    }

    return sm;
}

bool sm_initialized(struct state_machine *sm)
{
    unsigned int s, t;

    for (s = 0; s < sm->num_states; s++) {
        const struct state *state = &sm->states[s];

        if (state->name == NULL) {
            log_debug("State #%u is uninitialized.\n", s);
            return false;
        }

        for (t = 0; t < state->num_triggers; t++) {
            if (state->triggers[t].condition == SM_TRIGGER_COND_INVALID) {
                log_critical("{%s} trigger[%u].condition is initialized.\n",
                             sm->states[s].name, t);
                return false;
            }

            if (state->triggers[t].action == SM_TRIGGER_ACTION_INVALID) {
                log_critical("{%s} trigger[%u].action is initialized.\n",
                             sm->states[s].name, t);
                return false;
            }
        }
    }

    return true;
}

/* Get a pointer to the specified state. If it's not in our state list,
 * we'll reserve room for it and return a pointer to it.
 *
 * NULL is returned if we ran out of states in the list.  This shouldn't happen,
 * as the caller should have told us how many states there were when the
 * state machine was allocated.
 */
static inline struct state *
get_or_reserve_state(struct state_machine *sm, const char *state_name)
{
    unsigned int i;

    /* The reset state must always be the first state */
    if (!strcasecmp("reset", state_name) &&
        sm->states[STATE_RESET].name == NULL) {

        sm->states[STATE_RESET].name = strdup(state_name);
        if (sm->states[STATE_RESET].name) {
            log_verbose("Reserved state: %s (%d)\n", state_name, STATE_RESET);
            return &sm->states[STATE_RESET];
        } else {
            perror("strdup");
            return NULL;
        }
    }

    for (i = 0; i < sm->num_states; i++) {
        if (sm->states[i].name == NULL) {
            sm->states[i].name = strdup(state_name);
            if (sm->states[i].name) {
                log_verbose("Reserved state: %s (%d)\n", state_name, i);
                return &sm->states[i];
            } else {
                perror("strdup");
                return NULL;
            }
        } else if (!strcmp(sm->states[i].name, state_name)) {
            log_verbose("Found state in list: %s\n", state_name);
            return &sm->states[i];
        }
    }

    log_error("No room left to add state \"%s\"\n", state_name);
    return NULL;
}


bool sm_add_state(struct state_machine *sm, const char *name,
                  uint64_t duration_us, uint64_t timeout_us,
                  unsigned int num_triggers)
{
    struct state *s;

    s = get_or_reserve_state(sm, name);
    if (s == NULL) {
        return false;
    }

    if (s->triggers != NULL) {
        log_warning("State may be getting initialized more than once: %s\n",
                    name);
        free(s->triggers);
        s->triggers = NULL;
    }

    s->duration_us = duration_us;
    s->timeout_us = timeout_us;
    s->num_triggers = num_triggers;

    if (s->num_triggers != 0) {
        s->triggers = calloc(num_triggers, sizeof(s->triggers[0]));
        if (!s->triggers) {
            perror("calloc");
            free(s);
            return false;
        }
    }

    log_verbose("Added state: %s\n", s->name);
    return true;
}

bool sm_add_state_trigger(struct state_machine *sm,
                          const char *state_name,
                          enum sm_trigger_cond condition,
                          uint64_t duration_us,
                          const char *next_state,
                          enum sm_trigger_action action)
{
    unsigned int i;
    struct state *s = NULL;

    /* Determine which state we're updating */
    for (i = 0; i < sm->num_states && s == NULL; i++) {
        if (!strcmp(sm->states[i].name, state_name)) {
            s = &sm->states[i];
        }
    }

    if (s == NULL) {
        return false;
    }

    /* Append the trigger condition to state's list of them */
    for (i = 0; i < s->num_triggers; i++) {
        if (s->triggers[i].condition == SM_TRIGGER_COND_INVALID) {

            log_verbose("Added trigger condition %d: %s -> %s\n",
                        condition, s->name, next_state);

            s->triggers[i].condition = condition;
            s->triggers[i].duration_us = duration_us;
            s->triggers[i].action = action;
            s->triggers[i].next_state = get_or_reserve_state(sm, next_state);
            return s->triggers[i].next_state != NULL;
        }
    }

    return false;
}

enum sm_trigger_cond sm_trigger_cond_value(const char *string)
{
    if (!strcasecmp(string, "always")) {
        return SM_TRIGGER_COND_ALWAYS;
    } else if (!strcasecmp(string, "pulse_start")) {
        return SM_TRIGGER_COND_PULSE_START;
    } else if (!strcasecmp(string, "pulse_end")) {
        return SM_TRIGGER_COND_PULSE_END;
    } else if (!strcasecmp(string, "timeout")) {
        return SM_TRIGGER_COND_TIMEOUT;
    } else if (!strcasecmp(string, "msg_complete")) {
        return SM_TRIGGER_COND_MSG_COMPLETE;
    } else {
        return SM_TRIGGER_COND_INVALID;
    }
}

enum sm_trigger_action sm_trigger_action_value(const char *string)
{
    if (!strcasecmp(string, "none")) {
        return SM_TRIGGER_ACTION_NONE;
    } else if (!strcasecmp(string, "append_0")) {
        return SM_TRIGGER_ACTION_APPEND_0;
    } else if (!strcasecmp(string, "append_1")) {
        return SM_TRIGGER_ACTION_APPEND_1;
    } else if (!strcasecmp(string, "output_data")) {
        return SM_TRIGGER_ACTION_OUTPUT_DATA;
    } else {
        return SM_TRIGGER_ACTION_INVALID;
    }
}

/* Returns success */
static bool append_data_bit(struct state_machine *sm, bool one_bit)
{
    bool success = true;


    if (sm->num_bits <= sm->max_bits) {
        unsigned int byte_idx = sm->num_bits / 8;
        unsigned int bit_pos  = sm->num_bits % 8;

        if (one_bit) {
            sm->data[byte_idx] |= (1 << bit_pos);
        } else {
            sm->data[byte_idx] &= ~(1 << bit_pos);
        }
    } else {
        log_critical("Attemped to append bit into full data buffer.\n");
        success = false;
    }

    return success;
}

/* Returns true on success */
static inline enum sm_process_result handle_actions(struct state_machine *sm,
                                                    const struct trigger *t)
{
    enum sm_process_result result = SM_PROCESS_RESULT_NO_OUTPUT;

    switch (t->action) {
        case SM_TRIGGER_ACTION_NONE:
            break;

        case SM_TRIGGER_ACTION_APPEND_0:
            log_verbose("Bit %u: 0\n", sm->num_bits);
            append_data_bit(sm, false);
            sm->num_bits++;
            break;

        case SM_TRIGGER_ACTION_APPEND_1:
            log_verbose("Bit %u: 1\n", sm->num_bits);
            append_data_bit(sm, true);
            sm->num_bits++;
            break;

        case SM_TRIGGER_ACTION_OUTPUT_DATA:
            result = SM_PROCESS_RESULT_OUTPUT_READY;
            break;

        default:
            log_critical("Invalid action encountered: %d\n", t->action);
            result = SM_PROCESS_RESULT_ERROR;
    }

    return result;
}

static inline enum sm_process_result handle_rx_triggers(struct state_machine *sm,
                                                        bool b)
{
    unsigned int i;
    const unsigned int num_triggers = sm->curr_state->num_triggers;
    const struct trigger *active_trigger = NULL;
    bool check_duration = false;
    enum sm_process_result result = SM_PROCESS_RESULT_NO_OUTPUT;

    for (i = 0; i < num_triggers && active_trigger == NULL; i++) {
        const struct trigger *t = &sm->curr_state->triggers[i];

        if (matches_trigger_duration(sm, t)) {
            switch (t->condition) {
                case SM_TRIGGER_COND_ALWAYS:
                    active_trigger = t;
                    log_verbose("[State=%s] \"Always\" trigger @ sample %"PRIu64"\n",
                                sm->curr_state->name, sm->count_monotonic);
                    break;

                case SM_TRIGGER_COND_PULSE_START:
                    if (sm->prev_bit == false && b == true) {
                        active_trigger = t;
                        check_duration = true;
                        log_verbose("{%s} Pulse Start trigger @ sample %"PRIu64"\n",
                                    sm->curr_state->name, sm->count_monotonic);
                    }
                    break;

                case SM_TRIGGER_COND_PULSE_END:
                    if (sm->prev_bit == true && b == false) {
                        active_trigger = t;
                        check_duration = true;
                        log_verbose("{%s} Pulse End trigger @ sample %"PRIu64"\n",
                                    sm->curr_state->name, sm->count_monotonic);
                    }
                    break;

                case SM_TRIGGER_COND_TIMEOUT:
                    if (sm->elapsed_us >= sm->curr_state->timeout_us) {
                        active_trigger = t;
                        log_verbose("{%s} Timeout trigger @ sample %"PRIu64"\n",
                                    sm->curr_state->name, sm->count_monotonic);
                    }
                    break;

                case SM_TRIGGER_COND_MSG_COMPLETE:
                    if (sm->num_bits >= sm->max_bits) {
                        active_trigger = t;
                        log_verbose("{%s} Msg completion trigger @ sample %"PRIu64"\n",
                                    sm->curr_state->name, sm->count_monotonic);
                    }
                    break;

                default:
                    log_critical("Invalid trigger encoutered in state %s: %d\n",
                                sm->curr_state->name, t->condition);
                    return SM_PROCESS_RESULT_ERROR;
            }
        }
    }

    if (active_trigger != NULL) {
        bool expected_duration = matches_state_duration(sm, check_duration);

        if (expected_duration) {
            result = handle_actions(sm, active_trigger);
            if (result != SM_PROCESS_RESULT_ERROR) {
                if (sm->curr_state != active_trigger->next_state) {
                    log_verbose("Setting next state to: %s\n",
                                active_trigger->next_state->name);
                }

                sm->curr_state = active_trigger->next_state;

            }
        } else {
            result = SM_PROCESS_RESULT_ERROR;
            log_debug("{%s} Encountered invalid duration: %f, expected %f. ",
                      sm->curr_state->name, sm->elapsed_us,
                      (double) sm->curr_state->duration_us);
        }

        if (result == SM_PROCESS_RESULT_ERROR) {
            log_debug("Resetting state machine.\n") ;
            sm->curr_state = &sm->states[STATE_RESET];
        }

        sm->elapsed_us = 0;

    } else {
        sm->elapsed_us += to_duration_us(sm, 1);
    }

    sm->count_monotonic++;
    return result;
}

static int process(struct state_machine *sm, bool bits)
{
    int status = 0;

    /* Transition directly through reset */
    if (sm->curr_state == &sm->states[STATE_RESET]) {
        sm->num_bits = 0;
        memset(sm->data, 0, (sm->max_bits + 7) / 8);

        log_verbose("Reset state machine\n");

        status = handle_rx_triggers(sm, bits);
        if (status != 0) {
            return status;
        }
    }

    return handle_rx_triggers(sm, bits);
}

enum sm_process_result sm_process(struct state_machine *sm, const bool *data,
                                  unsigned int count, unsigned int *num_proc)
{
    unsigned int i;
    enum sm_process_result result = SM_PROCESS_RESULT_NO_OUTPUT;

    assert(sm->curr_state != NULL);

    for (i = 0; i < count && result == SM_PROCESS_RESULT_NO_OUTPUT; i++) {
        result = process(sm, data[i]);
        sm->prev_bit = data[i];
    }

    *num_proc = i;
    return result;
}




/*----------------------------------------------------------------------------
 * Transmit processing
 *---------------------------------------------------------------------------*/

struct gen_state {
    float on_val;
    bool curr_logic_val;

    struct complexf *samples;
    unsigned int alloc_len;
    unsigned int count;
};

bool append_samples(struct state_machine *sm, struct gen_state *gen,
                    uint64_t duration)
{
    bool success = true;
    unsigned int i;
    unsigned int count = to_sample_count(sm, duration);
    float value = (gen->curr_logic_val == true) ? gen->on_val : 0.0f;

    struct complexf *target;

    log_verbose("Appending %u samples of %f+0j\n", count, value);

    /* Don't have enough room for these samples */
    if ((gen->alloc_len - gen->count) < count) {

        void *tmp;
        unsigned int new_alloc_len = (gen->alloc_len << 1);

        if (new_alloc_len < gen->alloc_len) {
            log_error("Sample buffer allocation is too large!\n");
            return false;
        }

        /* Verify that we won't overrun buffer count */
        if ((UINT_MAX - gen->count) < count) {
            log_error("Sample buffer full!\n");
            return false;
        }

        tmp = realloc(gen->samples, new_alloc_len * sizeof(gen->samples[0]));
        if (!tmp) {
            log_error("Sample buffer reallocation failed!\n");
            return false;
        }

        gen->samples = (struct complexf *) tmp;
        gen->alloc_len = new_alloc_len;

        log_verbose("Sample buffer resized to fit %u samples\n", gen->alloc_len);
    }

    target = &gen->samples[gen->count];

    for (i = 0; i < count; i++) {
        target[i].real = value;
        target[i].imag = 0.0f;
    }

    gen->count += count;
    return success;
}

bool get_tx_trigger(struct state_machine *sm,
                    bool bit_val,
                    bool check_bit_action,
                    const struct trigger **active_trigger)
{
    unsigned int i;
    bool have_bit_action = false;
    const unsigned int num_triggers = sm->curr_state->num_triggers;

    *active_trigger = NULL;

    for (i = 0; i < num_triggers && *active_trigger == NULL; i++) {
        const struct trigger *t = &sm->curr_state->triggers[i];

        if (check_bit_action) {
            switch (t->action) {
                case SM_TRIGGER_ACTION_APPEND_0:
                    have_bit_action = !bit_val;
                    break;

                case SM_TRIGGER_ACTION_APPEND_1:
                    have_bit_action = bit_val;
                    break;

                case SM_TRIGGER_ACTION_OUTPUT_DATA:
                    have_bit_action = true;
                    break;

                default:
                    have_bit_action = false;
            }
        }

        if (check_bit_action && !have_bit_action)  {
            continue;
        }

        switch (t->condition) {
            case SM_TRIGGER_COND_MSG_COMPLETE:
                if (sm->num_bits == sm->max_bits) {
                    log_verbose("Active trigger: SM_TRIGGER_COND_MSG_COMPLETE\n");
                    *active_trigger = t;
                }
                break;

            case SM_TRIGGER_COND_ALWAYS:
                log_verbose("Active trigger: SM_TRIGGER_COND_ALWAYS\n");
                *active_trigger = t;
                break;

            case SM_TRIGGER_COND_PULSE_START:
                log_verbose("Active trigger: SM_TRIGGER_COND_PULSE_START\n");
                *active_trigger = t;
                break;

            case SM_TRIGGER_COND_PULSE_END:
                log_verbose("Active trigger: SM_TRIGGER_COND_PULSE_END\n");
                *active_trigger = t;
                break;

            /* A state machine should have paths through timeouts. This may
             * indicate that a timeout condition is placed before other
             * conditions that should have higher precedence. */
            case SM_TRIGGER_COND_TIMEOUT:
                log_error("Encountered SM_TRIGGER_COND_TIMEOUT while "
                          "generating samples. Bug in state machine design?\n");

                return false;

            default:
                log_critical("BUG: Unhandled condition: %d\n", t->condition);
                return false;;
        }
    }

    return true;
}

static bool handle_tx_triggers(struct state_machine *sm, bool bit_val,
                               struct gen_state *gen, bool *done)

{
    unsigned int i;
    bool success = false;

    const struct trigger *active_trigger = NULL;

    *done = false;

    /* First check for a trigger that has a matching bit action */
    success = get_tx_trigger(sm, bit_val, true, &active_trigger);
    if (!success) {
        return false;
    }


    /* If we didn't find any triggers associatd with a bit action for our
     * current bit value, get the first available trigger */
    if (!active_trigger) {
        success = get_tx_trigger(sm, bit_val, false, &active_trigger);
        if (!success) {
            return false;
        }
    }


    if (active_trigger) {

        /* State has no duration set, but the trigger requires we have
         * been in this state for some amount of time before the trigger
         * event */
        if (sm->curr_state->duration_us == 0 &&
            active_trigger->duration_us != 0) {

            log_verbose("Handling Active duration of %"PRIu64" us\n",
                         active_trigger->duration_us);

            append_samples(sm, gen, active_trigger->duration_us);
        }

        /* Perform any additional required sample generator state updates */
        switch (active_trigger->condition) {
            case SM_TRIGGER_COND_MSG_COMPLETE:
                *done = true;
                break;

            case SM_TRIGGER_COND_PULSE_START:
                if (gen->curr_logic_val == true) {
                    log_error("Bug? Logic value is already 1, got PULSE_START\n");
                    return false;
                } else {
                    gen->curr_logic_val = true;
                }
                break;

            case SM_TRIGGER_COND_PULSE_END:
                if (gen->curr_logic_val == false) {
                    log_error("Bug? Logic value is already 1, got PULSE_START\n");
                    return false;
                } else {
                    gen->curr_logic_val = false;
                }


            default:
                break;
        }

        /* Update bit count */
        if (active_trigger->action == SM_TRIGGER_ACTION_APPEND_0 ||
            active_trigger->action == SM_TRIGGER_ACTION_APPEND_1 ) {

            if (sm->num_bits < sm->max_bits) {
                sm->num_bits++;
                *done = true;

                log_verbose("Bit count updated to %u/%u\n",
                        sm->num_bits, sm->max_bits);

            } else if (sm->num_bits > sm->max_bits) {
                log_error("Bit count (%u) exceeded max (%u)\n",
                        sm->num_bits, sm->max_bits);
                return false;
            }
        }

        sm->curr_state = active_trigger->next_state;
        log_verbose("Updated state to: %s\n", sm->curr_state->name);


        if (sm->curr_state->duration_us != 0) {
            success = append_samples(sm, gen, sm->curr_state->duration_us);
        } else {
            success = true;
        }
    } else {
        success = false;
    }

    return success;
}


static bool generate(struct state_machine *sm, bool bit_val,
                     struct gen_state *gen)
{
    bool success = true;
    bool done = false;

    while (!done && success) {
        success = handle_tx_triggers(sm, bit_val, gen, &done);
    }

    return success;
}

struct complexf * sm_generate(struct state_machine *sm,
                              uint8_t *data, unsigned int count,
                              float on_val, unsigned int *num_samples_out)

{
    unsigned int i = 0;
    struct gen_state gen;
    bool success = true;

    *num_samples_out = 0;

    sm->curr_state = &sm->states[STATE_RESET];

    gen.count = 0;
    gen.alloc_len = 16384;

    gen.samples = calloc(gen.alloc_len, sizeof(gen.samples[0]));
    if (!gen.samples) {
        success = false;
        goto out;
    }

    gen.on_val = on_val;
    gen.curr_logic_val = false;

    /* Generate samples for each bit */
    for (i = 0; i < count && success; i++) {
        const unsigned int byte_pos = i / 8;
        const unsigned int bit_pos  = i % 8;
        const bool bit_val = (data[byte_pos] & (1 << bit_pos)) != 0;

        log_verbose("Generating samples for bit %u\n", i);
        success = generate(sm, bit_val, &gen);
    }

    /* Generate data-independent remainder of signal */
    if (success) {
        success = generate(sm, 0, &gen);
    }

out:
    if (success) {
        *num_samples_out = gen.count;
        return gen.samples;
    } else {
        *num_samples_out = 0;
        return NULL;
    }
}

void sm_deinit(struct state_machine *sm)
{
    unsigned int i;
    if (sm) {

        for (i = 0; i < sm->num_states; i++) {
            free(sm->states[i].name);
            free(sm->states[i].triggers);
        }

        free(sm->states);
        free(sm);
    }
}
