#include "SYS.h"

PROCENTRY(execle)
	movl	a_p0,d0
	lea	a_p1,a1
	movl	a1,d1
1:	tstl	a1@+
	bne	1b
	movl	a1@,sp@-
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_execve
	lea	sp@(12),sp
	rts		| execle(file, arg1, arg2, ..., env);
