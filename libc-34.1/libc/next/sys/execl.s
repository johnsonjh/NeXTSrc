#include "SYS.h"

PROCENTRY(execl)
	movl	a_p0,a0
	pea	a_p1
	movl	a0,sp@-
	jsr	_execv
	addql	#8,sp
	rts		| execl(file, arg1, arg2, ..., 0);
