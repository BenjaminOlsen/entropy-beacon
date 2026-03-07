#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/entropy/entropy.h"

static void test_delta_sign_basic(void)
{
    int16_t samples[] = {100, 105, 103, 110, 110};
    uint8_t bits[4];
    size_t n = entropy_delta_sign(samples, 5, bits, 4);

    assert(n == 4);
    assert(bits[0] == 1);  /* 105 > 100 */
    assert(bits[1] == 0);  /* 103 < 105 */
    assert(bits[2] == 1);  /* 110 > 103 */
    assert(bits[3] == 0);  /* 110 == 110 */
    printf("  PASS: test_delta_sign_basic\n");
}

static void test_delta_sign_too_few_samples(void)
{
    int16_t samples[] = {42};
    uint8_t bits[1];
    size_t n = entropy_delta_sign(samples, 1, bits, 1);

    assert(n == 0);
    printf("  PASS: test_delta_sign_too_few_samples\n");
}

static void test_delta_sign_output_cap(void)
{
    int16_t samples[] = {1, 2, 3, 4, 5};
    uint8_t bits[2];
    size_t n = entropy_delta_sign(samples, 5, bits, 2);

    assert(n == 2);
    assert(bits[0] == 1);
    assert(bits[1] == 1);
    printf("  PASS: test_delta_sign_output_cap\n");
}

/* --- MCV estimator tests --- */

static void test_mcv_all_same(void)
{
    /* All bits are 0 — max probability is 1.0, min-entropy is 0 */
    uint8_t bits[100];
    memset(bits, 0, 100);
    uint32_t h = entropy_mcv_estimate(bits, 100);
    assert(h == 0);
    printf("  PASS: test_mcv_all_same\n");
}

static void test_mcv_perfect_balance(void)
{
    /* Exactly 50/50 — min-entropy should be 1.0 bit = 1000 millebits */
    uint8_t bits[100];
    for (size_t i = 0; i < 100; i++)
        bits[i] = i % 2;
    uint32_t h = entropy_mcv_estimate(bits, 100);
    assert(h == 1000);
    printf("  PASS: test_mcv_perfect_balance\n");
}

static void test_mcv_biased(void)
{
    /* 80 zeros, 20 ones — p_max = 0.8, -log2(0.8) ≈ 0.322 = 322 millebits */
    uint8_t bits[100];
    memset(bits, 0, 100);
    for (size_t i = 0; i < 20; i++)
        bits[i] = 1;
    uint32_t h = entropy_mcv_estimate(bits, 100);
    /* Allow some rounding: 300-340 */
    assert(h >= 300 && h <= 340);
    printf("  PASS: test_mcv_biased (h=%u)\n", h);
}

static void test_mcv_empty(void)
{
    uint32_t h = entropy_mcv_estimate(NULL, 0);
    assert(h == 0);
    printf("  PASS: test_mcv_empty\n");
}

/* --- RCT tests --- */

static void test_rct_healthy(void)
{
    entropy_rct_t rct;
    entropy_rct_init(&rct, 5);  /* fail after 5 consecutive same values */

    uint8_t bits[] = {0, 1, 0, 1, 0, 1, 0, 1};
    for (size_t i = 0; i < 8; i++)
        assert(entropy_rct_update(&rct, bits[i]) == 0);
    printf("  PASS: test_rct_healthy\n");
}

static void test_rct_stuck(void)
{
    entropy_rct_t rct;
    entropy_rct_init(&rct, 5);

    int failed = 0;
    for (int i = 0; i < 10; i++) {
        if (entropy_rct_update(&rct, 0))
            failed = 1;
    }
    assert(failed == 1);
    printf("  PASS: test_rct_stuck\n");
}

/* --- APT tests --- */

static void test_apt_healthy(void)
{
    entropy_apt_t apt;
    entropy_apt_init(&apt, 64, 48);  /* window=64, fail if >48 same as ref */

    for (int i = 0; i < 128; i++)
        assert(entropy_apt_update(&apt, i % 2) == 0);
    printf("  PASS: test_apt_healthy\n");
}

static void test_apt_biased(void)
{
    entropy_apt_t apt;
    entropy_apt_init(&apt, 64, 48);

    int failed = 0;
    for (int i = 0; i < 64; i++) {
        if (entropy_apt_update(&apt, 0))
            failed = 1;
    }
    assert(failed == 1);
    printf("  PASS: test_apt_biased\n");
}

int main(void)
{
    printf("test_entropy:\n");
    test_delta_sign_basic();
    test_delta_sign_too_few_samples();
    test_delta_sign_output_cap();
    test_mcv_all_same();
    test_mcv_perfect_balance();
    test_mcv_biased();
    test_mcv_empty();
    test_rct_healthy();
    test_rct_stuck();
    test_apt_healthy();
    test_apt_biased();
    printf("all passed.\n");
    return 0;
}
