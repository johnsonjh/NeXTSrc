/*	$Header: write.h,v 1.1 89/06/08 14:20:46 mmeyer Locked $	*/
/*
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_WRITE_H
#define	_WRITE_H

#include <stdio.h>
#include "statement.h"

extern void WriteHeader(/* FILE *file, statement_t *stats */);
extern void WriteUser(/* FILE *file, statement_t *stats */);
extern void WriteUserIndividual(/* statement_t *stats */);
extern void WriteServer(/* FILE *file, statement_t *stats */);

#endif	_WRITE_H
