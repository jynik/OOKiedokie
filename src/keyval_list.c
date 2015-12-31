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
#include <assert.h>

#include "keyval_list.h"

struct keyval_list {
    size_t size;                /* # populated elements */
    size_t backing_size;        /* # of allocated elements */
    struct keyval *elt;
};

void keyval_list_deinit(struct keyval_list *list)
{
    if (!list) {
        return;
    }

    keyval_list_clear(list);
    free(list->elt);
    free(list);
}

struct keyval_list * keyval_list_init()
{
    struct keyval_list *list;

    list = malloc(sizeof(*list));
    if (!list) {
        return NULL;
    }

    list->size = 0;
    list->backing_size = 16;

    list->elt = calloc(list->backing_size, sizeof(list->elt[0]));
    if (!list->elt) {
        free(list);
        return NULL;
    }

    return list;
}

bool keyval_list_append(struct keyval_list *list, const struct keyval *kv)
{
    struct keyval *curr;

    if (!list) {
        return false;
    }

    assert(list->size <= list->backing_size);

    if (list->size == list->backing_size) {
        const size_t new_size = 2 * list->backing_size * sizeof(list->elt[0]);
        void *tmp = realloc(list->elt, new_size);

        if (!tmp) {
            return false;
        } else {
            list->elt = tmp;
            list->backing_size = new_size;
        }
    }

    curr = &list->elt[list->size];

    curr->key = (const char *) strdup(kv->key);
    if (!curr->key) {
        return false;
    }

    curr->value = (const char *) strdup(kv->value);
    if (!curr->value) {
        free((void *) curr->key);
        curr->key = NULL;
        return false;
    }

    list->size++;
    return true;
}

size_t keyval_list_size(const struct keyval_list *list) {
    if (list != NULL) {
        return list->size;
    } else {
        return 0;
    }
}

const struct keyval * keyval_list_at(const struct keyval_list *list, size_t idx)
{
    if (!list || idx >= list->size) {
        return NULL;
    } else {
        return &list->elt[idx];
    }
}

void keyval_list_clear(struct keyval_list *list)
{
    size_t i;

    if (!list || list->size == 0) {
        return;
    }

    for (i = 0; i < list->size; i++) {
        free((void *) list->elt[i].key);
        list->elt[i].key = NULL;

        free((void *) list->elt[i].value);
        list->elt[i].value = NULL;
    }

    list->size = 0;
}
