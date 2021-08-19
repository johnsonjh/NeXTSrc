// memset(dst, c, n)

	.globl	_memset
_memset:
	movl	d2,a0		// save d2 in a0
	movl	sp@(4),a1	// a1 = destination address
	movl	sp@(8),d2	// d2 = char to store
	andl	#0xff,d2
	jeq	6f
	movl	d2,d0		// expand c to cccc
	lsll	#8,d0
	orl	d0,d2		// d2 = cc
	movl	d2,d0
	swap	d0
	orl	d0,d2
6:	movl	sp@(12),d0	// d0 = length
	jle	5f		// length <= 0!

	movl	a1,d1		// compute dst - (dst & 3)
	negl	d1
	andl	#3,d1
	jra	2f

1:	movb	d2,a1@+		// store bytes to get dst to long word boundary
	subql	#1,d0
2:	dbeq	d1,1b		// byte count or alignment count exhausted

	movl	d0,d1		// 4 bytes moved per 2 byte instruction
	andl	#0x1c,d1	// so instruction offset is:
	lsrl	#1,d1		// 2 * (k - n % k)
	negl	d1
	addl	#18,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

3:	movl	d2,a1@+		// store pattern in next <k> longs
	movl	d2,a1@+
	movl	d2,a1@+
	movl	d2,a1@+

	movl	d2,a1@+
	movl	d2,a1@+
	movl	d2,a1@+
	movl	d2,a1@+

	subl	#32,d0		// decrement loop count by k*4
	jge	3b

	btst	#1,d0		// copy last short
	jeq	4f
	movw	d2,a1@+

4:	btst	#0,d0		// copy last byte
	jeq	5f
	movb	d2,a1@

5:	movl	sp@(4),d0	// return destination
	movl	a0,d2		// restore d2
	rts			// that's all folks!
