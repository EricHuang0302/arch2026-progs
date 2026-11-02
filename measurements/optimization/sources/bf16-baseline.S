# BFloat16 routines: hand-written RV32I.
#
# uint16_t bf16_from_f32_asm(uint32_t f32bits)   a0 -> a0
# uint16_t bf16_mul_asm(uint16_t a, uint16_t b)  a0, a1 -> a0
# uint16_t bf16_sqrt_asm(uint16_t a)             a0 -> a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions. Tests: tests/tbf16.c.

    .text

    .globl  bf16_from_f32_asm
    .type   bf16_from_f32_asm, @function
    .align  2
bf16_from_f32_asm:
    # a0: binary32 word, t0/t1: exponent and rounding scratch. Leaf.
    li      t0, 0x7f800000
    and     t1, a0, t0
    beq     t1, t0, .Lf32_top
    srli    t1, a0, 16
    andi    t1, t1, 1
    li      t0, 0x7fff
    add     a0, a0, t0
    add     a0, a0, t1
.Lf32_top:
    srli    a0, a0, 16
    ret
    .size   bf16_from_f32_asm, .-bf16_from_f32_asm

    .globl  bf16_mul_asm
    .type   bf16_mul_asm, @function
    .align  2
bf16_mul_asm:
    # a2/a3: exponents, a4: sign, t0/t1: significands, a5: product.
    # t2/t3 and a6 are scratch. Inputs stay intact for NaN propagation.
    # All registers are caller-saved and this function is a leaf.
    xor     a4, a0, a1
    li      t2, 0x8000
    and     a4, a4, t2
    srli    a2, a0, 7
    andi    a2, a2, 255
    srli    a3, a1, 7
    andi    a3, a3, 255
    andi    t0, a0, 127
    andi    t1, a1, 127
    li      t2, 255
    beq     a2, t2, .Lmul_special_a
    beq     a3, t2, .Lmul_special_b
    or      t2, a2, t0
    beqz    t2, .Lmul_zero
    or      t2, a3, t1
    beqz    t2, .Lmul_zero
    beqz    a2, .Lmul_denorm_a
    ori     t0, t0, 128
.Lmul_norm_b:
    beqz    a3, .Lmul_denorm_b
    ori     t1, t1, 128
.Lmul_product:
    # The leading bit of t0 is known: account for it once, then seven bits.
    slli    a5, t1, 7
    .rept   7
    andi    t2, t0, 1
    beqz    t2, 1f
    add     a5, a5, t1
1:
    srli    t0, t0, 1
    slli    t1, t1, 1
    .endr
    add     a2, a2, a3
    addi    a2, a2, -127
    srli    t2, a5, 15
    add     a2, a2, t2
    addi    t2, t2, 7
    srl     a5, a5, t2
    # Underflow shifts the stored fraction, excluding its hidden one.
    andi    a5, a5, 127
    li      t2, 255
    bge     a2, t2, .Lmul_inf
    blez    a2, .Lmul_underflow
    slli    a2, a2, 7
    or      a0, a2, a4
    or      a0, a0, a5
    ret
.Lmul_denorm_a:
    li      a2, 1
2:
    slli    t0, t0, 1
    addi    a2, a2, -1
    sltiu   t2, t0, 128
    bnez    t2, 2b
    j       .Lmul_norm_b
.Lmul_denorm_b:
    li      a3, 1
3:
    slli    t1, t1, 1
    addi    a3, a3, -1
    sltiu   t2, t1, 128
    bnez    t2, 3b
    j       .Lmul_product
.Lmul_special_a:
    bnez    t0, .Lmul_return
    or      t2, a3, t1
    beqz    t2, .Lmul_nan
    j       .Lmul_inf
.Lmul_special_b:
    beqz    t1, .Lmul_b_inf
    mv      a0, a1
    ret
.Lmul_b_inf:
    or      t2, a2, t0
    beqz    t2, .Lmul_nan
.Lmul_inf:
    li      a0, 0x7f80
    or      a0, a0, a4
    ret
.Lmul_underflow:
    li      t2, -6
    blt     a2, t2, .Lmul_zero
    li      t2, 1
    sub     t2, t2, a2
    srl     a5, a5, t2
    or      a0, a5, a4
    ret
.Lmul_nan:
    li      a0, 0x7fc0
    ret
.Lmul_zero:
    mv      a0, a4
.Lmul_return:
    ret
    .size   bf16_mul_asm, .-bf16_mul_asm

    .globl  bf16_sqrt_asm
    .type   bf16_sqrt_asm, @function
    .align  2
bf16_sqrt_asm:
    # t0: exponent, t1: sign/mantissa, t2: table address. Leaf.
    srli    t0, a0, 7
    andi    t0, t0, 255
    li      t1, 255
    beq     t0, t1, .Lsqrt_special
    li      t1, 0x7fff
    and     t1, a0, t1
    beqz    t1, .Lsqrt_zero
    srli    t1, a0, 15
    bnez    t1, .Lsqrt_nan
    beqz    t0, .Lsqrt_zero
    andi    t1, a0, 127
    andi    t2, t0, 1
    bnez    t2, .Lsqrt_lookup
    addi    t1, t1, 128
.Lsqrt_lookup:
    # Two 128-byte blocks cover m and 2*m; odd doubled values never occur.
    la      t2, .Lsqrt_table
    add     t2, t2, t1
    lbu     t1, 0(t2)
    andi    t1, t1, 127
    addi    t0, t0, 127
    srli    t0, t0, 1
    slli    a0, t0, 7
    or      a0, a0, t1
    ret
.Lsqrt_special:
    andi    t1, a0, 127
    bnez    t1, .Lsqrt_return
    srli    t1, a0, 15
    beqz    t1, .Lsqrt_return
.Lsqrt_nan:
    li      a0, 0x7fc0
    ret
.Lsqrt_zero:
    li      a0, 0
.Lsqrt_return:
    ret
    .size   bf16_sqrt_asm, .-bf16_sqrt_asm

    .section .rodata
.Lsqrt_table:
    .include "src/bf16/sqrt-table.inc"
