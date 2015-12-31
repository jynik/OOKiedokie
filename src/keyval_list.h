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

/**
 * Key-value pair of strings
 */
struct keyval
{
    const char *key;
    const char *value;
};

/**
 * Opaque handle to a key-value list
 */
struct keyval_list;

/**
 * Initialize and allocate a key-value list
 *
 * @return A handle on success, or NULL on failure
 */
struct keyval_list * keyval_list_init();

/**
 * Deinitalize the provided key-value list. Ally entries in the
 * list are deallocated.
 */
void keyval_list_deinit(struct keyval_list *list);

/* keyval contents are copied. Safe to change after this call
 * Returns true on success */

/**
 * Append the provided key-value pair to the list. The entries in this
 * pair are copied when inserting them into the keyval_list.; it is safe for
 * them to change after this call.
 *
 * @param   list        Key-value list to insert into
 * @param   kv          Key-value pair to insert
 *
 * @return true on success, false otherwise
 */
bool keyval_list_append(struct keyval_list *list, const struct keyval *kv);

/**
 * Get the current list size
 *
 * @param   list    List to check
 *
 * @return list size, in elements. 0 is returned if list is NULL.
 */
size_t keyval_list_size(const struct keyval_list *list);

/* Returned value only valid for the lifetime of the keyval_list */

/**
 * Get a reference to a key-value. This reference is only valid for the
 * life-time of the list, or until the list is cleared via keyval_list_clear().
 *
 * @param   list    List to access
 * @param   idx     Index of the desired key-value entry in the list
 *
 * @return key-value reference, or NULL if idx is out of bounds.
 */
const struct keyval * keyval_list_at(const struct keyval_list *list, size_t idx);

/**
 * Clear and deallocate all entries in the provided list. Ensure you do not
 * attempt to access elements previously accessed with keyval_list_at() after
 * calling this function.
 *
 * @param   list    List to clear
 *
 */
void keyval_list_clear(struct keyval_list *list);

#endif
