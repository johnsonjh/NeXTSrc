/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * netname_defs.h
 *
 * $Source: /usr1/dpj/netmsg/src/RCS/netname_defs.h,v $
 *
 * $Header: netname_defs.h,v 1.2 88/02/22 11:49:35 dpj Exp $
 *
 */

/*
 * Definitions for the mig interface to the network name service.
 */

/*
 * HISTORY:
 * 28-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added NETNAME_PENDING.
 *
 * 23-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Copied from the previous version of the network server.
 *
 */

#ifndef	_NETNAME_DEFS_
#define	_NETNAME_DEFS_

#define NETNAME_SUCCESS		(0)
#define	NETNAME_PENDING		(-1)
#define NETNAME_NOT_YOURS	(1000)
#define NETNAME_NOT_CHECKED_IN	(1001)
#define NETNAME_NO_SUCH_HOST	(1002)
#define NETNAME_HOST_NOT_FOUND	(1003)

typedef char netname_name_t[80];

#endif NETNAME_DEFS_
