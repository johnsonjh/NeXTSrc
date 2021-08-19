/*
 * Profiling code uses this to reserve space for profiling data
 */
	.text
	.globl	__setminbrk
__setminbrk:
	movl	sp@(4),minbrk
	rts
