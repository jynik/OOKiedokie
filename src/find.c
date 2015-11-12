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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "log.h"

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#ifndef OOKIEDOKIE_DATA_DIR
#   error  "OOKIEDOKIE_DATA_DIR is not defined"
#endif

enum pfx_type {
    PFX_NONE = 0,
    PFX_HOME_DIR,
    PFX_DATA_DIR,
};

struct search_path {
    enum pfx_type pfx;
    const char *path;
};

static const struct search_path search_paths[] = {
    /* Current working directory */
    { PFX_NONE,         "" },

    /* User directories */
    { PFX_HOME_DIR,     ".config/OOKiedokie/" },
    { PFX_HOME_DIR,     ".OOKiedokie/" },

    /* Install path items (e.g., /usr/local/share/OOKiedokie) */
    { PFX_DATA_DIR,     "" },
};

struct str {
    char    *buf;
    size_t  len;
    size_t  max_len;
};

#define STR_LEN_INC 64

static bool str_init(struct str *s)
{
    s->len      = 0;
    s->max_len  = STR_LEN_INC;
    s->buf      = calloc(1, s->max_len);
    return (s->buf != NULL);
}

static bool str_append(struct str *s, const char *to_append)
{
    size_t appended_len = s->len + strlen(to_append);

    if (appended_len >= s->max_len) {
        const size_t new_max = appended_len + STR_LEN_INC;
        void *tmp = realloc(s->buf, new_max);

        if (!tmp) {
            perror("realloc");
            return false;
        } else {
            s->buf      = (char *) tmp;
            s->max_len  = new_max;

            log_verbose("Realloc'd string buffer to %zd bytes\n", s->max_len);
        }
    }

    s->len += strlen(to_append);
    assert(s->len < s->max_len);
    strncat(s->buf, to_append, s->max_len - 1);
    return true;
}

static void str_reset(struct str *s)
{
    memset(s->buf, 0, s->max_len);
    s->len = 0;
}

static void str_deinit(struct str *s)
{
    if (s) {
        free(s->buf);
    }
}

FILE * find_file(const char *open_mode,
                 const char *prefix, const char *name, const char *extension)
{
    struct str path;
    FILE *f = NULL;
    size_t i;

    if (!str_init(&path)) {
        log_error("Failed to initialize path string.\n");
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(search_paths) && f == NULL; i++) {
        str_reset(&path);

        switch (search_paths[i].pfx) {
            case PFX_NONE:
                break;

            case PFX_HOME_DIR:
                if (!str_append(&path, getenv("HOME")) ||
                    !str_append(&path, "/")) {

                    log_error("Failed to prepend $HOME to search path.\n");
                    continue;
                }
                break;

            case PFX_DATA_DIR:
                if (!str_append(&path, OOKIEDOKIE_DATA_DIR)) {
                    log_error("Failed to append \"%s\" to search path.\n",
                              OOKIEDOKIE_DATA_DIR);
                    continue;
                }
                break;
        }

        if (!str_append(&path, search_paths[i].path)) {
            log_error("Failed to append search path entry.\n");
            continue;
        }

        if (prefix && !str_append(&path, prefix)) {
            log_error("Failed to append file name prefix to path.\n");
            continue;
        }

        if (!str_append(&path, name)) {
            log_error("Failed to append file name to path.\n");
            continue;
        }

        if (extension && !str_append(&path, extension)) {
            log_error("Failed to append extension to path.\n");
            continue;
        }

        log_debug("Searching for: %s\n", path.buf);
        f = fopen(path.buf, open_mode);
    }

out:
    str_deinit(&path);
    return f;
}

FILE * find_device_file(const char *device)
{
    FILE *ret;

    /* Full path to file with extension provided */
    ret = find_file("r", NULL, device, NULL);
    if (ret) {
        return ret;
    }

    /* Same, but extension not provided. (This makes sense in the case
     * where the the file is in the current working directory). */
    ret = find_file("r", NULL, device, ".json");
    if (ret) {
        return ret;
    }

    /* Search by name within search paths */
    if (!ret) {
        ret = find_file("r", "devices/", device, ".json");
    }

    return ret;
}

FILE *find_filter_file(const char *name)
{
    FILE *ret;

    /* Full path to file with extension provided */
    ret = find_file("r", NULL, name, NULL);
    if (ret) {
        return ret;
    }

    /* Same, but extension not provided. (This makes sense in the case
     * where the the file is in the current working directory). */
    ret = find_file("r", NULL, name, ".json");
    if (!ret) {
        ret = find_file("r", "filters/", name, ".json");
    }

    /* Search by name within search paths */
    if (!ret) {
        ret = find_file("r", "filters/", name, ".json");
    }

    return ret;
}
