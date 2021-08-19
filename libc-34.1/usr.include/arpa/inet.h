/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)inet.h	5.1 (Berkeley) 5/30/85
 */

/*
 * External definitions for
 * functions in inet(3N)
 */
#ifndef	_INET_H_
#define	_INET_H_	1

#include <sys/types.h>
#include <netinet/in.h>

#ifdef	__STRICT_BSD__
unsigned long inet_addr();
char	*inet_ntoa();
struct	in_addr inet_makeaddr();
unsigned long inet_network();
#else
extern unsigned long inet_addr (char *cp);
extern char *inet_ntoa (struct in_addr in);
extern struct in_addr inet_makeaddr (int net, int lna);
extern unsigned long inet_network (char *cp);
#endif	__STRICT_BSD__

#endif	_INET_H_
