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

struct product_stats {
    unsigned tested, wrapped;
    int64_t max_residual;
    int32_t a, b, result;
};

static int check_product(int32_t a, int32_t b, struct product_stats *stats)
{
    const int64_t unit = INT64_C(1) << 20;
    int64_t product = (int64_t) a * b;
    int64_t biased = product + unit / 2;
    /* C division truncates; explicitly take floor for a negative value. */
    int64_t reference =
        biased >= 0 ? biased / unit : -((-biased + unit - 1) / unit);
    int32_t result = q_mul(a, b);
    ++stats->tested;
    if ((uint32_t) result != (uint32_t) reference)
        return 1;
    if (reference < INT32_MIN || reference > INT32_MAX) {
        ++stats->wrapped;
        return 0;
    }
    /* Real output - exact represented-input product = residual / 2^40.
     * Compute the residual before conversion to double, avoiding cancellation.
     * Both terms fit int64_t when the rounded result does not wrap.
     */
    int64_t residual = (int64_t) result * unit - product;
    if (residual < 0)
        residual = -residual;
    if (residual > unit / 2)
        return 2;
    if (residual > stats->max_residual) {
        stats->max_residual = residual;
        stats->a = a;
        stats->b = b;
        stats->result = result;
    }
    return 0;
}

static void print_product_stats(const char *name,
                                const struct product_stats *stats)
{
    printf(
        "fixed error %s: tested=%u nonwrapping=%u wrapped=%u "
        "max_residual=%lld/2^40 max_abs_error=%.15g "
        "at raw a=%d b=%d result=%d\n",
        name, stats->tested, stats->tested - stats->wrapped, stats->wrapped,
        (long long) stats->max_residual,
        ldexp((double) stats->max_residual, -40), (int) stats->a,
        (int) stats->b, (int) stats->result);
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
    struct product_stats full_range = {0}, directed = {0}, bounded = {0};
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
        if (check_product(a, b, &full_range))
            return 1;
    }
    /* Include both half-way signs and format edges, including wrapping cases.
     */
    const int32_t edges[] = {0,        1,         -1,       3,       -3,
                             524287,   524288,    524289,   -524288, 1048576,
                             -1048576, INT32_MAX, INT32_MIN};
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
        for (unsigned j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
            if (check_product(edges[i], edges[j], &directed))
                return 1;
    /* Independent fixed seed; operands are represented values in [-32, 32).
     * Their products remain within the format, providing 100000 useful
     * quantization-error measurements rather than mostly wrapping products.
     */
    random = 0x5eedc0deu;
    for (unsigned i = 0; i < 100000; i++) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        int32_t a = (int32_t) (random & 0x3ffffffu) - 33554432;
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        int32_t b = (int32_t) (random & 0x3ffffffu) - 33554432;
        if (check_product(a, b, &bounded))
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
    print_product_stats("directed", &directed);
    print_product_stats("bounded", &bounded);
    print_product_stats("full-range", &full_range);
    printf(
        "fixed ties: raw (3,524288)->%d (-3,524288)->%d; "
        "absolute error %.15g for each\n",
        (int) q_mul(3, 524288), (int) q_mul(-3, 524288), ldexp(1.0, -21));
    if (directed.max_residual != (INT64_C(1) << 19) || bounded.wrapped)
        return 3;
    return sqrt_error < 0.004 && product_error < 1.0 / 128 ? 0 : 2;
}
