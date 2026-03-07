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

/*
 * Most Common Value (MCV) min-entropy estimator.
 * Operates on a buffer of bits (each byte is 0 or 1).
 * Returns min-entropy estimate in millebits (1/1000 of a bit) per sample
 * to avoid floating point. 1000 = 1.0 bit, 500 = 0.5 bits, etc.
 * Returns 0 if n_bits == 0.
 */
uint32_t entropy_mcv_estimate(const uint8_t *bits, size_t n_bits);

/*
 * Repetition Count Test (RCT) per NIST SP 800-90B.
 * Detects a source stuck on one value.
 * Call once per new bit. Returns 0 if healthy, 1 if failed.
 */
typedef struct {
    uint8_t last_value;
    uint16_t count;
    uint16_t cutoff;  /* fail threshold */
} entropy_rct_t;

void entropy_rct_init(entropy_rct_t *rct, uint16_t cutoff);
int entropy_rct_update(entropy_rct_t *rct, uint8_t bit);

/*
 * Adaptive Proportion Test (APT) per NIST SP 800-90B.
 * Detects bias in a sliding window.
 * Call once per new bit. Returns 0 if healthy, 1 if failed.
 */
typedef struct {
    uint8_t ref_value;
    uint16_t count;
    uint16_t window_idx;
    uint16_t window_size;
    uint16_t cutoff;  /* fail threshold */
} entropy_apt_t;

void entropy_apt_init(entropy_apt_t *apt, uint16_t window_size, uint16_t cutoff);
int entropy_apt_update(entropy_apt_t *apt, uint8_t bit);

#endif
