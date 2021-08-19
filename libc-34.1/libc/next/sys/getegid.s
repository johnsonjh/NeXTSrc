#include "SYS.h"

PSEUDO(getegid,getgid,0)
	movl	d1,d0
	rts
