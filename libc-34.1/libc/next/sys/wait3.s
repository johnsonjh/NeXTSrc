#include "SYS.h"

/*
 * C library -- wait3
 *
 * pid = wait3(&status, flags, &rusage);
 *
 * pid == -1 if error
 * status indicates fate of process, if given
 * flags may indicate process is not to hang or
 * that untraced stopped children are to be reported.
 * rusage optionally rtsurns detailed resource usage information
 */

#define	SYS_wait3	171

SYSCALL(wait3, 3)
	tstl	a_p0
	beq	1f
	movl	a_p0,a0
	movl	d1,a0@
1:	rts
