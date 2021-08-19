/*
 * Standard C macros
 *
 **********************************************************************
 * HISTORY
 * 02-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added check to allow multiple or recursive inclusion of this
 *	file.  Added bool enum from machine/types.h for regular users
 *	that want a real boolean type.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Also change spacing of MAX and MIN to coincide with that of
 *	sys/param.h.
 *
 * 19-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed the number of tabs between TRUE, FALSE and their
 *	respective values to match those in sys/types.h.
 *
 * 17-Dec-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Only define TRUE and FALSE if not defined.  Added caseE macro
 *	for using enumerated types in switch statements.
 *
 * 23-Apr-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added "sizeofS" and "sizeofA" macros which expand to the size
 *	of a string constant and array respectively.
 *
 **********************************************************************
 */

#ifndef	_C_INCLUDE_
#define	_C_INCLUDE_

#define ABS(x) ((x)>=0?(x):-(x))
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

#ifndef	FALSE
#define FALSE	0
#endif	FALSE
#ifndef	TRUE
#define TRUE	1
#endif	TRUE

#define	CERROR		(-1)

#ifndef	bool
typedef enum	{ false = 0, true = 1 } bool;
#endif	bool

#define	sizeofS(string)	(sizeof(string) - 1)
#define sizeofA(array)	(sizeof(array)/sizeof(array[0]))

#define caseE(enum_type)	case (int)(enum_type)

#endif	_C_INCLUDE_
