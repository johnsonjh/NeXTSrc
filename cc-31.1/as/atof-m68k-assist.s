	.globl	_packed_decimal_float_to_single
_packed_decimal_float_to_single:
	movl	sp@(4),a0	| the packed decimal pointer
	movl	sp@(8),a1	| float storage pointer
	.word	0xf210,0x4c00	| fmovep	a0@,fp0
	fmoves	fp0,a1@
	rts


	.globl	_packed_decimal_float_to_double
_packed_decimal_float_to_double:
	movl	sp@(4),a0	| the packed decimal pointer
	movl	sp@(8),a1	| float storage pointer
	.word	0xf210,0x4c00	| fmovep	a0@,fp0
	fmoved	fp0,a1@
	rts


	.globl	_packed_decimal_float_to_extended
_packed_decimal_float_to_extended:
	movl	sp@(4),a0	| the packed decimal pointer
	movl	sp@(8),a1	| float storage pointer
	.word	0xf210,0x4c00	| fmovep	a0@,fp0
	fmovex	fp0,a1@
	rts


