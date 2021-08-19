#include "SYS.h"

PROCENTRY(ptrace)
	clrl	_errno
SYSCALL_NONAME(ptrace, 4)
	rts
