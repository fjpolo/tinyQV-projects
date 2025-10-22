	.file	"rv2a03.c"
	.option nopic
	.attribute arch, "rv32e2p0_c2p0_zca1p0_zcb1p0"
	.attribute unaligned_access, 1
	.attribute stack_align, 4
	.text
	.section	.text.rv2a03_peripheral_init,"ax",@progbits
	.align	1
	.globl	rv2a03_peripheral_init
	.type	rv2a03_peripheral_init, @function
rv2a03_peripheral_init:
	li	a5,134217728
	li	a4,1
	sw	a4,544(a5)
	ret
	.size	rv2a03_peripheral_init, .-rv2a03_peripheral_init
	.section	.text.rv2a03_peripheral_deinit,"ax",@progbits
	.align	1
	.globl	rv2a03_peripheral_deinit
	.type	rv2a03_peripheral_deinit, @function
rv2a03_peripheral_deinit:
	li	a4,134217728
	sw	zero,544(a4)
	sh	zero,546(a4)
	ret
	.size	rv2a03_peripheral_deinit, .-rv2a03_peripheral_deinit
	.section	.text.rv2a03_peripheral_read_configuration0,"ax",@progbits
	.align	1
	.globl	rv2a03_peripheral_read_configuration0
	.type	rv2a03_peripheral_read_configuration0, @function
rv2a03_peripheral_read_configuration0:
	li	a5,134217728
	lbu	a0,544(a5)
	slli	a0,a0,24
	srai	a0,a0,24
	ret
	.size	rv2a03_peripheral_read_configuration0, .-rv2a03_peripheral_read_configuration0
	.ident	"GCC: () 15.1.0"
	.section	.note.GNU-stack,"",@progbits
