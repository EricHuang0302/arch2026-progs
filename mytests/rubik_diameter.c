/* Independent development sweep: every state at the cube's diameter.
 * The subset table establishes the distance; tests/trubik.c independently
 * establishes the cube model and validates the returned move text.
 */
#include "print.h"
#include "src/rubik/rubik.h"
#include "src/rubik/tables.inc"

uint64_t get_instret(void);

static int solves(const char *state, const char *moves)
{
    static const unsigned source[3][7] = {
        {1, 4, 2, 0, 3, 5, 6},
        {0, 1, 2, 4, 5, 6, 3},
        {0, 2, 5, 3, 1, 4, 6},
    };
    static const unsigned twist[3][7] = {
        {1, 2, 0, 2, 1, 0, 0},
        {0, 0, 0, 1, 2, 1, 2},
        {0, 0, 0, 0, 0, 0, 0},
    };
    unsigned p[7], o[7], n = 0;
    for (unsigned i = 0; i < 7; i++) {
        p[i] = (unsigned) (state[i] - '1');
        o[i] = (unsigned) (state[i + 7] - '1');
    }
    while (*moves) {
        unsigned f = *moves == 'R' ? 0 : *moves == 'B' ? 1 : 2;
        if (*moves != 'R' && *moves != 'B' && *moves != 'D')
            return 0;
        moves++;
        unsigned turns = 1;
        if (*moves == '2' || *moves == '\'') {
            turns = *moves == '2' ? 2 : 3;
            moves++;
        }
        for (unsigned t = 0; t < turns; t++) {
            unsigned pp[7], oo[7];
            for (unsigned i = 0; i < 7; i++) {
                pp[i] = p[source[f][i]];
                oo[i] = o[source[f][i]] + twist[f][i];
                if (oo[i] >= 3)
                    oo[i] -= 3;
            }
            for (unsigned i = 0; i < 7; i++) {
                p[i] = pp[i];
                o[i] = oo[i];
            }
        }
        n++;
        if (*moves) {
            if (*moves != ' ' || !moves[1])
                return 0;
            moves++;
        }
    }
    if (n != 11)
        return 0;
    for (unsigned i = 0; i < 7; i++)
        if (p[i] != i || o[i])
            return 0;
    return 1;
}

static void unpack(uint32_t key, char state[15])
{
    unsigned used = 0, sum = 0;
    for (unsigned i = 0; i < 5; i++) {
        unsigned p = (key >> (3 * i)) & 7u;
        state[i] = (char) ('1' + p);
        used |= 1u << p;
    }
    unsigned a = 0, b = 0;
    for (unsigned i = 0; i < 7; i++) {
        if (used & (1u << i))
            continue;
        if (!a)
            a = i + 1;
        else
            b = i + 1;
    }
    unsigned order = (key >> 15) & 1u;
    state[5] = (char) ('0' + (order ? b : a));
    state[6] = (char) ('0' + (order ? a : b));
    for (unsigned i = 0; i < 6; i++) {
        unsigned o = (key >> (16 + 2 * i)) & 3u;
        state[7 + i] = (char) ('1' + o);
        sum += o;
    }
    while (sum >= 3)
        sum -= 3;
    state[13] = (char) ('1' + (sum ? 3 - sum : 0));
    state[14] = '\0';
}

int main(void)
{
    const unsigned count = sizeof diameter_states / sizeof diameter_states[0];
    uint64_t worst_c = 0, worst_asm = 0;
    char state[15], worst_state_c[15], worst_state_asm[15], output[40];
    for (unsigned i = 0; i < count; i++) {
        unpack(diameter_states[i], state);
        uint64_t begin = get_instret();
        int n = rubik_solve(state, output, sizeof output);
        uint64_t c = get_instret() - begin;
        if (n != 11 || !solves(state, output))
            return 1;
        begin = get_instret();
        n = rubik_solve_asm(state, output, sizeof output);
        uint64_t s = get_instret() - begin;
        if (n != 11 || !solves(state, output))
            return 2;
        if (c > worst_c) {
            worst_c = c;
            for (unsigned j = 0; j < sizeof state; j++)
                worst_state_c[j] = state[j];
        }
        if (s > worst_asm) {
            worst_asm = s;
            for (unsigned j = 0; j < sizeof state; j++)
                worst_state_asm[j] = state[j];
        }
        if ((i & 127u) == 127u)
            bm_printf("diameter checked=%u/%u worst_c=%llu worst_asm=%llu\n",
                      i + 1, count, worst_c, worst_asm);
        if (c > RUBIK_C_INSN_BUDGET || s > RUBIK_INSN_BUDGET) {
            bm_printf("over budget: %s c=%llu asm=%llu\n", state, c, s);
            return 3;
        }
    }
    bm_printf("diameter checked=%u c=%llu at %s asm=%llu at %s\n", count,
              worst_c, worst_state_c, worst_asm, worst_state_asm);
    return 0;
}
