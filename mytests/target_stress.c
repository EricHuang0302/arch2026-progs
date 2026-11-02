/* Independent arithmetic references and a second, full-range input workload. */
#include <stdint.h>
#include "src/bf16/bf16.h"
#include "src/fixed/fixed.h"
#include "src/gcd/gcd.h"

#ifdef __riscv
#include "bench.h"
#define REPORT bm_printf
#else
#include <stdio.h>
#define REPORT printf
#define bf16_from_f32_asm bf16_from_f32
#define gcd_u32_asm gcd_u32
#define q_mul_asm q_mul
#endif

static uint32_t seed = 0x6a09e667u;

static uint32_t next_word(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static int32_t signed_word(uint32_t value)
{
    return value <= INT32_MAX ? (int32_t) value : -1 - (int32_t) ~value;
}

/* Four 16x16 products, rather than the implementation's bit-serial loop. */
static uint32_t reference_product(uint32_t a, uint32_t b)
{
    uint32_t al = a & 65535u, bl = b & 65535u;
    uint32_t ah = a >> 16, bh = b >> 16;
    uint32_t ll = al * bl, lh = al * bh, hl = ah * bl, hh = ah * bh;
    uint64_t product = ll + ((uint64_t) lh << 16) + ((uint64_t) hl << 16) +
                       ((uint64_t) hh << 32);
    if (a >> 31)
        product -= (uint64_t) b << 32;
    if (b >> 31)
        product -= (uint64_t) a << 32;
    return (uint32_t) ((product + (1u << 19)) >> 20);
}

/* Long division gives Euclid's reference without a division instruction. */
static uint32_t remainder(uint32_t a, uint32_t b)
{
    uint32_t r = 0;
    for (unsigned i = 32; i; i--) {
        unsigned carry = r >> 31;
        r = (r << 1) | ((a >> (i - 1)) & 1u);
        if (carry || r >= b)
            r -= b;
    }
    return r;
}

static uint32_t reference_gcd(uint32_t a, uint32_t b)
{
    while (b) {
        uint32_t r = remainder(a, b);
        a = b;
        b = r;
    }
    return a;
}

static uint16_t reference_narrow(uint32_t a)
{
    unsigned high = a >> 16, low = a & 65535u;
    if (((a >> 23) & 255u) != 255u &&
        (low > 32768u || (low == 32768u && (high & 1u))))
        high++;
    return (uint16_t) high;
}

static int check(uint32_t a, uint32_t b, int check_gcd)
{
    int32_t x = signed_word(a), y = signed_word(b);
    uint32_t want = reference_product(a, b);
#ifndef __riscv
    int64_t scaled = (int64_t) x * y + (INT64_C(1) << 19);
    int64_t native =
        scaled >= 0
            ? scaled / (INT64_C(1) << 20)
            : -((-scaled + (INT64_C(1) << 20) - 1) / (INT64_C(1) << 20));
    if (want != (uint32_t) native)
        return 1;
#endif
    if ((uint32_t) q_mul(x, y) != want || (uint32_t) q_mul_asm(x, y) != want)
        return 2;
    if (bf16_from_f32(a) != reference_narrow(a) ||
        bf16_from_f32_asm(a) != reference_narrow(a))
        return 3;
    if (check_gcd) {
        uint32_t g = reference_gcd(a, b);
        if (gcd_u32(a, b) != g || gcd_u32_asm(a, b) != g)
            return 4;
    }
    return 0;
}

#ifdef __riscv
static uint32_t words[256], partners[256], indices[256];

static void measure(void)
{
    for (unsigned i = 0; i < 256; i++) {
        words[i] = next_word();
        partners[i] = next_word();
        indices[i] = i;
    }
    BENCH("stress", "from_f32", "c", indices, 256, bf16_from_f32(words[in]));
    BENCH("stress", "from_f32", "asm", indices, 256,
          bf16_from_f32_asm(words[in]));
    BENCH("stress", "q_mul", "c", indices, 256,
          q_mul(signed_word(words[in]), signed_word(partners[in])));
    BENCH("stress", "q_mul", "asm", indices, 256,
          q_mul_asm(signed_word(words[in]), signed_word(partners[in])));
    BENCH("stress", "gcd", "c", indices, 256, gcd_u32(words[in], partners[in]));
    BENCH("stress", "gcd", "asm", indices, 256,
          gcd_u32_asm(words[in], partners[in]));
}
#endif

int main(void)
{
    static const uint32_t edges[] = {
        0u,          1u,          2u,          3u,
        0xffffffffu, 0xfffffffeu, 0xfffffffdu, 0x80000000u,
        0x7fffffffu, 0x00080000u, 0xfff80000u, 0x00100000u,
        0xfff00000u, 0x00010000u, 0x00008000u, 0x7f808000u,
    };
    for (unsigned i = 0; i < sizeof edges / sizeof edges[0]; i++)
        for (unsigned j = 0; j < sizeof edges / sizeof edges[0]; j++) {
            int rc = check(edges[i], edges[j], 1);
            if (rc)
                return rc;
        }
    for (unsigned i = 0; i < 100000; i++) {
        uint32_t a = next_word(), b = next_word();
        int rc = check(a, b, i < 10000);
        if (rc) {
            REPORT("FAIL i=%u a=0x%x b=0x%x kind=%d\n", i, a, b, rc);
            return rc;
        }
    }
    REPORT(
        "PASS 256 directed pairs; 100000 random products and narrowings; "
        "10000 random Euclidean GCD references\n");
#ifdef __riscv
    measure();
#endif
    return 0;
}
