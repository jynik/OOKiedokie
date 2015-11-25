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
#include <stdbool.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>

#include "fir.h"
#include "ookiedokie.h"
#include "complexf.h"
#include "keyval_list.h"

struct rx {
    struct complexf *samples;
    struct complexf *post_filter;

    struct {
        FILE *out;
        bool *samples;
        uint64_t sample_no;
        int8_t prev;
    } dig;
};

struct tx {
    struct complexf *samples;
};

static bool g_running = true;

static void ctrlc_handler(int signal, siginfo_t *info, void *unused)
{
    g_running = false;
}

static void init_signal_handling()
{
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_sigaction = ctrlc_handler;
    sigact.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
}

static void rx_deinit(struct rx *rx)
{
    if (rx) {
        if (rx->dig.out) {
            fclose(rx->dig.out);
        }

        free(rx->samples);
        free(rx->dig.samples);
        free(rx->post_filter);
        free(rx);
    }
}

static struct rx * rx_init(struct sdr *sdr,
                           struct fir_filter *filter,
                           struct device *device,
                           const struct ookiedokie_cfg *cfg)
{
    int status = -1;
    struct rx *rx;
    const unsigned int num_samples = cfg->samples_per_buffer;

    rx = calloc(1, sizeof(rx[0]));
    if (!rx) {
        perror("malloc");
        return NULL;
    }

    if (cfg->rx_rec_dig) {
        rx->dig.out = fopen(cfg->rx_rec_dig, "w");
        if (!rx->dig.out) {
            log_error("Failed to open %s: %s\n",
                      cfg->rx_rec_dig,
                      strerror(errno));
            goto out;
        }
    }

    rx->dig.sample_no = 0;
    rx->dig.prev = false;

    rx->samples = malloc(num_samples * sizeof(rx->samples[0]));
    if (!rx->samples) {
        perror("malloc");
        goto out;
    }

    rx->post_filter = malloc(num_samples * sizeof(rx->post_filter[0]));
    if (!rx->post_filter) {
        perror("malloc");
        goto out;
    }

    rx->dig.samples = malloc(num_samples * sizeof(rx->dig.samples[0]));
    if (!rx->dig.samples) {
        perror("malloc");
        goto out;
    }


    init_signal_handling();

    status = 0;

out:
    if (status != 0) {
        rx_deinit(rx);
        rx = NULL;
    }

    return rx;
}

static void record_dig(struct rx *rx, unsigned int count)
{
    unsigned int i;

    if (rx->dig.sample_no == 0) {
        rx->dig.prev = rx->dig.samples[0];
        fprintf(rx->dig.out, "0, %c\n", rx->dig.samples[0] ? '1' : '0');
    }

    for (i = 0; i < count; i++) {
        const bool curr = rx->dig.samples[i];
        const bool update = (curr != rx->dig.prev);

        if (update) {
            fprintf(rx->dig.out, "%"PRIu64", %c\n%"PRIu64", %c\n",
                    rx->dig.sample_no + i - 1, rx->dig.prev   ? '1' : '0',
                    rx->dig.sample_no + i, rx->dig.samples[i] ? '1' : '0');

            rx->dig.prev = curr;
        }
    }

    rx->dig.sample_no += count;
}

static inline void threshold(struct rx *rx, float threshold,
                             struct complexf *input, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        rx->dig.samples[i] = complexf_magnitude(&input[i]) >= threshold;
    }
}

int ookiedokie_rx(struct sdr *sdr, struct fir_filter *filter,
                  struct device *device, struct sdr *recorder,
                  const struct ookiedokie_cfg *cfg)
{
    int status = -1;
    struct rx *rx;
    const struct keyval_list *values;
    const unsigned int num_samples = cfg->samples_per_buffer;

    rx = rx_init(sdr, filter, device, cfg);
    if (!rx) {
        log_error("Failed to initialize RX state.\n");
        goto out;
    }

    while (g_running) {
        unsigned int i;
        size_t count;
        struct complexf *to_threshold;

        status = sdr_rx(sdr, rx->samples, num_samples);
        if (status != 0) {
            goto out;
        }

        if (recorder && cfg->rx_rec_input) {
            status = sdr_tx(recorder, rx->samples, num_samples);
            if (status != 0) {
                goto out;
            }
        }

        if (filter) {
            to_threshold = rx->post_filter;
            count = fir_filter_and_decimate(filter, rx->samples, num_samples,
                                            rx->post_filter);

        } else {
            to_threshold = rx->samples;
            count = num_samples;
        }

        if (recorder && !cfg->rx_rec_input) {
            status = sdr_tx(recorder, rx->post_filter, count);
            if (status != 0) {
                goto out;
            }
        }

        if (device || rx->dig.out) {
            threshold(rx, cfg->rx_threshold, to_threshold, count);
        }

        if (rx->dig.out) {
            record_dig(rx, count);
        }

        if (device) {
            values = device_process(device, rx->dig.samples, count);
            if (values && keyval_list_size(values) != 0) {
                size_t i;
                for (i = 0; i < keyval_list_size(values); i++) {
                    const struct keyval *kv = keyval_list_at(values, i);
                    printf("%20s: %s\n", kv->key, kv->value);
                }

                putchar('\n');
            }
        }

    }

out:
    if (status == SDR_FILE_EOF) {
        status = 0;
    }

    rx_deinit(rx);
    return status;
}

int ookiedokie_tx(struct sdr *sdr, struct device *device,
                  const struct ookiedokie_cfg *cfg)
{
    int i;
    bool success;
    int status;
    struct complexf *samples;
    struct complexf *zeros;
    unsigned int num_samples;

    const unsigned int delay_samples = (unsigned int) (
            (uint64_t) cfg->samplerate * cfg->tx_delay_us / 1000000
    );

    zeros = calloc(delay_samples, sizeof(zeros[0]));
    if (zeros == NULL) {
        perror("calloc");
        return -1;
    }

    success = device_generate(device, cfg->device_params,
                              &samples, &num_samples);
    if (!success) {
        return -1;
    }

    for (i = 0; i < cfg->tx_count; i++) {
        status = sdr_tx(sdr, zeros, delay_samples);
        if (status != 0) {
            goto out;
        }

        status = sdr_tx(sdr, samples, num_samples);
        if (status != 0) {
            goto out;
        }
    }

    status = sdr_flush_tx(sdr);

out:
    free(samples);
    return status;
}
