// memmove(dst, src, n)

	.globl	_memmove
_memmove:
	movl	sp@(4),a1	// a1 = destination address
	movl	sp@(8),a0	// a0 = source address
	movl	sp@(12),d0	// d0 = length
	jle	5f		// length <= 0!

	cmpl	a0,a1
	jhi	rcopy		// gotta do reverse copy
	jeq	5f		// src == dst!

	movl	a1,d1		// compute dst - (dst & 3)
	negl	d1
	andl	#3,d1
	jra	2f

1:	movb	a0@+,a1@+	// copy bytes to get dst to long word boundary
	subql	#1,d0
2:	dbeq	d1,1b		// byte count or alignment count exhausted

	movl	d0,d1		// 4 bytes moved per 2 byte instruction
	andl	#0x1c,d1	// so instruction offset is:
	lsrl	#1,d1		// 2 * (k - n % k)
	negl	d1
	addl	#18,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

3:	movl	a0@+,a1@+	// copy next <k> longs
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+

	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+

	subl	#32,d0		// decrement loop count by k*4
	jge	3b

	btst	#1,d0		// copy last short
	jeq	4f
	movw	a0@+,a1@+

4:	btst	#0,d0		// copy last byte
	jeq	5f
	movb	a0@,a1@

5:	movl	sp@(4),d0	// return destination
	rts			// that's all folks!

/*
 * Reverse copy
 */
rcopy:	addl	d0,a0		// point at end of source
	addl	d0,a1		// point at end of destination
	movl	a1,d1		// compute (dst & 3)
	andl	#3,d1
	jra	2f

1:	movb	a0@-,a1@-	// copy bytes to get src to long word boundary
	subql	#1,d0
2:	dbeq	d1,1b		// byte count or alignment count exhausted

	movl	d0,d1		// 4 bytes moved per 2 byte instruction
	andl	#0x1c,d1	// so instruction offset is:
	lsrl	#1,d1		// 2 * (k - n % k)
	negl	d1
	addl	#18,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

3:	movl	a0@-,a1@-	// copy next <k> longs
	movl	a0@-,a1@-
	movl	a0@-,a1@-
	movl	a0@-,a1@-

	movl	a0@-,a1@-
	movl	a0@-,a1@-
	movl	a0@-,a1@-
	movl	a0@-,a1@-

	subl	#32,d0		// decrement loop count by k*4
	jge	3b

	btst	#1,d0		// copy last short
	jeq	4f
	movw	a0@-,a1@-

4:	btst	#0,d0		// copy last byte
	jeq	5f
	movb	a0@-,a1@-

5:	movl	sp@(4),d0	// return destination
	rts			// that's all folks!
