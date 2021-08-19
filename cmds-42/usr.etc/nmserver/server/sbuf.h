/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * sbuf.h 
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/sbuf.h,v $ 
 *
 * $Header: sbuf.h,v 1.1 88/09/30 15:45:18 osdev Exp $ 
 *
 */

/*
 * Definitions for the sbuf structures used to represent message data as a
 * list of independent segments. An sbuf itself is a contiguous list of
 * segments descriptors for each block of data. Any segment in the list may
 * have a size of 0. There is a pointer to the first free segment after the
 * used part of the list. 
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 26-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Grow sbufs in SBUF_APPEND by the sbuf size.
 *
 * 20-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Do not panic in SBUF_EXTRACT if there is nothing left to copy.
 *
 *  2-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Revised SBUF_GET_SEG again!  Added SBUF_SEG_INIT.
 *
 *  2-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added SBUF_GET_SIZE.
 *
 * 25-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added SBUF_REINIT and extern definitions of sbuf_grow and sbuf_printf.
 *	Revised definition of SBUF_GET_SEG.
 *
 * 19-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added SBUF_FREE, SBUF_GET_SEG, SBUF_POS_AT_END and SBUF_SAFE_EXTRACT.
 *
 * 15-Nov-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifndef	_SBUF_
#define	_SBUF_

#include <mach.h>
#include "mem.h"

extern void sbuf_grow();
/*
sbuf_ptr_t		sbuf_ptr;
int			inc;
*/

extern void sbuf_printf();
/*
FILE			where
sbuf_ptr_t		sbuf_ptr;
*/


typedef struct sbuf_seg {	/* one segment of an sbuf */
	pointer_t       p;	/* pointer to start of data */
	unsigned long   s;	/* data size in bytes */
} sbuf_seg_t;

typedef sbuf_seg_t *sbuf_seg_ptr_t;

typedef struct sbuf {
	sbuf_seg_ptr_t  end;	/* first unused segment at the end */
	sbuf_seg_ptr_t  segs;	/* pointer to the list of segments */
	int             free;	/* number of free segments at the end of the list */
	int             size;	/* number of segments allocated in the list (for GC) */
} sbuf_t;

typedef sbuf_t *sbuf_ptr_t;


/*
 * Initialize an empty sbuf of a given size (# of segments).
 */
#define SBUF_INIT(sb,sz) {						\
	sbuf_seg_ptr_t	_s;						\
									\
	MEM_ALLOC(_s,sbuf_seg_ptr_t,(int)(sz) * sizeof(sbuf_seg_t),FALSE);\
	(sb).end = (sb).segs = _s;					\
	(sb).free = (sz);						\
	(sb).size = (sz);						\
}

/*
 * Initialise an empty sbuf with one segment (given).
 */
#define SBUF_SEG_INIT(sb,sp) {						\
	(sb).end = (sb).segs = (sp);					\
	(sb).free = (sb).size = 1;					\
}

/*
 * Reinitialize an sbuf.
 */
#define SBUF_REINIT(sb) {						\
	(sb).end = (sb).segs;						\
	(sb).free = (sb).size;						\
}

/*
 * Free an sbuf -- data associated with the sbuf is not freed.
 */
#define SBUF_FREE(sb) MEM_DEALLOC((pointer_t)(sb).segs, ((sb).size * sizeof(sbuf_seg_t)))

/*
 * Add a segment at the end of an sbuf. Allocate some space if necessary. 
 */
#define SBUF_APPEND(sb,ptr,seg_size) {					\
	if ((sb).free == 0) sbuf_grow(&(sb),(sb).size);			\
	(sb).end->p = (pointer_t)(ptr);					\
	(sb).end->s = (seg_size);					\
	(sb).end++;							\
	(sb).free--;							\
}

/*
 * Get the first segment from an sbuf.
 */
#define SBUF_GET_SEG(sb,sp,type) (sp) = (type)((sb).segs->p);

/*
 * Get the size of the data in an sbuf.
*/
#define SBUF_GET_SIZE(sb, size) {					\
    register sbuf_seg_ptr_t ssp = (sb).segs;				\
    (size) = 0;								\
    for (; ssp != (sb).end; ssp++) (size) += (int)ssp->s;		\
}


/*
 * Add the contents of an sbuf at the end of another sbuf. Allocate
 * some space if necessary.
 */
#define	SBUF_SB_APPEND(to,from) {				\
	register sbuf_seg_ptr_t		from_ptr;		\
	register sbuf_seg_ptr_t		to_ptr;			\
	int				count;			\
								\
	count = (from).size - (from).free;			\
	if ((to).free < count) sbuf_grow(&(to),(count + 10));	\
	from_ptr = (from).segs;					\
	to_ptr = (to).end;					\
	for (; count; count--) *to_ptr++ = *from_ptr++;		\
	(to).end = to_ptr;					\
	(to).free -= ((from).size - (from).free);		\
}


/*
 * Copy an sbuf in to a contiguous area of storage.
 *	source is a pointer to an sbuf
 *	destination is any old pointer
 *	numbytes is set to the number of bytes copied
 */
#define SBUF_FLATTEN(source, destination, numbytes) {			\
    register sbuf_seg_ptr_t end = (source)->end;			\
    register sbuf_seg_ptr_t ssp = (source)->segs;			\
    register char *dest = (char *)(destination);			\
    register int seg_size;						\
    for (; ssp != end; ssp++) {						\
	seg_size = (int)ssp->s;						\
	if (seg_size) {							\
	    bcopy((char *)ssp->p, (char *)dest, seg_size);		\
	    dest += seg_size;						\
	}								\
    }									\
    (numbytes) = (long)dest - (long)(destination);			\
}

/*
 * Structure used to store a position within an sbuf.
 */
typedef	struct {
	sbuf_seg_ptr_t		seg_ptr;
	pointer_t		data_ptr;
	int			data_left;
} sbuf_pos_t;

/*
 * Position pos at offset off from the start of sbuf sb.
 */
#define	SBUF_SEEK(sb,pos,off) {						\
	register int		loc = (off);				\
	register sbuf_seg_ptr_t	seg_ptr = (sb).segs;			\
									\
	while (loc > seg_ptr->s) {					\
		loc -= seg_ptr->s;					\
		seg_ptr++;						\
		if (seg_ptr == (sb).end) panic("SBUF_SEEK");		\
	}								\
	(pos).seg_ptr = seg_ptr;					\
	(pos).data_ptr = (pointer_t)((char *)(seg_ptr->p) + loc);	\
	(pos).data_left = seg_ptr->s - loc;				\
}


/*
 * Copy count bytes from position pos in sbuf sb to address addr.
 * Update pos and addr after the operation.
 */
#define	SBUF_EXTRACT(sb,pos,addr,count) {					\
	register int		total_left = (count);				\
										\
	while (total_left > (pos).data_left) {					\
		bcopy((char *)(pos).data_ptr,(char *)(addr),(pos).data_left);	\
		total_left -= (pos).data_left;					\
		(addr) = (pointer_t)(((char *)addr) + (pos).data_left);		\
		(pos).seg_ptr++;						\
		if ((pos).seg_ptr == (sb).end) {				\
			if (total_left) panic("SBUF_EXTRACT");			\
		}								\
		else {								\
			(pos).data_ptr = (pos).seg_ptr->p;			\
			(pos).data_left = (pos).seg_ptr->s;			\
		}								\
	}									\
	if (total_left) {							\
		bcopy((char *)(pos).data_ptr,(char *)(addr),total_left);	\
		(addr) = (pointer_t)(((char *)addr) + total_left);		\
		(pos).data_ptr += total_left;					\
		(pos).data_left -= total_left;					\
	}									\
}


/*
 * Copy count bytes from position pos in sbuf sb to address addr.
 * Update pos, addr and count after the operation.
 * Count should say how many bytes were actually copied.
 */
#define	SBUF_SAFE_EXTRACT(sb,pos,addr,count) {					\
	register int		total_left = (count);				\
										\
	(count) = 0;								\
	while (total_left > (pos).data_left) {					\
		bcopy((char *)(pos).data_ptr,(char *)(addr),(pos).data_left);	\
		total_left -= (pos).data_left;					\
		(count) += (pos).data_left;					\
		(addr) = (pointer_t)(((char *)addr) + (pos).data_left);		\
		(pos).seg_ptr++;						\
		if ((pos).seg_ptr != (sb).end) {				\
			(pos).data_ptr = (pos).seg_ptr->p;			\
			(pos).data_left = (pos).seg_ptr->s;			\
		}								\
		else {								\
			total_left = 0;						\
			break;							\
		}								\
	}									\
	if (total_left) {							\
		bcopy((char *)(pos).data_ptr,(char *)(addr),total_left);	\
		(addr) = (pointer_t)(((char *)addr) + total_left);		\
		(pos).data_ptr += total_left;					\
		(pos).data_left -= total_left;					\
		(count) += total_left;						\
	}									\
}

/*
 * Is the position pos at the end of the sbuf sb?
 */
#define SBUF_POS_AT_END(sb,pos) ((pos).seg_ptr == (sb).end)


#endif	_SBUF_
