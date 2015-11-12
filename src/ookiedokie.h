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

#ifndef OOKIEDOKIE_H_
#define OOKIEDOKIE_H_

#include "device.h"
#include "sdr/sdr.h"
#include "fir.h"
#include "log.h"

/**
 * Receive samples and decode samples and decode and/or record them
 *
 * @param   sdr         SDR handle for receiving samples
 *
 * @param   filter      Decimating FIR filter to apply to samples. If NULL,
 *                      no filtering is performed.
 *
 * @param   device      Device handle used to decode samples. If NULL,
 *                      no decoding occurs.
 *
 * @param   recorder    File-backed "SDR" implementation used to
 *                      record samples. If NULL, no samples are recorded.
 *
 * @param   cfg         Configuration parameters
 *
 * @return 0 on success or non-zero on error.
 */
int ookiedokie_rx(struct sdr *sdr, struct fir_filter *filter,
                  struct device *device, struct sdr *recorder,
                  const struct ookiedokie_cfg *cfg);

/**
 * Transmit a message for the specified device type
 *
 * @param   sdr         SDR handle for transmitting samples
 * @param   device      Handle for target device information
 * @param   cfg         Configuration parameters
 *
 * @return 0 on success or non-zero on error.
 */
int ookiedokie_tx(struct sdr *sdr, struct device *device,
                  const struct ookiedokie_cfg *cfg);

#endif
