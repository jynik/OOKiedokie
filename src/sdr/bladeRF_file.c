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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "sdr.h"
#include "ookiedokie_cfg.h"
#include "complexf.h"
#include "minmax.h"
#include "log.h"

/* This is not generally useful, except when debugging */
#ifndef ENABLE_BLADERF_FILE_VERBOSE
#   undef log_verbose
#   define log_verbose(...)
#endif

struct sdr_bladerf_file {
    FILE *file;
    int16_t *buf;
    unsigned int buf_len;
};

void sdr_bladerf_file_deinit(void *dev)
{
    struct sdr_bladerf_file *sdr = (struct sdr_bladerf_file *) dev;

    if (dev) {
        if (sdr->file) {
            fclose(sdr->file);
        }

        free(sdr->buf);
        free(sdr);
    }
}

void * sdr_bladerf_file_init(const struct ookiedokie_cfg *config)
{
    int status = -1;
    const char *openmode = config->direction == DIRECTION_RX ? "rb" : "wb";
    struct sdr_bladerf_file *sdr = calloc(1, sizeof(sdr[0]));

    if (!sdr) {
        perror("malloc");
        return NULL;
    }

    sdr->buf_len = config->samples_per_buffer;

    sdr->buf = malloc(2 * sizeof(int16_t) * sdr->buf_len);
    if (!sdr->buf) {
        perror("malloc");
        goto out;
    }

    sdr->file = fopen(config->sdr_args, openmode);
    if (!sdr->file) {
        log_error("Failed to open %s: %s\n",
                   config->sdr_args, strerror(errno));
        goto out;
    }

    status = 0;

out:
    if (status != 0) {
        sdr_bladerf_file_deinit(sdr);
        sdr = NULL;
    }

    return sdr;
}

int sdr_bladerf_file_rx(void *dev, struct complexf *samples, unsigned int count)
{
    int status = 0;
    size_t n;
    unsigned int to_read, total_read;
    struct sdr_bladerf_file *sdr = (struct sdr_bladerf_file *) dev;

    total_read = 0;

    while (status == 0 && total_read < count) {
        to_read = uint_min(sdr->buf_len, count - total_read);
        log_verbose("Reading %u samples...\n", to_read);

        n = fread(sdr->buf, 2 * sizeof(int16_t), to_read, sdr->file);
        if (n == 0) {
            status = SDR_FILE_EOF;
        } else if (n < to_read) {
            /* Zero out the remaining samples. We're about to hit an EOF. */
            int16_t *to_zero = sdr->buf + (2 * n);
            memset(to_zero, 0, 2 * sizeof(int16_t) * (to_read - n));
        }

        sc16q11_to_complexf(sdr->buf, samples, to_read);

        samples += to_read;
        total_read += to_read;
    }

    return status;
}

int sdr_bladerf_file_tx(void *dev, struct complexf *samples, unsigned int count)
{
    int status = 0;
    size_t n;
    unsigned int to_write, total_written;
    struct sdr_bladerf_file *sdr = (struct sdr_bladerf_file *) dev;

    total_written = 0;

    while (status == 0 && total_written< count) {
        to_write = uint_min(sdr->buf_len, count - total_written);

        complexf_to_sc16q11(samples, sdr->buf, to_write);

        log_verbose("Writing'ing %u samples...\n", to_write);

        n = fwrite(sdr->buf, 2 * sizeof(int16_t), to_write, sdr->file);
        if (n != to_write) {
            log_debug("Sample file write was truncated.\n");
            status = -1;
        }

        samples += to_write;
        total_written += to_write;
    }

    return status;
}

int sdr_bladerf_file_flush(void *dev)
{
    /* No need to flush samples on file handler */
    return 0;
}
