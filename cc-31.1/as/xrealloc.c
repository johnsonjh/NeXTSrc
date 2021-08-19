/* xrealloc.c -new memory or bust- */

/* Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of Gas, the GNU Assembler.

The GNU assembler is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU Assembler General
Public License for full details.

Everyone is granted permission to copy, modify and redistribute
the GNU Assembler, but only under the conditions described in the
GNU Assembler General Public License.  A copy of this license is
supposed to have been given to you along with the GNU Assembler
so you can know your rights and responsibilities.  It should be
in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.  */


/* 

NAME
	xrealloc () - get more memory or bust
INDEX
	xrealloc () uses realloc ()
SYNOPSIS
	char   *my_memory;

	my_memory = xrealloc (my_memory, 42);
	/* my_memory gets (perhaps new) address of 42 chars *//*

DESCRIPTION

	Use xrealloc () as an "error-free" realloc ().It does almost the same
	job.  When it cannot honour your request for memory it BOMBS your
	program with a "virtual memory exceeded" message.  Realloc() returns
	NULL and does not bomb your program.

SEE ALSO
	realloc ()
*/

#ifdef USG
#include <malloc.h>
#endif

char   *
xrealloc (ptr, n)
register char  *ptr;
long    n;
{
    char   *realloc ();
    void	error();

    if ((ptr = realloc (ptr, (unsigned)n)) == 0)
	error ("virtual memory exceeded");
    return (ptr);
}

/* end: xrealloc.c */
