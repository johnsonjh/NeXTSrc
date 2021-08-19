/* read.h - of read.c */

/* Copyright (C) 1986 Free Software Foundation, Inc.

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

extern	char	* input_line_pointer; /* -> char we are parsing now. */

#define PERMIT_WHITESPACE	/* Define to make whitespace be allowed in */
				/* many syntactically unnecessary places. */
				/* Normally undefined. For compatibility */
				/* with ancient GNU cc. */
#undef PERMIT_WHITESPACE

#ifdef PERMIT_WHITESPACE
#define SKIP_WHITESPACE() {if (* input_line_pointer == ' ') ++ input_line_pointer;}
#else
#define SKIP_WHITESPACE() ASSERT( * input_line_pointer != ' ' )
#endif


#define	LEX_NAME	(1)	/* may continue a name */		      
#define LEX_BEGIN_NAME	(2)	/* may begin a name */			      
						        		      
#define is_name_beginner(c)     ( lex_type[c] & LEX_BEGIN_NAME )
#define is_part_of_name(c)      ( lex_type[c] & LEX_NAME       )

extern char lex_type[];

void		read_begin();
void		read_end();
void		read_a_source_file();

/* end: read.h */
