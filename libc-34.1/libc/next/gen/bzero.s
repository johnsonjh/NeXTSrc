/* bzero(s1, n) */

	.globl	_bzero
_bzero: movl	sp@(4),a0	// a0 = destination address
	movl	sp@(8),d0	// d0 = length
	jle	5f		// length <= 0!

	movl	a0,d1		// compute dst - (dst & 3)
	negl	d1
	andl	#3,d1
	jra	2f

1:	clrb	a0@+		// clear bytes to get dst to long word boundary
	subql	#1,d0
2:	dbeq	d1,1b		// byte count or alignment count exhausted

	movl	d0,d1		// 4 bytes moved per 2 byte instruction
	andl	#0x1c,d1	// so instruction offset is:
	lsrl	#1,d1		// 2 * (k - n % k)
	negl	d1
	addl	#18,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

3:	clrl	a0@+		// clear next <k> longs
	clrl	a0@+
	clrl	a0@+
	clrl	a0@+

	clrl	a0@+
	clrl	a0@+
	clrl	a0@+
	clrl	a0@+

	subl	#32,d0		// decrement loop count by k*4
	jge	3b

	btst	#1,d0		// clear last short
	jeq	4f
	clrw	a0@+

4:	btst	#0,d0		// clear last byte
	jeq	5f
	clrb	a0@

5:	rts			// thats all folks!

