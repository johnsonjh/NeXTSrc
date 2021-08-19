/* gdb_symbols.c - */

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
 * During assembly, note requests to place symbol values in the GDB
 * symbol file. When symbol values are known and the symbol file is
 * in memory, place the symbol values in the memory image of the file.
 *
 * This has static data: it is not data_sharable.
 *
 * gdb_symbols_begin ()
 *		Call once before using this package.
 *
 * gdb_symbols_fixup (symbolP, offset_in_file)
 *		Remember to put the value of a symbol into the GDB file.
 *
 * gdb_symbols_emit  ()
 *		Perform all the symbol fixups.
 *
 * uses:
 *	xmalloc()
 *	gdb_alter()
 */

#include "as.h"
#include "struc-symbol.h"

#define SYM_GROUP (100)		/* We allocate storage in lumps this big. */


struct gdb_symbol		/* 1 fixup request. */
{
  symbolS *	gs_symbol;
  long int	gs_offset;	/* Where in GDB symbol file. */
};
typedef struct gdb_symbol gdb_symbolS;

struct symbol_fixup_group
{
  struct symbol_fixup_group *	sfg_next;
  gdb_symbolS			sfg_item [SYM_GROUP];
};
typedef struct symbol_fixup_group symbol_fixup_groupS;

static symbol_fixup_groupS *	root;
static short int		used; /* # of last slot used. */
				/* Counts down from SYM_GROUP. */

static symbol_fixup_groupS *	/* Make storage for some more reminders. */
new_sfg ()
{
  symbol_fixup_groupS *		newP;
  char *			xmalloc();

  newP = (symbol_fixup_groupS *) xmalloc ((long)sizeof(symbol_fixup_groupS));
  newP -> sfg_next = root;
  used = SYM_GROUP;
  root = newP;
  return (newP);
}


void
gdb_symbols_begin ()
{
  root = 0;
  (void)new_sfg ();
}


void				/* Build a reminder to put a symbol value */
gdb_symbols_fixup (sy, offset)	/* into the GDB symbol file. */
     symbolS *	sy;		/* Which symbol. */
     long int	offset;		/* Where in GDB symbol file. */
{
  register symbol_fixup_groupS *	p;
  register gdb_symbolS *		q;
      
  p = root;
  know( used >= 0 );
  if ( used == 0)
    {
      p = new_sfg ();
    }
  q = p -> sfg_item + -- used;
  q -> gs_symbol = sy;
  q -> gs_offset = offset;
}

void
gdb_symbols_emit ()		/* Append GDB symbols to object file. */
{
  symbol_fixup_groupS *	sfgP;
  void gdb_alter();
  
  for (sfgP = root;  sfgP;  sfgP = sfgP -> sfg_next)
    {
      register gdb_symbolS *	gsP;
      register gdb_symbolS *	limit;

      limit = sfgP -> sfg_item +
	(sfgP -> sfg_next ? 0 : used);
      for (gsP = sfgP -> sfg_item + SYM_GROUP - 1;
	   gsP >= limit;
	   gsP --)
	{
#if NeXT
	  if (gsP->gs_symbol)
#endif
	    gdb_alter (gsP -> gs_offset,
		     (long int) gsP -> gs_symbol -> sy_value);
	}
    }
}

/* end: gdb_symbols.c */
