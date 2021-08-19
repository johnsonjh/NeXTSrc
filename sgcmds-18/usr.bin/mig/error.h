/*	$Header: error.h,v 1.1 89/06/08 14:19:00 mmeyer Locked $	*/
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_ERROR_H
#define	_ERROR_H

#include <mach_error.h>

extern void fatal(/* char *format, ... */);
extern void warn(/* char *format, ... */);
extern void error(/* char *format, ... */);

#if	NeXT
#include <stddef.h>	/* for errno */
#else	NeXT
extern int errno;
#endif	NeXT

extern char *unix_error_string();

extern int errors;
extern void set_program_name(/* char *name */);

#endif	_ERROR_H
