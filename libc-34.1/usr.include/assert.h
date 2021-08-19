/*	assert.h	4.2	85/01/21	*/

/* Copyright (c) 1988 NeXT, Inc. - 9/13/88 CCH */

#ifndef _ASSERT_H
#define _ASSERT_H

#ifdef __STRICT_BSD__
extern void abort(void);
#else
#include <stdio.h>
#include <stdlib.h>
#endif /* __STRICT_BSD__ */

/* for BSD compatibility */
#define _assert(x) assert(x)
#endif /* _ASSERT_H */

/* placed outside so can be multiply included after a #define or #undef
 * NDEBUG to turn assertion checking on and off within a compile */
#undef assert

#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#define assert(expression)  \
    do { \
	if (!(expression)) { \
	    fprintf (stderr, "Assertion failed: " #expression \
	      ", file " __FILE__ ", line %d.\n", __LINE__); \
	    abort (); \
	} \
    } while (0)
#endif

