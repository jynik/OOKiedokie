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
 * THE SOFTWARE.  */
/* This file is intended to be included once, in sdr.c */

#ifndef OOKIEDOKIE_SDR_SUPPORTED_DEVICES_H_
#define OOKIEDOKIE_SDR_SUPPORTED_DEVICES_H_

#include "ookiedokie_cfg.h"
#include "complexf.h"
#include "sdr/sdr.h"

#define SDR_FILE_HANDLER true
#define SDR_HARDWARE     false

#define NO_FILTER NULL

#define SDR_PROTOTYPES(name) \
    void * sdr_##name##_init(const struct ookiedokie_cfg *); \
    void sdr_##name##_deinit(void *); \
    int sdr_##name##_rx(void *, struct complexf *, unsigned int); \
    int sdr_##name##_tx(void *, const struct complexf *, unsigned int); \
    int sdr_##name##_flush(void *) \

#define SDR_INTERFACE(name_, is_file_handler_, filter_) { \
    .name               = #name_, \
    .is_file_handler    = is_file_handler_, \
    .default_filter     = filter_, \
    .init               = sdr_##name_##_init, \
    .deinit             = sdr_##name_##_deinit, \
    .rx                 = sdr_##name_##_rx, \
    .tx                 = sdr_##name_##_tx, \
    .flush              = sdr_##name_##_flush, \
}

#define NO_DEVICES_ENABLED 1

#if ENABLE_BLADERF
#   undef NO_DEVICES_ENABLED
#   define SDR_BLADERF \
        SDR_INTERFACE(bladerf, SDR_HARDWARE, "fs128_fs16_dec4"),

    SDR_PROTOTYPES(bladerf);
#else
#   define SDR_BLADERF
#endif

#if ENABLE_BLADERF_SC16Q11_FILE
#   undef NO_DEVICES_ENABLED
#   define SDR_BLADERF_SC16Q11_FILE \
        SDR_INTERFACE(bladerf_file, SDR_FILE_HANDLER, "fs128_fs16_dec4"),

    SDR_PROTOTYPES(bladerf_file);
#else
    #define SDR_BLADERF_SC16Q11_FILE
#endif

#ifdef NO_DEVICES_ENABLED
#   error "No supported devices or file formats are enabled."
#endif

#define SDR_SUPPORTED_DEVICES { \
    SDR_BLADERF \
    SDR_BLADERF_SC16Q11_FILE \
}

#endif
