/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * datagram.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/datagram.h,v $
 *
 * $Header: datagram.h,v 1.1 88/09/30 15:43:15 osdev Exp $
 *
 */

/*
 * Definitions for the datagram transport protocol.
 */

/*
 * HISTORY:
 *  4-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added datagram_max_data_size.  Removed datagram_abort.
 *
 *  3-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_DATAGRAM_
#define	_DATAGRAM_

#include <sys/boolean.h>
#include "transport.h"

/*
 * Datagram specific failure codes.
 */
#define DATAGRAM_ERROR_BASE		(-(TR_DATAGRAM_ENTRY * 16))
#define DATAGRAM_TOO_LARGE		(1 + DATAGRAM_ERROR_BASE)
#define DATAGRAM_SEND_FAILURE		(2 + DATAGRAM_ERROR_BASE)

/*
 * The maximum amount of data that can be placed in a datagram.
 */
extern int datagram_max_data_size;

/*
 * Exported functions.
 */
extern boolean_t datagram_init();
extern int datagram_send();

#endif	_DATAGRAM_
