/*
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
#ifndef OOKIEDOKIE_CFG_H_
#define OOKIEDOKIE_CFG_H_

#include <stdio.h>
#include <stdbool.h>
#include "log.h"

enum ookiedokie_dir {
    DIRECTION_INVALID = -1,
    DIRECTION_RX,
    DIRECTION_TX,
};

struct ookiedokie_cfg {

    /* Required config */
    const char *sdr_type;
    enum ookiedokie_dir direction;

    /* SDR config */
    const char *sdr_args;
    unsigned int frequency;
    unsigned int bandwidth;
    unsigned int samplerate;
    int gain;

    /* Target device */
    const char *device;

    /* Transmit options */
    unsigned int tx_count;
    unsigned int tx_delay_us;
    struct keyval_list *device_params;

    /* Receive options */
    const char *rx_rec_filename;
    const char *rx_rec_type;
    const char *rx_filter;
    const char *rx_rec_dig;
    bool rx_rec_input; /* Else record post-filter */

    /* Stream config */
    unsigned int samples_per_buffer;
    unsigned int num_buffers;
    unsigned int num_transfers;
    unsigned int stream_timeout_ms;
    unsigned int sync_timeout_ms;

    /* Other */
    enum log_level verbosity;
};

int ookiedokie_cfg_init(struct ookiedokie_cfg *cfg);
void ookiedokie_cfg_deinit(struct ookiedokie_cfg *cfg);

#endif
