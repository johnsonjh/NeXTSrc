/* gdb_file.c -o/s specific- */

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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

static long file_len;
static FILE *file;
extern long get_len();


void
gdb_file_begin ()
{
}

void
gdb_file_end()
{
}

long int			/* Open file, return size. 0: failed. */
gdb_file_size (filename)
char *filename;
{
  struct stat stat_buf;
  void as_perror();

  file= fopen (filename, "r");
  if (file == (FILE *)NULL)
    {
      as_perror ("Can't read GDB symbolic information file", filename);
      file_len=0;
    } else {
	(void)fstat (fileno(file), &stat_buf);
	file_len=stat_buf . st_size;
    }
  return ((long int)file_len );
}

void				/* Read the file, don't return if failed. */
gdb_file_read (buffer, filename)
     char *	buffer;
     char *	filename;
{
  register off_t	size_wanted;
  void as_perror();

  size_wanted = file_len;
  if (fread (buffer, size_wanted, 1, file) != 1)
    {
      as_perror ("Can't read GDB symbolic info file", filename);
      as_fatal ("Failed to read %ld. chars of GDB symbolic information",
		size_wanted);
    }
  if (fclose(file)==EOF)
    {
      as_perror ("Can't close GDB symbolic info file", filename);
      as_fatal ("I quit in disgust");
    }
}

/* end: gdb_file.c */
