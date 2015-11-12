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
#ifndef OOKIEDOKIE_FIND_H_
#define OOKIEDOKIE_FIND_H_

#include <stdio.h>

/**
 * Search for a file by name and open it if it is found
 *
 * @param[in]   open_mode   Required. fopen() mode string.
 * @param[in]   name_prefix Optional - may be NULL. Prefix to prepend to name.
 * @param[in]   name        Required. File name.
 * @param[in]   extension   Optional - may be NULL. File extension, including
 *                          dot.
 * @return file handle on success, NULL if not found (or upon failure)
 */
FILE * find_file(const char *open_mode, const char *name_prefix,
                 const char *name, const char *extension);

/**
 * Search for a device file
 *
 * @param[in]   device  Device file name, without the .json extension
 *
 * @return file handle on success, NULL if not found (or upon failure)
 */
FILE * find_device_file(const char *device);

/**
 * Search for a filter of the specified name
 *
 * @param[in]   name    Filter name, without the .filt extension
 *
 * @return file handle on success, NULL if not found (or upon failure)
 */
FILE * find_filter_file(const char *name);

#endif
