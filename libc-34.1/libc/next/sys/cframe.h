/*	@(#)cframe.h	1.0	10/28/86	(c) 1986 NeXT	*/

/*
 * NOTE: PROF version of PROCENTRY and ASENTRY will work with
 * both gprof and prof and should normally be used.  GPROF
 * version will only work with gprof, but has slightly lower
 * overhead.
 */

#ifndef _CFRAME_
#define	_CFRAME_

#if __GNU__
#if defined(GPROF)
#define	PROCENTRY(name)					\
	.globl	_##name					;\
_##name:						;\
	link	a6,\#0					;\
	jbsr	mcount					;\
	unlk	a6					;
#endif GPROF

#if defined(PROF)
#define	PROCENTRY(name)					\
	.globl	_##name					;\
_##name:						;\
	link	a6,\#0					;\
	movl	\#name##mcountvar,a0				;\
	jbsr	mcount					;\
	unlk	a6					;\
	.lcomm	name##mcountvar,4
#endif	PROF

#if !defined(GPROF) && !defined(PROF)
#define	PROCENTRY(name)					\
	.globl	_##name					;\
_##name:
#endif	!GPROF && !PROF

#if defined(GPROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:							;\
	link	a6,\#0					;\
	jbsr	mcount					;\
	unlk	a6					;
#endif GPROF

#if defined(PROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:							;\
	link	a6,\#0					;\
	movl	\#name##mcountvar,a0					;\
	jbsr	mcount					;\
	unlk	a6					;\
	.lcomm	name##mcountvar,4
#endif	PROF

#if !defined(GPROF) && !defined(PROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:
#endif	!GPROF && !PROF
#else !__GNU__
#if defined(GPROF)
#define	PROCENTRY(name)					\
	.globl	_/**/name				;\
_/**/name:						;\
	link	a6,#0					;\
	jbsr	mcount					;\
	unlk	a6					;
#endif GPROF

#if defined(PROF)
#define	PROCENTRY(name)					\
	.globl	_/**/name				;\
_/**/name:						;\
	link	a6,#0					;\
	movl	#1f,a0					;\
	jbsr	mcount					;\
	unlk	a6					;\
	.bss						;\
	.even						;\
1:	.skip	4					;\
	.text						;
#endif	PROF

#if !defined(GPROF) && !defined(PROF)
#define	PROCENTRY(name)					\
	.globl	_/**/name				;\
_/**/name:
#endif	!GPROF && !PROF

#if defined(GPROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:							;\
	link	a6,#0					;\
	jbsr	mcount					;\
	unlk	a6					;
#endif GPROF

#if defined(PROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:							;\
	link	a6,#0					;\
	movl	#1f,a0					;\
	jbsr	mcount					;\
	unlk	a6					;\
	.bss						;\
	.even						;\
1:	.skip	4					;\
	.text						;
#endif	PROF

#if !defined(GPROF) && !defined(PROF)
#define	ASENTRY(name)					\
	.globl	name					;\
name:
#endif	!GPROF && !PROF
#endif __GNU__

/* following the link/unlink protocol makes the stack look like this */
#define	c_fp	a6@
#define	c_ra	a6@(0x4)
#define	c_p0	a6@(0x8)
#define	c_p1	a6@(0xc)
#define	c_p2	a6@(0x10)
#define	c_p3	a6@(0x14)
#define	c_p4	a6@(0x18)
#define	c_p5	a6@(0x1c)
#define	c_p6	a6@(0x20)
#define	c_p7	a6@(0x24)

/* without a link in the prolog the C parameters look like this */
#define	a_ra	sp@
#define	a_p0	sp@(0x4)
#define	a_p1	sp@(0x8)
#define	a_p2	sp@(0xc)
#define	a_p3	sp@(0x10)
#define	a_p4	sp@(0x14)
#define	a_p5	sp@(0x18)
#define	a_p6	sp@(0x1c)
#define	a_p7	sp@(0x20)

#endif _CFRAME_
