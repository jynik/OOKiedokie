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

/**
 * Direction to operate OOKiedokie in
 */
enum ookiedokie_dir {
    DIRECTION_INVALID = -1, /**< Default "uninitialized" value */
    DIRECTION_RX,           /**< Receive samples */
    DIRECTION_TX,           /**< Transmit samples */
};

/**
 * Runtime configuration parameters
 */
struct ookiedokie_cfg {

    /* Required config */
    const char *sdr_type;           /**< Type of SDR or file format to use */
    enum ookiedokie_dir direction;  /**< Direction to operate in */

    /* SDR config */
    const char *sdr_args;           /**< SDR-specific arguments */
    unsigned int frequency;         /**< SDR frequency, in Hz */
    unsigned int bandwidth;         /**< SDR filter BW, in Hz */
    unsigned int samplerate;        /**< SDR sample rate, in Hz */
    int gain;                       /**< SDR-specific gain value */

    /* Target device */
    const char *device;             /**< Name of target OOK device */

    /* Transmit options */
    unsigned int tx_count;          /**< Number of times to re-transmit msg */
    unsigned int tx_delay_us;       /**< Intra-repeat delay, in microseconds */
    struct keyval_list *device_params; /**< Message field parameters */

    /* Receive options */
    const char *rx_rec_filename;    /**< Filename to record samples to */
    const char *rx_rec_type;        /**< File format type to record with */
    const char *rx_filter;          /**< Filename of RX filter to user */
    const char *rx_rec_dig;         /**< Filename to record digital samples to */
    bool rx_rec_input;              /**< If true, record pre-filtered input,
                                     *   otherwise record post-filtered
                                     *   samples. */

    /* Stream config - specific to SDR stream implementation  */
    unsigned int samples_per_buffer;    /**< # Samples per buffer */
    unsigned int num_buffers;           /**< Total # of buffers to use */
    unsigned int num_transfers;         /**< Max # of in-flight transfers */
    unsigned int stream_timeout_ms;     /**< Stream timeout, in milliseconds */
    unsigned int sync_timeout_ms;       /**< Millisecond timeout per RX/TX */

    /* Other */
    enum log_level verbosity;           /**< Output verbosity level */
};

/**
 * Initialize the provided configuration to default values.
 *
 * @param   cfg     Runtime-configuration to initialize
 *
 * @return 0 on success, non-zero on failure
 */
int ookiedokie_cfg_init(struct ookiedokie_cfg *cfg);

/**
 * Deinitialize and deallocate fields in the provided runtime configuration.
 *
 * @param   cfg     Runtime configuration to deinitialize
 */
void ookiedokie_cfg_deinit(struct ookiedokie_cfg *cfg);

#endif
