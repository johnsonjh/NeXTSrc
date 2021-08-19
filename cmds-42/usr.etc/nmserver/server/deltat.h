/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * delta.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/deltat.h,v $
 *
 * $Header: deltat.h,v 1.1 88/09/30 15:43:22 osdev Exp $
 *
 */

/*
 * Public definitions for delta-t transport protocol.
 */

/*
 * HISTORY:
 *  1-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for new request-reply transport interface. (RPCMOD)
 *
 *  4-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed retry characteristic constants - they are now in the param record.
 *
 *  4-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed deltat_abort.
 *
 * 13-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_DELTAT_
#define	_DELTAT_

#include <sys/boolean.h>

#include "transport.h"

/*
 * Delta-t specific failure codes.
 */
#define DELTAT_SUCCESS		TR_SUCCESS
#define DELTAT_ERROR_BASE	(-(TR_DELTAT_ENTRY * 16))
#define DELTAT_FAILURE		(1 + DELTAT_ERROR_BASE)
#define DELTAT_REMOTE_ABORT	(2 + DELTAT_ERROR_BASE)

/*
 * Exported functions.
 */
extern boolean_t deltat_init();
extern int deltat_retry();
#if	RPCMOD
extern int deltat_sendrequest();
#else	RPCMOD
extern int deltat_send();
#endif	RPCMOD

#endif	_DELTAT_
