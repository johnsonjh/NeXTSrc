/*	$Header: alloc.h,v 1.1 89/06/08 14:18:53 mmeyer Locked $	*/
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_ALLOC_H
#define	_ALLOC_H

extern char *malloc();
extern char *calloc();
extern void free();

#endif	_ALLOC_H
