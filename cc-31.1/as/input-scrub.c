/* input_scrub.c - */

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

#include "as.h"
#include "read.h"
#include "input-file.h"
#ifdef	VMS
#include <errno.h>	/* Need this to make errno declaration right */
#include <perror.h>	/* Need this to make sys_errlist/sys_nerr right */
#endif /* VMS */

/*
 * O/S independent module to supply buffers of sanitised source code
 * to rest of assembler. We get raw input data of some length.
 * Also looks after line numbers, for e.g. error messages.
 * This module used to do the sanitising, but now a pre-processor program
 * (app) does that job so this module is degenerate.
 * Now input is pre-sanitised, so we only worry about finding the
 * last partial line. A buffer of full lines is returned to caller.
 * The last partial line begins the next buffer we build and return to caller.
 * The buffer returned to caller is preceeded by BEFORE_STRING and followed
 * by AFTER_STRING. The last character before AFTER_STRING is a newline.
 */

/*
 * We expect the following sanitation has already been done.
 *
 * No comments, reduce a comment to a space.
 * Reduce a tab to a space unless it is 1st char of line.
 * All multiple tabs and spaces collapsed into 1 char. Tab only
 *   legal if 1st char of line.
 * # line file statements converted to .line x;.file y; statements.
 * Escaped newlines at end of line: remove them but add as many newlines
 *   to end of statement as you removed in the middle, to synch line numbers.
 */

#define BEFORE_STRING ("\n")
#define AFTER_STRING (" ")	/* bcopy of 0 chars might choke. */
#define BEFORE_SIZE (1)
#define AFTER_SIZE  (1)

static char *	buffer_start;	/* -> 1st char of full buffer area. */
static char *	partial_where;	/* -> after last full line in buffer. */
static int	partial_size;	/* >=0. Number of chars in partial line in buffer. */
static char	save_source [AFTER_SIZE];
				/* Because we need AFTER_STRING just after last */
				/* full line, it clobbers 1st part of partial */
				/* line. So we preserve 1st part of partial */
				/* line here. */
static int	buffer_length;	/* What is the largest size buffer that */
				/* input_file_give_next_buffer() could */
				/* return to us? */

static void as_1_char ();

/*
We never have more than one source file open at once.
We may, however, read more than 1 source file in an assembly.
NULL means we have no file open right now.
*/


/*
We must track the physical file and line number for error messages.
We also track a "logical" file and line number corresponding to (C?)
compiler source line numbers.
Whenever we open a file we must fill in physical_input_file. So if it is NULL
we have not opened any files yet.
*/

static
char *		physical_input_file,
     *		logical_input_file;



typedef unsigned int line_numberT;	/* 1-origin line number in a source file. */
				/* A line ends in '\n' or eof. */

static
line_numberT	physical_input_line,
		logical_input_line;

void
input_scrub_begin ()
{
  know( strlen(BEFORE_STRING) == BEFORE_SIZE );
  know( strlen( AFTER_STRING) ==  AFTER_SIZE );

  input_file_begin ();

  buffer_length = input_file_buffer_size ();

  buffer_start = xmalloc ((long)(BEFORE_SIZE + buffer_length + buffer_length + AFTER_SIZE));
  bcopy (BEFORE_STRING, buffer_start, (int)BEFORE_SIZE);

  /* Line number things. */
  logical_input_line = 0;
  logical_input_file = (char *)NULL;
  physical_input_file = NULL;	/* No file read yet. */
  do_scrub_begin();
}

void
input_scrub_end ()
{
  input_file_end ();
}

char *				/* Return start of caller's part of buffer. */
input_scrub_new_file (filename)
     char *	filename;
{
  input_file_open (filename, !flagseen['f']);
  physical_input_file = filename[0] ? filename : "{standard input}";
  physical_input_line = 0;

  partial_size = 0;
  return (buffer_start + BEFORE_SIZE);
}

char *
input_scrub_next_buffer (bufp)
char **bufp;
{
  register char *	limit;	/* -> just after last char of buffer. */
  char *p;
  char *out_string;
  int out_length;
  static char *save_buffer = 0;
  extern int preprocess;

#ifdef DONTDEF
  if(preprocess) {
    if(save_buffer) {
      *bufp = save_buffer;
      save_buffer = 0;
    }
    limit = input_file_give_next_buffer(buffer_start+BEFORE_SIZE);
    if (!limit) {
      partial_where = 0;
      if(partial_size)
        as_warn("Partial line at end of file ignored");
      return partial_where;
    }

    if(partial_size)
      bcopy(save_source, partial_where,(int)AFTER_SIZE);
    do_scrub(partial_where,partial_size,buffer_start+BEFORE_SIZE,limit-(buffer_start+BEFORE_SIZE),&out_string,&out_length);
    limit=out_string + out_length;
    for(p=limit;*--p!='\n';)
      ;
    p++;
    if(p<=buffer_start+BEFORE_SIZE)
      as_fatal("Source line too long.  Please change file '%s' and re-make the assembler.",__FILE__);

    partial_where = p;
    partial_size = limit-p;
    bcopy(partial_where, save_source,(int)AFTER_SIZE);
    bcopy(AFTER_STRING, partial_where, (int)AFTER_SIZE);

    save_buffer = *bufp;
    *bufp = out_string;

    return partial_where;
  }

  /* We're not preprocessing.  Do the right thing */
#endif
  if (partial_size)
    {
      bcopy (partial_where, buffer_start + BEFORE_SIZE, (int)partial_size);
      bcopy (save_source, buffer_start + BEFORE_SIZE, (int)AFTER_SIZE);
    }
  limit = input_file_give_next_buffer (buffer_start + BEFORE_SIZE + partial_size);
  if (limit)
    {
      register char *	p;	/* Find last newline. */

      for (p = limit;   * -- p != '\n';   )
	{
	}
      ++ p;
      if (p <= buffer_start + BEFORE_SIZE)
	{
	  as_fatal ("Source line too long. Please change file %s then rebuild assembler.", __FILE__);
	}
      partial_where = p;
      partial_size = limit - p;
      bcopy (partial_where, save_source,  (int)AFTER_SIZE);
      bcopy (AFTER_STRING, partial_where, (int)AFTER_SIZE);
    }
  else
    {
      partial_where = 0;
      if (partial_size > 0)
	{
	  as_warn( "Partial line at end of file ignored" );
	}
    }
  return (partial_where);
}

/*
 * The remaining part of this file deals with line numbers, error
 * messages and so on.
 */


int
seen_at_least_1_file ()		/* TRUE if we opened any file. */
{
  return (physical_input_file != NULL);
}

void
bump_line_counters ()
{
  ++ physical_input_line;
  ++ logical_input_line;
}

/*
 *			new_logical_line()
 *
 * Tells us what the new logical line number and file are.
 * If the line_number is <0, we don't change the current logical line number.
 * If the fname is NULL, we don't change the current logical file name.
 */
void
new_logical_line (fname, line_number)
     char *	fname;		/* DON'T destroy it! We point to it! */
     int	line_number;
{
  if ( fname )
    {
      logical_input_file = fname;
    }
  if ( line_number >= 0 )
    {
      logical_input_line = line_number;
    }
}

/*
 *			a s _ w h e r e ( )
 *
 * Write a line to stderr locating where we are in reading
 * input source files.
 * As a sop to the debugger of AS, pretty-print the offending line.
 */
void
as_where()
{
  char *p;
  line_numberT line;

  if (physical_input_file)
    {				/* we tried to read SOME source */
      if (input_file_is_open())
	{			/* we can still read lines from source */
#ifdef DONTDEF
	  fprintf (stderr," @ physical line %ld., file \"%s\"",
		   (long) physical_input_line, physical_input_file);
	  fprintf (stderr," @ logical line %ld., file \"%s\"\n",
		   (long) logical_input_line, logical_input_file);
	  (void)putc(' ', stderr);
	  as_howmuch (stderr);
	  (void)putc('\n', stderr);
#else
		p = logical_input_file ? logical_input_file : physical_input_file;
		line = logical_input_line ? logical_input_line : physical_input_line;
		fprintf(stderr,"%s:%u:", p, line);
#endif
	}
      else
	{
#ifdef DONTDEF
	  fprintf (stderr," After reading source.\n");
#else
	p = logical_input_file ? logical_input_file : physical_input_file;
	line = logical_input_line ? logical_input_line : physical_input_line;
	fprintf (stderr,"%s:unknown:", p);
#endif
	}
    }
  else
    {
#ifdef DONTDEF
      fprintf (stderr," Before reading source.\n");
#else
#endif
    }
}



/*
 *			a s _ p e r r o r
 *
 * Like perror(3), but with more info.
 */
void
as_perror(gripe, filename)
     char *	gripe;		/* Unpunctuated error theme. */
     char *	filename;
{
  extern int errno;		/* See perror(3) for details. */
  extern int sys_nerr;
  extern char * sys_errlist[];

  fprintf (stderr,"as:file(%s) %s! ",
	   filename, gripe
	   );
  if (errno > sys_nerr)
    {
      fprintf (stderr, "Unknown error #%d.", errno);
    }
  else
    {
      fprintf (stderr, "%s.", sys_errlist [errno]);
    }
  (void)putc('\n', stderr);
  errno = 0;			/* After reporting, clear it. */
  if (input_file_is_open())	/* RMS says don't mention line # if not needed. */
    {
      as_where();
    }
}

/*
 *			a s _ h o w m u c h ( )
 *
 * Output to given stream how much of line we have scanned so far.
 * Assumes we have scanned up to and including input_line_pointer.
 * No free '\n' at end of line.
 */
void
as_howmuch (stream)
     FILE * stream;		/* Opened for write please. */
{
  register	char *	p;	/* Scan input line. */
  /* register	char	c; JF unused */

  for (p = input_line_pointer - 1;   * p != '\n';   --p)
    {
    }
  ++ p;				/* p -> 1st char of line. */
  for (;  p <= input_line_pointer;  p++)
    {
      /* Assume ASCII. EBCDIC & other micro-computer char sets ignored. */
      /* c = *p & 0xFF; JF unused */
      as_1_char (*p, stream);
    }
}

static void
as_1_char (c,stream)
     unsigned char c;
     FILE *	stream;
{
  if ( c > 127 )
    {
      (void)putc( '%', stream);
      c -= 128;
    }
  if ( c < 32 )
    {
      (void)putc( '^', stream);
      c += '@';
    }
  (void)putc( c, stream);
}

/* end: input_scrub.c */
