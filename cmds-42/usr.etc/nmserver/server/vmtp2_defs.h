/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * vmtp2_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/vmtp2_defs.h,v $
 *
 * $Header: vmtp2_defs.h,v 1.1 88/09/30 15:46:07 osdev Exp $
 *
 */

/*
 * Definitions for the VMTP module.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 14-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Revised for new transport interface. Modified the crypt_header
 *	handling to make it a trailer instead.
 *
 *  3-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created from vmtp1_defs.h.
 */

#ifndef	_VMTP2_DEFS_
#define	_VMTP2_DEFS_

#define KERNEL_FEATURES	1

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netinet/vmtp_so.h>
#include	<netinet/vmtp.h>

#include	"mem.h"
#include	"crypt.h"
#include	"sbuf.h"


#if	RPCMOD
#else	RPCMOD
 ! You lose !
#endif	RPCMOD

/*
 * Substitute for the crypt_header.
 */
typedef struct vmtp_crypt_trailer {
    unsigned short	ct_checksum;
    unsigned short	ct_data_size;
} vmtp_crypt_trailer_t, *vmtp_crypt_trailer_ptr_t;


/*
 * Maximum VMTP segment size, preallocated in the transport records.
 */
#define	VMTP_SEGSIZE	16384
#define VMTP_MAX_DATA	(VMTP_SEGSIZE				\
				- sizeof(vmtp_crypt_trailer_t)	\
				- (2 * sizeof(long)))

typedef struct vmtp_segment {
	char			data[VMTP_MAX_DATA];
	/*
	 * The following field may be shifted up in the structure if
	 * the data area is not full.
	 */
	vmtp_crypt_trailer_t	vmtp_crypt_trailer;
	long			pad[2];
} vmtp_segment_t, *vmtp_segment_ptr_t;

/*
 * VMTP transport record.
 */
typedef struct vmtp_rec {
	vmtp_segment_t	segment;	/* maximum-size VMTP segment */
	struct vmtp_rec	*next;		/* next in free queue */
	int		socket;		/* associated socket (if any) */
	struct vmtpmcb	mcb;		/* VMTP message control block */
	sbuf_t		sbuf;		/* sbuf for data */
	sbuf_seg_t	sbuf_seg;	/* segment for sbuf */
	int		client_id;	/* higher-level (ipc) ID */
#if	RPCMOD
	int		(*reply_proc)();/* client reply procedure */
#else	RPCMOD
	int		(*callback)();	/* client callback procedure */
#endif	RPCMOD
} vmtp_rec_t, *vmtp_rec_ptr_t;


/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_VMTPREC;


#endif	_VMTP2_DEFS_
