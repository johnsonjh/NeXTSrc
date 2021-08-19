/* input_file.h header for input-file.c */

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

/*"input_file.c":Operating-system dependant functions to read source files.*/


/*
 * No matter what the operating system, this module must provide the
 * following services to its callers.
 *
 * input_file_begin()			Call once before anything else.
 *
 * input_file_end()			Call once after everything else.
 *
 * input_file_buffer_size()		Call anytime. Returns largest possible
 *					delivery from
 *					input_file_give_next_buffer().
 *
 * input_file_open(name)		Call once for each input file.
 *
 * input_file_give_next_buffer(where)	Call once to get each new buffer.
 *					Return 0: no more chars left in file,
 *					   the file has already been closed.
 *					Otherwise: return a pointer to just
 *					   after the last character we read
 *					   into the buffer.
 *					If we can only read 0 characters, then
 *					end-of-file is faked.
 *
 * All errors are reported (using as_perror) so caller doesn't have to think
 * about I/O errors. No I/O errors are fatal: an end-of-file may be faked.
 */

void	input_file_begin();
void	input_file_end();
int	input_file_buffer_size();
int	input_file_is_open();
void	input_file_open();
char *	input_file_give_next_buffer();

/* end: input_file.h */
