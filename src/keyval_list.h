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
#ifndef KEYVAL_LIST_H_
#define KEYVAL_LIST_H_

#include <stdlib.h>
#include <stdbool.h>

struct keyval
{
    const char *key;
    const char *value;
};

struct keyval_list;

struct keyval_list * keyval_list_init();

void keyval_list_deinit(struct keyval_list *list);

/* keyval contents are copied. Safe to change after this call
 * Returns true on success */
bool keyval_list_append(struct keyval_list *list, const struct keyval *kv);

size_t keyval_list_size(const struct keyval_list *list);

/* Returned value only valid for the lifetime of the keyval_list */
const struct keyval * keyval_list_at(const struct keyval_list *list, size_t idx);

void keyval_list_clear(struct keyval_list *list);

#endif
