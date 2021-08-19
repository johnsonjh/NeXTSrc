#include "syscall.h"
#include "cframe.h"
#include <next/syscall_sw.h>

#define	SYSCALL(name, nargs)		\
PROCENTRY(name);			\
	movl	sp,a0;			\
	save_registers_##nargs;		\
	kernel_trap_args_##nargs;	\
	movl	\#SYS_##name,d0;	\
	trap	\#4;			\
	jcc	1f;			\
	jsr	cerror;			\
	restore_registers_##nargs;	\
	rts;				\
1:	restore_registers_##nargs;

#define	SYSCALL_NONAME(name, nargs)	\
	movl	sp,a0;			\
	save_registers_##nargs;		\
	kernel_trap_args_##nargs;	\
	movl	\#SYS_##name,d0;	\
	trap	\#4;			\
	jcc	1f;			\
	jsr	cerror;			\
	restore_registers_##nargs;	\
	rts;				\
1:	restore_registers_##nargs;

#define	PSEUDO(pseudo, name, nargs)		\
PROCENTRY(pseudo);				\
SYSCALL_NONAME(name, nargs);
