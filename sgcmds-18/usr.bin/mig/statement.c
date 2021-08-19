/*****************************************************
 *  ABSTRACT:
 *	Called by the parser to allocate a new
 *	statement structure and thread into the
 *	statement list:
 *   Exports:
 *	stats - pointer to head of statement list
 *
 *
 *	$Header: statement.c,v 1.1 89/06/08 14:20:12 mmeyer Locked $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 ***************************************************/

#include "error.h"
#include "alloc.h"
#include "statement.h"

statement_t *stats = stNULL;
static statement_t **last = &stats;

statement_t *
stAlloc()
{
    register statement_t *new;

    new = (statement_t *) malloc(sizeof *new);
    if (new == stNULL)
	fatal("stAlloc(): %s", unix_error_string(errno));
    *last = new;
    last = &new->stNext;
    new->stNext = stNULL;
    return new;
}
