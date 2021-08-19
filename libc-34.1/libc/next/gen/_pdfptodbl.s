	.globl	__pdfptodbl
__pdfptodbl:
	movl	sp@(4),a0
	.word	0xf210,0x4c00	| fmovep	a0@,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	rts
