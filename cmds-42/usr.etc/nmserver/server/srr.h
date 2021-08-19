/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * srr.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/srr.h,v $
 *
 * $Header: srr.h,v 1.1 88/09/30 15:45:30 osdev Exp $
 *
 */

/*
 * Definitions for the Simple request-response transport protocol.
 */

/*
 * HISTORY:
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed retry characteristic constants - they are now in the param record.
 *
 *  4-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added srr_max_data_size.  Removed srr_abort.
 *
 *  5-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_SRR_
#define	_SRR_

#include <sys/boolean.h>

#include "transport.h"

/*
 * srr specific failure codes.
 */
#define SRR_SUCCESS		(0)
#define SRR_ERROR_BASE		(-(TR_SRR_ENTRY * 16))
#define SRR_TOO_LARGE		(1 + SRR_ERROR_BASE)
#define SRR_FAILURE		(2 + SRR_ERROR_BASE)
#define SRR_ENCRYPT_FAILURE	(3 + SRR_ERROR_BASE)


/*
 * The maximum amount of data that can be placed in a request or a response.
 */
extern int srr_max_data_size;

/*
 * Exported functions.
 */
extern boolean_t srr_init();
extern int srr_send();

#endif	_SRR_
