/* hash.h - for hash.c */

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

#ifndef hashH
#define hashH

struct hash_entry
{
  char *      hash_string;	/* points to where the symbol string is */
				/* NULL means slot is not used */
				/* DELETED means slot was deleted */
  char *      hash_value;	/* user's datum, associated with symbol */
};


#define HASH_STATLENGTH	(6)
struct hash_control
{
  struct hash_entry * hash_where; /* address of hash table */
  int         hash_sizelog;	/* Log of ( hash_mask + 1 ) */
  int         hash_mask;	/* masks a hash into index into table */
  int         hash_full;	/* when hash_stat[STAT_USED] exceeds this, */
				/* grow table */
  struct hash_entry * hash_wall; /* point just after last (usable) entry */
				/* here we have some statistics */
  int hash_stat[HASH_STATLENGTH]; /* lies & statistics */
				/* we need STAT_USED & STAT_SIZE */
};


/*						returns		  */
struct hash_control *	hash_new();	/* [control block]	  */
void			hash_die();
void			hash_say();
char *			hash_delete();	/* previous value         */
char *			hash_relpace();	/* previous value         */
char *			hash_insert();	/* error string           */
char *			hash_apply();	/* 0 means OK             */
char *			hash_find();	/* value                  */
char *			hash_jam();	/* error text (internal)  */
static char *		hash_grow();	/* error text (internal)  */
#endif				/* #ifdef hashH */

/* end: hash.c */
