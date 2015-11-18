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

#ifndef OOKIEDOKIE_SDR_H_
#define OOKIEDOKIE_SDR_H_

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "ookiedokie_cfg.h"
#include "complexf.h"

/**
 * Status code returned from raw sample files when EOF is hit
 */
#define SDR_FILE_EOF    INT_MIN

/**
 * Opaque SDR handle
 */
struct sdr;

/**
 * Open the specified SDR device
 *
 * @param[in]   config      Device configuration parameters
 * @param[in]   file_only   Only initialize if the specified configuration
 *                          is for a file-based "device" implementation
 *
 * @return  SDR handle on success, NULL on failure
 */
struct sdr * sdr_init(const struct ookiedokie_cfg *config, bool file_only);

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
int sdr_rx(struct sdr *dev, struct complexf *samples, unsigned int count);

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
int sdr_tx(struct sdr *dev, const struct complexf *samples, unsigned int count);

/**
 * Flush the number of required zero samples (0 + 0j) through the system
 * to ensure samples provided to sdr_tx() exit the RFFE.
 *
 * @param[in]   dev         SDR handle
 *
 * @return 0 on success, non-zero on failure.
 *         Non-zero error codes will propagate from the underlying SDR APIs.
 */
int sdr_flush_tx(struct sdr *dev);

/**
 * Get the default OOKiedokie filter file for this SDR device/file format
 *
 * @param[in]   dev         SDR handle
 *
 * @return  Filter file name
 */
const char * sdr_default_filter(const struct sdr *dev);

/**
 * Get the default file handler implemenation associated with this device
 *
 * @param[in]   dev         SDR handle
 *
 * @return Default SDR file handler implementation
 */
const char * sdr_default_file_handler(const struct sdr *dev);

/**
 * Check if SDR device is a "file handler" rather than an implementation
 * that interacts with hardware.
 *
 * @return true if the specified device is a file handler implemenation,
 *         false otherwise
 */
bool sdr_is_filehandler(const struct sdr *dev);

/**
 * Close and deinitialize a device.
 *
 * @param[in]   dev     SDR handle
 */
void sdr_deinit(struct sdr *dev);

#endif
