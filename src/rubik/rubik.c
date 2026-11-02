#include "rubik.h"

#include "tables.inc"

/* Seven three-bit cubie identities; only six two-bit twists are independent.
 * After the first five identities, the last pair needs only its ordering bit.
 * This gives an injective 28-bit key, leaving four bits for the distance.
 */
static uint32_t state_key(uint32_t p, unsigned o)
{
    return (p & 32767u) | (((p >> 15 & 7u) > (p >> 18)) << 15) | (o << 16);
}

static int near_distance(uint32_t p, unsigned o)
{
    uint32_t k = state_key(p, o);
    unsigned a = (k ^ (k >> 7) ^ (k >> 14)) & 16383u;
    unsigned b = (k ^ (k >> 5) ^ (k >> 17)) & 16383u;
    uint32_t e = near_states[a];
    if ((e & 0x0fffffffu) == k)
        return (int) (e >> 28);
    e = near_states[b];
    if ((e & 0x0fffffffu) == k)
        return (int) (e >> 28);
    e = near_states[a ^ b];
    return (e & 0x0fffffffu) == k ? (int) (e >> 28) : -1;
}

static uint32_t quarter(uint32_t p, unsigned face)
{
    switch (face) {
    case 0:
        return (p & 0x1f81c0u) | ((p >> 3) & 7u) | ((p >> 9) & 0x38u) |
               ((p << 3) & 0x7000u) | ((p << 9) & 0xe00u);
    case 1:
        return (p & 0x1ffu) | ((p >> 3) & 0x3fe00u) | ((p << 9) & 0x1c0000u);
    default:
        return (p & 0x1c0e07u) | ((p >> 3) & 0x38u) | ((p >> 9) & 0x1c0u) |
               ((p << 3) & 0x38000u) | ((p << 9) & 0x7000u);
    }
}

/* A table distance supplies a shortest suffix: take a neighbor at d-1.
 * There is no stored solution and no table of the complete state space.
 */
static void suffix(uint32_t p, unsigned o, int d, unsigned char *path)
{
    while (d) {
        for (unsigned f = 0; f < 3; f++) {
            uint32_t pp = p;
            unsigned oo = o;
            for (unsigned t = 0; t < 3; t++) {
                pp = quarter(pp, f);
                oo = orientation_moves[f][oo];
                if (near_distance(pp, oo) == d - 1) {
                    *path++ = (unsigned char) (3 * f + t);
                    p = pp;
                    o = oo;
                    d--;
                    goto next;
                }
            }
        }
    next:;
    }
}

/* Consecutive turns of one face combine into one turn or cancel, so omitting
 * them never removes a shortest path. Only the prefix is searched on target.
 */
static int search(uint32_t p,
                  unsigned o,
                  unsigned left,
                  unsigned previous,
                  unsigned char *path)
{
    if (!left) {
        int d = near_distance(p, o);
        if (d >= 0)
            suffix(p, o, d, path);
        return d;
    }
    for (unsigned f = 0; f < 3; f++) {
        if (f == previous)
            continue;
        uint32_t pp = p;
        unsigned oo = o;
        for (unsigned t = 0; t < 3; t++) {
            pp = quarter(pp, f);
            oo = orientation_moves[f][oo];
            int d = search(pp, oo, left - 1, f, path + 1);
            if (d >= 0) {
                *path = (unsigned char) (3 * f + t);
                return d + 1;
            }
        }
    }
    return -1;
}

static int at_diameter(uint32_t k)
{
    unsigned low = 0;
    unsigned high = sizeof diameter_states / sizeof diameter_states[0];
    while (low < high) {
        unsigned mid = low + ((high - low) >> 1);
        if (diameter_states[mid] < k)
            low = mid + 1;
        else
            high = mid;
    }
    return low < sizeof diameter_states / sizeof diameter_states[0] &&
           diameter_states[low] == k;
}

int rubik_solve(const char *state, char *out, size_t cap)
{
    for (unsigned i = 0; i < RUBIK_STATE_LEN; i++)
        if (!state[i])
            return -1;
    if (state[RUBIK_STATE_LEN])
        return -1;
    uint32_t p = 0;
    unsigned o = 0, seen = 0, sum = 0;
    for (unsigned i = 0; i < 7; i++) {
        unsigned v = (unsigned char) state[i] - (unsigned) '1';
        unsigned t = (unsigned char) state[i + 7] - (unsigned) '1';
        if (v > 6 || t > 2 || (seen & (1u << v)))
            return -1;
        seen |= 1u << v;
        p |= v << (3 * i);
        if (i < 6)
            o |= t << (2 * i);
        sum += t;
    }
    /* A bounded subtraction avoids a division helper on RV32I. */
    while (sum >= 3)
        sum -= 3;
    if (sum)
        return -1;
    unsigned char path[11];
    int length = near_distance(p, o);
    if (length >= 0)
        suffix(p, o, length, path);
    else {
        unsigned first = at_diameter(state_key(p, o)) ? 6 : 1;
        for (unsigned depth = first; depth <= 6; depth++) {
            length = search(p, o, depth, 3, path);
            if (length >= 0)
                break;
        }
    }
    size_t needed = length ? (size_t) length * 2 : 1;
    for (int i = 0; i < length; i++)
        if (path[i] == 1 || path[i] == 2 || path[i] == 4 || path[i] == 5 ||
            path[i] == 7 || path[i] == 8)
            needed++;
    if (needed > cap) {
        if (cap)
            out[0] = '\0';
        return -2;
    }
    char *cursor = out;
    static const char faces[] = "RRRBBBDDD";
    static const char endings[] =
        "\0"
        "2'\0"
        "2'\0"
        "2'";
    for (int i = 0; i < length; i++) {
        unsigned move = path[i];
        if (i)
            *cursor++ = ' ';
        *cursor++ = faces[move];
        if (endings[move])
            *cursor++ = endings[move];
    }
    *cursor = '\0';
    return length;
}
