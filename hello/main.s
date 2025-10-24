	.file	"main.c"
	.option nopic
	.attribute arch, "rv32e2p0_c2p0_zca1p0_zcb1p0"
	.attribute unaligned_access, 1
	.attribute stack_align, 4
	.text
	.section	.text.startup.main,"ax",@progbits
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-8
	sw	s0,0(sp)
	sw	ra,4(sp)
	lui	s0,%hi(tx_char)
.L2:
	li	a5,65
	sb	a5,%lo(tx_char)(s0)
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	lbu	a0,%lo(tx_char)(s0)
	call	uart_putc
	j	.L2
	.size	main, .-main
	.globl	tx_char
	.section	.sbss.tx_char,"aw",@nobits
	.type	tx_char, @object
	.size	tx_char, 1
tx_char:
	.zero	1
	.globl	a
	.section	.sdata.a,"aw"
	.align	2
	.type	a, @object
	.size	a, 4
a:
	.word	3
	.ident	"GCC: () 15.1.0"
	.section	.note.GNU-stack,"",@progbits
