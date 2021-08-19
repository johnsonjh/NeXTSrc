#ifndef lint
static char sccsid[] = 	"@(#)domainname.c	1.2 88/05/20 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
/*
 * domainname -- get (or set domainname)
 */
#include <stdio.h>
#include <sys/param.h>

char domainname[MAXHOSTNAMELEN];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (setdomainname(*argv,strlen(*argv)))
			perror("setdomainname");
		myerrno = errno;
	} else {
		getdomainname(domainname,sizeof(domainname));
		myerrno = errno;
		printf("%s\n",domainname);
	}
	exit(myerrno);
}
