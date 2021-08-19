/* subsegs.h -> subsegs.c */

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
 * For every sub-segment the user mentions in the ASsembler program,
 * we make one struct frchain. Each sub-segment has exactly one struct frchain
 * and vice versa.
 *
 * Struct frchain's are forward chained (in ascending order of sub-segment
 * code number). The chain runs through frch_next of each subsegment.
 * This makes it hard to find a subsegment's frags
 * if programmer uses a lot of them. Most programs only use text0 and
 * data0, so they don't suffer. At least this way:
 * (1)	There are no "arbitrary" restrictions on how many subsegments
 *	can be programmed;
 * (2)	Subsegments' frchain-s are (later) chained together in the order in
 *	which they are emitted for object file viz text then data.
 *
 * From each struct frchain dangles a chain of struct frags. The frags
 * represent code fragments, for that sub-segment, forward chained.
 */

struct frchain			/* control building of a frag chain */
{				/* FRCH = FRagment CHain control */
  struct frag *	frch_root;	/* 1st struct frag in chain, or NULL */
  struct frag *	frch_last;	/* last struct frag in chain, or NULL */
  struct frchain * frch_next;	/* next in chain of struct frchain-s */
  segT		frch_seg;	/* SEG_TEXT or SEG_DATA. */
  subsegT	frch_subseg;	/* subsegment number of this chain */
};

typedef struct frchain frchainS;

extern frchainS * frchain_root;	/* NULL means no frchains yet. */
				/* all subsegments' chains hang off here */

extern frchainS * frchain_now;
				/* Frchain we are assembling into now */
				/* That is, the current segment's frag */
				/* chain, even if it contains no (complete) */
				/* frags. */

extern frchainS * data0_frchainP;
				/* Sentinel for frchain crawling. */
				/* Points to the 1st data-segment frchain. */
				/* (Which is pointed to by the last text- */
				/* segment frchain.) */

/* end: subsegs.h */
