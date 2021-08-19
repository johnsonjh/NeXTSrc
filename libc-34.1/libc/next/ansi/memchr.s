// memchr(src, c, n)

	.globl	_memchr
_memchr:
	movl	sp@(12),d0	// d0 = number of chars to check
	jeq	3f
	movl	d2,a1		// save d2 in a1
	movl	sp@(4),a0	// a0 = destination address
	movl	sp@(8),d2	// d2 = char to find

	movl	d0,d1		// 1 byte compared per 4 bytes of instruction
	andl	#0x3,d1
	lsll	#2,d1
	negl	d1
	addl	#18,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

1:	cmpb	a0@+,d2		// compare bytes
	beq	2f
	cmpb	a0@+,d2
	beq	2f
	cmpb	a0@+,d2
	beq	2f
	cmpb	a0@+,d2
	beq	2f

	subql	#4,d0
	jge	1b

	movql	#0,d0		// didn't find it
	movl	a1,d2
	rts

2:	movl	a0,d0
	subql	#1,d0
	movl	a1,d2
3:	rts
