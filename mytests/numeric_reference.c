/* Host-only numerical analysis against floating point and native int64 math. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "src/bf16/bf16.h"
#include "src/fixed/fixed.h"

static double value(uint16_t x)
{
    return ldexp((double) (128u + (x & 127u)), (int) ((x >> 7) & 255u) - 134);
}

int main(void)
{
    double sqrt_error = 0, product_error = 0;
    unsigned worst_sqrt = 0, worst_a = 0, worst_b = 0;
    for (unsigned x = 0x80; x < 0x7f80; x++) {
        double reference = sqrt(value((uint16_t) x));
        double error = fabs(value(bf16_sqrt((uint16_t) x)) / reference - 1);
        if (error > sqrt_error) {
            sqrt_error = error;
            worst_sqrt = x;
        }
    }
    for (unsigned a = 0x3f80; a <= 0x3fff; a++)
        for (unsigned b = 0x3f80; b <= 0x3fff; b++) {
            double reference = value((uint16_t) a) * value((uint16_t) b);
            double error = fabs(
                value(bf16_mul((uint16_t) a, (uint16_t) b)) / reference - 1);
            if (error > product_error) {
                product_error = error;
                worst_a = a;
                worst_b = b;
            }
        }
    uint32_t random = 0x12345678u;
    for (unsigned i = 0; i < 100000; i++) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        int32_t a =
            random <= INT32_MAX ? (int32_t) random : -1 - (int32_t) ~random;
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        int32_t b =
            random <= INT32_MAX ? (int32_t) random : -1 - (int32_t) ~random;
        int64_t scaled = (int64_t) a * b + (INT64_C(1) << 19);
        int64_t reference =
            scaled >= 0
                ? scaled / (INT64_C(1) << 20)
                : -((-scaled + (INT64_C(1) << 20) - 1) / (INT64_C(1) << 20));
        if ((uint32_t) q_mul(a, b) != (uint32_t) reference)
            return 1;
    }
    printf(
        "sqrt: all 32512 positive normals; max relative error %.9f%% "
        "at 0x%04x\n",
        100 * sqrt_error, worst_sqrt);
    printf(
        "multiply: all 16384 normal mantissa pairs; max relative error "
        "%.9f%% at 0x%04x,0x%04x\n",
        100 * product_error, worst_a, worst_b);
    puts(
        "fixed product: 100000 full-range signed pairs match native int64 "
        "math");
    return sqrt_error < 0.004 && product_error < 1.0 / 128 ? 0 : 2;
}
