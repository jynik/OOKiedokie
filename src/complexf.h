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

#ifndef OOKIEDOKIE_COMPLEXF_H_
#define OOKIEDOKIE_COMPLEXF_H_

#include <math.h>

/**
 * Simple complex number representation
 */
struct complexf {
    float real; /** Real component */
    float imag; /** Imaginary component */
};

/**
 * Compute power of provided complex value
 *
 * @param x Complex value
 *
 * @return power
 */
static inline float complexf_power(const struct complexf *x)
{
    return x->real * x->real + x->imag * x->imag;
}

/**
 * Compute the magnitude of the provided complex value
 *
 * @param x Complex value
 *
 * @return magnitude
 */
static inline float complexf_magnitude(const struct complexf *x)
{
    return sqrtf(complexf_power(x));
}

/**
 * Convert an array of SC16Q11 values (bladeRF ADC/DAC format) to
 * complexf values.
 *
 * @param[in    in      Input  SC16Q11 values (interleaved IQ)
 * @param[out]  out     Output complex f values.
 * @param[in]   n       Length of both `in` and `out`
 */
static inline void sc16q11_to_complexf(const int16_t *in,
                                       struct complexf *out, unsigned int n)
{
    unsigned int i, j;

    for (i = j = 0; i < (2 * n); i += 2, j++) {
        out[j].real = (float) in[i]   * (1.0f / 2048.0f);
        out[j].imag = (float) in[i+1] * (1.0f / 2048.0f);
    }
}

/**
 * Convert an array of complexf values to SC16Q11
 * values (bladeRF ADC/DAC format)
 *
 * @param[in    in      Input SC16Q11 values (interleaved IQ)
 * @param[out]  out     Output complex f values.
 * @param[in]   n       Length of both `in` and `out`
 */
static inline void complexf_to_sc16q11(const struct complexf *in,
                                       int16_t *out, unsigned int n)
{
    unsigned int i, j;

    for (i = j = 0; i < n; i++, j += 2) {
        out[j]   = (int16_t) (in[i].real * 2048.0f);
        out[j+1] = (int16_t) (in[i].imag * 2048.0f);
    }
}


#endif
