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
	addi	sp,sp,-4
	sw	ra,0(sp)
	call	rv2a03_peripheral_init
.L3:
	call	rv2a03_peripheral_init
	li	a5,999424
	addi	a5,a5,576
.L2:
	addi	a5,a5,-1
	bne	a5,zero,.L2
	j	.L3
	.size	main, .-main
	.globl	a
	.section	.sdata.a,"aw"
	.align	2
	.type	a, @object
	.size	a, 4
a:
	.word	3
	.ident	"GCC: () 15.1.0"
	.section	.note.GNU-stack,"",@progbits
