/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)conf.h	5.15 (Berkeley) 1/1/89
 */

/*
**  CONF.H -- All user-configurable parameters for sendmail
*/

#if	NeXT_MOD
/*
 * Reject messages to large mailing lists that have no body.
 */
# define REJECT_MIN		10		/* minimum bytes in body */
#endif	NeXT_MOD

/*
**  Table sizes, etc....
**	There shouldn't be much need to change these....
*/

# define MAXLINE	1024		/* max line length */
# define MAXNAME	256		/* max length of a name */
# define MAXFIELD	2500		/* max total length of a hdr field */
# define MAXPV		40		/* max # of parms to mailers */
# define MAXHOP		17		/* max value of HopCount */
# define MAXATOM	100		/* max atoms per address */
# define MAXMAILERS	25		/* maximum mailers known to system */
#if	NeXT_MOD
# define MAXRWSETS	50		/* max # of sets of rewriting rules */
#else	NeXT_MOD
# define MAXRWSETS	30		/* max # of sets of rewriting rules */
#endif	NeXT_MOD
# define MAXPRIORITIES	25		/* max values for Precedence: field */
# define MAXTRUST	30		/* maximum number of trusted users */
# define MAXUSERENVIRON	40		/* max # of items in user environ */
# define QUEUESIZE	600		/* max # of jobs per queue run */
#if	NeXT_MOD
# define MAXMXHOSTS	10		/* max # of MX records */
#endif	NeXT_MOD

/*
**  Compilation options.
**
**	#define these if they are available; comment them out otherwise.
*/

# define DBM		1	/* use DBM library (requires -ldbm) */
# define NDBM		1	/* new DBM library available (requires DBM) */
# define LOG		1	/* enable logging */
# define SMTP		1	/* enable user and server SMTP */
# define QUEUE		1	/* enable queueing */
# define UGLYUUCP	1	/* output ugly UUCP From lines */
# define DAEMON		1	/* include the daemon (requires IPC & SMTP) */
/* # define FLOCK		1	use flock file locking */
# define SETPROCTITLE	1	/* munge argv to display current status */
/* # define WIZ		1	allow wizard mode */
/* #define NEWTZCODE	1	use new timezone code */
/*#define USG		1	/* building for USG (S3, S5) system */
# define SCANF		1	/* enable scanf format in F lines */

#ifdef	YELLOWPAGES
# define YELLOW		1	/* Call yellow pages for aliases */
# define ALIAS_MAP	"mail.aliases"	/* default yp map for aliases */
#endif	YELLOWPAGES

# define FreezeMode 	0644	/* creation mode for Freeze file: */
				/* Must be public read if using NFS */
