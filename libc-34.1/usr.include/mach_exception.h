/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/* 
 **********************************************************************
 * HISTORY
 * 25-Apr-88  Karl Hauth (hauth) at Carnegie-Mellon University
 *	Created.
 *
 *
 **********************************************************************
 */ 

#ifndef	_MACH_EXCEPTION_
#define	_MACH_EXCEPTION_	1

#include <sys/kern_return.h>

char		*mach_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception
#endif	c_plusplus
				);


void		mach_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception
#endif	c_plusplus
				);


char		*mach_NeXT_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);


void		mach_NeXT_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

char		*mach_sun3_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);


void		mach_sun3_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

char		*mach_romp_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

void		mach_romp_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

char		*mach_vax_exception_string(
/*
 *	Returns a string appropriate to the error argument given
 */
#if	c_plusplus
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

void		mach_vax_exception(
/*
 *	Prints an appropriate message on the standard error stream
 */
#if	c_plusplus
	char	*str,
	int	exception,
	int	code,
	int	subcode
#endif	c_plusplus
				);

#endif	_MACH_EXCEPTION_
