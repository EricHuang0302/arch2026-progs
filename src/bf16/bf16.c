#include "bf16.h"

uint16_t bf16_from_f32(uint32_t f32bits)
{
    if ((f32bits & 0x7f800000u) == 0x7f800000u)
        return (uint16_t) (f32bits >> 16);
    return (uint16_t) ((f32bits + 0x7fffu + ((f32bits >> 16) & 1u)) >> 16);
}

uint16_t bf16_mul(uint16_t a, uint16_t b)
{
    unsigned sign = (a ^ b) & 0x8000u;
    int ea = (a >> 7) & 255;
    int eb = (b >> 7) & 255;
    unsigned ma = a & 127u;
    unsigned mb = b & 127u;
    if (ea == 255) {
        if (ma)
            return a;
        return (b & 0x7fffu) ? (uint16_t) (sign | 0x7f80u) : 0x7fc0u;
    }
    if (eb == 255) {
        if (mb)
            return b;
        return (a & 0x7fffu) ? (uint16_t) (sign | 0x7f80u) : 0x7fc0u;
    }
    if (!(a & 0x7fffu) || !(b & 0x7fffu))
        return (uint16_t) sign;
    if (ea)
        ma |= 128u;
    else {
        ea = 1;
        while (ma < 128u) {
            ma <<= 1;
            ea--;
        }
    }
    if (eb)
        mb |= 128u;
    else {
        eb = 1;
        while (mb < 128u) {
            mb <<= 1;
            eb--;
        }
    }
    unsigned product = ma * mb;
    int exponent = ea + eb - 127;
    unsigned fraction = product >> 7;
    if (product & 0x8000u) {
        fraction >>= 1;
        exponent++;
    }
    /* The quiz contract shifts the stored fraction, without the hidden bit. */
    fraction &= 127u;
    if (exponent >= 255)
        return (uint16_t) (sign | 0x7f80u);
    if (exponent <= 0) {
        if (exponent < -6)
            return (uint16_t) sign;
        return (uint16_t) (sign | (fraction >> (1 - exponent)));
    }
    return (uint16_t) (sign | ((unsigned) exponent << 7) | fraction);
}

uint16_t bf16_sqrt(uint16_t a)
{
    unsigned exponent = (a >> 7) & 255u;
    if (exponent == 255u) {
        if (a & 127u)
            return a;
        return (a & 0x8000u) ? 0x7fc0u : a;
    }
    if (!(a & 0x7fffu))
        return 0;
    if (a & 0x8000u)
        return 0x7fc0u;
    if (!exponent)
        return 0;
    unsigned mantissa = (a & 127u) | 128u;
    if (!(exponent & 1u))
        mantissa <<= 1;
    /* floor(mid*mid/128) <= m iff mid*mid <= 128*(m+1)-1. */
    unsigned remainder = ((mantissa + 1u) << 7) - 1u;
    unsigned root = 0;
    for (unsigned bit = 1u << 14; bit; bit >>= 2) {
        unsigned trial = root + bit;
        if (remainder >= trial) {
            remainder -= trial;
            root = (root >> 1) + bit;
        } else
            root >>= 1;
    }
    return (uint16_t) (((exponent + 127u) >> 1) << 7 | (root & 127u));
}
