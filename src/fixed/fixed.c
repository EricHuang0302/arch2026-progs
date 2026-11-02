#include "fixed.h"

uint32_t op1(uint32_t a, uint32_t b, int *n)
{
    *n = 0;
    while (b) {
        uint32_t carry = (a & b) << 1;
        a ^= b;
        b = carry;
        ++*n;
    }
    return a;
}

uint32_t op2(uint32_t a, uint32_t b)
{
    return a - b;
}

uint32_t op3(uint32_t a, uint32_t b)
{
    /* Disjoint carry and sum bits avoid an overflowing intermediate sum. */
    return (a & b) + ((a ^ b) >> 1);
}

int32_t q_mul(int32_t a, int32_t b)
{
    uint32_t x = (uint32_t) a;
    uint32_t y = (uint32_t) b;
    uint32_t low = 0, high = 0;
    uint32_t term_low = x, term_high = 0;
    while (y) {
        uint32_t mask = 0u - (y & 1u);
        uint32_t addend = term_low & mask;
        uint32_t next = low + addend;
        high += (term_high & mask) + (next < low);
        low = next;
        term_high = (term_high << 1) | (term_low >> 31);
        term_low <<= 1;
        y >>= 1;
    }
    /* Convert the unsigned product to the signed product modulo 2^64. */
    if (a < 0)
        high -= (uint32_t) b;
    if (b < 0)
        high -= x;
    /* Test the carry before the intentionally wrapping low-word addition. */
    high += low > UINT32_MAX - (1u << (Q - 1));
    uint32_t rounded = low + (1u << (Q - 1));
    uint32_t result = (rounded >> Q) | (high << (32 - Q));
    /* Avoid an out-of-range unsigned-to-signed conversion. */
    return result <= INT32_MAX ? (int32_t) result : -1 - (int32_t) ~result;
}
