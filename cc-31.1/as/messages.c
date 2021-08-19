/* messages.c - error reporter - */

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

#include <stdio.h>		/* define stderr */
#include "as.h"
#ifndef NO_VARARGS
#include <varargs.h>
#endif

/*
		ERRORS

	We print the error message 1st, beginning in column 1.
	All ancillary info starts in column 2 on lines after the
	key error text.
	We try to print a location in logical and physical file
	just after the main error text.
	Caller then prints any appendices after that, begining all
	lines with at least 1 space.

	Optionally, we may die.
	There is no need for a trailing '\n' in your error text format
	because we supply one.

as_warn(fmt,args)  Like fprintf(stderr,fmt,args) but also call errwhere().

as_fatal(fmt,args) Like as_warn() but exit with a fatal status.

*/



/*
 *			a s _ w a r n ( )
 *
 * Send to stderr a string (with bell) (JF: Bell is obnoxious!) as a warning, and locate warning
 * in input file(s).
 * Please only use this for when we have some recovery action.
 * Please explain in string (which may have '\n's) what recovery was done.
 */

#ifdef NO_VARARGS
/*VARARGS1*/
as_warn(Format,args)
char *Format;
{
  if ( ! flagseen ['W'])	/* -W supresses warning messages. */
    {
      as_where();
      _doprnt (Format, &args, stderr);
      (void)putc ('\n', stderr);
      /* as_where(); */
    }
}
#else
void
as_warn(Format,va_alist)
char *Format;
va_dcl
{
  va_list args;

  if( ! flagseen['W'])
    {
      as_where();
      va_start(args);
      vfprintf(stderr, Format, args);
      va_end(args);
      (void) putc('\n', stderr);
    }
}
#endif
#ifdef DONTDEF
void
as_warn(Format,aa,ab,ac,ad,ae,af,ag,ah,ai,aj,ak,al,am,an)
char *format;
{
	if(!flagseen['W']) {
		as_where();
		fprintf(stderr,Format,aa,ab,ac,ad,ae,af,ag,ah,ai,aj,ak,al,am,an);
		(void)putc('\n',stderr);
	}
}
#endif
/*
 *			a s _ f a t a l ( )
 *
 * Send to stderr a string (with bell) (JF: Bell is obnoxious!) as a fatal
 * message, and locate stdsource in input file(s).
 * Please only use this for when we DON'T have some recovery action.
 * It exit()s with a warning status.
 */

#ifdef NO_VARARGS
/*VARARGS1*/
as_fatal (Format, args)
char *Format;
{
  as_where();
  fprintf(stderr,"FATAL:");
  _doprnt (Format, &args, stderr);
  (void)putc ('\n', stderr);
  /* as_where(); */
  exit(42);			/* What is a good exit status? */
}
#else
void
as_fatal(Format,va_alist)
char *Format;
va_dcl
{
  va_list args;

  as_where();
  va_start(args);
  fprintf (stderr, "FATAL:");
  vfprintf(stderr, Format, args);
  (void) putc('\n', stderr);
  va_end(args);
  exit(42);
}
#endif
#ifdef DONTDEF
void
as_fatal(Format,aa,ab,ac,ad,ae,af,ag,ah,ai,aj,ak,al,am,an)
char *Format;
{
  as_where();
  fprintf (stderr, "FATAL:");
  fprintf(stderr, Format,aa,ab,ac,ad,ae,af,ag,ah,ai,aj,ak,al,am,an);
  (void) putc('\n', stderr);
  exit(42);
}
#endif

/* end: messages.c */
