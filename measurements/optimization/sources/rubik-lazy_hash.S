# rubik_solve_asm: optimal 2x2x2 cube solver, hand-written RV32I.
#
# int rubik_solve_asm(const char *state, char *out, size_t cap)
#                                   a0 = state, a1 = out, a2 = cap; result in a0
#
# RV32I and Zicsr only: no mul, div, rem, no floating point, no compressed
# instructions, no heap. Prefix recursion is bounded by six levels, with a
# 48-byte frame per level. Tests: tests/trubik.c.

    .section .rodata
    .balign 4
    .include "src/rubik/tables-asm.inc"

    .text

    .globl  rubik_solve_asm
    .type   rubik_solve_asm, @function
    .align  2
rubik_solve_asm:
    # s0/s1: packed cube, s2/s3: output/capacity, s4: local path, s5: length.
    # Only caller-saved registers are used by parsing and table lookups.
    addi    sp, sp, -64
    sw      ra, 60(sp)
    sw      s0, 56(sp)
    sw      s1, 52(sp)
    sw      s2, 48(sp)
    sw      s3, 44(sp)
    sw      s4, 40(sp)
    sw      s5, 36(sp)
    mv      s2, a1
    mv      s3, a2
    mv      s4, sp
    mv      t0, a0
    addi    t2, a0, 14
.Llength:
    lbu     t1, 0(t0)
    beqz    t1, .Linvalid
    addi    t0, t0, 1
    bne     t0, t2, .Llength
    lbu     t1, 0(t0)
    bnez    t1, .Linvalid
    mv      t0, a0
    li      s0, 0
    li      s1, 0
    li      a3, 0
    li      a4, 0
    li      a5, 0
    li      a6, 0
.Lparse:
    lbu     t1, 0(t0)
    lbu     t2, 7(t0)
    addi    t1, t1, -49
    addi    t2, t2, -49
    sltiu   t3, t1, 7
    beqz    t3, .Linvalid
    sltiu   t3, t2, 3
    beqz    t3, .Linvalid
    li      t3, 1
    sll     t3, t3, t1
    and     t4, t3, a5
    bnez    t4, .Linvalid
    or      a5, a5, t3
    sll     t1, t1, a3
    or      s0, s0, t1
    add     a6, a6, t2
    sltiu   t3, a3, 18
    beqz    t3, .Lparse_next
    sll     t2, t2, a4
    or      s1, s1, t2
.Lparse_next:
    addi    t0, t0, 1
    addi    a3, a3, 3
    addi    a4, a4, 2
    sltiu   t3, a3, 21
    bnez    t3, .Lparse
.Lsum:
    sltiu   t0, a6, 3
    bnez    t0, .Lsum_done
    addi    a6, a6, -3
    j       .Lsum
.Lsum_done:
    bnez    a6, .Linvalid
    mv      a0, s0
    mv      a1, s1
    call    .Lnear
    bltz    a5, .Lchoose_depth
    mv      s5, a5
    mv      a2, a5
    mv      a4, s4
    call    .Lsuffix
    j       .Lformat
.Lchoose_depth:
    # .Lnear leaves the injective key in t0. Binary search the diameter list.
    mv      t4, t0
    li      t1, 0
    li      t2, DIAMETER_COUNT
    la      t3, .Ldiameter_states
.Ldiameter_loop:
    bgeu    t1, t2, .Ldiameter_done
    add     t5, t1, t2
    srli    t5, t5, 1
    slli    t6, t5, 2
    add     t6, t3, t6
    lw      t6, 0(t6)
    bltu    t6, t4, .Ldiameter_low
    mv      t2, t5
    j       .Ldiameter_loop
.Ldiameter_low:
    addi    t1, t5, 1
    j       .Ldiameter_loop
.Ldiameter_done:
    li      s5, 1
    li      t2, DIAMETER_COUNT
    bgeu    t1, t2, .Ldepth_loop
    slli    t1, t1, 2
    add     t1, t3, t1
    lw      t1, 0(t1)
    bne     t1, t4, .Ldepth_loop
    li      s5, 6
.Ldepth_loop:
    mv      a0, s0
    mv      a1, s1
    mv      a2, s5
    li      a3, 3
    mv      a4, s4
    call    .Lsearch
    bgez    a0, .Lfound
    addi    s5, s5, 1
    sltiu   t0, s5, 7
    bnez    t0, .Ldepth_loop
    j       .Linvalid
.Lfound:
    mv      s5, a0
.Lformat:
    # Measure before writing, preserving out on invalid input and obeying cap.
    slli    t0, s5, 1
    bnez    s5, .Lmeasure_start
    li      t0, 1
.Lmeasure_start:
    li      t1, 0
    la      t3, .Lendings
.Lmeasure:
    bge     t1, s5, .Lcapacity
    add     t2, s4, t1
    lbu     t2, 0(t2)
    add     t2, t3, t2
    lbu     t2, 0(t2)
    snez    t2, t2
    add     t0, t0, t2
    addi    t1, t1, 1
    j       .Lmeasure
.Lcapacity:
    bltu    s3, t0, .Lsmall_buffer
    li      t0, 0
    mv      t1, s2
    la      t4, .Lfaces
    la      t5, .Lendings
.Lwrite:
    bge     t0, s5, .Lterminate
    beqz    t0, .Lwrite_move
    li      t2, 32
    sb      t2, 0(t1)
    addi    t1, t1, 1
.Lwrite_move:
    add     t2, s4, t0
    lbu     t2, 0(t2)
    add     t3, t4, t2
    lbu     t3, 0(t3)
    sb      t3, 0(t1)
    addi    t1, t1, 1
    add     t2, t5, t2
    lbu     t2, 0(t2)
    beqz    t2, .Lwrite_next
    sb      t2, 0(t1)
    addi    t1, t1, 1
.Lwrite_next:
    addi    t0, t0, 1
    j       .Lwrite
.Lterminate:
    sb      zero, 0(t1)
    mv      a0, s5
    j       .Lreturn
.Lsmall_buffer:
    beqz    s3, .Lsmall_return
    sb      zero, 0(s2)
.Lsmall_return:
    li      a0, -2
    j       .Lreturn
.Linvalid:
    li      a0, -1
.Lreturn:
    lw      ra, 60(sp)
    lw      s0, 56(sp)
    lw      s1, 52(sp)
    lw      s2, 48(sp)
    lw      s3, 44(sp)
    lw      s4, 40(sp)
    lw      s5, 36(sp)
    addi    sp, sp, 64
    ret
    .size   rubik_solve_asm, .-rubik_solve_asm

# a0/a1: cube, a5: returned distance (-1 outside radius five).
# Only t0..t6 and a5 change. In particular a6 may hold the leaf return address.
.Lnear:
    li      t3, 32767
    and     t0, a0, t3
    srli    t1, a0, 15
    andi    t1, t1, 7
    srli    t2, a0, 18
    sltu    t1, t2, t1
    slli    t1, t1, 15
    or      t0, t0, t1
    slli    t1, a1, 16
    or      t0, t0, t1
    srli    t1, t0, 7
    srli    t2, t0, 14
    xor     t1, t1, t2
    xor     t1, t1, t0
    # Mask to 14 bits and scale by four with two shifts per candidate.
    slli    t1, t1, 18
    srli    t1, t1, 16
    la      t3, .Lnear_states
    li      t6, 0x0fffffff
    add     t4, t3, t1
    lw      t4, 0(t4)
    and     t5, t4, t6
    beq     t5, t0, .Lnear_hit
    srli    t2, t0, 5
    srli    t4, t0, 17
    xor     t2, t2, t4
    xor     t2, t2, t0
    slli    t2, t2, 18
    srli    t2, t2, 16
    add     t4, t3, t2
    lw      t4, 0(t4)
    and     t5, t4, t6
    beq     t5, t0, .Lnear_hit
    xor     t1, t1, t2
    add     t4, t3, t1
    lw      t4, 0(t4)
    and     t5, t4, t6
    beq     t5, t0, .Lnear_hit
    li      a5, -1
    ret
.Lnear_hit:
    srli    a5, t4, 28
    ret

# a0/a1: packed permutation/twists; a2: face; returns the quarter-turned cube.
# t0..t2 are scratch. The orientation table includes the inferred last twist.
.Lquarter:
    slli    t0, a2, 13
    la      t1, .Lorientation_moves
    add     t1, t1, t0
    slli    t2, a1, 1
    add     t1, t1, t2
    lhu     a1, 0(t1)
    beqz    a2, .Lquarter_r
    li      t0, 1
    beq     a2, t0, .Lquarter_b
    li      t0, 0x1c0e07
    and     t0, a0, t0
    srli    t1, a0, 3
    andi    t1, t1, 0x38
    or      t0, t0, t1
    srli    t1, a0, 9
    andi    t1, t1, 0x1c0
    or      t0, t0, t1
    slli    t1, a0, 3
    li      t2, 0x38000
    and     t1, t1, t2
    or      t0, t0, t1
    slli    t1, a0, 9
    li      t2, 0x7000
    and     t1, t1, t2
    or      a0, t0, t1
    ret
.Lquarter_r:
    li      t0, 0x1f81c0
    and     t0, a0, t0
    srli    t1, a0, 3
    andi    t1, t1, 7
    or      t0, t0, t1
    srli    t1, a0, 9
    andi    t1, t1, 0x38
    or      t0, t0, t1
    slli    t1, a0, 3
    li      t2, 0x7000
    and     t1, t1, t2
    or      t0, t0, t1
    slli    t1, a0, 9
    li      t2, 0xe00
    and     t1, t1, t2
    or      a0, t0, t1
    ret
.Lquarter_b:
    andi    t0, a0, 0x1ff
    srli    t1, a0, 3
    li      t2, 0x3fe00
    and     t1, t1, t2
    or      t0, t0, t1
    slli    t1, a0, 9
    li      t2, 0x1c0000
    and     t1, t1, t2
    or      a0, t0, t1
    ret

# Greedy decreasing-distance suffix. s0/s1: cube; s2: remaining distance;
# s3: path pointer; s4/s5: face/turn; s6/s7: trial cube. Bounded by five moves.
.Lsuffix:
    addi    sp, sp, -48
    sw      ra, 44(sp)
    sw      s0, 40(sp)
    sw      s1, 36(sp)
    sw      s2, 32(sp)
    sw      s3, 28(sp)
    sw      s4, 24(sp)
    sw      s5, 20(sp)
    sw      s6, 16(sp)
    sw      s7, 12(sp)
    mv      s0, a0
    mv      s1, a1
    mv      s2, a2
    mv      s3, a4
.Lsuffix_next:
    beqz    s2, .Lsuffix_done
    li      s4, 0
.Lsuffix_face:
    li      s5, 0
    mv      s6, s0
    mv      s7, s1
.Lsuffix_turn:
    mv      a0, s6
    mv      a1, s7
    mv      a2, s4
    call    .Lquarter
    mv      s6, a0
    mv      s7, a1
    call    .Lnear
    addi    t0, s2, -1
    beq     a5, t0, .Lsuffix_accept
    addi    s5, s5, 1
    sltiu   t0, s5, 3
    bnez    t0, .Lsuffix_turn
    addi    s4, s4, 1
    j       .Lsuffix_face
.Lsuffix_accept:
    add     t0, s4, s4
    add     t0, t0, s4
    add     t0, t0, s5
    sb      t0, 0(s3)
    addi    s3, s3, 1
    addi    s2, s2, -1
    mv      s0, s6
    mv      s1, s7
    j       .Lsuffix_next
.Lsuffix_done:
    lw      ra, 44(sp)
    lw      s0, 40(sp)
    lw      s1, 36(sp)
    lw      s2, 32(sp)
    lw      s3, 28(sp)
    lw      s4, 24(sp)
    lw      s5, 20(sp)
    lw      s6, 16(sp)
    lw      s7, 12(sp)
    addi    sp, sp, 48
    ret

# Prefix DFS. a0/a1: cube, a2: depth left, a3: previous face, a4: path.
# s0..s4 retain these arguments; s5/s6: face/turn, s7/s8: trial cube.
.Lsearch:
    beqz    a2, .Lleaf
    addi    sp, sp, -48
    sw      ra, 44(sp)
    sw      s0, 40(sp)
    sw      s1, 36(sp)
    sw      s2, 32(sp)
    sw      s3, 28(sp)
    sw      s4, 24(sp)
    sw      s5, 20(sp)
    sw      s6, 16(sp)
    sw      s7, 12(sp)
    sw      s8, 8(sp)
    mv      s0, a0
    mv      s1, a1
    mv      s2, a2
    mv      s3, a3
    mv      s4, a4
    li      s5, 0
.Lsearch_face:
    beq     s5, s3, .Lsearch_next_face
    li      s6, 0
    mv      s7, s0
    mv      s8, s1
.Lsearch_turn:
    mv      a0, s7
    mv      a1, s8
    mv      a2, s5
    call    .Lquarter
    mv      s7, a0
    mv      s8, a1
    addi    a2, s2, -1
    mv      a3, s5
    addi    a4, s4, 1
    call    .Lsearch
    bgez    a0, .Lsearch_accept
    addi    s6, s6, 1
    sltiu   t0, s6, 3
    bnez    t0, .Lsearch_turn
.Lsearch_next_face:
    addi    s5, s5, 1
    sltiu   t0, s5, 3
    bnez    t0, .Lsearch_face
    li      a0, -1
    j       .Lsearch_return
.Lsearch_accept:
    add     t0, s5, s5
    add     t0, t0, s5
    add     t0, t0, s6
    sb      t0, 0(s4)
    addi    a0, a0, 1
.Lsearch_return:
    lw      ra, 44(sp)
    lw      s0, 40(sp)
    lw      s1, 36(sp)
    lw      s2, 32(sp)
    lw      s3, 28(sp)
    lw      s4, 24(sp)
    lw      s5, 20(sp)
    lw      s6, 16(sp)
    lw      s7, 12(sp)
    lw      s8, 8(sp)
    addi    sp, sp, 48
    ret
.Lleaf:
    mv      a6, ra
    call    .Lnear
    mv      ra, a6
    bgez    a5, .Lleaf_suffix
    mv      a0, a5
    ret
.Lleaf_suffix:
    addi    sp, sp, -16
    sw      ra, 12(sp)
    sw      a5, 8(sp)
    mv      a2, a5
    call    .Lsuffix
    lw      a0, 8(sp)
    lw      ra, 12(sp)
    addi    sp, sp, 16
    ret

    .section .rodata
.Lfaces:
    .ascii "RRRBBBDDD"
.Lendings:
    .byte 0, 50, 39, 0, 50, 39, 0, 50, 39
