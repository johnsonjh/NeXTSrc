/* expr.h -> header file for expr.c */

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
 * Abbreviations (mnemonics).
 *
 *	O	operator
 *	Q	quantity,  operand
 *	X	eXpression
 */

/*
 * By popular demand, we define a struct to represent an expression.
 * This will no doubt mutate as expressions become baroque.
 *
 * Currently, we support expressions like "foo-bar+42".
 * In other words we permit a (possibly undefined) minuend, a
 * (possibly undefined) subtrahend and an (absolute) augend.
 * RMS says this is so we can have 1-pass assembly for any compiler
 * emmissions, and a 'case' statement might emit 'undefined1 - undefined2'.
 *
 * To simplify table-driven dispatch, we also have a "segment" for the
 * entire expression. That way we don't require complex reasoning about
 * whether particular components are defined; and we can change component
 * semantics without re-working all the dispatch tables in the assembler.
 * In other words the "type" of an expression is its segment.
 */

typedef struct
{
	symbolS *X_add_symbol;		/* foo */
	symbolS *X_subtract_symbol;	/* bar */
	long int X_add_number;		/* 42.    Must be signed. */
	segT	X_seg;			/* What segment (expr type)? */
}
expressionS;

				/* result should be type (expressionS *). */
#define expression(result) expr(0,result)

				/* If an expression is SEG_BIG, look here */
				/* for its value. These common data may */
				/* be clobbered whenever expr() is called. */
extern FLONUM_TYPE generic_floating_point_number; /* Flonums returned here. */
				/* Enough to hold most precise flonum. */
extern LITTLENUM_TYPE generic_bignum []; /* Bignums returned here. */
#define SIZE_OF_LARGE_NUMBER (20)	/* Number of littlenums in above. */


segT		expr();
char		get_symbol_end();

/* end: expr.h */
