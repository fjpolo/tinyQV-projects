.section .text

.extern front_buffer

.globl tqv_user_interrupt04_raw
tqv_user_interrupt04_raw:
    sw4 x12, -0x1f4(gp)  # Save x12-x15 to gp-0x1f4 (following on from save context in isr_entry)
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
    
    .2byte 0x3702       # Load context, x9-x15 from gp-0x200
    mret
