#include "entropy.h"
#include <math.h>
#include <string.h>
#include <cfx/sha256.h>

size_t entropy_delta_sign(const int16_t *samples, size_t n_samples,
                          uint8_t *out_bits, size_t out_cap)
{
    if (n_samples < 2) {
        return 0;
    }

    size_t n_bits = n_samples - 1;
    if (n_bits > out_cap) {
        n_bits = out_cap;
    }

    for (size_t i = 0; i < n_bits; i++) {
        out_bits[i] = (samples[i + 1] > samples[i]) ? 1 : 0;
    }

    return n_bits;
}

size_t entropy_delta_sign_u16(const uint16_t *samples, size_t n_samples,
                              uint8_t *out_bits, size_t out_cap)
{
    if (n_samples < 2) {
        return 0;
    }

    size_t n_bits = n_samples - 1;
    if (n_bits > out_cap) {
        n_bits = out_cap;
    }

    for (size_t i = 0; i < n_bits; i++) {
        out_bits[i] = (samples[i + 1] > samples[i]) ? 1 : 0;
    }

    return n_bits;
}

uint32_t entropy_mcv_estimate(const uint8_t *bits, size_t n_bits)
{
    if (n_bits == 0) {
        return 0;
    }

    /* Count ones; for binary data, most common value is max(ones, zeros) */
    size_t ones = 0;
    for (size_t i = 0; i < n_bits; i++) {
        ones += bits[i];
    }

    size_t zeros = n_bits - ones;
    size_t max_count = (ones > zeros) ? ones : zeros;

    if (max_count == n_bits) {
        return 0;  /* all same value, no entropy */
    }

    double p_max = (double)max_count / (double)n_bits;
    double h = -log2(p_max);

    return (uint32_t)(h * 1000.0 + 0.5);
}

void entropy_rct_init(entropy_rct_t *rct, uint16_t cutoff)
{
    rct->last_value = 0xFF;  /* invalid initial value */
    rct->count = 0;
    rct->cutoff = cutoff;
}

int entropy_rct_update(entropy_rct_t *rct, uint8_t bit)
{
    if (bit == rct->last_value) {
        rct->count++;
    } else {
        rct->last_value = bit;
        rct->count = 1;
    }

    return (rct->count >= rct->cutoff) ? 1 : 0;
}

void entropy_apt_init(entropy_apt_t *apt, uint16_t window_size, uint16_t cutoff)
{
    apt->ref_value = 0xFF;
    apt->count = 0;
    apt->window_idx = 0;
    apt->window_size = window_size;
    apt->cutoff = cutoff;
}

int entropy_apt_update(entropy_apt_t *apt, uint8_t bit)
{
    if (apt->window_idx == 0) {
        /* First sample in window sets the reference */
        apt->ref_value = bit;
        apt->count = 1;
        apt->window_idx = 1;
        return 0;
    }

    if (bit == apt->ref_value) {
        apt->count++;
    }

    apt->window_idx++;

    int failed = (apt->count >= apt->cutoff) ? 1 : 0;

    if (apt->window_idx >= apt->window_size) {
        apt->window_idx = 0;  /* reset window */
    }

    return failed;
}

void entropy_pool_init(entropy_pool_t *pool)
{
    pool->n_bits = 0;
    pool->accum_millibits = 0;
    memset(pool->data, 0, sizeof(pool->data));
}

int entropy_pool_feed(entropy_pool_t *pool, const uint8_t *bits,
                      size_t n_bits, uint32_t h_millebits_per_bit)
{
    for (size_t i = 0; i < n_bits; i++) {
        if (pool->n_bits < ENTROPY_POOL_CAP) {
            pool->data[pool->n_bits / 8] |= (bits[i] << (7 - (pool->n_bits % 8)));
            pool->n_bits++;
        }
    }
    pool->accum_millibits += (uint32_t)n_bits * h_millebits_per_bit;

    return (pool->accum_millibits >= ENTROPY_TARGET_MILLIBITS) ? 1 : 0;
}

int entropy_pool_output(entropy_pool_t *pool, uint8_t out[32])
{
    if (pool->accum_millibits < ENTROPY_TARGET_MILLIBITS) {
        return -1;
    }

    size_t packed_len = (pool->n_bits + 7) / 8;
    cfx_sha256(out, pool->data, packed_len);

    /* Reset pool */
    pool->n_bits = 0;
    pool->accum_millibits = 0;
    memset(pool->data, 0, sizeof(pool->data));

    return 0;
}
