#ifndef lint
static char sccsid[] = 	"@(#)yppasswd.c	1.2 88/07/29 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <rpc/rpc.h>
#ifdef SECURE_RPC
#include <rpc/key_prot.h>
#include <rpcsvc/ypclnt.h>
#endif SECURE_RPC
#include <rpcsvc/yppasswd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <errno.h>

#ifdef SECURE_RPC
#define PKMAP	"publickey.byname"
#endif SECURE_RPC

extern char *index();
extern char *sprintf();
extern long time();

struct yppasswd *getyppw();
char *oldpass;
char *newpass;

main(argc, argv)	
	char **argv;
{
	int ans, port, ok;
	struct yppasswd *yppasswd;
	char *domain;
	char *master;

	if (yp_get_default_domain(&domain) != 0) {
		(void)fprintf(stderr, "can't get domain\n");
		exit(1);
	}
	if (yp_master(domain, "passwd.byname", &master) != 0) {
		(void)fprintf(stderr, "can't get master for passwd file\n");
		exit(1);
	}
	port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
		IPPROTO_UDP);
	if (port == 0) {
		(void)fprintf(stderr, "%s is not running yppasswd daemon\n",
			      master);
		exit(1);
	}
	if (port >= IPPORT_RESERVED) {
		(void)fprintf(stderr,
		    "yppasswd daemon is not running on privileged port\n");
		exit(1);
	}
	yppasswd = getyppw(argc, argv);
	ans = callrpc(master, YPPASSWDPROG, YPPASSWDVERS,
	    YPPASSWDPROC_UPDATE, xdr_yppasswd, yppasswd, xdr_int, &ok);
	if (ans != 0) {
		clnt_perrno(ans);
		(void)fprintf(stderr, "\n");
		(void)fprintf(stderr, "couldn't change passwd\n");
		exit(1);
	}
	if (ok != 0) {
		(void)fprintf(stderr, "couldn't change passwd\n");
		exit(1);
	}
	(void)printf("yellow pages passwd changed on %s\n", master);
#ifdef SECURE_RPC
	reencrypt_secret(domain);
#endif SECURE_RPC
}

#ifdef SECURE_RPC
/*
 * If the user has a secret key, reencrypt it.
 * Otherwise, be quiet.
 */
reencrypt_secret(domain)
	char *domain;
{
	char who[MAXNETNAMELEN+1];
	char secret[HEXKEYBYTES+1];
	char public[HEXKEYBYTES+1];
	char crypt[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char pkent[sizeof(crypt) + sizeof(public) + 1];
	char *master;

	getnetname(who);
	if (!getsecretkey(who, secret, oldpass)) {
		/*
		 * Quiet: net is not running secure RPC
		 */
		return;
	}
	if (secret[0] == 0) {
		/*
		 * Quiet: user has no secret key
		 */
		return;
	}
	if (!getpublickey(who, public)) {
		(void)fprintf(stderr, 
			      "Warning: can't find public key for %s.\n", who);
		return;
	}
	bcopy(secret, crypt, HEXKEYBYTES); 
	bcopy(secret, crypt + HEXKEYBYTES, KEYCHECKSUMSIZE); 
	crypt[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0; 
	xencrypt(crypt, newpass); 
	(void)sprintf(pkent, "%s:%s", public, crypt);
	if (yp_update(domain, PKMAP, YPOP_STORE,
	              who, strlen(who), pkent, strlen(pkent)) != 0) {

		(void)fprintf(stderr, 
			      "Warning: couldn't reencrypt secret key for %s\n",
			      who);
		return;
	}
	if (yp_master(domain, PKMAP, &master) != 0) {
		master = "yp master";	/* should never happen */
	}
	(void)printf("secret key reencrypted for %s on %s\n", who, master);
}
#endif SECURE_RPC

struct	passwd *pwd;
struct	passwd *getpwent();
int	endpwent();
char	*strcpy();
char	*crypt();
char	*getpass();
char	*getlogin();
char	*pw;
char	pwbuf[10];
char	pwbuf1[10];
char	hostname[256];
extern	int errno;

struct yppasswd *
getyppw(argc, argv)
	char *argv[];
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c, pwlen;
	char *uname;
	static struct yppasswd yppasswd;

	insist = 0;
	uname = NULL;
	if (argc > 1)
		uname = argv[1];
	if (uname == NULL) {
		if ((uname = getlogin()) == NULL) {
			(void)fprintf(stderr, "you don't have a login name\n");
			exit(1);
		}
		(void)gethostname(hostname, sizeof(hostname));
		(void)printf("Changing yp password for %s\n", uname);
	}

	while (((pwd = getpwent()) != NULL) && strcmp(pwd->pw_name, uname))
		;
	u = getuid();
	if (pwd == NULL) {
		(void)printf("Not in passwd file.\n");
		exit(1);
	}
	if (u != 0 && u != pwd->pw_uid) {
		(void)printf("Permission denied.\n");
		exit(1);
	}
	endpwent();
	(void)strcpy(pwbuf1, getpass("Old yp password:"));
	oldpass = pwbuf1;
tryagain:
	(void)strcpy(pwbuf, getpass("New password:"));
	newpass = pwbuf;
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		(void)printf("Password unchanged.\n");
		exit(1);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		(void)printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		(void)printf("Mismatch - password unchanged.\n");
		exit(1);
	}
	(void)time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	pw = crypt(pwbuf, saltc);
	yppasswd.oldpass = pwbuf1;
	pwd->pw_passwd = pw;
	yppasswd.newpw = *pwd;
	return (&yppasswd);
}
