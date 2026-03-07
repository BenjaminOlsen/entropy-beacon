#ifndef ENTROPY_H
#define ENTROPY_H

#include <stdint.h>
#include <stddef.h>

/*
 * Delta-sign bit extraction.
 * Given a buffer of raw sensor samples, extract one bit per consecutive
 * pair: 1 if sample increased, 0 if it decreased or stayed the same.
 * Returns the number of bits written to out_bits.
 */
size_t entropy_delta_sign(const int16_t *samples, size_t n_samples,
                          uint8_t *out_bits, size_t out_cap);

#endif
