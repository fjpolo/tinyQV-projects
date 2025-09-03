.section .text

.extern front_buffer

.globl tqv_user_interrupt04
tqv_user_interrupt04:
    lbu a4, 0x103(tp)
    lbu a5, 0x102(tp)
    li a3, 48
    mul16 a4, a4, a3
    add a5, a5, a4
    addi a5, a5, 1
    slti a4, a5, 768
    li a3, 1020
    mul16 a4, a4, a3
    and a5, a5, a4
    slli a5, a5, 0x4

    lbu x0, 0x101(tp)

    la a4, front_buffer
    lw a4, (a4)
    add a4, a4, a5

    lw4 a0, (a4)
    sw4 a0, 0x100(tp)
    lw4 a0, 0x10(a4)
    sw4 a0, 0x110(tp)
    lw4 a0, 0x20(a4)
    sw4 a0, 0x120(tp)
    lw4 a0, 0x30(a4)
    sw4 a0, 0x130(tp)
    ret
