 # $Header: fork.c,v 5.0 86/02/01 01:10:32 ibmacis ibm42a $
 # $Source: /ibm/acis/usr/src/lib/libc/_ca/ca/sys/RCS/fork.c,v $

/* changed SYS.ibm-rt.h to SYS.ibmrt.h   mrt Dec 5  */
/* Fixed to save return address/check correct return value -bjb Jan 13 '87 */

#include "SYS.ibmrt.h"
SYSCODE(fork)
	SVC(fork)
	btb	syserror
	cis	r0,0		/* r0 == 0 in parent, 1 in child */
	ber	r15
	ai	r1,r1,-4	# allocate stack space for return address
	st	r15,0(r1)	# save return address
	get	r0,$_mach_init
	bali	r15,_.mach_init
	l	r15,0(r1)	# restore return address from stack
	ai	r1,r1,4		# deallocate stack space
	brx	r15
	lis	r2,0
syserror:
	get	r5,$_errno
	sts	r2,0(r5)        #update error number
	get	r2,$-1		#indicate error to C caller
	br	r15             #return to caller
	TTNOFRM



