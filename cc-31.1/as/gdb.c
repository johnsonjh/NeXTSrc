/* gdb.c -as supports gdb- */

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

/* This code is independent of the underlying operating system. */

#include "as.h"

static long int		size;	/* 0 or size of GDB symbol file. */
static char *		where;	/* Where we put symbol file in memory. */

#define SUSPECT		/* JF */

long int			/* 0 means don't call gdb_... routines */
gdb_begin (filename)		/* because we failed to establish file */
				/* in memory. */
     char * filename;		/* NULL: we have nothing to do. */
{
  long int		gdb_file_size();
  char *		xmalloc();
  void			gdb_file_begin();
  void			gdb_file_read();
  void			gdb_block_begin();
  void			gdb_symbols_begin();

  gdb_file_begin();
  size = 0;
  if (filename && (size = gdb_file_size (filename)))
    {
      where = xmalloc( (long) size );
      gdb_file_read (where, filename);	/* Read, then close file. */
      gdb_block_begin();
      gdb_symbols_begin();
    }
  return (size);
}

void
gdb_end()
{
  void gdb_file_end();

  gdb_file_end();
}

char *
gdb_mach_O_emit(sizeaddr)
int *sizeaddr;
{
  void gdb_block_emit();
  void gdb_symbols_emit();
  void gdb_lines_emit();

  gdb_block_emit ();
  gdb_symbols_emit();
  gdb_lines_emit();
  *sizeaddr = size;
  return where;
}

void
gdb_emit (filename)	/* Append GDB symbols to object file. */
char *	filename;
{
  void gdb_block_emit();
  void gdb_symbols_emit();
  void gdb_lines_emit();
  void output_file_append();

  gdb_block_emit ();
  gdb_symbols_emit ();
  gdb_lines_emit();
  output_file_append (where, size, filename);
}



/*
	Notes:	We overwrite what was there.
		We assume all overwrites are 4-char numbers.
*/

void
gdb_alter (offset, value)	/* put value into GDB file + offset. */
     long int	offset;
     long int	value;
{
  void md_number_to_chars();
#ifdef SUSPECT

  if (offset > size - sizeof(long int) || offset < 0)
    {
      as_warn( "gdb_alter: offset=%d. size=%ld.\n", offset, size );
    }
  else
    {

#endif

      md_number_to_chars (where + offset, value, 4);

#ifdef SUSPECT
    }
#endif
}

/* end: gdb.c */
