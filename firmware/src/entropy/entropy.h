#ifndef ENTROPY_H
#define ENTROPY_H

#include <stdint.h>
#include <stddef.h>

/* Security target in millebits (256 bits = 256000) */
#define ENTROPY_TARGET_MILLIBITS  256000

/*
 * Entropy pool — accumulates raw bits and tracks estimated entropy.
 * When accumulated entropy exceeds the target (256 bits), conditioning
 * with SHA-256 produces a 32-byte output.
 */
#define ENTROPY_POOL_CAP  512  /* max raw bits to buffer */

typedef struct {
    uint8_t  data[ENTROPY_POOL_CAP / 8];  /* packed bits */
    size_t   n_bits;             /* number of raw bits in pool */
    uint32_t accum_millibits;    /* accumulated entropy estimate */
} entropy_pool_t;

void entropy_pool_init(entropy_pool_t *pool);

/*
 * Feed raw bits into the pool along with their per-bit min-entropy
 * estimate (in millebits). Returns 1 if the pool has enough entropy
 * to produce conditioned output, 0 otherwise.
 */
int entropy_pool_feed(entropy_pool_t *pool, const uint8_t *bits,
                      size_t n_bits, uint32_t h_millebits_per_bit);

/*
 * Condition the pool contents with SHA-256 and write 32 bytes to out.
 * Resets the pool. Returns 0 on success, -1 if not enough entropy
 * has been accumulated.
 */
int entropy_pool_output(entropy_pool_t *pool, uint8_t out[32]);

/*
 * Delta-sign bit extraction.
 * Given a buffer of raw sensor samples, extract one bit per consecutive
 * pair: 1 if sample increased, 0 if it decreased or stayed the same.
 * Returns the number of bits written to out_bits.
 */
size_t entropy_delta_sign(const int16_t *samples, size_t n_samples,
                          uint8_t *out_bits, size_t out_cap);

/* Same as above but for unsigned 16-bit samples (e.g. lux readings). */
size_t entropy_delta_sign_u16(const uint16_t *samples, size_t n_samples,
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
