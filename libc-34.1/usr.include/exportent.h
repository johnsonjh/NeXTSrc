/*	@(#)exportent.h	1.1 88/03/15 4.0NFSSRC SMI	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.5 88/02/07 (C) 1986 SMI
 */


/*
 * Exported file system table, see exportent(3)
 */ 

#define TABFILE "/etc/xtab"		/* where the table is kept */

/*
 * Options keywords
 */
#define ACCESS_OPT	"access"	/* machines that can mount fs */
#define ROOT_OPT	"root"		/* machines with root access of fs */
#define RO_OPT		"ro"		/* export read-only */
#define RW_OPT		"rw"		/* export read-mostly */
#define ANON_OPT	"anon"		/* uid for anonymous requests */
#define SECURE_OPT	"secure"	/* require secure NFS for access */
#define WINDOW_OPT	"window"	/* expiration window for credential */

struct exportent {
	char *xent_dirname;	/* directory (or file) to export */
	char *xent_options;	/* options, as above */
};

#ifdef __STRICT_BSD__
extern FILE *setexportent();
extern void endexportent();
extern int remexportent();
extern int addexportent();
extern char *getexportopt();
extern struct exportent *getexportent();
#else
extern FILE *setexportent(void);
extern void endexportent(FILE *filep);
extern int remexportent(FILE *filep, char *dirname);
extern int addexportent(FILE *filep, char *dirname, char *options);
extern char *getexportopt(struct exportent *xent, char *opt);
extern struct exportent *getexportent(FILE *filep);
#endif __STRICT_BSD__
