#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)getrpcport.c	1.1 88/05/10 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.4
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>

getrpcport(host, prognum, versnum, proto)
	char *host;
{
	struct sockaddr_in addr;
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == NULL)
		return (0);
	bcopy(hp->h_addr, (char *) &addr.sin_addr, hp->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port =  0;
	return (pmap_getport(&addr, prognum, versnum, proto));
}
