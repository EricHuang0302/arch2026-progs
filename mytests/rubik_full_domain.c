/* Host-only verification. Reuse the offline generator's enumeration and
 * published face model, but never call its file-writing entry point. The BFS
 * distance and path replay do not use the target's lookup or quarter turn.
 */
#define main table_generator_main
#include "generate_rubik_tables.c"
#undef main
#include <time.h>
#include "src/rubik/rubik.h"

static int replay(unsigned p, unsigned o, const char *text, unsigned length)
{
    for (unsigned i = 0; i < length; i++) {
        unsigned face = *text == 'R' ? 0 : *text == 'B' ? 1 : 2;
        if (*text != 'R' && *text != 'B' && *text != 'D')
            return 0;
        text++;
        unsigned turns = 1;
        if (*text == '2' || *text == '\'') {
            turns = *text == '2' ? 2 : 3;
            text++;
        }
        for (unsigned t = 0; t < turns; t++) {
            p = turn_p(p, face);
            o = turn_o(o, face);
        }
        if (i + 1 < length) {
            if (*text != ' ')
                return 0;
            text++;
        }
    }
    return !*text && p == 0x1ac688u && !o;
}

static void state_text(unsigned p, unsigned o, char state[15])
{
    unsigned sum = 0;
    for (unsigned i = 0; i < 7; i++)
        state[i] = (char) ('1' + ((p >> (3 * i)) & 7u));
    for (unsigned i = 0; i < 6; i++) {
        unsigned twist = (o >> (2 * i)) & 3u;
        state[i + 7] = (char) ('1' + twist);
        sum += twist;
    }
    state[13] = (char) ('1' + (3 - sum % 3) % 3);
    state[14] = '\0';
}

int main(void)
{
    clock_t begin = clock();
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
        for (unsigned f = 0; f < 3; f++)
            omove[i][f] =
                (uint16_t) orientation_rank[turn_o(orientations[i], f)];
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
    puts("BFS complete: 3674160 states; checking lengths and replay");
    fflush(stdout);
    for (unsigned i = 0; i < N; i++) {
        unsigned p = permutations[i / NO], o = orientations[i % NO];
        char state[15], text[40];
        state_text(p, o, state);
        int length = rubik_solve(state, text, sizeof text);
        if (length != distance[i] || !replay(p, o, text, (unsigned) length)) {
            printf("FAIL state=%s length=%d distance=%u\n", state, length,
                   distance[i]);
            return 3;
        }
        if ((i & 262143u) == 262143u) {
            printf("verified %u/%u\n", i + 1, (unsigned) N);
            fflush(stdout);
        }
    }
    for (unsigned i = 0; i < 12; i++)
        printf("distance %u: %u\n", i, histogram[i]);
    printf(
        "PASS all %u states: exact BFS length and path replay; "
        "host CPU seconds %.3f\n",
        (unsigned) N, (double) (clock() - begin) / CLOCKS_PER_SEC);
    free(distance);
    free(queue);
    return 0;
}
