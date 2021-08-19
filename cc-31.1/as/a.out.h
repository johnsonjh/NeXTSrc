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

/* JF I'm not sure where this file came from.  I put the permit.text message in
   it anyway.  This file came to me as part of the original VAX assembler */

#define OMAGIC 0407
#define NMAGIC 0410
#define ZMAGIC 0413

struct exec {
	long	a_magic;			/* number identifies as .o file and gives type of such. */
	unsigned a_text;		/* length of text, in bytes */
	unsigned a_data;		/* length of data, in bytes */
	unsigned a_bss;		/* length of uninitialized data area for file, in bytes */
	unsigned a_syms;		/* length of symbol table data in file, in bytes */
	unsigned a_entry;		/* start address */
	unsigned a_trsize;		/* length of relocation info for text, in bytes */
	unsigned a_drsize;		/* length of relocation info for data, in bytes */
};

#define N_BADMAG(x) \
 (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)

#define N_TXTOFF(x) \
 ((x).a_magic == ZMAGIC ? 1024 : sizeof(struct exec))

#define N_SYMOFF(x) \
 (N_TXTOFF(x) + (x).a_text + (x).a_data + (x).a_trsize + (x).a_drsize)

#define N_STROFF(x) \
 (N_SYMOFF(x) + (x).a_syms)

struct nlist {
	union {
		char	*n_name;
		struct nlist *n_next;
		long	n_strx;
	} n_un;
	char	n_type;
	char	n_other;
	short	n_desc;
	unsigned n_value;
};

#define N_UNDF	0
#define N_ABS	2
#define N_TEXT	4
#define N_DATA	6
#define N_BSS	8
#define	N_SECT	14
#define N_FN	31		/* JF: Someone claims this should be 31 instead of
			   15.  I just inherited this file; I didn't write
			   it.  Who is right? */

#define N_EXT 1
#define N_TYPE 036
#define N_STAB 0340

struct relocation_info {
	int	 r_address;
	unsigned r_symbolnum:24,
		 r_pcrel:1,
		 r_length:2,
		 r_extern:1,
		 r_bsr:1, /* OVE: used on ns32k based systems, if you want */
		 r_disp:1, /* OVE: used on ns32k based systems, if you want */
		 nuthin:2;
};

#ifdef NeXT
/*
 * To make scattered loading by the link editor work correctly "local"
 * relocation entries can't be used when the item to be relocated is the value
 * of a symbol plus an offset (where the resulting expresion is outside the
 * block the link editor is moving).  In this case the link editor needs to
 * know more than just the section the symbol was defined in but the actual
 * value of the symbol without the offset so it can do the relocation correctly
 * based on where the value of the symbol got relocated to not the value of the
 * expression (with the offset added to the symbol value).  So for the NeXT 2.0
 * release NO "local" relocation entries are ever used.  The "external"
 * relocation entrys remain unchanged.
 *
 * The implemention is quite messy given the compatiblity with the existing
 * relocation entry format.  The ASSUMPTION is that a section will never be
 * bigger than 2**24 - 1 (0x00ffffff or 16,777,215) bytes.  This assumption
 * allows the r_address (which is really an offset) to fit in 24 bits and high
 * bit of the r_address field in the old relocation_info structure to indicate
 * it is really a scattered_relocation_info structure.  Since these are only
 * used in place of "local" relocation entries and not "external" relocation
 * entries the r_extern feild has been removed.
 */
#define R_SCATTERED 0x80000000	/* mask to be applied to the r_address field 
				   of a relocation_info structure to tell that
				   is is really a scattered_relocation_info
				   stucture */
struct scattered_relocation_info {
   unsigned int r_scattered:1,	/* 1=scattered, 0=non-scattered (see above) */
		r_pcrel:1, 	/* was relocated pc relative already */
		r_length:2,	/* 0=byte, 1=word, 2=long */
		r_reserved:4,	/* reserved */
   		r_address:24;	/* offset in the section to what is being
				   relocated */
   long		r_value;	/* the value the item to be relocated is
				   refering to (without any offset added) */
};
#endif NeXT
