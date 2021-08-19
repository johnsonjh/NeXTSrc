/*	@(#)bptst.c	1.2 88/02/05 D/NFS */
/*
 *  Client side bootparam service tester.  Intended for bootparamd
 *  testing only.  This program should not be included in any customer
 *  shipments.o
 *
 *	usage: bootparam [-b] [servername] args
 *		args:	w[hoami] [clientname]
 *			g[etfile] key [clientname]
 *			n[ullproc]
 *
 *  Copyright 1988 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>
#include <sys/socket.h>

char *Prog;
int bcast = 0;				/* send broadcast */
int unique = 0;				/* print unique replies only */
char *server = NULL;			/* server to send to */
char *target;				/* target client name */
char *filekey;				/* key for getfile op */
char op;				/* operation */

#define MYNAMELEN 64
char myname[MYNAMELEN];

bp_whoami_res  *bootparamproc_whoami_1();
bp_getfile_res *bootparamproc_getfile_1();

#define MAXREPLIES 255
struct in_addr replies[MAXREPLIES];

main(argc, argv)
char **argv;
{
	register char *p;
	int sock, n_res;
	struct sockaddr_in *sin, *getipaddr();
	struct bp_whoami_arg   w_arg;
	struct bp_whoami_res  *w_res, wres;
	struct bp_getfile_arg  g_arg;
	struct bp_getfile_res *g_res, gres;
	char tmpaddr[4];
	struct timeval tv;
	CLIENT *client;
	enum clnt_stat clntstat, clnt_broadcast();
	bool_t each_wresult(), each_gresult(), each_nresult();
	int prog = BOOTPARAMPROG;
	int vers = BOOTPARAMVERS;

	Prog = argv[0];
	for (argc--, argv++; p = argv[0]; argc--, argv++) {
		if (*p != '-')
			break;
		for (p++; *p; p++) {
			switch (*p) {
			    case 'b':
				/* broadcast */
				bcast = 1;
				break;
			    case 'u':
				/* unique replies only */
				unique = 1;
				break;
			    default:
				usage();
			}
		}
	}

	if (!bcast) {
		server = argv[0];
		argc--, argv++;
	}

	if (argc == 0)
		usage();
	if (gethostname(myname, MYNAMELEN) < 0) {
		perror("gethostname");
		exit(1);
	}
	switch (op = argv[0][0]) {
	    case 'n':
		switch (argc) {
		    case 1:
			break;
		    case 3:
			vers = atoi(argv[2]);
			/* fall through to... */
		    case 2:
			prog = atoi(argv[1]);
			printf("[NULLPROC to program %d, vers %d]\n",
				prog, vers);
			break;
		    default:
			usage();
		}
		break;
	    case 'w':
		switch (argc) {
		    case 1:
			/* use myname as the target client */
			target = myname;
			break;
		    case 2:
			/* target client name specified */
			target = argv[1];
			break;
		    default:
			usage();
		}
		sin = getipaddr(target);
		*(unsigned long *)tmpaddr = sin->sin_addr.s_addr;
		w_arg.client_address.bp_address.ip_addr.net   = tmpaddr[0];
		w_arg.client_address.bp_address.ip_addr.host  = tmpaddr[1];
		w_arg.client_address.bp_address.ip_addr.lh    = tmpaddr[2];
		w_arg.client_address.bp_address.ip_addr.impno = tmpaddr[3];
		w_arg.client_address.address_type = IP_ADDR_TYPE;
		break;
	    case 'g':
		switch (argc) {
		    case 2:
			filekey = argv[1];
			target = myname;
			break;
		    case 3:
			filekey = argv[1];
			target = argv[2];
			break;
		    default:
			usage();
		}
		g_arg.client_name = target;
		g_arg.file_id = filekey;
		break;
	    default:
		usage();
	}

	/* have all the args -- go do it... */

	if (bcast) {
		switch(op) {
		    case 'n':
			printf("sending broadcast nullproc request...\n");
			clntstat = clnt_broadcast(prog, vers, 0,
				xdr_void, NULL, xdr_void, NULL,
				each_nresult);
			if (clntstat) {
				fprintf(stderr, "broadcast: ");
				clnt_perrno(clntstat);
				fprintf(stderr, "\n");
			}
			break;
		    case 'w':
			printf("sending broadcast whoami(%s) request...\n",
				target);
			clntstat = clnt_broadcast(prog, vers,
				BOOTPARAMPROC_WHOAMI,
				xdr_bp_whoami_arg, &w_arg,
				xdr_bp_whoami_res, &wres, each_wresult);
			if (clntstat) {
				fprintf(stderr, "broadcast: ");
				clnt_perrno(clntstat);
				fprintf(stderr, "\n");
			}
			break;
		    case 'g':
			printf("sending broadcast getfile(%s, %s) request...\n",
				target, filekey);
			clntstat = clnt_broadcast(prog, vers,
				BOOTPARAMPROC_GETFILE,
				xdr_bp_getfile_arg, &g_arg,
				xdr_bp_getfile_res, &gres, each_gresult);
			if (clntstat) {
				fprintf(stderr, "broadcast: ");
				clnt_perrno(clntstat);
				fprintf(stderr, "\n");
			}
			break;
		}
	} else {
		sin = getipaddr(server);
		sock = RPC_ANYSOCK;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		if ((client = clntudp_create(sin, prog, vers, tv, &sock))
		     == NULL) {
			clnt_pcreateerror(server);
			exit(1);
		}
		switch (op) {
		    case 'n':
			printf("sending nullproc request to %s...\n", server);
			n_res = nullproc(client);
			if (n_res == NULL) {
				clnt_perror(client, "nullproc");
				exit(1);
			}
			prt_n_res(sin);
			break;
		    case 'w':
			printf("sending whoami(%s) request to %s...\n",
				target, server);
			w_res = bootparamproc_whoami_1(&w_arg, client);
			if (w_res == NULL) {
				clnt_perror(client, "whoami");
				exit(1);
			}
			prt_w_res(w_res, sin);
			break;
		    case 'g':
			printf("sending getfile(%s, %s) request to %s...\n",
				target, filekey, server);
			g_res = bootparamproc_getfile_1(&g_arg, client);
			if (g_res == NULL) {
				clnt_perror(client, "getfile");
				exit(1);
			}
			prt_g_res(g_res, sin);
			break;
		}
	}
	exit(0);
}

usage()
{
	fprintf(stderr, "\
usage: %s [-b] [servername] args\n\
\targs\tw[hoami] [clientname]\n\
\t\tg[etfile] key [clientname]\n\
\t\tn[ullproc]\n", Prog);
	exit(1);
}


struct sockaddr_in *
getipaddr(host)
char *host;
{
	static struct sockaddr_in sin;
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr, "%s: unknown host %s\n",
			Prog, host);
		exit(1);
	}
	if (hp->h_addrtype != AF_INET) {
		fprintf(stderr, "%s: address for host %s is not AF_INET\n",
			Prog, host);
		exit(1);
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
	return &sin;
}

prt_n_res(sin)
struct sockaddr_in *sin;
{
	printf("[");
	prt_hostname(sin, "]\tnullproc reply\n");
	fflush(stdout);
}

prt_w_res(res, sin)
bp_whoami_res *res;
struct sockaddr_in *sin;
{
	printf("[");
	prt_hostname(sin, "");
	printf("]\tname %s, domain %s, router ",
			res->client_name, res->domain_name);
	prt_bp_address(&res->router_address, "\n");
	fflush(stdout);
}

prt_g_res(res, sin)
bp_getfile_res *res;
struct sockaddr_in *sin;
{
	printf("[");
	prt_hostname(sin, "");
	printf("]\tpath \"%s:%s\" (server addr ",
		res->server_name, res->server_path);
	prt_bp_address(&res->server_address, ")\n");
	fflush(stdout);
}

prt_bp_address(addr, s)
bp_address *addr;
char *s;
{
	struct sockaddr_in sin;
	union {
		struct in_addr	inaddr;
		char	tmpaddr[4];
	} rin;

	if (addr->address_type == IP_ADDR_TYPE) {
		rin.tmpaddr[0] = addr->bp_address.ip_addr.net;
		rin.tmpaddr[1] = addr->bp_address.ip_addr.host;
		rin.tmpaddr[2] = addr->bp_address.ip_addr.lh;
		rin.tmpaddr[3] = addr->bp_address.ip_addr.impno;
		bzero((char*)&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr = rin.inaddr;
		prt_hostname(&sin, s);
	} else {
		printf("unknown type %d%s", addr->address_type, s);
	}
}

prt_hostname(sin, s)
struct sockaddr_in *sin;
char *s;
{
	struct hostent *hp;
	register int addr;
	static char buf[100];
	char *name;

	if (hp = gethostbyaddr((char *)&sin->sin_addr.s_addr,
	    sizeof(sin->sin_addr.s_addr), sin->sin_family))
		name = hp->h_name;
	else {
		addr = sin->sin_addr.s_addr;
		sprintf(buf, "[%d.%d.%d.%d]",
			(addr & 0xff000000) >> 24,
			(addr & 0x00ff0000) >> 16,
			(addr & 0x0000ff00) >>  8,
			(addr & 0x000000ff)		);
		name = buf;
	}
	printf("%s%s", name, s);
}

bool_t
each_wresult(res, from)
bp_whoami_res *res;
struct sockaddr_in *from;
{
	if (checkreply(from) == 0) {
		if (res)
			prt_w_res(res, from);
		else {
			printf("[");
			prt_hostname(from, "]\tNULL results pointer\n");
		}
	}
	return FALSE;
}

bool_t
each_gresult(res, from)
bp_getfile_res *res;
struct sockaddr_in *from;
{
	if (checkreply(from) == 0) {
		if (res)
			prt_g_res(res, from);
		else {
			printf("[");
			prt_hostname(from, "]\tNULL results pointer\n");
		}
	}
	return FALSE;
}

bool_t
each_nresult(res, from)
char *res;			/* NULL */
struct sockaddr_in *from;
{
	if (checkreply(from) == 0)
		prt_n_res(from);
	return FALSE;
}

/*
 *  Check the reply address:
 *	return values:	 1: already got a reply from this source
 *			 0: no reply yet from this source
 *	if unique is not set, then always return 0
 */
checkreply(from)
struct sockaddr_in *from;
{
	static int state = 0;
	register int i;
	register struct in_addr *ap;
	struct in_addr addr;

	if (!unique)
		return 0;

	if (!state) {
		for (i = 0; i < MAXREPLIES; i++)
			replies[i].s_addr = 0;
		state = 1;
	}
	addr.s_addr = from->sin_addr.s_addr;
	for (ap = replies;
	     ap < &replies[MAXREPLIES] && ap->s_addr != 0;
	     ap++) {
		if (addr.s_addr == ap->s_addr)
			return 1;
	}
	/* first reply from this source */
	if (ap < &replies[MAXREPLIES])
		ap->s_addr = addr.s_addr;
	else if (state == 1) {
		printf("[unique reply buffer is full]\n");
		state = 2;
	}
	return 0;
}
