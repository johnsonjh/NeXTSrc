/*
 * Useful memory managment macros
 * Copyright (C) 1989 by NeXT, Inc.
 */
#define  mm_used() mstats()

#define MM_ALLOC(obj) obj = ((void *)malloc(sizeof(*(obj))))

#define MM_FREE(obj)  free((void *)(obj))

#define MM_ZERO(obj)  bzero((void *)(obj), sizeof(*(obj)))

#define MM_BCOPY(b1, b2, size) bcopy((void *)(b1), (void *)(b2), \
				     (unsigned)(size))

#define MM_BEQ(b1, b2, size) (bcmp((void *)(b1), (void *)(b2), \
				   (unsigned)(size)) == 0)

#define MM_ALLOC_ARRAY(obj, len)  \
	obj = ((void *)malloc(sizeof(*(obj)) * (len)))

#define MM_ZERO_ARRAY(obj, len) bzero((void *)(obj), sizeof(*obj) * len)

#define MM_FREE_ARRAY(obj, len) free((void *)(obj))

#define MM_GROW_ARRAY(obj, len) \
	((obj == NULL) ? (MM_ALLOC_ARRAY((obj), (len) + 1)) : \
	 (obj = (void *)realloc((void *)(obj), \
				sizeof(*(obj)) * ((len) + 1))))

#define MM_SHRINK_ARRAY(obj, len) \
	obj = (void *)realloc((void *)(obj), \
			      sizeof(*(obj)) * ((len) - 1))

