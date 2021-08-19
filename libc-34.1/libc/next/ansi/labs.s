#include "cframe.h"

PROCENTRY(labs)
	movl	a_p0,d0
	bpl	1f
	negl	d0
1:
	rts
