/* symbols.h - */

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

extern struct obstack	notes; /* eg FixS live here. */

#define symbol_table_lookup(name) ((symbolS *)(symbol_find (name)))

extern unsigned int local_bss_counter; /* Zeroed before a pass. */
				/* Only used by .lcomm directive. */


extern symbolS * symbol_rootP;	/* all the symbol nodes */
extern symbolS * symbol_lastP;	/* last struct symbol we made, or NULL */

extern symbolS	abs_symbol;

symbolS *	symbol_find();
void		symbol_begin();
char *		local_label_name();
void		local_colon();
symbolS *	symbol_new();
void		colon();
void		symbol_table_insert();
symbolS *	symbol_find_or_make();

/* end: symbols.h */
