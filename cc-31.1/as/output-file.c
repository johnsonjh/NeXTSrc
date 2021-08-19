/* output-file.c - */

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
 * Confines all details of emitting object bytes to this module.
 * All O/S specific crocks should live here.
 * What we lose in "efficiency" we gain in modularity.
 * Note we don't need to #include the "as.h" file. No common coupling!
 */

/* #include "style.h" */
#include <stdio.h>

void	as_perror();

static FILE *
stdoutput;

void
output_file_create (name)
     char *		name;
{
#ifdef CROSS
  unlink(name);
#endif
  if ( ! (stdoutput = fopen( name, "w" )) )
    {
      as_perror ("Can't create object file", name);
      as_fatal("Can't continue");
    }
}



void
output_file_close (filename)
     char *	filename;
{
  if ( EOF == fclose( stdoutput ) )
    {
      as_perror ("Can't close object file", filename);
      as_fatal("Can't continue");
    }
  stdoutput = NULL;		/* Trust nobody! */
}

void
output_file_append (where, length, filename)
     char *		where;
     long int		length;
     char *		filename;
{

  for (; length; length--,where++)
    {
    	(void)putc(*where,stdoutput);
	if(ferror(stdoutput))
      /* if ( EOF == (putc( *where, stdoutput )) ) */
	{
	  as_perror("Failed to emit an object byte", filename);
	  as_fatal("Can't continue");
	}
    }
}

/* Support for atom */

output_seek(to)
{
  if (fseek(stdoutput, to, 0) < 0)
    as_fatal("Can't seek to %d in output file\n", to);
}

output_write(what, size)
char *what;
int size;
{
  if (fwrite(what, 1, size, stdoutput) != size) {
    as_fatal("Can't write %d bytes to output file\n", size);
  }
}
/* end: output-file.c */
