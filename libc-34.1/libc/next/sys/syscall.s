#include "SYS.h"

	.globl	cerror

PROCENTRY(syscall)
	movl	sp,a0;
	addql	#4,a0;
	movl	a0@,d0;			| code
	save_registers_6;
	kernel_trap_args_6;
	trap	#4;
	jcc	1f;
	jsr	cerror;
1:	restore_registers_6;
	rts

