#include "SYS.h"

PSEUDO(geteuid,getuid,0)
	movl	d1,d0
	rts
