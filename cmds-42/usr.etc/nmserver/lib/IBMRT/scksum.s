#
# Mach Operating System
# Copyright (c) 1987 Carnegie-Mellon University
# All rights reserved.  The CMU software License Agreement specifies
# the terms and conditions for use and redistribution.
#
	#
	# Fast routines for Internet checksum - IBMRT version.
 	#

	.globl	.oVncs
	.set	.oVncs, 0

	#
	# Simple checksum of a block of data.
	#
	# Callable from C as
	# 	int scksum(start,len)
	#	char *start;	/* start of block to checksum */
	#	int  len;	/* number of bytes in block */
	#
	# Notes: the start address must be aligned on a 16-bit boundary.
	# 	 the number of bytes must be even.
	#
	# On entry:
	#	r2 = start
	#	r3 = len
	#
	# On exit:
	#	r2 = checksum
	#
  	.data
	.globl	_scksum
	.globl	_.scksum
_scksum:
	.long	_.scksum
  	.long	0
  	.long	0

  	.text
_.scksum:

	#
	# Check for zero length
	#
	cis	r3,0
	beq	return

	st	r4,-4(r1)
	#
	# Take care of a possible short word at the head, to get to
	# a 32-bit boundary.
	#
	nilz	r4,r2,2		# check starting address
	bz	after_head
	lhs	r4,0(r2)	# get first short word
	ais	r2,2
	sis	r3,2
after_head:
	s	r5,r5		# clear r5
	a	r5,r5		# clear carry

	#
	# Iterate over all long words.
	#
	cis	r3,4	
	bl	after_long	# if no long words, skip
loop_long:
	ls	r5,0(r2)	# get one long word
	dec	r3,4		# one less word to do
	ae	r4,r5		# add it to checksum
	cis	r3,4
	bhex	loop_long
		inc	r2,4	# update address

	#
	# Take care of a possible short word at the tail.
	#
after_long:
	nilz	r3,r3,2		# r3 = number of remaining bytes
	bz	after_tail
	lhs	r5,0(r2)	# get last short word
	ae	r4,r5

after_tail:
	#
	# Fold to 16 bits and fix all the carries.
	#
	aei	r4,r4,0		# add the last carry
    	aei	r4,r4,0		# add the very last carry
	nilz	r2,r4,0xffff	# r2 = lower bits of r4
	sri16	r4,0		# r4 = upper bits of r4
	a	r2,r4
  	nilz	r4,r2,0xffff
  	sri16	r2,0
  	a	r2,r4
  #	nilz	r4,r2,0xffff
  #	sri16	r2,0
  #	a	r2,r4
	nilz	r2,r2,0xffff

	l	r4,-4(r1)	# restore r4
return:
	br	r15		# return

	.long	0xdf02df00	# trace table
