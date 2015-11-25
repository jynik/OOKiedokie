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

#include <stdio.h>
#include "ookiedokie_cfg.h"
#include "keyval_list.h"

#define DEFAULT_THRESHOLD   0.1f
#define DEFAULT_TX_DELAY_US         4000
#define DEFAULT_TX_COUNT            1
#define DEFAULT_FREQ                433920000
#define DEFAULT_GAIN                35
#define DEFAULT_RATE                3000000
#define DEFAULT_BW                  1500000
#define DEFAULT_SAMPLES_PER_BUF     8192
#define DEFAULT_NUM_BUFFERS         64
#define DEFAULT_NUM_TRANSFERS       16
#define DEFAULT_STREAM_TIMEMOUT_MS  1500
#define DEFAULT_SYNC_TIMEOUT_MS     3000

int ookiedokie_cfg_init(struct ookiedokie_cfg *c)
{

    /* Required items */
    c->sdr_type             = NULL;
    c->direction            = DIRECTION_INVALID;

    /* SDR config */
    c->sdr_args = NULL;
    c->frequency            = DEFAULT_FREQ;
    c->bandwidth            = DEFAULT_BW;
    c->samplerate           = DEFAULT_RATE;
    c->gain                 = DEFAULT_GAIN;

    /* Sample stream config */
    c->samples_per_buffer   = DEFAULT_SAMPLES_PER_BUF;
    c->num_buffers          = DEFAULT_NUM_BUFFERS;
    c->num_transfers        = DEFAULT_NUM_TRANSFERS;
    c->stream_timeout_ms    = DEFAULT_STREAM_TIMEMOUT_MS;
    c->sync_timeout_ms      = DEFAULT_SYNC_TIMEOUT_MS;

    /* Transmit config */
    c->tx_count             = DEFAULT_TX_COUNT;
    c->tx_delay_us          = DEFAULT_TX_DELAY_US;

    c->device_params = keyval_list_init();
    if (!c->device_params) {
        fprintf(stderr, "Failed to allocated device parameter container.\n");
        return -1;
    }

    /* Target device */
    c->device = NULL;

    /* Receive items */
    c->rx_threshold = DEFAULT_THRESHOLD;
    c->rx_rec_type = NULL;
    c->rx_rec_filename = NULL;
    c->rx_filter = NULL;
    c->rx_rec_input = false;
    c->rx_rec_dig = NULL;

    /* Misc */
    c->verbosity = LOG_LEVEL_INFO;

    return 0;
}

void ookiedokie_cfg_deinit(struct ookiedokie_cfg *c)
{
    free((void*) c->device);
    keyval_list_deinit(c->device_params);
    free((void*) c->rx_rec_filename);
    free((void*) c->rx_rec_type);
    free((void*) c->rx_filter);
    free((void*) c->rx_rec_dig);
    free((void*) c->sdr_args);
    free((void*) c->sdr_type);
}
