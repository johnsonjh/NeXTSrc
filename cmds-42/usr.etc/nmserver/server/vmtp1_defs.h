/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * vmtp1_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/vmtp1_defs.h,v $
 *
 * $Header: vmtp1_defs.h,v 1.1 88/09/30 15:46:04 osdev Exp $
 *
 */

/*
 * Definitions for the VMTP module.
 */

/*
 * HISTORY:
 * 23-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added support for encrypted data.
 *
 *  8-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VMTP1_DEFS_
#define	_VMTP1_DEFS_

#define KERNEL_FEATURES	1

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netinet/vmtp_so.h>
#include	<netinet/vmtp.h>

#include	"crypt.h"
#include	"sbuf.h"


/*
 * Maximum VMTP segment size, preallocated in the transport records.
 */
#define	VMTP_SEGSIZE	16384
#define VMTP_MAX_DATA	(VMTP_SEGSIZE - sizeof(crypt_header_t) - sizeof(long))

typedef struct vmtp_segment {
	crypt_header_t	crypt_header;
	char		data[VMTP_MAX_DATA];
	long		pad;
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
	int		(*callback)();	/* client callback procedure */
} vmtp_rec_t, *vmtp_rec_ptr_t;


#endif	_VMTP1_DEFS_
