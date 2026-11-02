# gcd_u32_asm: binary GCD, hand-written RV32I.
#
# uint32_t gcd_u32_asm(uint32_t a, uint32_t b)   a in a0, b in a1, result in a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. Tests: tests/tgcd.c.

    .text

    .globl  gcd_u32_asm
    .type   gcd_u32_asm, @function
    .align  2
gcd_u32_asm:
    # a0/a1: odd cores, t0: common shift, t1/t2: scratch. Leaf.
    beqz    a1, .Lgcd_done
    beqz    a0, .Lgcd_zero
    li      t0, 0
.Lgcd_common:
    or      t1, a0, a1
    andi    t1, t1, 1
    bnez    t1, .Lgcd_a
    srli    a0, a0, 1
    srli    a1, a1, 1
    addi    t0, t0, 1
    j       .Lgcd_common
.Lgcd_a:
    andi    t1, a0, 1
    beqz    t1, .Lgcd_a_shift
    j       .Lgcd_b
.Lgcd_a_shift:
    # A byte at a time is useful for powers of two and large common factors.
    j       .Lgcd_a_test
.Lgcd_a_bit:
    srli    a0, a0, 1
.Lgcd_a_test:
    andi    t1, a0, 1
    beqz    t1, .Lgcd_a_bit
.Lgcd_b:
    andi    t1, a1, 1
    beqz    t1, .Lgcd_b_shift
.Lgcd_compare:
    bltu    a0, a1, .Lgcd_sub_b
    sub     a0, a0, a1
    beqz    a0, .Lgcd_equal
.Lgcd_strip_a:
    srli    a0, a0, 1
    andi    t1, a0, 1
    beqz    t1, .Lgcd_strip_a
    j       .Lgcd_compare
.Lgcd_sub_b:
    sub     a1, a1, a0
.Lgcd_b_shift:
    srli    a1, a1, 1
    andi    t1, a1, 1
    beqz    t1, .Lgcd_b_shift
    j       .Lgcd_compare
.Lgcd_equal:
    mv      a0, a1
    sll     a0, a0, t0
.Lgcd_done:
    ret
.Lgcd_zero:
    mv      a0, a1
    ret
    .size   gcd_u32_asm, .-gcd_u32_asm
