#include "SYS.h"

SYSCALL(wait, 0)
	tstl	a_p0
	beq	1f
	movl	a_p0,a0
	movl	d1,a0@
1:	rts
