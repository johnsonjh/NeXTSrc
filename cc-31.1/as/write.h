/* write.h -> write.c */

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

/* The bit_fix was implemented to support machines that need variables
   to be inserted in bitfields other than 1, 2 and 4 bytes. 
   Furthermore it gives us a possibillity to mask in bits in the symbol
   when it's fixed in the objectcode and check the symbols limits.

   The or-mask is used to set the huffman bits in displacements for the
   ns32k port.
   The acbi, addqi, movqi, cmpqi instruction requires an assembler that
   can handle bitfields. Ie handle an expression, evaluate it and insert
   the result in an some bitfield. ( ex: 5 bits in a short field of a opcode) 
 */


struct bit_fix {
  int			fx_bit_size;	/* Length of bitfield		*/
  int			fx_bit_offset;	/* Bit offset to bitfield	*/
  long			fx_bit_base;	/* Where do we apply the bitfix.
					If this is zero, default is assumed. */
  long                  fx_bit_base_adj;/* Adjustment of base */
  long			fx_bit_max;	/* Signextended max for bitfield */
  long			fx_bit_min;	/* Signextended min for bitfield */
  long			fx_bit_add;	/* Or mask, used for huffman prefix */
};
typedef struct bit_fix bit_fixS;
/*
 * FixSs may be built up in any order.
 */

struct fix
{
  fragS *		fx_frag;	/* Which frag? */
  long int		fx_where;	/* Where is the 1st byte to fix up? */
  symbolS *		fx_addsy; /* NULL or Symbol whose value we add in. */
  symbolS *		fx_subsy; /* NULL or Symbol whose value we subtract. */
  long int		fx_offset;	/* Absolute number we add in. */
  struct fix *		fx_next;	/* NULL or -> next fixS. */
  short int		fx_size;	/* How many bytes are involved? */
  char			fx_pcrel;	/* TRUE: pc-relative. */
  char			fx_pcrel_adjust;/* pc-relative offset adjust */
  char			fx_im_disp;	/* TRUE: value is a displacement */
  bit_fixS *		fx_bit_fixP;	/* IF NULL no bitfix's to do */  
  char			fx_bsr;		/* sequent-hack */
};

typedef struct fix	fixS;

#ifdef NeXT
/*
 * Bit number 1 (i.e. the value 2) is used in fx_pcrel to indicate a variation
 * of pc relative fix-ups that force a relocation entry to be generated.  This
 * is allows the NeXT linker to do scattered relocation.
 */
#endif

COMMON fixS *	text_fix_root;	/* Chains fixSs. */
COMMON fixS *	data_fix_root;	/* Chains fixSs. */
COMMON fixS **	seg_fix_rootP;	/* -> one of above. */

bit_fixS *bit_fix_new();
/* end: write.h */

