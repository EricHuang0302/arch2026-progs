# op1, op2, op3 and q_mul: hand-written RV32I.
#
# uint32_t op1_asm(uint32_t a, uint32_t b, int *n)   a0, a1, a2 -> a0
# uint32_t op2_asm(uint32_t a, uint32_t b)           a0, a1 -> a0
# uint32_t op3_asm(uint32_t a, uint32_t b)           a0, a1 -> a0
# int32_t  q_mul_asm(int32_t a, int32_t b)           a0, a1 -> a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. q_mul forms a 64-bit product in a register pair. Tests:
# tests/tfixed.c.

    .text

    .globl  op1_asm
    .type   op1_asm, @function
    .align  2
op1_asm:
    # t0: pass count, t1: carry. a2 remains the output pointer. Leaf.
    li      t0, 0
    beqz    a1, .Lop1_done
.Lop1_loop:
    and     t1, a0, a1
    xor     a0, a0, a1
    slli    a1, t1, 1
    addi    t0, t0, 1
    bnez    a1, .Lop1_loop
.Lop1_done:
    sw      t0, 0(a2)
    ret
    .size   op1_asm, .-op1_asm

    .globl  op2_asm
    .type   op2_asm, @function
    .align  2
op2_asm:
    # a0/a1: operands; no scratch registers. Leaf.
    sub     a0, a0, a1
    ret
    .size   op2_asm, .-op2_asm

    .globl  op3_asm
    .type   op3_asm, @function
    .align  2
op3_asm:
    # t0: carry bits, a0: half the non-carry bits. Leaf.
    and     t0, a0, a1
    xor     a0, a0, a1
    srli    a0, a0, 1
    add     a0, a0, t0
    ret
    .size   op3_asm, .-op3_asm

    .globl  q_mul_asm
    .type   q_mul_asm, @function
    .align  2
q_mul_asm:
    # a3:a2 accumulates the product; t0:a0 is the shifted multiplicand.
    # The sign bit of a4 selects full-product negation; t1/t2 are scratch.
    # a1 is the shifting multiplier. All registers are caller-saved. Leaf.
    xor     a4, a0, a1
    bgez    a0, .Lq_abs_b
    neg     a0, a0
.Lq_abs_b:
    bgez    a1, .Lq_abs_done
    neg     a1, a1
.Lq_abs_done:
    bgeu    a0, a1, .Lq_ordered
    mv      t1, a0
    mv      a0, a1
    mv      a1, t1
.Lq_ordered:
    li      a2, 0
    li      a3, 0
    li      t0, 0
    beqz    a1, .Lq_round
.Lq_loop:
    andi    t1, a1, 1
    beqz    t1, .Lq_shift
    add     a2, a2, a0
    sltu    t1, a2, a0
    add     a3, a3, t0
    add     a3, a3, t1
.Lq_shift:
    srli    a1, a1, 1
    beqz    a1, .Lq_round
    srli    t1, a0, 31
    slli    t0, t0, 1
    or      t0, t0, t1
    slli    a0, a0, 1
    j       .Lq_loop
.Lq_round:
    bgez    a4, .Lq_positive
    seqz    t1, a2
    neg     a2, a2
    not     a3, a3
    add     a3, a3, t1
.Lq_positive:
    li      t0, 0x80000
    add     t0, a2, t0
    sltu    t1, t0, a2
    add     a3, a3, t1
    srli    a0, t0, 20
    slli    a3, a3, 12
    or      a0, a0, a3
    ret
    .size   q_mul_asm, .-q_mul_asm
