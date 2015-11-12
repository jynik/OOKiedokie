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
#include <limits.h>
#include <errno.h>
#include <jansson.h>

#include "conversions.h"
#include "fir.h"
#include "find.h"
#include "log.h"

#define BUF_LEN 4096

void usage(const char *argv0)
{
    printf("Pass samples through the specified filter.\n");
    printf("\n");
    printf("Usage: %s <filter file> <infile> <outfile> [count]\n", argv0);
    printf("\n");
    printf("<infile> and <outfile> shall be binary files of containing\n");
    printf("interleaved IQ samples, with I and Q being 32-bit floats.\n");
    printf("\n");
    printf("[count] is the number of samples to process during each filtering "
           "operation.\n");
    printf("\n");
}

struct complexf * load_input(const char *filename, size_t *input_len)
{
    int status = -1;
    FILE *in;
    struct complexf *ret = NULL;
    long len, i;
    size_t n_read;

    const size_t sample_len = (sizeof(ret[0].real) + sizeof(ret[0].imag));

    in = fopen(filename, "rb");
    if (!in) {
        log_error("Failed to open %s: %s\n", filename, strerror(errno));
        goto out;
    }

    status = fseek(in, 0, SEEK_END);
    if (status != 0) {
        log_error("Failed to seek to EOF: %s\n", strerror(errno));
        goto out;
    }

    len = ftell(in);
    if (len < 0) {
        log_error("Failed to determine file length: %s\n",
                  strerror(errno));
        goto out;
    } else if (len == 0) {
        log_error("Error: %s is an empty file.\n", filename);
        goto out;
    } else if ((len % sample_len) != 0) {
        log_error("Error: %s contains incomplete samples.\n", filename);
        goto out;
    }

    /* Convert length to samples */
    len /= sample_len;

    status = fseek(in, 0, SEEK_SET);
    if (status != 0) {
        log_error("Failed to rewind file: %s\n", strerror(errno));
        goto out;
    }

    ret = malloc(len * sizeof(ret[0]));
    if (!ret) {
        log_error("Failed to allocated input sample buffer: %s\n",
                  strerror(errno));
        goto out;
    }

    for (i = 0; i < len; i++) {
        n_read = fread(&ret[i].real, sizeof(ret[i].real), 1, in);
        if (n_read != 1) {
            log_error("Failed to read sample I[%ld]\n", i);
            goto out;
        }

        n_read = fread(&ret[i].imag, sizeof(ret[i].imag), 1, in);
        if (n_read != 1) {
            log_error("Failed to read sample Q[%ld]\n", i);
            goto out;
        }
    }

    status = 0;

out:
    if (in) {
        fclose(in);
    }

    if (status != 0) {
        free(ret);
        ret = NULL;
        *input_len = 0;
    } else {
        *input_len = len;
    }

    return ret;
}

void setup_log_level()
{
    bool ok;
    enum log_level level = LOG_LEVEL_WARNING;

    const char *env = getenv("LOG_LEVEL");
    if (env) {
        level = str2loglevel(env, &ok);
        if (!ok) {
            level = LOG_LEVEL_WARNING;
            log_error("Invalid LOG_LEVEL: %s. Defaulting to 'warning'\n");
        }
    }

    log_set_verbosity(level);
}

int main(int argc, char *argv[])
{
    int status = 0;
    unsigned int total_decimation = 1;

    FILE *outfile = NULL;
    struct fir_filter *filter = NULL;

    struct complexf *sig_in = NULL;
    size_t sig_in_len = 0;

    struct complexf *sig_out = NULL;
    size_t sig_out_len = 0;

    size_t i = 0;
    size_t to_proc = 0;
    size_t n_out= 0;

    /* How many input samples we process per filtering operation */
    unsigned int chunk_size = 32;

    switch (argc) {
        case 4:
            break;

        case 5: {
            bool conv_ok;

            chunk_size = str2uint(argv[4], 1, UINT_MAX, &conv_ok);
            if (!conv_ok) {
                log_error("Invalid sample count: %s\n", argv[4]);
                return EXIT_FAILURE;
            }
            break;
        }

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
    }

    setup_log_level();

    filter = fir_init(argv[1], chunk_size);
    if (!filter) {
        log_error("Failed to load filter.\n");
        status = EXIT_FAILURE;
        goto out;
    }

    total_decimation = fir_get_total_decimation(filter);
    if (total_decimation == 0) {
        log_error("Bug: Filter contains invalid decimation value!\n");
        status = EXIT_FAILURE;
        goto out;
    }

    log_info("Loaded filter from %s. Total decimation = %u.\n",
             argv[1], total_decimation);

    sig_in = load_input(argv[2], &sig_in_len);
    if (!sig_in) {
        log_error("Failed to load input samples.\n");
        status = EXIT_FAILURE;
        goto out;
    }

    log_info("Loaded %zd input samples from %s\n", sig_in_len, argv[2]);

    outfile = fopen(argv[3], "wb");
    if (!outfile) {
        log_error("Failed to open %s: %s\n", argv[3], strerror(errno));
        status = EXIT_FAILURE;
        goto out;
    }

    sig_out_len = (chunk_size + (total_decimation - 1)) / total_decimation;
    log_info("Output buffer is %zd samples.\n", sig_out_len);
    if (sig_out_len == 0) {
        log_error("Bug: Invalid output buffer length calculated!\n");
        goto out;
    }

    sig_out = malloc(sig_out_len * sizeof(sig_out[0]));
    if (!sig_out) {
        log_error("Failed to allocate output buffer: %s\n", strerror(errno));
        status = EXIT_FAILURE;
        goto out;
    }

    log_verbose("Top-level output buffer @ %p\n", sig_out);

    log_info("Processing input at %u samples per call.\n", chunk_size);

    for (i = 0; i < sig_in_len; i += to_proc) {
        if ((sig_in_len - i) < chunk_size) {
            to_proc = sig_in_len - i;
        } else {
            to_proc = chunk_size;
        }

        n_out = fir_filter_and_decimate(filter, &sig_in[i], to_proc, sig_out);

        if (n_out != 0) {
            size_t j, w;

            for (j = 0; j < n_out; j++) {
                w = fwrite(&sig_out[j].real, sizeof(sig_out[j].real), 1, outfile);
                if (w != 1) {
                    log_error("Failed to write I[%zd]\n", i + j);
                    status = EXIT_FAILURE;
                    goto out;
                }

                w = fwrite(&sig_out[j].imag, sizeof(sig_out[j].imag), 1, outfile);
                if (w != 1) {
                    log_error("Failed to write Q[%zd]\n", i + j);
                    status = EXIT_FAILURE;
                    goto out;
                }
            }

        }
    }

out:
    free(sig_out);

    if (outfile) {
        fclose(outfile);
    }

    free(sig_in);
    fir_deinit(filter);

    return status;
}
