#include "SYS.h"

#ifdef SHLIB
	.globl	__libsys_environ
#else
	.globl	_environ
#endif

PROCENTRY(execv)
#ifdef SHLIB
	movl	__libsys_environ,a0
	movl	a0@, sp@-
#else
	movl	_environ,sp@-
#endif
	movl	sp@(0xc),sp@-
	movl	sp@(0xc),sp@-
	jsr	_execve
	lea	sp@(12),sp
	rts			| execv(file, argv)
