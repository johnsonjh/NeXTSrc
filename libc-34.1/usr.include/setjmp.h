/*	setjmp.h	4.1	83/05/03	*/

/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _SETJMP_H
#define _SETJMP_H

#ifndef __STRICT_ANSI__
/*
 * WARNING: the first portion of this must match a struct sigcontext
 */
#define	JB_ONSTACK	0
#define	JB_SIGMASK	1
#define	JB_SP		2
#define	JB_PC		3
#define	JB_PS		4
#define JB_D0		5
/* from here on down, independent of struct sigcontext */
#define	JB_D2		6
#define	JB_D3		7
#define	JB_D4		8
#define	JB_D5		9
#define	JB_D6		10
#define	JB_D7		11
#define	JB_A2		12
#define	JB_A3		13
#define	JB_A4		14
#define	JB_A5		15
#define	JB_A6		16
#define	JB_FP2		17
#define	JB_FP3		20
#define	JB_FP4		21
#define	JB_FP5		26
#define	JB_FP6		29
#define	JB_FP7		32
#define	JB_FPCR		35
#define	JB_FPSR		36
#define	JB_FPIAR	37
#define	JB_MAGIC	38
#define	JB_NREGS	(JB_MAGIC+1)

#define	JB_MAGICNUM	0xbeeffeed

#if !defined(LOCORE) && !defined(ASSEMBLER)
typedef int jmp_buf[JB_NREGS];

#ifndef __STRICT_BSD__
extern int setjmp(jmp_buf env);
#ifdef	__GNUC__
extern volatile void longjmp(jmp_buf env, int val);
#else
extern void longjmp(jmp_buf env, int val);
#endif	__GNUC__
#endif /* __STRICT_BSD__ */
#endif
#else /* __STRICT_ANSI__ */
#if !defined(LOCORE) && !defined(ASSEMBLER)
typedef int jmp_buf[39];

extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);
#endif
#endif /* __STRICT_ANSI__ */

#endif /* _SETJMP_H */

