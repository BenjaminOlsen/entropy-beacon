#include <assert.h>
#include <stdio.h>
#include <string.h>
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

int main(void)
{
    printf("test_entropy:\n");
    test_delta_sign_basic();
    test_delta_sign_too_few_samples();
    test_delta_sign_output_cap();
    printf("all passed.\n");
    return 0;
}
