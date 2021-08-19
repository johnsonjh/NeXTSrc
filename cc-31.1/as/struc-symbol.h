/* struct_symbol.h - */

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

#ifdef hpux
#include <a.out.h>
#else
#ifndef		VMS
#include "a.out.h"		/* Needed to define struct nlist. Sigh. */
#else
#include "a_out.h"
#endif
#endif

struct symbol			/* our version of an nlist node */
{
  struct nlist	sy_nlist;	/* what we write in .o file (if permitted) */
  long unsigned sy_name_offset;	/* 4-origin position of sy_name in symbols */
				/* part of object file. */
				/* 0 for (nameless) .stabd symbols. */
				/* Not used until write_object_file() time. */
  long int	sy_number;	/* 24 bit symbol number. */
				/* Symbol numbers start at 0 and are */
				/* unsigned. */
  struct symbol * sy_next;	/* forward chain, or NULL */
  struct frag *	sy_frag;	/* NULL or -> frag this symbol attaches to. */
  struct symbol *sy_forward;	/* value is really that of this other symbol */
};

typedef struct symbol symbolS;

#define sy_name		sy_nlist .n_un. n_name
				/* Name field always points to a string. */
				/* 0 means .stabd-like anonymous symbol. */
#define sy_type 	sy_nlist.	n_type
#define sy_other	sy_nlist.	n_other
#define sy_desc		sy_nlist.	n_desc
#define sy_value	sy_nlist.	n_value
				/* Value of symbol is this value + object */
				/* file address of sy_frag. */

typedef unsigned valueT;	/* The type of n_value. Helps casting. */

/* end: struct_symbol.h */
#ifndef WORKING_DOT_WORD
struct broken_word {
	struct broken_word *next_broken_word;/* One of these strucs per .word x-y */
	fragS	*frag;		/* Which frag its in */
	char	*word_goes_here;/* Where in the frag it is */
	fragS	*dispfrag;	/* where to add the break */
	symbolS	*add;		/* symbol_x */
	symbolS	*sub;		/* - symbol_y */
	long	addnum;		/* + addnum */
	int	added;		/* nasty thing happend yet? */
				/* 1: added and has a long-jump */
				/* 2: added but uses someone elses long-jump */
	struct broken_word *use_jump; /* points to broken_word with a similar
					 long-jump */
};
extern struct broken_word *broken_words;
#endif
