#include "SYS.h"

PSEUDO(getppid,getpid,0)
	movl	d1,d0
	rts
