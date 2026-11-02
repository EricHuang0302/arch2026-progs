/* Offline generator: retain only radius five, diameter states, and twists.
 * The complete distance array is temporary host memory, never target data.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NP = 5040, NO = 729, N = NP * NO, NH = 16384 };
static const unsigned source[3][7] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const unsigned delta[3][7] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};
static uint32_t permutations[NP], near[NH], far[N];
static int16_t permutation_rank[1 << 21], orientation_rank[4096];
static uint16_t orientations[NO], pmove[NP][3], omove[NO][3];
static uint16_t twists[3][4096];
static unsigned permutation_count;

static void enumerate(unsigned p, unsigned used, unsigned position)
{
    if (position == 7) {
        permutation_rank[p] = (int16_t) permutation_count;
        permutations[permutation_count++] = p;
        return;
    }
    for (unsigned v = 0; v < 7; v++)
        if (!(used & (1u << v)))
            enumerate(p | (v << (3 * position)), used | (1u << v),
                      position + 1);
}

static unsigned turn_p(unsigned p, unsigned face)
{
    unsigned result = 0;
    for (unsigned i = 0; i < 7; i++)
        result |= ((p >> (3 * source[face][i])) & 7u) << (3 * i);
    return result;
}

static unsigned turn_o(unsigned o, unsigned face)
{
    unsigned values[7], sum = 0, result = 0;
    for (unsigned i = 0; i < 6; i++) {
        values[i] = (o >> (2 * i)) & 3u;
        sum += values[i];
    }
    values[6] = (3 - sum % 3) % 3;
    for (unsigned i = 0; i < 6; i++)
        result |= ((values[source[face][i]] + delta[face][i]) % 3) << (2 * i);
    return result;
}

static unsigned key(unsigned p, unsigned o)
{
    return (p & 32767u) | (((p >> 15 & 7u) > (p >> 18)) << 15) | (o << 16);
}

static unsigned hash(unsigned k)
{
    return (k ^ (k >> 7) ^ (k >> 14)) & (NH - 1);
}

static unsigned hash2(unsigned k)
{
    return (k ^ (k >> 5) ^ (k >> 17)) & (NH - 1);
}

static unsigned random_word = 0x31415926;

static unsigned random_next(void)
{
    random_word ^= random_word << 13;
    random_word ^= random_word >> 17;
    random_word ^= random_word << 5;
    return random_word;
}

static int insert(unsigned entry)
{
    for (unsigned kick = 0; kick < 20000; kick++) {
        unsigned k = entry & 0x0fffffffu;
        unsigned a = hash(k), b = hash2(k);
        const unsigned slots[3] = {a, b, a ^ b};
        for (unsigned i = 0; i < 3; i++)
            if (!near[slots[i]]) {
                near[slots[i]] = entry;
                return 1;
            }
        unsigned slot = slots[random_next() % 3];
        unsigned old = near[slot];
        near[slot] = entry;
        entry = old;
    }
    return 0;
}

static int compare(const void *a, const void *b)
{
    unsigned x = *(const unsigned *) a, y = *(const unsigned *) b;
    return (x > y) - (x < y);
}

static void write_words(FILE *file,
                        const char *name,
                        const uint32_t *data,
                        unsigned count)
{
    fprintf(file, "static const uint32_t %s[%u] = {\n", name, count);
    for (unsigned i = 0; i < count; i++) {
        if (!(i & 7u))
            fputs("    ", file);
        fprintf(file, "0x%08xu,%s", data[i], (i & 7u) == 7 ? "\n" : " ");
    }
    fputs("\n};\n", file);
}

int main(void)
{
    memset(permutation_rank, -1, sizeof permutation_rank);
    memset(orientation_rank, -1, sizeof orientation_rank);
    enumerate(0, 0, 0);
    for (unsigned i = 0; i < NO; i++) {
        unsigned value = i, packed = 0;
        for (unsigned j = 0; j < 6; j++) {
            packed |= (value % 3) << (2 * j);
            value /= 3;
        }
        orientations[i] = (uint16_t) packed;
        orientation_rank[packed] = (int16_t) i;
    }
    for (unsigned i = 0; i < NP; i++)
        for (unsigned f = 0; f < 3; f++)
            pmove[i][f] =
                (uint16_t) permutation_rank[turn_p(permutations[i], f)];
    for (unsigned i = 0; i < NO; i++)
        for (unsigned f = 0; f < 3; f++) {
            unsigned next = turn_o(orientations[i], f);
            omove[i][f] = (uint16_t) orientation_rank[next];
            twists[f][orientations[i]] = (uint16_t) next;
        }
    uint8_t *distance = malloc(N);
    uint32_t *queue = malloc(N * sizeof *queue);
    if (!distance || !queue)
        return 1;
    memset(distance, 255, N);
    unsigned solved = (unsigned) permutation_rank[0x1ac688] * NO;
    distance[solved] = 0;
    unsigned head = 0, tail = 1, histogram[12] = {0};
    queue[0] = solved;
    while (head < tail) {
        unsigned index = queue[head++];
        unsigned p = index / NO, o = index % NO;
        histogram[distance[index]]++;
        for (unsigned f = 0; f < 3; f++) {
            unsigned pp = p, oo = o;
            for (unsigned t = 0; t < 3; t++) {
                pp = pmove[pp][f];
                oo = omove[oo][f];
                unsigned next = pp * NO + oo;
                if (distance[next] == 255) {
                    distance[next] = distance[index] + 1;
                    queue[tail++] = next;
                }
            }
        }
    }
    if (tail != N)
        return 2;
    unsigned nf = 0, nn = 0;
    for (unsigned i = 0; i < N; i++) {
        unsigned k = key(permutations[i / NO], orientations[i % NO]);
        if (distance[i] == 11)
            far[nf++] = k;
        if (distance[i] > 5)
            continue;
        if (!insert(k | (unsigned) distance[i] << 28)) {
            fputs("cuckoo insertion failed\n", stderr);
            return 5;
        }
        nn++;
    }
    qsort(far, nf, sizeof *far, compare);
    FILE *c = fopen("src/rubik/tables.inc", "w");
    FILE *s = fopen("src/rubik/tables-asm.inc", "w");
    if (!c || !s)
        return 3;
    fputs("/* Generated by mytests/generate_rubik_tables.c. */\n", c);
    write_words(c, "near_states", near, NH);
    write_words(c, "diameter_states", far, nf);
    fputs("static const uint16_t orientation_moves[3][4096] = {\n", c);
    for (unsigned f = 0; f < 3; f++) {
        fputs("    {\n", c);
        for (unsigned i = 0; i < 4096; i++) {
            if (!(i & 15u))
                fputs("        ", c);
            fprintf(c, "%u,%s", twists[f][i], (i & 15u) == 15 ? "\n" : " ");
        }
        fputs("    },\n", c);
    }
    fputs("};\n", c);
    fprintf(s,
            "# Generated by mytests/generate_rubik_tables.c.\n"
            ".equ DIAMETER_COUNT, %u\n.Lnear_states:\n",
            nf);
    for (unsigned i = 0; i < NH; i++)
        fprintf(s, "    .word 0x%08x\n", near[i]);
    fputs(".Ldiameter_states:\n", s);
    for (unsigned i = 0; i < nf; i++)
        fprintf(s, "    .word 0x%08x\n", far[i]);
    fputs(".Lorientation_moves:\n", s);
    for (unsigned f = 0; f < 3; f++)
        for (unsigned i = 0; i < 4096; i++)
            fprintf(s, "    .half %u\n", twists[f][i]);
    fclose(c);
    fclose(s);
    for (unsigned i = 0; i < 12; i++)
        printf("distance %u: %u\n", i, histogram[i]);
    printf("near=%u, diameter=%u, cuckoo candidates=3\n", nn, nf);
    free(distance);
    free(queue);
    return nf == 2644 ? 0 : 4;
}
