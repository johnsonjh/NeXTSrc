#ifdef sccs
static char     sccsid[] = "@(#)pcnfsd.c	1.4";

#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

/*
 * pcnfsd.c 
 *
 * pcnfsd is intended to remedy the lack of certain critical generic network
 * services by providing an simple, customizable set of RPC-based
 * mechanisms. For this reason, Sun Microsystems Inc. is distributing it
 * in source form as part of the PC-NFS release. 
 *
 * Background: The first NFS networks were composed of systems running
 * derivatives of the 4.2BSD release of Unix (Sun's, VAXes, Goulds and
 * Pyramids). The immediate utility of the resulting networks was derived
 * not only from NFS but also from the availability of a number of TCP/IP
 * based network services derived from 4.2BSD. Furthermore the thorny
 * question of network-wide user authentication, while remaining a
 * security hole, was solved at least in terms of a convenient usage model
 * by the Yellow Pages distributed data base facility, which allows
 * multiple Unix systems to refer to common password and group files. 
 *
 * The PC-NFS Dilemma: When Sun Microsystems Inc. ported NFS to PC's, two
 * things became apparent. First, the memory constraints of the typical PC
 * meant that it would be impossible to incorporate the pervasive TCP/IP
 * based service suite in a resident fashion. Indeed it was not at all
 * clear that the 4.2BSD services would prove sufficient: with the advent
 * of Unix System V and (experimental) VAX-VMS NFS implementations, we had
 * to consider the existence of networks with no BSD-derived Unix hosts.
 * The two key types of functionality we needed to provide were remote
 * login and print spooling. The second critical issue  was that of user
 * authentication. Traditional time-sharing systems such as Unix and VMS
 * have well- established user authentication mechanisms based upon user
 * id's and passwords: by defining appropriate mappings, these could
 * suffice for network-wide authentication provided that appropriate
 * administrative procedures were enforced. The PC, however, is typically
 * a single-user system, and the standard DOS operating environment
 * provides no user authentication mechanisms. While this is acceptable
 * within a single PC, it causes problems when attempting to connect to a
 * heterogeneous network of systems in which access control, file space
 * allocation, and print job accounting and routing may all be based upon
 * a user's identity. The initial (and default) approach is to use the
 * pseudo-identity 'nobody' defined as part of NFS to handle problems such
 * as this. However, taking ease of use into consideration, it became
 * necessary to provide a mechanism for establishing a user's identity. 
 *
 * Initially we felt that we needed to implement two types of functionality:
 * user authentication and print spooling. (Remote login is addressed by
 * the Telnet module.) Since no network services were defined within the
 * NFS architecture to support these, it was decided to implement them in
 * a fairly portable fashion using Sun's Remote Procedure Call protocol.
 * Since these mechanisms will need to be re-implemented ion a variety of
 * software environments, we have tried to define a very general model. 
 *
 * Authentication: NFS adopts the Unix model of using a pair of integers
 * (uid, gid) to define a user's identity. This happens to map tolerably
 * well onto the VMS system. 'pcnfsd' implements a Remote Procedure which
 * is required to map a username and password into a (uid, gid) pair.
 * Since we cannot predict what mapping is to be performed, and since we
 * do not wish to pass clear-text passwords over the net, both the
 * username and the password are mildly scrambled using a simple XOR
 * operation. The intent is not to be secure (the present NFS architecture
 * is inherently insecure) but to defeat "browsers". 
 *
 * The authentication RPC will be invoked when the user enters the PC-NFS
 * command: 
 *
 * NET NAME user [password|*] 
 *
 *
 * Printing: The availability of NFS file operations simplifies the print
 * spooling mechanisms. There are two services which 'pcnfsd' has to
 * provide:
 *   pr_init:	given the name of the client system, return the
 * name of a directory which is exported via NFS and in which the client
 * may create spool files.
 *  pr_start: given a file name, a user name, the printer name, the client
 * system name and an option string, initiate printing of the file
 * on the named printer. The file name is relative to the directory
 * returned by pr_init. pr_start is to be "idempotent": a request to print
 * a file which is already being printed has no effect. 
 *
 * Intent: The first versions of these procedures are implementations for Sun
 * 2.0/3.0 software, which will also run on VAX 4.2BSD systems. The intent
 * is to build up a set of implementations for different architectures
 * (Unix System V, VMS, etc.). Users are encouraged to submit their own
 * variations for redistribution. If you need a particular variation which
 * you don't see here, either code it yourself (and, hopefully, send it to
 * us at Sun) or contact your Customer Support representative. 
 */

#include <sys/types.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <pwd.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>

/*  #define DEBUG 1  */

#ifdef DEBUG
int             buggit = 0;

#endif DEBUG

/*
 * *************** RPC parameters ******************** 
 */
#define	PCNFSDPROG	(long)150001
#define	PCNFSDVERS	(long)1
#define	PCNFSD_AUTH	(long)1
#define	PCNFSD_PR_INIT	(long)2
#define	PCNFSD_PR_START	(long)3

/*
 * ************* Other #define's ********************** 
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define	zchar		0x5b

/*
 * *********** XDR structures, etc. ******************** 
 */
enum arstat {
	AUTH_RES_OK, AUTH_RES_FAKE, AUTH_RES_FAIL
};
enum pirstat {
	PI_RES_OK, PI_RES_NO_SUCH_PRINTER, PI_RES_FAIL
};
enum psrstat {
	PS_RES_OK, PS_RES_ALREADY, PS_RES_NULL, PS_RES_NO_FILE,
	PS_RES_FAIL
};

struct auth_args {
	char           *aa_ident;
	char           *aa_password;
};

struct auth_results {
	enum arstat     ar_stat;
	long            ar_uid;
	long            ar_gid;
};

struct pr_init_args {
	char           *pia_client;
	char           *pia_printername;
};

struct pr_init_results {
	enum pirstat    pir_stat;
	char           *pir_spooldir;
};

struct pr_start_args {
	char           *psa_client;
	char           *psa_printername;
	char           *psa_username;
	char           *psa_filename;	/* within the spooldir */
	char           *psa_options;
};

struct pr_start_results {
	enum psrstat    psr_stat;
};


/*
 * ****************** Misc. ************************ 
 */

char           *authproc();
char           *pr_start();
char           *pr_init();
struct stat     statbuf;

char            pathname[MAXPATHLEN];
char            new_pathname[MAXPATHLEN];
char            spoolname[MAXPATHLEN];

/*
 * ************** Support procedures *********************** 
 */
scramble(s1, s2)
	char           *s1;
	char           *s2;
{
	while (*s1) {
		*s2++ = (*s1 ^ zchar) & 0x7f;
		s1++;
	}
	*s2 = 0;
}

free_child()
{
	int             pid;
	int             pstatus;

	pid = wait(&pstatus);	/* clear exit of child process */

#ifdef DEBUG
	if (buggit || pstatus)
		fprintf(stderr, "FREE_CHILD: process #%d exited with status 0X%x\r\n",
			pid, pstatus);
#endif DEBUG
	return;
}

/*
 * *************** XDR procedures ***************** 
 */
bool_t
xdr_auth_args(xdrs, aap)
	XDR            *xdrs;
	struct auth_args *aap;
{
	return (xdr_string(xdrs, &aap->aa_ident, 32) &&
		xdr_string(xdrs, &aap->aa_password, 64));
}

bool_t
xdr_auth_results(xdrs, arp)
	XDR            *xdrs;
	struct auth_results *arp;
{
	return (xdr_enum(xdrs, &arp->ar_stat) &&
		xdr_long(xdrs, &arp->ar_uid) &&
		xdr_long(xdrs, &arp->ar_gid));
}

bool_t
xdr_pr_init_args(xdrs, aap)
	XDR            *xdrs;
	struct pr_init_args *aap;
{
	return (xdr_string(xdrs, &aap->pia_client, 64) &&
		xdr_string(xdrs, &aap->pia_printername, 64));
}

bool_t
xdr_pr_init_results(xdrs, arp)
	XDR            *xdrs;
	struct pr_init_results *arp;
{
	return (xdr_enum(xdrs, &arp->pir_stat) &&
		xdr_string(xdrs, &arp->pir_spooldir, 255));
}

bool_t
xdr_pr_start_args(xdrs, aap)
	XDR            *xdrs;
	struct pr_start_args *aap;
{
	return (xdr_string(xdrs, &aap->psa_client, 64) &&
		xdr_string(xdrs, &aap->psa_printername, 64) &&
		xdr_string(xdrs, &aap->psa_username, 64) &&
		xdr_string(xdrs, &aap->psa_filename, 64) &&
		xdr_string(xdrs, &aap->psa_options, 64));
}

bool_t
xdr_pr_start_results(xdrs, arp)
	XDR            *xdrs;
	struct pr_start_results *arp;
{
	return (xdr_enum(xdrs, &arp->psr_stat));
}



/*
 * ********************** main ********************* 
 */

main(argc, argv)
	int             argc;
	char          **argv;
{
	int             f1, f2, f3;

		extern          xdr_string_array();

	if (fork() == 0) {
		if (argc < 2)
			strcpy(spoolname, "/usr/spool/lp");
		else
			strcpy(spoolname, argv[1]);
#ifdef DEBUG
		if (argc > 2)
			buggit++;
#endif DEBUG

		if (stat(spoolname, &statbuf) || !(statbuf.st_mode & S_IFDIR)) {
			fprintf(stderr,
				"pcnfsd: invalid spool directory %s\r\n", spoolname);
			exit(1);
		}

/*  Comment out for now

		if ((f1 = open("/dev/null", O_RDONLY)) == -1) {
			fprintf(stderr, "pcnfsd: couldn't open /dev/null\r\n");
			exit(1);
		}
		if ((f2 = open("/dev/console", O_WRONLY)) == -1) {
			fprintf(stderr, "pcnfsd: couldn't open /dev/console\r\n");
			exit(1);
		}
		if ((f3 = open("/dev/console", O_WRONLY)) == -1) {
			fprintf(stderr, "pcnfsd: couldn't open /dev/console\r\n");
			exit(1);
		}
		dup2(f1, 0);
		dup2(f2, 1);
		dup2(f3, 2);
		
end of commented out stuff */

		registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_AUTH, authproc,
			xdr_auth_args, xdr_auth_results);
		registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_PR_INIT, pr_init,
			xdr_pr_init_args, xdr_pr_init_results);
		registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_PR_START, pr_start,
			xdr_pr_start_args, xdr_pr_start_results);
		svc_run();
		fprintf(stderr, "pcnfsd: error: svc_run returned\r\n");
		exit(1);
	}
	
}

/*
 * ******************* RPC procedures ************** 
 */

char           *
authproc(a)
	struct auth_args *a;
{
	static struct auth_results r;
	char            username[32];
	char            password[64];
	int             c1, c2;
	struct passwd  *p;

	r.ar_stat = AUTH_RES_FAIL;	/* assume failure */
	scramble(a->aa_ident, username);
	scramble(a->aa_password, password);

#ifdef DEBUG
	if (buggit)
		fprintf(stderr, "AUTHPROC username=%s\r\n", username);
#endif DEBUG

	p = getpwnam(username);
	if (p == NULL)
		return ((char *) &r);
	c1 = strlen(password);
	c2 = strlen(p->pw_passwd);
	if ((c1 && !c2) || (c2 && !c1) ||
		(strcmp(p->pw_passwd, crypt(password, p->pw_passwd)))) {
		return ((char *) &r);
	}
	r.ar_stat = AUTH_RES_OK;
	r.ar_uid = p->pw_uid;
	r.ar_gid = p->pw_gid;
	return ((char *) &r);
}


char           *
pr_init(pi_arg)
	struct pr_init_args *pi_arg;
{
	int             dir_mode = 0777;
	static struct pr_init_results pi_res;

	/* get pathname of current directory and return to client */
	strcpy(pathname, spoolname);	/* first the spool area */
	strcat(pathname, "/");	/* append a slash */
	strcat(pathname, pi_arg->pia_client);
	/* now the host name */
	mkdir(pathname);	/* ignore the return code */
	if (stat(pathname, &statbuf) || !(statbuf.st_mode & S_IFDIR)) {
		fprintf(stderr,
			"pcnfsd: unable to create spool directory %s\r\n",
			pathname);
		pathname[0] = 0;/* null to tell client bad vibes */
		pi_res.pir_stat = PI_RES_FAIL;
	} else {
		pi_res.pir_stat = PI_RES_OK;
	}
	pi_res.pir_spooldir = &pathname[0];
	chmod(pathname, dir_mode);

#ifdef DEBUG
	if (buggit)
		fprintf(stderr, "PR_INIT pathname=%s\r\n", pathname);
#endif DEBUG

	return ((char *) &pi_res);
}

char           *
pr_start(ps_arg)
	struct pr_start_args *ps_arg;
{
	static struct pr_start_results ps_res;
	int             pid;
	int             free_child();
	char            printer_opt[64];
	char            username_opt[64];
	char            clientname_opt[64];
	struct passwd  *p;
	long		rnum;
	char		snum[20];
	int		z;

	strcpy(printer_opt, "-P");
	strcpy(username_opt, "-J");
	strcpy(clientname_opt, "-C");

	signal(SIGCHLD, free_child);	/* when child terminates it sends */
	/* a signal which we must get */
	strcpy(pathname, spoolname);	/* build filename */
	strcat(pathname, "/");
	strcat(pathname, ps_arg->psa_client);	/* /spool/host */
	strcat(pathname, "/");	/* /spool/host/ */
	strcat(pathname, ps_arg->psa_filename);	/* /spool/host/file */

#ifdef DEBUG
	if (buggit) {
		fprintf(stderr, "PR_START pathname=%s\r\n", pathname);
		fprintf(stderr, "PR_START username= %s\r\n", ps_arg->psa_username);
		fprintf(stderr, "PR_START client= %s\r\n", ps_arg->psa_client);
	}
#endif DEBUG

	strcat(printer_opt, ps_arg->psa_printername);
	/* make it (e.g.) -Plw	 */
	strcat(username_opt, ps_arg->psa_username);
	/* make it (e.g.) -Jbilly	 */
	strcat(clientname_opt, ps_arg->psa_client);
	/* make it (e.g.) -Cmypc	 */

	if (stat(pathname, &statbuf)) {
		/*
		 * We can't stat the file. Let's try appending '.spl' and
		 * see if it's already in progress. 
		 */

#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...can't stat it.\r\n");
#endif DEBUG

		strcat(pathname, ".spl");
		if (stat(pathname, &statbuf)) {
			/*
			 * It really doesn't exist. 
			 */

#ifdef DEBUG
			if (buggit)
				fprintf(stderr, "...PR_START returns PS_RES_NO_FILE\r\n");
#endif DEBUG

			ps_res.psr_stat = PS_RES_NO_FILE;
			return ((char *) &ps_res);
		}
		/*
		 * It is already on the way. 
		 */

#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...PR_START returns PS_RES_ALREADY\r\n");
#endif DEBUG

		ps_res.psr_stat = PS_RES_ALREADY;
		return ((char *) &ps_res);
	}
	if (statbuf.st_size == 0) {
		/*
		 * Null file - don't print it, just kill it. 
		 */
		unlink(pathname);

#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...PR_START returns PS_RES_NULL\r\n");
#endif DEBUG

		ps_res.psr_stat = PS_RES_NULL;
		return ((char *) &ps_res);
	}
	/*
	 * The file is real, has some data, and is not already going out.
	 * We rename it by appending '.spl' and exec "lpr" to do the
	 * actual work. 
	 */
	strcpy(new_pathname, pathname);
	strcat(new_pathname, ".spl");

#ifdef DEBUG
	if (buggit)
		fprintf(stderr, "...renaming %s -> %s\r\n", pathname, new_pathname);
#endif DEBUG

	/*
	 * See if the new filename exists so as not to overwrite it.
	 */


	for(z = 0; z <100; z++) {
		if (!stat(new_pathname, &statbuf)){
			strcpy(new_pathname, pathname);  /* rebuild a new name */
			sprintf(snum,"%ld",random());		/* get some number */
			strncat(new_pathname, snum, 3);
			strcat(new_pathname, ".spl");		/* new spool file */
#ifdef DEBUG
			if (buggit)
				fprintf(stderr, "...created new spl file -> %s\r\n", new_pathname);

#endif DEBUG
		} else
			break;
	}
		

	if (rename(pathname, new_pathname)) {
		/*
		 * CAVEAT: Microsoft changed rename for Microsoft C V3.0.
		 * Check this if porting to Xenix. 
		 */
		/*
		 * Should never happen. 
		 */
		fprintf(stderr, "pcnfsd: spool file rename (%s->%s) failed.\r\n",
			pathname, new_pathname);
		ps_res.psr_stat = PS_RES_FAIL;
		return ((char *) &ps_res);
	}
	pid = fork();
	if (pid == 0) {
#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...print options =%s\r\n", ps_arg->psa_options);
#endif DEBUG
		
		if (ps_arg->psa_options[1] == 'd') {
			/*
			 * This is a Diablo print stream. Apply the ps630
			 * filter with the appropriate arguments. 
			 */
#ifdef DEBUG
			if (buggit)
				fprintf(stderr, "...run_ps630 invoked\r\n");
#endif DEBUG
			run_ps630(new_pathname, ps_arg->psa_options);
		}
		execlp("/usr/ucb/lpr",
			"lpr",
			"-s",
			"-r",
			printer_opt,
			username_opt,
			clientname_opt,
			new_pathname,
			0);
		perror("pcnfsd: exec lpr failed");
		exit(0);	/* end of child process */
	} else if (pid == -1) {
		perror("pcnfsd: fork failed");

#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...PR_START returns PS_RES_FAIL\r\n");
#endif DEBUG

		ps_res.psr_stat = PS_RES_FAIL;
		return ((char *) &ps_res);
	} else {

#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...forked child #%d\r\n", pid);
#endif DEBUG


#ifdef DEBUG
		if (buggit)
			fprintf(stderr, "...PR_START returns PS_RES_OK\r\n");
#endif DEBUG

		ps_res.psr_stat = PS_RES_OK;
		return ((char *) &ps_res);
	}
}

char           *
mapfont(f, i, b)
	char            f;
	char            i;
	char            b;
{
	static char     fontname[64];

	fontname[0] = 0;	/* clear it out */

	switch (f) {
	case 'c':
		strcpy(fontname, "Courier");
		break;
	case 'h':
		strcpy(fontname, "Helvetica");
		break;
	case 't':
		strcpy(fontname, "Times");
		break;
	default:
		strcpy(fontname, "Times-Roman");
		goto exit;
	}
	if (i != 'o' && b != 'b') {	/* no bold or oblique */
		if (f == 't')	/* special case Times */
			strcat(fontname, "-Roman");
		goto exit;
	}
	strcat(fontname, "-");
	if (b == 'b')
		strcat(fontname, "Bold");
	if (i == 'o')		/* o-blique */
		strcat(fontname, f == 't' ? "Italic" : "Oblique");

exit:	return (&fontname[0]);
}

/*
 * run_ps630 performs the Diablo 630 emulation filtering process. ps630 is
 * currently broken in the Sun release: it will not accept point size or
 * font changes. If your version is fixed, define the symbol
 * PS630_IS_FIXED and rebuild pcnfsd. 
 */


run_ps630(file, options)
	char           *file;
	char           *options;
{
	char            tmpfile[256];
	char            commbuf[256];
	int             i;

	strcpy(tmpfile, file);
	strcat(tmpfile, "X");	/* intermediate file name */

#ifdef PS630_IS_FIXED
	sprintf(commbuf, "ps630 -s %c%c -p %s -f ",
		options[2], options[3], tmpfile);
	strcat(commbuf, mapfont(options[4], options[5], options[6]));
	strcat(commbuf, " -F ");
	strcat(commbuf, mapfont(options[7], options[8], options[9]));
	strcat(commbuf, "  ");
	strcat(commbuf, file);
#else PS630_IS_FIXED
	/*
	 * The pitch and font features of ps630 appear to be broken at
	 * this time. If you think it's been fixed at your site, define
	 * the compile-time symbol `ps630_is_fixed'. 
	 */
	sprintf(commbuf, "/usr/local/bin/ps630 -p %s %s", tmpfile, file);
#endif PS630_IS_FIXED


	if (i = system(commbuf)) {
		/*
		 * Under (un)certain conditions, ps630 may return -1
		 * even if it worked. Hence the commenting out of this
		 * error report. 
		 */
		 /* fprintf(stderr, "\r\n\nrun_ps630 rc = %d\r\n", i) */ ;
		/* exit(1); */
	}
	if (rename(tmpfile, file)) {
		perror("run_ps630: rename");
		exit(1);
	}
}
