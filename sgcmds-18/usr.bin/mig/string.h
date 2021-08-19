/**************************************************************
 * ABSTRACT:
 *   Header file for string utility programs (string.c)
 *
 *
 *	$Header: string.h,v 1.1 89/06/08 14:20:22 mmeyer Locked $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Fixed strNULL to be the null string instead of the null string
 *	pointer.
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 *****************************************************************/

#ifndef	_MIG_STRING_H
#define	_MIG_STRING_H

#include <strings.h>

typedef char *string_t;
typedef string_t identifier_t;

extern char	charNULL;
#define	strNULL		&charNULL

extern string_t strmake(/* char *string */);
extern string_t strconcat(/* string_t left, right */);
extern void strfree(/* string_t string */);

#define	streql(a, b)	(strcmp((a), (b)) == 0)

extern char *strbool(/* boolean_t bool */);
extern char *strstring(/* string_t string */);

#endif	_MIG_STRING_H
