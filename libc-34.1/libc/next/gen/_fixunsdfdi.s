	.text
	.globl	__fixunsdfdi
__fixunsdfdi:
	movl	sp@(4),d0
	movl	sp@(8),d1
	moveml	d0/d1,sp@-
	cmpl	#0x41e00000,d0		| 2^31 (double precision)
	bges	L1
	fintrzd	sp@+,fp0
	fmovel	fp0,d0
	bras	L2
L1:
	fmoved	sp@+,fp0
	fsubs	#0x4f000000,fp0		| 2^31 (single precision)
	fintrzx	fp0,fp0
	fmovel	fp0,d0
	bchg	#0x1f,d0
L2:
	moveq	#0,d1
	rts
