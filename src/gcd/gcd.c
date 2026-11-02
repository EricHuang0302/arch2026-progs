#include "gcd.h"

uint32_t gcd_u32(uint32_t a, uint32_t b)
{
    if (!a)
        return b;
    if (!b)
        return a;
    unsigned common = 0;
    while (!((a | b) & 1u)) {
        a >>= 1;
        b >>= 1;
        common++;
    }
    while (!(a & 1u))
        a >>= 1;
    do {
        while (!(b & 1u))
            b >>= 1;
        if (a > b) {
            uint32_t t = a;
            a = b;
            b = t;
        }
        b -= a;
    } while (b);
    return a << common;
}
