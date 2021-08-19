/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)atof.s	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

#include "cframe.h"

/*
 *	atof: convert ascii to floating
 *
 *	C usage:
 *
 *		double atof (s)
 *		char *s;
 *
 *	Register usage:
 *
 *		d0,d1:	value being developed
 *		a0:	first section: pointer to the next character
 *		a1:	second section: scratch
 *		d2:	flags
 *		d3:	first section: the current character
 *		d3:	second section: scratch
 *		d4:	first section: scratch
 *		d4:	second section: binary exponent
 *		d5:	the decimal exponent
 *		d6,d7:	scratch
 */

#define	MSIGN	1		/* mantissa has negative sign */
#define	ESIGN	2		/* exponent has negative sign */
#define	DECPT	3		/* decimal point encountered */

#define	ELIM	1023		/* abs max exponent accepted */

PROCENTRY(atof)
	link	a6,#-48
	moveml	#0xfc,a6@(-24)	| save d7-d2
	fmovem	#3,a6@(-48)	| save fp6 - fp7
/*
 *	Initialization
 */
	moveq	#0,d2		| All flags start out false
	movl	c_p0,a0		| Address the first character
	moveq	#0,d5		| Clear starting exponent
/*
 *	Skip leading white space
 */
sk0:
	moveq	#0,d3
	movb	a0@+,d3		| Fetch the next (first) character
	cmpb	#0x20,d3	| Is it blank?
	beq	sk0		|   ...yes
	cmpb	#8,d3		| 8 is lowest of white-space group
	blt	sk1		| Jump if char too low to be white space
	cmpb	#13,d3		| 13 is highest of white-space group
	ble	sk0		| Jump if character is white space
sk1:
/*
 *	Check for a sign
 */
	cmpb	#0x2b,d3	| Positive sign?
	beq	cs1		|   ... yes
	cmpb	#0x2d,d3	| Negative sign?
	bne	cs2		|   ... no
	bset	#MSIGN,d2	| Indicate a negative mantissa
cs1:
	moveq	#0,d3
	movb	a0@+,d3		| Skip the character
cs2:
/*
 *	Accumulate digits, keeping track of the exponent
 */
	moveq	#0,d0		| Clear the accumulator
	moveq	#0,d1		| Clear the accumulator
ad0:	cmpb	#0x30,d3	| Do we have a digit?
	blt	ad4		|   ... no, too small
	cmpb	#0x39,d3
	bgt	ad4		|   ... no, too large
/*
 *	We got a digit.  Accumulate it
 */
	cmpl	#214748364,d1	| Would this digit cause overflow?
	bge	ad1		|   ... yes
/*
 *	Multiply (d0,d1) by 10.  This is done by developing
 *	(d0,d1)*2 in (d6,d7), shifting (d0,d1) left three bits,
 *	and adding the two quadwords.
 */
	movl	d0,d6
	movl	d1,d7
	asll	#1,d6
	roxll	#1,d7		| (d6,d7)<<1
	moveq	#2,d4
1:
	asll	#1,d0
	roxll	#1,d1
	dbra	d4,1b		| (d6,d7)<<3

	addl	d6,d0		| Add low halves
	nop			| 040 chip bug workaround
	addxl	d7,d1		| Add high halves
/*
 *	Add in the digit
 */
	subl	#0x30,d3	| Get the digit value
	moveq	#0,d4
	addl	d3,d0		| Add it into the accumulator
	nop			| 040 chip bug workaround
	addxl	d4,d1		| Possible carry into high half
	bra	ad2		| Join common code
/*
 *	Here when the digit won't fit in the accumulator
 */
ad1:	addql	#1,d5		| Ignore the digit, bump exponent
/*
 *	If we have seen a decimal point, decrease the exponent by 1
 */
ad2:
	btst	#DECPT,d2
	beq	ad3		| Jump if decimal point not seen
	subql	#1,d5		| Decrease exponent
ad3:
/*
 *	Fetch the next character, back for more
 */
	moveq	#0,d3
	movb	a0@+,d3		| Fetch
	bra	ad0		| Try again
/*
 *	Not a digit.  Could it be a decimal point?
 */
ad4:	cmpb	#0x2e,d3	| If its not a decimal point, either its
	bne	ad5		|   the end of the number or the start of
				|   the exponent.
	bset	#DECPT,d2
	beq	ad3		| If it IS a decimal point, we record that
				|   weve seen one, and keep collecting
				|   digits if it is the first one.
/*
 *	Check for an exponent
 */
ad5:	moveq	#0,d6		| Initialize the exponent accumulator

	cmpb	#0x65,d3	| We allow both lower case e
	beq	ex1		|   ... and ...
	cmpb	#0x45,d3	|   upper-case E
	bne	ex7
/*
 *	Does the exponent have a sign?
 */
ex1:
	moveq	#0,d3
	movb	a0@+,d3		| Get next character
	cmpb	#0x2b,d3	| Positive sign?
	beq	ex2		|   ... yes ...
	cmpb	#0x2d,d3	| Negative sign?
	bne	ex3		|   ... no ...
	bset	#ESIGN,d2	| Indicate exponent is negative
ex2:
	moveq	#0,d3
	movb	a0@+,d3		| Grab the next character
/*
 *	Accumulate exponent digits in d6
 */
ex3:	cmpb	#0x30,d3	| A digit is within the range
	blt	ex4		| '0' through
	cmpb	#0x39,d3	| '9',
	bgt	ex4		| inclusive.
	cmpl	#214748364,d6	| Exponent outrageously large already?
	bge	ex2		|   ... yes
	movl	d6,d4
	addl	d6,d6
	asll	#3,d4
	addl	d4,d6		| d6 *= 10
	subl	#0x30,d3
	addl	d3,d6		| d6 += d3 - '0'
	bra	ex2		| Go round again
ex4:
/*
 *	Now get the final exponent and force it within a reasonable
 *	range so our scaling loops don't take forever for values
 *	that will ultimately cause overflow or underflow anyway.
 *	A tight check on over/underflow will be done by ldexp.
 */
	btst	#ESIGN,d2
	beq	ex5		| Jump if exponent not negative
	negl	d6		| If sign, negate exponent
ex5:	addl	d6,d5		| Add given exponent to calculated exponent
	cmpl	#-ELIM,d5	| Absurdly small?
	bgt	ex6		|   ... no
	movl	#-ELIM,d5	|   ... yes, force within limit
ex6:	cmpl	#ELIM,d5	| Absurdly large?
	blt	ex7		|   ... no
	movl	#ELIM,d5	|   ... yes, force within bounds
ex7:
/*
 *	Our number has now been reduced to a mantissa and an exponent.
 *	The mantissa is a 63-bit positive binary integer in d0,d1,
 *	and the exponent is a signed power of 10 in d5.  The msign
 *	bit in d2 will be on if the mantissa should ultimately be
 *	considered negative.
 *
 *	We now have to convert it to a standard format floating point
 *	number.  This will be done by accumulating a binary exponent
 *	in d4, as we progressively get d5 closer to zero.
 *
 *	Don't bother scaling if the mantissa is zero
 */
	tstl	d0		| Mantissa zero?
	bne	1f
	tstl	d1
	beq	exit		|   ... yes
1:

	moveq	#0,d4		| Initialize binary exponent
	tstl	d5		| Which way to scale?
	ble	sd0		| Scale down if decimal exponent <= 0
/*
 *	Scale up by "multiplying" d0,d1 by 10 as many times as necessary,
 *	as follows:
 *
 *	Step 1: Shift d0,d1 right as necessary to ensure that no
 *	overflow can occur when multiplying.
 */
su0:	cmpl	#429496729,d1	| Compare high word to (2**31)/5
	blt	su1		| Jump out if guaranteed safe
	asrl	#1,d1
	roxrl	#1,d0
	addql	#1,d4		|    bump exponent to compensate
	bra	su0		|    and go back to test again.
/*
 *	Step 2: Multiply d0,d1 by 5, by appropriate shifting and
 *	double-precision addition
 */
su1:	movl	d0,d6
	movl	d1,d7
	asll	#1,d6
	roxll	#1,d7
	asll	#1,d6
	roxll	#1,d7		| (d6,d7)<<2
	addl	d6,d0		| Add low-order halves
	nop			| 040 chip bug workaround
	addxl	d7,d1		|   and high-order halves
/*
 *	Step 3: Increment the binary exponent to take care of the final
 *	factor of 2, and go back if we still need to scale more.
 */
	addql	#1,d4		| Increment the exponent
	subql	#1,d5
	bgt	su0		|    and back for more (maybe)
	bra	cm0		| Merge to build final value

/*
 *	Scale down.  We must "divide" d0,d1 by 10 as many times
 *	as needed, as follows:
 *
 *	Step 0: Right now, the condition codes reflect the state
 *	of d5.  If it's zero, we are done.
 */
sd0:	beq	cm0		| If finished, build final number
/*
 *	Step 1: Shift d0,d1 left until the high-order bit (not counting
 *	the sign bit) is nonzero, so that the division will preserve
 *	as much precision as possible.
 */
	tstl	d1		| Is the entire high-order half zero?
	bne	sd2		|   ...no, go shift one bit at a time
	movl	d4,a1		| save d4
	movl	#29,d4		|   ...yes, shift left 30,
1:
	asll	#1,d0
	roxll	#1,d1
	dbra	d4,1b		| (d0,d1)<<30
	movl	a1,d4
	subl	#30,d4		|   decrement the exponent to compensate,
				|   and now its known to be safe to shift
				|   at least once more.
sd1:	asll	#1,d0		| Shift (d0,d1) left one, and
	roxll	#1,d1
	subql	#1,d4		|   decrement the exponent to compensate
sd2:	btst	#30,d1
	beq	sd1		| If the high-order bit is off, go shift
/*
 *	Step 2: Divide the high-order part of (d0,d1) by 5,
 *	giving a quotient in d1 and a remainder in d7.
 */
sd3:	moveq	#0,d7		| Zero-extend to 64 bits
	divsl	#5,d7:d1	| Divide (cannot overflow)
/*
 *	Step 3: Divide the low-order part of (d0,d1) by 5,
 *	using the remainder from step 2 for rounding.
 *	Note that the result of this computation is unsigned,
 *	so we have to allow for the fact that an ordinary division
 *	by 5 could overflow.  We make allowance by dividing by 10,
 *	multiplying the quotient by 2, and using the remainder
 *	to adjust the modified quotient.
 */
	movl	d0,d6
	moveq	#0,d0
	addql	#2,d6		| Dividend is low part of (d0,d1) plus
	nop			| 040 chip bug workaround
	addxl	d0,d7		|  2 for rounding plus
				|  (2**32) * previous remainder
	divsl	#10,d7:d6
	movl	d6,d0		| d0 := quotient, d7 := remainder.
	addl	d0,d0		| Make d0 result of dividing by 5
	cmpl	#5,d7		| If remainder is 5 or greater,
	bls	sd4		|   increment the adjustted quotient.
	addql	#1,d0
/*
 *	Step 4: Increment the decimal exponent, decrement the binary
 *	exponent (to make the division by 5 into a division by 10),
 *	and back for another iteration.
 */
sd4:	subql	#1,d4		| Binary exponent
	addql	#1,d5
	blt	sd2		| Still less than 0
/*
 *	We now have the following:
 *
 *	d0:	low-order half of a 64-bit integer
 *	d1:	high-order half of the same 64-bit integer
 *	d4:	a binary exponent
 *
 *	Our final result is the integer represented by (d0,d1)
 *	multiplied by 2 to the power contained in d4.
 *	We will transform (d0,d1) into a floating-point value,
 *	set the sign appropriately, and let ldexp do the
 *	rest of the work.
 *
 *	Step 1: if the high-order bit (excluding the sign) of
 *	the high-order half (d1) is 1, then we have 63 bits of
 *	fraction, too many to convert easily.  However, we also
 *	know we won't need them all, so we will just throw the
 *	low-order bit away (and adjust the exponent appropriately).
 */
cm0:	btst	#30,d1
	beq	cm1		| jump if no adjustment needed
	asrl	#1,d1
	roxrl	#1,d0
	addql	#1,d4		| increase the exponent to compensate
/*
 *	Step 2: split the 62-bit number in (d0,d1) into two
 *	31-bit positive quantities
 */
cm1:	asll	#1,d0		| put the high-order bits in d1
	roxll	#1,d1		|   and a 0 in the bottom of d0
	lsrl	#1,d0		| right-justify the bits in d0
				|   and zero the sign bit.
/*
 *	Step 3: convert both halves to floating point
 */
	fmovel	d0,fp6		| low-order part in fp6
	fmovel	d1,fp7		| high-order part in fp7
/*
 *	Step 4: multiply the high order part by 2**31 and combine them
 */
	fmuld	two31,fp7	| multiply
	faddx	fp6,fp7		| combine
	fnop			| 040 chip bug workaround
	nop			| 040 chip bug workaround
/*
 *	Step 5: if appropriate, negate the floating value
 */
	btst	#MSIGN,d2
	beq	cm2		| Jump if mantissa not signed
	fnegx	fp7,fp7		| If negative, make it so
/*
 *	Step 6: call ldexp to complete the job
 */
cm2:	movl	d4,sp@-		| Put exponent in parameter list
	fmoved	fp7,sp@-	|    and also mantissa
	jsr	_ldexp		| go combine them
	lea	sp@(12),sp

exit:
	fmovem	a6@(-48),#3	| save fp6 - fp7
	moveml	a6@(-24),#0xfc	| save d7-d2
	unlk	a6
	rts

	.even
two31:	.long	0x41e00000,0	| 2 ** 31 (=2147483648) in floating-point
