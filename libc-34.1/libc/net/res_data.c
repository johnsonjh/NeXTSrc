/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * This file contains global data but it is a `private_extern' in the
 * shared library so that its address and size can change.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_init.c	6.5 (Berkeley) 4/11/86";
#endif LIBC_SCCS and not lint

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>
#include <resolv.h>

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

/*
 * Resolver state default settings
 */

#ifndef RES_TIMEOUT
#define RES_TIMEOUT 4
#endif	RES_TIMEOUT

struct state _res = {
    RES_TIMEOUT,                 /* retransmition time interval */
    4,                           /* number of times to retransmit */
    RES_RECURSE|RES_DEFNAMES,    /* options flags */
    1,                           /* number of name servers */
};

