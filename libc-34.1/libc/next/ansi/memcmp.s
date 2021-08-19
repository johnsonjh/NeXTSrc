// memcmp(src1, src2, n)

	.globl	_memcmp
_memcmp:
	movl	sp@(4),a0	// a0 = source1 address
	movl	sp@(8),a1	// a1 = source2 address
	movl	sp@(12),d0	// d0 = length
	jle	5f		// length <= 0!

	cmpl	a0,a1
	jeq	5f		// src1 == src2

	movl	d0,d1		// 4 bytes moved per 4 byte instruction pair
	andl	#0x1c,d1	// so instruction offset is:
	negl	d1
	addl	#34,d1		// + fudge term of 2 for indexed jump
	jmp	pc@(0,d1)	// now dive into middle of unrolled loop

3:	cmpml	a0@+,a1@+	// cmp next <k> longs
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f

	cmpml	a0@+,a1@+
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f
	cmpml	a0@+,a1@+
	bne	7f

	subl	#32,d0		// decrement loop count by k*4
	jge	3b

	btst	#1,d0		// cmp last short
	jeq	4f
	cmpmw	a0@+,a1@+
	bne	8f

4:	btst	#0,d0		// copy last byte
	jeq	5f
	cmpmb	a0@+,a1@+
	bne	6f

5:	movql	#0,d0		// mem contents equal
	rts			// that's all folks!

7:	subql	#4,a0
	subql	#4,a1
	cmpmb	a0@+,a1@+
	bne	6f
	cmpmb	a0@+,a1@+
	bne	6f
	cmpmb	a0@+,a1@+
	bne	6f
	bra	9f

8:	subql	#2,a0
	subql	#2,a1
	cmpmb	a0@+,a1@+
	bne	6f
	bra	9f

6:	subql	#1,a0
	subql	#1,a1

9:	movb	a0@,d0
	extbl	d0
	movb	a1@,d1
	extbl	d1
	subl	d1,d0
	rts
