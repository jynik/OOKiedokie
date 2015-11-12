#ifndef OOKIEDOKIE_SC16Q11_H_
#define OOKIEDOKIE_SC16Q11_H_

#include "complexf.h"

static inline void sc16q11_to_complexf(const int16_t *in,
                                       struct complexf *out, unsigned int n)
{
    unsigned int i, j;

    for (i = j = 0; i < (2 * n); i += 2, j++) {
        out[j].real = (float) in[i]   * (1.0f / 2048.0f);
        out[j].imag = (float) in[i+1] * (1.0f / 2048.0f);
    }
}

#endif
