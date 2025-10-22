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
	li	a5,134217728
	addi	a5,a5,544
	li	a4,1
.L2:
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	sw	a4,0(a5)
	j	.L2
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
