/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * sbuf.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/sbuf.c,v $
 *
 */

#ifndef	lint
char sbuf_rcsid[] = "$Header: sbuf.c,v 1.1 88/09/30 15:42:06 osdev Exp $";
#endif not lint

/*
 * Auxiliary functions for sbuf management.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 28-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added sbuf_printf.
 *
 * 25-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <stdio.h>

#include "mem.h"
#include "netmsg.h"
#include "nm_extra.h"
#include "sbuf.h"


/*
 * sbuf_grow
 *	Increase the number of segments in an sbuf.
 *
 * Parameters:
 *	sbuf_ptr	: a pointer to the sbuf to grow
 *	inc		: the number of new segments required
 *
 * Design:
 *	Allocates space for the new segments.
 *	Copies the entries from the old segments into the new ones.
 *	Deallocates the old segments.
 *	Inserts the new segments into the sbuf.
 *
 */
PUBLIC void sbuf_grow(sbuf_ptr, inc)
register sbuf_ptr_t	sbuf_ptr;
int			inc;
BEGIN("sbuf_grow")
    register sbuf_seg_ptr_t	new_segs, old_seg_ptr, new_seg_ptr;
    register int		segs_used;
    int				num_segs;

    /*
     * Allocate space for the new segments.
     */
    num_segs = sbuf_ptr->size + inc;
    MEM_ALLOC(new_segs,sbuf_seg_ptr_t,(num_segs * sizeof(struct sbuf_seg)), FALSE);

    /*
     * Copy the old segments into the new segments.
     * Also sets new_seg_ptr to point at the first unused segment in new_segs.
     */
    old_seg_ptr = sbuf_ptr->segs;
    new_seg_ptr = new_segs;
    segs_used = 0;
    for (; old_seg_ptr != sbuf_ptr->end; old_seg_ptr++, new_seg_ptr++) {
	*new_seg_ptr = *old_seg_ptr;
	segs_used ++;
    }

    /*
     * Dellocate the old segments.
     */
    MEM_DEALLOC((pointer_t)sbuf_ptr->segs, (sbuf_ptr->size * sizeof(struct sbuf_seg)));

    /*
     * Place the new segments into the sbuf.
     */
    sbuf_ptr->segs = new_segs;
    sbuf_ptr->end = new_seg_ptr;
    sbuf_ptr->free = num_segs - segs_used;
    sbuf_ptr->size = num_segs;

END



/*
 * sbuf_printf
 *	print out an sbuf
 *
 * Parameters:
 *	where	: output file
 *	sb_ptr	: a pointer to the sbuf to print
 *
 */
EXPORT void sbuf_printf(where, sb_ptr)
FILE		*where;
sbuf_ptr_t	sb_ptr;
BEGIN("sbuf_printf")
    sbuf_seg_ptr_t	seg_ptr;

    fprintf(where, "sbuf at %d:\n", (long)sb_ptr);
    fprintf(where, "  end = %d, segs = %d, free = %d & size = %d\n",
		 (long)sb_ptr->end, (long)sb_ptr->segs, sb_ptr->free, sb_ptr->size);
    seg_ptr = sb_ptr->segs;
    for (; seg_ptr != sb_ptr->end; seg_ptr++) {
	fprintf(where, "    segment at %d: data = %d, size = %d.\n",
			(long)seg_ptr, (long)seg_ptr->p, seg_ptr->s);
    }
    RET;

END
