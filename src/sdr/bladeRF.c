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
#include <stdio.h>
#include <string.h>
#include <libbladeRF.h>

#include "sdr.h"
#include "log.h"
#include "minmax.h"
#include "ookiedokie_cfg.h"
#include "complexf.h"

struct sdr_bladerf {
    struct bladerf *handle;
    bladerf_module module;
    unsigned int timeout_ms;

    /* Sample buffer */
    int16_t *buf;
    unsigned int buf_len;
};

static inline bladerf_module dir2module(enum ookiedokie_dir dir)
{
    switch (dir) {
        case DIRECTION_RX:
            return BLADERF_MODULE_RX;

        case DIRECTION_TX:
            return BLADERF_MODULE_TX;

        default:
            return -1;
    }
}

static inline bladerf_log_level log_level(enum log_level l)
{
    switch (l) {
        case LOG_LEVEL_CRITICAL:
            return BLADERF_LOG_LEVEL_CRITICAL;

        case LOG_LEVEL_ERROR:
            return BLADERF_LOG_LEVEL_ERROR;

        case LOG_LEVEL_WARNING:
            return BLADERF_LOG_LEVEL_WARNING;

        case LOG_LEVEL_INFO:
            return BLADERF_LOG_LEVEL_INFO;

        case LOG_LEVEL_DEBUG:
            return BLADERF_LOG_LEVEL_DEBUG;

        case LOG_LEVEL_VERBOSE:
            return BLADERF_LOG_LEVEL_VERBOSE;

        default:
            return BLADERF_LOG_LEVEL_SILENT;
    }
}

void * sdr_bladerf_init(const struct ookiedokie_cfg *config)
{
    int status = 0;
    struct sdr_bladerf *sdr = NULL;
    const bladerf_log_level verbosity = log_level(config->verbosity);

    unsigned int freq = config->frequency;
    unsigned int rate = config->samplerate;
    unsigned int bw   = config->bandwidth;

    sdr = malloc(sizeof(sdr[0]));
    if (!sdr) {
        perror("malloc");
        return NULL;
    }

    sdr->buf_len = (config->num_buffers + 1) * config->samples_per_buffer;
    sdr->buf = malloc(2 * sizeof(int16_t) * sdr->buf_len);
    if (!sdr->buf) {
        free(sdr);
        perror("malloc");
        return NULL;
    }

    sdr->module = dir2module(config->direction);
    sdr->timeout_ms = config->sync_timeout_ms;

    bladerf_log_set_verbosity(verbosity);

    status = bladerf_open(&sdr->handle, config->sdr_args);
    if (status != 0) {
        log_error("Unable to open bladeRF: %s\n", bladerf_strerror(status));
        goto out;
    }

    /* TODO XB-200 support */

    if (freq < BLADERF_FREQUENCY_MIN) {
        freq = BLADERF_FREQUENCY_MIN;
        log_info("Clamping bladeRF frequency to min: %u Hz\n", freq);
    } else if (freq > BLADERF_FREQUENCY_MAX) {
        freq = BLADERF_FREQUENCY_MAX;
        log_info("Clamping bladeRF frequency to max: %u Hz\n", freq);
    }

    status = bladerf_set_frequency(sdr->handle, sdr->module, freq);
    if (status != 0) {
        log_debug("Unable to set bladeRF frequency: %s\n",
                  bladerf_strerror(status));
        goto out;
    }

    if (rate < 2000000) {
        /* We want to keep our samplerate higher than the LMS6002D's
         * rolloff point of its lowest bandwidth setting */
        rate = 2000000;
        log_info("Clamping bladeRF sample rate to min: %u Hz\n", rate);

    } else if (rate > BLADERF_SAMPLERATE_REC_MAX) {
        rate = BLADERF_SAMPLERATE_REC_MAX;
        log_info("Clamping bladeRF sample rate to max: %u Hz\n", rate);
    }

    status = bladerf_set_sample_rate(sdr->handle, sdr->module, rate, NULL);
    if (status != 0) {
        log_error("Unable to set bladeRF sample rate: %s\n",
                  bladerf_strerror(status));
        goto out;
    }

    if (bw < BLADERF_BANDWIDTH_MIN) {
        bw = BLADERF_BANDWIDTH_MIN;
        log_info("Claming bladeRF bandwidth to min: %u Hz\n", bw);
    } else if (bw > BLADERF_BANDWIDTH_MAX){
        bw = BLADERF_BANDWIDTH_MAX;
        log_info("Claming bladeRF bandwidth to max: %u Hz\n", bw);
    }

    status = bladerf_set_bandwidth(sdr->handle, sdr->module,
                                   config->bandwidth, NULL);
    if (status != 0) {
        log_error("Unable to set bladeRF bandwidth: %s\n",
                   bladerf_strerror(status));
        goto out;
    }

    /* This function will clamp for us */
    status = bladerf_set_gain(sdr->handle, sdr->module, config->gain);
    if (status != 0) {
        log_error("Failed to set bladeRF gain: %s\n", bladerf_strerror(status));
        goto out;
    }


    status = bladerf_sync_config(sdr->handle,
                                 sdr->module,
                                 BLADERF_FORMAT_SC16_Q11,
                                 config->num_buffers,
                                 config->samples_per_buffer,
                                 config->num_transfers,
                                 config->stream_timeout_ms);

    if (status != 0) {
        log_error("Failed to set up bladeRF stream: %s\n",
                  bladerf_strerror(status));
        goto out;
    }

    status = bladerf_enable_module(sdr->handle, sdr->module, true);
    if (status != 0) {
        log_error("Failed to enable module: %s\n", bladerf_strerror(status));
    }

out:
    if (status != 0) {
        free(sdr->buf);
        free(sdr);
        sdr = NULL;
    }

    return sdr;
}

void sdr_bladerf_deinit(void *dev)
{
    struct sdr_bladerf *bladerf = (struct sdr_bladerf *) dev;

    if (bladerf) {
        bladerf_enable_module(bladerf->handle, bladerf->module, false);
        bladerf_close(bladerf->handle);
        free(bladerf->buf);
        free(bladerf);
    }
}

int sdr_bladerf_rx(void *dev, struct complexf *samples, unsigned int count)
{
    int status = 0;
    unsigned int to_read, total_read;
    struct sdr_bladerf *sdr = (struct sdr_bladerf *) dev;

    total_read = 0;

    while (status == 0 && total_read < count) {
        to_read = uint_min(sdr->buf_len, count - total_read);

        status = bladerf_sync_rx(sdr->handle, sdr->buf, to_read,
                                 NULL, sdr->timeout_ms);

        if (status != 0) {
            log_error("RX failure: %s\n", bladerf_strerror(status));
            continue;
        }

        sc16q11_to_complexf(sdr->buf, samples, to_read);

        samples += to_read;
        total_read += to_read;
    }

    return status;
}

int sdr_bladerf_tx(void *dev, struct complexf *samples, unsigned int count)
{
    int status = 0;
    unsigned int to_write, total_written;
    struct sdr_bladerf *sdr = (struct sdr_bladerf *) dev;

    total_written = 0;

    while (status == 0 && total_written< count) {
        to_write = uint_min(sdr->buf_len, count - total_written);

        complexf_to_sc16q11(samples, sdr->buf, to_write);

        status = bladerf_sync_tx(sdr->handle, sdr->buf, to_write,
                                 NULL, sdr->timeout_ms);

        if (status != 0) {
            log_error("TX failure: %s\n", bladerf_strerror(status));
            continue;
        }

        samples += to_write;
        total_written += to_write;
    }

    return status;
}

int sdr_bladerf_flush(void *dev)
{
    struct sdr_bladerf *sdr = (struct sdr_bladerf *) dev;
    memset(sdr->buf, 0, 2 * sizeof(int16_t) * sdr->buf_len);

    return  bladerf_sync_tx(sdr->handle,
                            sdr->buf, sdr->buf_len,
                            NULL, sdr->timeout_ms);
}
