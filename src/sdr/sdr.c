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
#include <string.h>
#include <stdbool.h>

#include "sdr/sdr.h"
#include "sdr/supported_devices.h"

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define SDR_NOT_FOUND   (-1)
#define SDR_NO_FILE     (-2)
#define SDR_ERROR       (-3)


/*
 * Minimalistic SDR abstraction.
 *
 * This doesn't seek to use any neat features from any particular device.
 * We just want to RX/TX samples.
 *
 * If you want a real device abstraction layer, check out:
 *
 * gr-osmosdr: http://sdr.osmocom.org/trac/wiki/GrOsmoSDR
 * SoapySDR:   https://github.com/pothosware/SoapySDR/wiki
 *
 */
struct sdr_interface
{
    /** SDR name */
    const char *name;

    /** Denotes this reads/writes sample files and is not actual hardware */
    const bool is_file_handler;

    /**
     * Default filter file.
     * Should be one of the filter shipped with OOKiedokie or NULL for "none".
     */
    const char *default_filter;

    /**
     * Initialize a device
     *
     * @param[in]   cfg         Device configuration parameters
     *
     * @return device handle on success, NULL on failure
     */
    void * (*init)(const struct ookiedokie_cfg *cfg);

    /**
     * Deinitialize device.
     *
     * Do not call this directly. Use sdr_deinit().
     *
     * @param[in]   dev         Handle to deinitialize
     */
    void (*deinit)(void *handle);

    /**
     * Receive the specified number of samples
     *
     * @param[in]   dev         SDR handle
     * @param[out]  samples     Buffer to store samples in
     * @param[in]   count       Number of samples to receive
     *
     * @return 0 on success, non-zero on failure.
     *         Non-zero error codes will propagate from the underlying SDR APIs.
     */
    int (*rx)(void *handle, struct complexf *samples, unsigned int count);

    /**
     * Transmit the specified number of samples
     *
     * @param[in]   dev         SDR handle
     * @param[in]   samples     Buffer containing samples to transmit
     * @param[in]   count       Number of samples to transmit
     *
     * @return 0 on success, non-zero on failure.
     *         Non-zero error codes will propagate from the underlying SDR APIs.
     */
    int (*tx)(void *handle, const struct complexf *samples, unsigned int count);

    /**
     * Flush the number of required zero samples (0 + 0j) through the system
     * to ensure samples provided to sdr_tx() exit the RFFE.
     *
     * @param[in]   dev
     *
     * @return 0 on success, non-zero on failure.
     *         Non-zero error codes will propagate from the underlying SDR APIs.
     */
    int (*flush)(void *handle);
};

struct sdr {
    void *handle;
    const struct sdr_interface *iface;
};

static const struct sdr_interface sdrs[] = SDR_SUPPORTED_DEVICES;

struct sdr * sdr_init(const struct ookiedokie_cfg *config, bool file_only)
{
    size_t i;
    int status = -1;
    struct sdr *ret = NULL;

    ret = calloc(1, sizeof(ret[0]));
    if (!ret) {
        perror("malloc");
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(sdrs); i++) {
        if (file_only && !sdrs[i].is_file_handler) {
            continue;
        }

        if (!strcasecmp(config->sdr_type, sdrs[i].name)) {
            log_verbose("Specified device type: %s\n", config->sdr_type);
            ret->iface = &sdrs[i];

            if (ret->iface->is_file_handler && config->sdr_args == NULL) {
                log_error("A filename must be provided as \"SDR args\" when using %s.\n",
                          config->sdr_type);
                free(ret);
                return NULL; }

            ret->handle = ret->iface->init(config);
            break;
        }
    }

    if (i >= ARRAY_SIZE(sdrs)) {
        log_error("Invalid device type specified: %s\n", config->sdr_type);
        free(ret);
        ret = NULL;
    } else if (ret->handle == NULL) {
        log_debug("Failed to initialize device.\n");
        free(ret);
        ret = NULL;
    }

    return ret;
}

void sdr_deinit(struct sdr *dev)
{
    if (dev != NULL) {
        dev->iface->deinit(dev->handle);
        free(dev);
    }
}

int sdr_rx(struct sdr *dev, struct complexf *samples, unsigned int count)
{
    return dev->iface->rx(dev->handle, samples, count);
}

int sdr_tx(struct sdr *dev, const struct complexf *samples, unsigned int count)
{
    return dev->iface->tx(dev->handle, samples, count);
}

int sdr_flush_tx(struct sdr *dev)
{
    return dev->iface->flush(dev->handle);
}

const char * sdr_default_filter(struct sdr *dev)
{
    return dev->iface->default_filter;
}

