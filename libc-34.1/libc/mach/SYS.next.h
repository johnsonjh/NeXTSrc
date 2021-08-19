#include <syscall.h>
#include "next/syscall_sw.h"

#ifdef PROF
	.globl  mcount

#define	PROCENTRY(x)				\
	.globl	_##x				;\
_##x:	link	a6,\#0				;\
	lea	x##1,a0				;\
	.data					;\
x##1:	.long	0				;\
	.text					;\
	jsr	mcount				;\
	unlk	a6

#else !PROF

#define	PROCENTRY(x)				\
	.globl	_##x				;\
_##x:
#endif PROF

#define PARAM		sp@(4)
#define PARAM2		sp@(8)

#define	SYSCALL(name, nargs)			\
PROCENTRY(name);				\
	movl	sp,a0;			\
	save_registers_##nargs;		\
	kernel_trap_args_##nargs;	\
	movl	\#SYS_##name,d0;	\
	trap	\#4;			\
	jcc	1f;			\
	jsr	cerror;			\
1:	restore_registers_##nargs;
