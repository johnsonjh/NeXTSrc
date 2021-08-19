#include "SYS.h"

SYSCALL(pipe, 0)
	movl	a_p0,a0
	movl	d0,a0@+
	movl	d1,a0@+
	clrl	d0
	rts
