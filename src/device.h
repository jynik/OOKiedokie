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
#ifndef OOKIEDOKIE_DEVICE_H_
#define OOKIEDOKIE_DEVICE_H_

/* This file provides an interface for processing samples from and generating
 * samples for a device that utilizes on-off keying. */

#include <stdbool.h>
#include "complexf.h"
#include "keyval_list.h"

/**
 * Opaque handle to a device specifications object.
 */
struct device;

/**
 * Open the device specification for the specified device and create
 * an object representing that specfication.
 *
 * @param   name            Device name or file path.
 * @param   sample_rate     Sample rate that will be used in conjunction
 *                          with input or output samples.
 *
 * @return device specification handle on sucess, NULL on failure
 */
struct device * device_init(const char *name, unsigned int sample_rate);


/**
 * Process a stream of received digital samples
 *
 * @param   d               Device specification handle
 * @param   data            Array of digital input samples
 * @param   count           Number of input samples in `data`
 *
 * @return A key-value list of decoded message fields. Values in this
 *         keyval-list are only valid until the next time this function is
 *         called.
 */
const struct keyval_list * device_process(struct device *d,
                                          const bool *data, unsigned int count);

/**
 * Generate complex samples for a single message
 *
 * The caller is responsible for freeing (*samples).
 *
 * @param{in]   d               Device specification handle
 *
 * @param{in]   params          Key-value list of message parameters to use
 *                              when filling in message fields.
 *
 * @param[out]  samples         Will be updated to point to an array of complex
 *                              samples that contain the modulated OOK signal
 *                              for a message.
 *
 * @param[out]  num_samples     Updated to indicate the number of samples in
 *                              `samples.`
 *
 * @return true on success, false on failure
 */
bool device_generate(struct device *d, const struct keyval_list *params,
                     struct complexf **samples, unsigned int *num_samples);

/**
 * Deallocate and deinitialize the provided device specifications object
 *
 * @param   d       Device specification object to deinitialize
 */
void device_deinit(struct device *d);

#endif
