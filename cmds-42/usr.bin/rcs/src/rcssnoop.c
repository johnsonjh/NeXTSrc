/*
 *                     Logging of RCS commands co and ci
 *
 * $Header: snoop.c,v 4.7 88/10/05 17:36:34 osdev Exp $ Purdue CS
 *******************************************************************
 * This program appends argv[1] to the file SNOOPFILE.
 * To avoid overlaps, it creates a lockfile with name lock in the same
 * directory as SNOOPFILE. SNOOPFILE must be defined in the cc command. 
 * Prints an error message if lockfile doesn't get deleted after
 * MAXTRIES tries.
 *******************************************************************
 *
 * Copyright (C) 1982 by Walter F. Tichy
 *                       Purdue University
 *                       Computer Science Department
 *                       West Lafayette, IN 47907
 *
 * Copyright (C) 1988 NeXT, Inc.
 *
 * All rights reserved. No part of this software may be sold or distributed
 * in any form or by any means without the prior written permission of the 
 * author.
 * Report problems and direct all inquiries to Tichy@purdue (ARPA net).
 */


/* $Log:	snoop.c,v $
 * Revision 4.7  88/10/05  17:36:34  osdev
 * Makefile changes
 * 
 * Revision 4.6  88/10/05  17:09:05  osdev
 * Makefile changes
 * 
 * Revision 4.5  88/10/05  13:58:22  root
 * New version
 * 
 * Revision 4.4  88/10/05  01:35:03  root
 * More cleanup
 * 
 * Revision 4.3  88/10/05  01:18:06  osdev
 * Makefile cleanup
 * 
 * Revision 4.2  88/10/05  01:06:04  osdev
 * Makefile cleanup
 * 
 * Revision 3.6  88/10/03  00:20:27  gk
 * New Version.
 * 
 * Revision 3.5  88/09/24  19:07:21  gk
 * ifdef SNOOPFILE'd snoop.c
 * 
 * Revision 3.4  88/09/24  18:48:50  gk
 * Commented out version string.
 * 
 * Revision 3.3  88/09/22  21:32:22  gk
 * Initial NeXT revision.
 * 
 * Revision 3.2  82/12/04  17:14:31  wft
 * Added rcsbase.h, changed SNOOPDIR to SNOOPFILE, reintroduced
 * error message in case of permanent locking.
 * 
 * Revision 3.1  82/10/18  21:22:03  wft
 * Number of polls now 20, no error message if critical section can't
 * be entered.
 * 
 * Revision 2.3  82/07/01  23:49:28  wft
 * changed copyright notice only.
 * 
 * Revision 2.2  82/06/03  20:00:10  wft
 * changed name from rcslog to snoop, replaced LOGDIR with SNOOPDIR.
 * 
 * Revision 2.1  82/05/06  17:55:54  wft
 * Initial revision
 *
 */

#include "rcsbase.h"
#define fflsbuf _flsbuf
/* undo redefinition of putc in rcsbase.h */

char  lockfname[NCPPN];
FILE * logfile;
int lockfile;

#define MAXTRIES 20

main(argc,argv)
int argc; char * argv[];
/* writes argv[1] to SNOOPFILE and appends a newline. Invoked as follows:
 * rcslog logmessage
 */
{       int tries;
        register char * lastslash, *sp;

#if	NeXT
	sp = strrchr(SNOOPFILE, '/');
	if (sp)
		sp++;
	else
		sp = SNOOPFILE;
	sprintf(lockfname, "/tmp/%s,lockfile", sp);
#else	NeXT
        strcpy(lockfname,SNOOPFILE);
        lastslash = sp = lockfname;
        while (*sp) if (*sp++ =='/') lastslash=sp; /* points beyond / */
        strcpy(lastslash,",lockfile");
#endif	NeXT
        tries=0;
        while (((lockfile=creat(lockfname, 000)) == -1) && (tries<=MAXTRIES)) {
                tries++;
                sleep(5);
        }
        if (tries<=MAXTRIES) {
                close(lockfile);
                if ((logfile=fopen(SNOOPFILE,"a")) ==NULL) {
                        fprintf(stderr,"Can't open logfile %s\n",SNOOPFILE);
                } else {
                        fputs(argv[1],logfile);
                        putc('\n',logfile);
                        fclose(logfile);
                }
                unlink(lockfname);
        } else {
                fprintf(stderr,"RCS logfile %s seems permanently locked.\n",SNOOPFILE);
                fprintf(stderr,"Please alert system administrator\n");
        }
}
