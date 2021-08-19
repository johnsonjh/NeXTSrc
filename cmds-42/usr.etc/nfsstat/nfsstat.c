#ifndef lint
static char sccsid[] = 	"@(#)nfsstat.c	1.2 88/05/17 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1984, 1987 by Sun Microsystems, Inc.
 */

/* 
 * nfsstat: Network File System statistics
 *
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <nlist.h>

struct nlist nl[] = {
#define	X_RCSTAT	0
	{{ "_rcstat" }},
#define	X_CLSTAT	1
	{{ "_clstat" }},
#define	X_RSSTAT	2
	{{ "_rsstat" }},
#define	X_SVSTAT	3
	{{ "_svstat" }},
	{{""}},
};

#define coreadj(x)	((int)x&0x7fffffff)

int kflag = 0;			/* set if using core instead of kmem */
int kmem;			/* file descriptor for /dev/kmem */
char *core = "/dev/kmem";	/* name for /dev/kmem */

#ifdef NeXT_MOD
char *mach = "/mach";		/* name for /mach */
#else
char *vmunix = "/vmunix";	/* name for /vmunix */
#endif NeXT_MOD

/*
 * client side rpc statistics
 */
struct {
        int     rccalls;
        int     rcbadcalls;
        int     rcretrans;
        int     rcbadxids;
        int     rctimeouts;
        int     rcwaits;
        int     rcnewcreds;
} rcstat;

/*
 * client side nfs statistics
 */
struct {
        int     nclsleeps;              /* client handle waits */
        int     nclgets;                /* client handle gets */
        int     ncalls;                 /* client requests */
        int     nbadcalls;              /* rpc failures */
        int     reqs[32];               /* count of each request */
} clstat;

/*
 * Server side rpc statistics
 */
struct {
        int     rscalls;
        int     rsbadcalls;
        int     rsnullrecv;
        int     rsbadlen;
        int     rsxdrcall;
} rsstat;

/*
 * server side nfs statistics
 */
struct {
        int     ncalls;         /* number of calls received */
        int     nbadcalls;      /* calls that failed */
        int     reqs[32];       /* count for each request */
} svstat;


static int	ccode, scode; 	/* Identify server and client code present */

main(argc, argv)
	char *argv[];
{
	char *options;
	int	cflag = 0;		/* client stats */
	int	sflag = 0;		/* server stats */
	int	nflag = 0;		/* nfs stats */
	int	rflag = 0;		/* rpc stats */
	int	zflag = 0;		/* zero stats after printing */


	if (argc >= 2 && *argv[1] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'c':
				cflag++;
				break;
			case 'n':
				nflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'z':
				if (getuid()) {
					fprintf(stderr,
					    "Must be root for z flag\n");
					exit(1);
				}
				zflag++;
				break;
			default:
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}
	if (argc >= 2) {
#ifdef NeXT_MOD
		mach = argv[1];
#else		
		vmunix = argv[1];
#endif NeXT_MOD
		argv++;
		argc--;
		if (argc == 2) {
			kflag++;
			core = argv[1];
			argv++;
			argc--;
		}
	}
	if (argc != 1) {
		usage();
	}


	setup(zflag);
	getstats();
	if (sflag && (!scode)) {
		fprintf(stderr,"nfsstat: kernel is not configured with the server nfs and rpc code.\n");
	}
	if ((sflag || (!sflag && !cflag)) && scode) {
		if (rflag || (!rflag && !nflag)) {
			sr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			sn_print(zflag);
		}
	}
	if (cflag && (!ccode)) {
		fprintf(stderr,"nfsstat: kernel is not configured with the client nfs and rpc code.\n");
	}
	if ((cflag || (!sflag && !cflag)) && ccode) {
		if (rflag || (!rflag && !nflag)) {
			cr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			cn_print(zflag);
		}
	}
	if (zflag) {
		putstats();
	}
	exit(0);
}

getstats()
{
	int size;

	if (ccode) {
		if (nl[X_RCSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (read(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
				fprintf(stderr, "can't read rcstat from kmem\n");
				exit(1);
			}
		} else {
			bzero(&rcstat, sizeof rcstat);
		}

		if (nl[X_CLSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (read(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
				fprintf(stderr, "can't read clstat from kmem\n");
				exit(1);
			}
		} else {
			bzero(&clstat, sizeof clstat);
		}
	}

	if (scode) {
		if (nl[X_RSSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (read(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
				fprintf(stderr, "can't read rsstat from kmem\n");
				exit(1);
			}
		} else {
			bzero(&rsstat, sizeof rsstat);
		}

		if (nl[X_SVSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (read(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
				fprintf(stderr, "can't read svstat from kmem\n");
				exit(1);
			}
		} else {
			bzero(&svstat, sizeof svstat);
		}
	}
}

putstats()
{
	if (ccode) {
		if (nl[X_RCSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (write(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
				fprintf(stderr, "can't write rcstat to kmem\n");
				exit(1);
			}
		}

		if (nl[X_CLSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (write(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
				fprintf(stderr, "can't write clstat to kmem\n");
				exit(1);
			}
		}
	}

	if (scode) {
		if (nl[X_RSSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (write(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
				fprintf(stderr, "can't write rsstat to kmem\n");
				exit(1);
			}
		}

		if (nl[X_SVSTAT].n_value) {
			if (lseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
				fprintf(stderr, "can't seek in kmem\n");
				exit(1);
			}
			if (write(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
				fprintf(stderr, "can't write svstat to kmem\n");
				exit(1);
			}
		}
	}
}

klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{

	if (kflag) {
		loc = coreadj(loc);
	}
	(void) lseek(fd, (long)loc, off);
}

setup(zflag)
	int zflag;
{
	register struct nlist *nlp;

#ifdef NeXT_MOD
	nlist(mach, nl);
#else
	nlist(vmunix, nl);
#endif NeXT_MOD
	if (nl[0].n_value == 0) {
		fprintf (stderr, "Variables missing from namelist\n");
		exit (1);
	}
	if (kflag) {
		for (nlp = nl; nlp < &nl[sizeof (nl)/sizeof (nl[0])]; nlp++)
			nlp->n_value = coreadj(nlp->n_value);
	}
	if ((kmem = open(core, zflag ? 2 : 0)) < 0) {
		perror(core);
		exit(1);
	}
	/* check to see if the client code is present */
	ccode = (nl[X_RCSTAT].n_type != 0);
	scode = (nl[X_RSSTAT].n_type != 0);
}

cr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nClient rpc:\n");
	fprintf(stdout,
	 "calls      badcalls   retrans    badxid     timeout    wait       newcred\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d%-11d%-11d\n",
	    rcstat.rccalls,
            rcstat.rcbadcalls,
            rcstat.rcretrans,
            rcstat.rcbadxids,
            rcstat.rctimeouts,
            rcstat.rcwaits,
            rcstat.rcnewcreds);
	if (zflag) {
		bzero(&rcstat, sizeof rcstat);
	}
}

sr_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer rpc:\n");
	fprintf(stdout,
	    "calls      badcalls   nullrecv   badlen     xdrcall\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d%-11d\n",
           rsstat.rscalls,
           rsstat.rsbadcalls,
           rsstat.rsnullrecv,
           rsstat.rsbadlen,
           rsstat.rsxdrcall);
	if (zflag) {
		bzero(&rsstat, sizeof rsstat);
	}
}

#define RFS_NPROC       18
char *nfsstr[RFS_NPROC] = {
	"null",
	"getattr",
	"setattr",
	"root",
	"lookup",
	"readlink",
	"read",
	"wrcache",
	"write",
	"create",
	"remove",
	"rename",
	"link",
	"symlink",
	"mkdir",
	"rmdir",
	"readdir",
	"fsstat" };

cn_print(zflag)
	int zflag;
{
	int i;

	fprintf(stdout, "\nClient nfs:\n");
	fprintf(stdout,
	    "calls      badcalls   nclget     nclsleep\n");
	fprintf(stdout,
	    "%-11d%-11d%-11d%-11d\n",
            clstat.ncalls,
            clstat.nbadcalls,
            clstat.nclgets,
            clstat.nclsleeps);
	req_print((int *)clstat.reqs, clstat.ncalls);
	if (zflag) {
		bzero(&clstat, sizeof clstat);
	}
}

sn_print(zflag)
	int zflag;
{
	fprintf(stdout, "\nServer nfs:\n");
	fprintf(stdout, "calls      badcalls\n");
	fprintf(stdout, "%-11d%-11d\n", svstat.ncalls, svstat.nbadcalls);
	req_print((int *)svstat.reqs, svstat.ncalls);
	if (zflag) {
		bzero(&svstat, sizeof svstat);
	}
}


req_print(req, tot)
	int	*req;
	int	tot;
{
	int	i, j;
	char	fixlen[128];

	for (i=0; i<=RFS_NPROC / 7; i++) {
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			fprintf(stdout, "%-11s", nfsstr[j]);
		}
		fprintf(stdout, "\n");
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			if (tot) {
				sprintf(fixlen,
				    "%d %2d%% ", req[j], (req[j]*100)/tot);
			} else {
				sprintf(fixlen, "%d 0%% ", req[j]);
			}
			fprintf(stdout, "%-11s", fixlen);
		}
		fprintf(stdout, "\n");
	}
}

usage()
{
#ifdef NeXT_MOD
	fprintf(stderr, "nfsstat [-cnrsz] [mach] [core]\n");
#else
	fprintf(stderr, "nfsstat [-cnrsz] [vmunix] [core]\n");
#endif NeXT_MOD
	exit(1);
}

min(a,b)
	int a,b;
{
	if (a<b) {
		return(a);
	}
	return(b);
}
