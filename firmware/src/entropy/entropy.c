#include "entropy.h"

size_t entropy_delta_sign(const int16_t *samples, size_t n_samples,
                          uint8_t *out_bits, size_t out_cap)
{
    if (n_samples < 2)
        return 0;

    size_t n_bits = n_samples - 1;
    if (n_bits > out_cap)
        n_bits = out_cap;

    for (size_t i = 0; i < n_bits; i++)
        out_bits[i] = (samples[i + 1] > samples[i]) ? 1 : 0;

    return n_bits;
}
