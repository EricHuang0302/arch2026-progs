/* Same caller and exact input for the diameter-list ablation. */
#include "print.h"
#include "src/rubik/rubik.h"

uint64_t get_instret(void);

int main(void)
{
    static const char state[] = "54721631111111";
    char output[40];
    uint64_t begin = get_instret();
    int n = rubik_solve_asm(state, output, sizeof output);
    uint64_t used = get_instret() - begin;
    bm_printf("state=%s length=%d instructions=%llu solution=%s\n", state, n,
              used, output);
    return n != 11 || used > RUBIK_INSN_BUDGET;
}
