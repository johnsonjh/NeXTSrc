#ifndef lint
static char sccsid[] = 	"@(#)spray.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <rpcsvc/spray.h>

#define DEFBYTES 100000		/* default numbers of bytes to send */
#define MAXPACKETLEN 1514

char	*adrtostr();
char	*host;
int	adr;
int	lnth, cnt;
int	icmp;

main(argc, argv)
	char **argv;
{
	int	err, i, rcved;
	int	delay = 0;
	int	psec, bsec;
	int	buf[SPRAYMAX/4];
	struct	hostent *hp;
	struct	sprayarr arr;	
	struct	spraycumul cumul;
	
	if (argc < 2)
		usage();
	
	cnt = -1;
	lnth = SPRAYOVERHEAD;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			switch(argv[1][1]) {
				case 'd':
					delay = atoi(argv[2]);
					argc--;
					argv++;
					break;
				case 'i':
					icmp++;
					break;
				case 'c':
					cnt = atoi(argv[2]);
					argc--;
					argv++;
					break;
				case 'l':
					lnth = atoi(argv[2]);
					argc--;
					argv++;
					break;
				default:
					usage();
			}
		}
		else {
			if (host)
				usage();
			else
				host = argv[1];
		}
		argc--;
		argv++;
	}
	if (host == NULL)
		usage();
	if (isinetaddr(host)) {
		adr = inet_addr(host);
		host = adrtostr(adr);
	}
	else {
		if ((hp = gethostbyname(host)) == NULL) {
			fprintf(stderr, "%s is unknown host name\n", host);
			exit(1);
		}
		adr = *((int *)hp->h_addr);
	}
	if (icmp)
		doicmp(adr, delay);
	if (cnt == -1)
		cnt = DEFBYTES/lnth;
	if (lnth < SPRAYOVERHEAD)
		lnth = SPRAYOVERHEAD;
	else if (lnth >= SPRAYMAX)
		lnth = SPRAYMAX;
	if (lnth <= MAXPACKETLEN && lnth % 4 != 2)
		lnth = ((lnth+5)/4)*4 - 2;
	arr.lnth = lnth - SPRAYOVERHEAD;
	arr.data = buf;
	printf("sending %d packets of lnth %d to %s ...", cnt, lnth, host);
	fflush(stdout);
	
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_CLEAR,
	    xdr_void, NULL, xdr_void, NULL)) {
		fprintf(stderr, "SPRAYPROC_CLEAR ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
		return;
	}
	for (i = 0; i < cnt; i++) {
		callrpcnowait(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_SPRAY,
		    xdr_sprayarr, &arr, xdr_void, NULL);
		if (delay > 0)
			slp(delay);
	}
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_GET,
	    xdr_void, NULL, xdr_spraycumul, &cumul)) {
		fprintf(stderr, "SPRAYPROC_GET ");
		fprintf(stderr, "%s ", host);
		clnt_perrno(err);
		fprintf(stderr, "\n");
		return;
	}
	if (cumul.counter < cnt)
		printf("\n\t%d packets (%.3f%%) dropped by %s\n",
		    cnt - cumul.counter,
		    100.0*(cnt - cumul.counter)/cnt, host);
	else
		printf("\n\tno packets dropped by %s\n", host);
	psec = (1000000.0 * cumul.counter)
	    / (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);
	bsec = (lnth * 1000000.0 * cumul.counter)/
	    (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);
	printf("\t%d packets/sec, %d bytes/sec\n", psec, bsec);
}

/* 
 * like callrpc, but with addr instead of host name
 */
mycallrpc(addr, prognum, versnum, procnum, inproc, in, outproc, out)
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& adr == oldadr) {
		/* reuse old client */		
	}
	else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 10;
		bcopy(&adr, &server_addr.sin_addr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	return ((int) clnt_stat);
}

callrpcnowait(adr, prognum, versnum, procnum, inproc, in, outproc, out)
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& oldadr == adr) {
		/* reuse old client */		
	}
	else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
		bcopy(&adr, &server_addr.sin_addr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
	}
	tottimeout.tv_sec = 0;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 * since timeout is zero, normal return value is RPC_TIMEDOUT
	 */
	if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT)
		valid = 0;
	return ((int) clnt_stat);
}

char *
adrtostr(adr)
int adr;
{
	struct hostent *hp;
	char buf[100];		/* hope this is long enough */
	
	hp = gethostbyaddr(&adr, sizeof(adr), AF_INET);
	if (hp == NULL) {
	    	sprintf(buf, "0x%x", adr);
		return buf;
	}
	else
		return hp->h_name;
}

slp(usecs)
{
	struct timeval tv;
	
	tv.tv_sec = usecs / 1000000;
	tv.tv_usec = usecs % 1000000;
	select(32, 0, 0, 0, &tv);
}

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>

#define	MAXICMP (2082 - IPHEADER) /* experimentally determined to be max */
#define	IPHEADER	34	/* size of ether + ip header */
#define MINICMP		8	/* minimum icmp length */

struct	timeval tv1, tv2;
int	pid, rcvd;
int	die(), done();

doicmp(adr, delay)
{
	char buf[MAXICMP];
	struct icmp *icp = (struct icmp *)buf;
	int	i, s;
	int	fromlen, size;
	struct sockaddr_in to, from;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("ping: socket");
		exit(1);
	}
	if (lnth >= IPHEADER + MINICMP)
		lnth -= IPHEADER;
	else
		lnth = MINICMP;
	if (lnth > MAXICMP) {
		fprintf(stderr, "%d is max packet size\n",
		    MAXICMP + IPHEADER);
		exit(1);
	}
	if (cnt == -1)
		cnt = DEFBYTES/(lnth+IPHEADER);
	to.sin_family = AF_INET;
	to.sin_port = 0;
	to.sin_addr.s_addr = adr;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_id = 1;
	icp->icmp_seq = 1;
	icp->icmp_cksum = in_cksum(icp, lnth);

	printf("sending %d packets of lnth %d to %s ...", cnt, lnth+IPHEADER,
	    host);
	fflush(stdout);

	if ((pid = fork()) < 0) {
	    perror("ping: fork");
	    exit(1);
	}
	if (pid == 0) {		/* child */
		sleep(1);	/* wait a second to give parent time to recv */
		for (i = 0; i < cnt; i++) {
			if (sendto(s, icp, lnth, 0, (struct sockaddr *)&to, sizeof(to)) != lnth) {
				fprintf(stderr, "\n");
				perror("ping: sendto");
				kill (pid, SIGKILL);
				exit(1);
			}
			if (delay > 0)
				slp(delay);
		}
		sleep(1);	/* wait for last echo to get thru */
		exit(0);
	}

	if (pid != 0) {		/* parent */
		signal(SIGCHLD, done);
		signal(SIGINT, die);
		rcvd = 0;
		for (i = 0; ; i++) {
			fromlen = sizeof(from);
			if ((size = recvfrom(s, buf, sizeof(buf), 0,
			    (struct sockaddr *)&from, &fromlen)) < 0) {
				perror("ping: recvfrom");
				continue;
			}
			if (i == 0)
				gettimeofday(&tv1, 0);
			else if (i == cnt-1)
				gettimeofday(&tv2, 0);
			rcvd++;
		}
	}
}

in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register u_short *ptr;
	register int sum;
	u_short *lastptr;

	sum = 0;
	ptr = (u_short *)addr;
	lastptr = ptr + (len/2);
	for (; ptr < lastptr; ptr++) {
		sum += *ptr;
		if (sum & 0x10000) {
			sum &= 0xffff;
			sum++;
		}
	}
	return (~sum & 0xffff);
}

die()
{
	kill (pid, SIGKILL);
	exit(1);
}

done()
{
	int	psec, bsec;
	
	if (tv2.tv_usec == 0 && tv2.tv_usec == 0) {/* estimate */
		gettimeofday(&tv2, 0);
		tv2.tv_sec -= 1;	/* allow for sleep(1) */
	}
	if (rcvd != cnt)
		printf("\n\t%d packets (%.3f%%) dropped by %s\n",
		    cnt - rcvd, 100.0*(cnt - rcvd)/cnt, host);
	else
		printf("\n\tno packets dropped by %s\n", host);
	if (tv2.tv_usec < tv1.tv_usec) {
		tv2.tv_usec += 1000000;
		tv2.tv_sec -= 1;
	}
	tv2.tv_sec -= tv1.tv_sec;
	tv2.tv_usec -= tv1.tv_usec;
	psec = (1000000.0*cnt) / (1000000.0*tv2.tv_sec + tv2.tv_usec);
	bsec = ((lnth + IPHEADER) * 1000000.0 * cnt)/
	    (1000000.0 * tv2.tv_sec + tv2.tv_usec);
	printf("\t%d packets/sec, %d bytes/sec\n", psec, bsec);
	exit(0);
}

usage()
{
	fprintf(stderr,
	    "Usage: spray host [-i] [-c cnt] [-l lnth] [-d usecs]\n");
	exit(1);
}

/*
 * A better way to check for an inet address : scan the entire string for
 * nothing but . and digits. If a letter is found return FALSE. Yes, you can
 * get some degenerate cases by it, but who names a host with *all* numbers?
 */

int
isinetaddr(str)

char	*str;

{
	int	i;
	while (*str) 
		if (((*str >= '0') && (*str <= '9')) || (*str == '.')) str++;
		else return(FALSE);
	return(TRUE);
}
