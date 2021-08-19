#ifndef lint
static char sccsid[] = 	"@(#)rpc.rstatd.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* 
 * rstat demon:  called from inet
 *
 */

#include <signal.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <nlist.h>
#include <sys/dk.h>
#include <sys/errno.h>
#include <sys/vmmeter.h>
#include <net/if.h>
#include <sys/time.h>
#include <rpcsvc/rstat.h>
#include <syslog.h>
#if	NeXT
#define LSCALE 1000
#endif	NeXT

struct nlist nl[] = {
#define	X_CPTIME	0
	{{ "_cp_time" }},
#define	X_SUM		1
	{{ "_sum" }},
#define	X_IFNET		2
	{{ "_ifnet" }},
#define	X_DKXFER	3
	{{ "_dk_xfer" }},
#define	X_BOOTTIME	4
	{{ "_boottime" }},
#define	X_AVENRUN	5
	{{ "_avenrun" }},
#define X_HZ		6
	{{ "_hz" }},

#if	NeXT
#define X_MACH_FACTOR	7
	{{ "_mach_factor"}},
#endif	NeXT

	{{""}},
};
int kmem;
int firstifnet, numintfs;	/* chain of ethernet interfaces */
int stats_service();

int sincelastreq = 0;		/* number of alarms since last request */
#define CLOSEDOWN 20		/* how long to wait before do_exiting */

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats;

int updatestat();
extern int errno;


#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

static setup();
static int stats_service();
static havedisk();

#if	NeXT
#undef DK_NDRIVE
#define DK_NDRIVE 4
#endif	NeXT

complain(s)
	char *s;
{
#ifdef DEBUG
	/* use stderr */
	fprintf(stderr, "rstatd: %s.\n", s);
	fflush(stderr);
#else
	static int initted;

	if (!initted) {
		openlog("rstatd", LOG_PID, LOG_DAEMON);
		initted = 1;
	}
	syslog(LOG_ERR, s);
#endif
}

main(argc, argv)
	char **argv;
{
	SVCXPRT *transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int readfds, port, readfdstmp;


#ifdef DEBUG
	{
	int s;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("inet: socket");
		return -1;
	}
	if (bind(s, &addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	if (getsockname(s, &addr, &len) != 0) {
		perror("inet: getsockname");
		(void)close(s);
		return -1;
	}
	pmap_unset(RSTATPROG, RSTATVERS_ORIG);
	pmap_set(RSTATPROG, RSTATVERS_ORIG, IPPROTO_UDP,ntohs(addr.sin_port));
	pmap_unset(RSTATPROG, RSTATVERS_SWTCH);
	pmap_set(RSTATPROG, RSTATVERS_SWTCH,IPPROTO_UDP,ntohs(addr.sin_port));
	pmap_unset(RSTATPROG, RSTATVERS_TIME);
	pmap_set(RSTATPROG, RSTATVERS_TIME,IPPROTO_UDP,ntohs(addr.sin_port));
	if (dup2(s, 0) < 0) {
		perror("dup2");
		do_exit(1);
	}
	}
#endif	
	if (getsockname(0, (struct sockaddr *)&addr, &len) != 0) {
		complain("getsockname: error");
		do_exit(1);
	}
	if ((transp = svcudp_bufcreate(0, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))
	    == NULL) {
		complain("svc_rpc_udp_create: error");
		do_exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_ORIG,stats_service, 0)) {
		complain("svc_rpc_register: error");
		do_exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_SWTCH,stats_service,0)) {
		complain("svc_rpc_register: error");
		do_exit(1);
	}
	if (!svc_register(transp,RSTATPROG,RSTATVERS_TIME,stats_service,0)) {
		complain("svc_rpc_register: error");
		do_exit(1);
	}
	setup();
	updatestat();
#if	NeXT
	alarm(1);
#else
	alarm(5);
#endif	NeXT
	signal(SIGALRM, updatestat);
	svc_run();
	complain("svc_run should never return");
}

static int
stats_service(reqp, transp)
	 struct svc_req  *reqp;
	 SVCXPRT  *transp;
{
	int have;
	
#ifdef DEBUG
	complain("entering stats_service");
#endif
	switch (reqp->rq_proc) {
		case RSTATPROC_STATS:
			sincelastreq = 0;
			if (reqp->rq_vers == RSTATVERS_ORIG) {
				if (svc_sendreply(transp, xdr_stats,
				    &stats.s1, TRUE) == FALSE) {
					complain("err: svc_rpc_send_results");
					do_exit(1);
				}
				return;
			}
			if (reqp->rq_vers == RSTATVERS_SWTCH) {
				if (svc_sendreply(transp, xdr_statsswtch,
				    &stats.s2, TRUE) == FALSE) {
					complain("err: svc_rpc_send_results");
					do_exit(1);
				    }
				return;
			}
			if (reqp->rq_vers == RSTATVERS_TIME) {
				if (svc_sendreply(transp, xdr_statstime,
				    &stats.s3, TRUE) == FALSE) {
					complain("err: svc_rpc_send_results");
					do_exit(1);
				    }
				return;
			}
		case RSTATPROC_HAVEDISK:
			have = havedisk();
			if (svc_sendreply(transp,xdr_long, &have, TRUE) == 0){
			    complain("err: svc_sendreply");
			    do_exit(1);
			}
			return;
		case 0:
			if (svc_sendreply(transp, xdr_void, 0, TRUE)
			    == FALSE) {
				complain("err: svc_rpc_send_results");
				do_exit(1);
			    }
			return;
		default: 
			svcerr_noproc(transp);
			return;
		}
}

updatestat()
{
	int off, i, hz;
	struct vmmeter sum;
	struct ifnet ifnet;
#if	NeXT
	long avrun[3];
#else
	double avrun[3];
#endif	NeXT
	struct timeval tm, btm;
	struct timezone tz;
	
#ifdef DEBUG
	complain("entering updatestat");
#endif
	if (sincelastreq >= CLOSEDOWN) {
#ifdef DEBUG
	complain("about to closedown");
#endif
		do_exit(0);
	}
	sincelastreq++;
	if (lseek(kmem, (long)nl[X_HZ].n_value, 0) == -1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
	if (read(kmem, &hz, sizeof hz) != sizeof hz) {
		complain("can't read hz from kmem");
		do_exit(1);
	}
	if (lseek(kmem, (long)nl[X_CPTIME].n_value, 0) == -1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
 	if (read(kmem, stats.s1.cp_time, sizeof (stats.s1.cp_time))
	    != sizeof (stats.s1.cp_time)) {
		complain("can't read cp_time from kmem");
		do_exit(1);
	}
	if (lseek(kmem, (long)nl[X_AVENRUN].n_value, 0) ==-1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
#if	NeXT
 	if (read(kmem, avrun, sizeof (avrun)) != sizeof (avrun)) {
		complain("can't read avenrun from kmem");
		do_exit(1);
	}
	stats.s2.avenrun[0] = avrun[0] * FSCALE / LSCALE;
	stats.s2.avenrun[1] = avrun[1] * FSCALE / LSCALE;
	stats.s2.avenrun[2] = avrun[2] * FSCALE / LSCALE;
#endif	NeXT
#ifdef vax /* ifdef logic humor */
 	if (read(kmem, avrun, sizeof (avrun)) != sizeof (avrun)) {
		complain("can't read avenrun from kmem");
		do_exit(1);
	}
	stats.s2.avenrun[0] = avrun[0] * FSCALE;
	stats.s2.avenrun[1] = avrun[1] * FSCALE;
	stats.s2.avenrun[2] = avrun[2] * FSCALE;
#endif vax
#ifdef sun
 	if (read(kmem, stats.s2.avenrun, sizeof (stats.s2.avenrun))
	    != sizeof (stats.s2.avenrun)) {
		complain("can't read avenrun from kmem");
		do_exit(1);
	}
#endif
	if (lseek(kmem, (long)nl[X_BOOTTIME].n_value, 0) == -1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
 	if (read(kmem, &btm, sizeof (stats.s2.boottime))
	    != sizeof (stats.s2.boottime)) {
		complain("can't read boottime from kmem");
		do_exit(1);
	}
	stats.s2.boottime = btm;


#ifdef DEBUG
	{
	char logline[64];

	sprintf(logline, "%d %d %d %d", stats.s1.cp_time[0],
            stats.s1.cp_time[1], stats.s1.cp_time[2], stats.s1.cp_time[3]);
	complain(logline);
	}
#endif

	if (lseek(kmem, (long)nl[X_SUM].n_value, 0) ==-1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
 	if (read(kmem, &sum, sizeof sum) != sizeof sum) {
		complain("can't read sum from kmem");
		do_exit(1);
	}
	stats.s1.v_pgpgin = sum.v_pgpgin;
	stats.s1.v_pgpgout = sum.v_pgpgout;
	stats.s1.v_pswpin = sum.v_pswpin;
	stats.s1.v_pswpout = sum.v_pswpout;
	stats.s1.v_intr = sum.v_intr;
	gettimeofday(&tm, NULL);
	stats.s1.v_intr -= hz*(tm.tv_sec - btm.tv_sec) +
	    hz*(tm.tv_usec - btm.tv_usec)/1000000;
	stats.s2.v_swtch = sum.v_swtch;

	if (lseek(kmem, (long)nl[X_DKXFER].n_value, 0) == -1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
 	if (read(kmem, stats.s1.dk_xfer, sizeof (stats.s1.dk_xfer))
	    != sizeof (stats.s1.dk_xfer)) {
		complain("can't read dk_xfer from kmem");
		do_exit(1);
	}

	stats.s1.if_ipackets = 0;
	stats.s1.if_opackets = 0;
	stats.s1.if_ierrors = 0;
	stats.s1.if_oerrors = 0;
	stats.s1.if_collisions = 0;
	for (off = firstifnet, i = 0; off && i < numintfs; i++) {
		if (lseek(kmem, off, 0) == -1) {
			complain("can't seek in kmem");
			do_exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			complain("can't read ifnet from kmem");
			do_exit(1);
		}
		stats.s1.if_ipackets += ifnet.if_ipackets;
		stats.s1.if_opackets += ifnet.if_opackets;
		stats.s1.if_ierrors += ifnet.if_ierrors;
		stats.s1.if_oerrors += ifnet.if_oerrors;
		stats.s1.if_collisions += ifnet.if_collisions;
		off = (int) ifnet.if_next;
	}
	gettimeofday(&stats.s3.curtime, NULL);
/*
 * We have better things to do with our CPU than run Sun's programs
 */
#if	NeXT
	alarm(15);
#else
	alarm(1);
#endif	NeXT
}

static 
setup()
{
	struct ifnet ifnet;
	int off, *ip;
	
#if	NeXT
	nlist("/mach", nl);
#else
	nlist("/vmunix", nl);
#endif	NeXT

	if (nl[0].n_value == 0) {
		complain("Variables missing from namelist");
		do_exit(1);
	}
	if ((kmem = open("/dev/kmem", 0)) < 0) {
		complain("can't open kmem");
		do_exit(1);
	}

	off = nl[X_IFNET].n_value;
	if (lseek(kmem, off, 0) == -1) {
		complain("can't seek in kmem");
		do_exit(1);
	}
	if (read(kmem, &firstifnet, sizeof(int)) != sizeof (int)) {
		complain("can't read firstifnet from kmem");
		do_exit(1);
	}
	numintfs = 0;
	for (off = firstifnet; off;) {
		if (lseek(kmem, off, 0) == -1) {
			complain("can't seek in kmem");
			do_exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			complain("can't read ifnet from kmem");
			do_exit(1);
		}
		numintfs++;
		off = (int) ifnet.if_next;
	}
}

/* 
 * returns true if have a disk
 */
static
havedisk()
{
	int i, cnt;
	long  xfer[DK_NDRIVE];

	if (nl[X_DKXFER].n_value == 0) {
		syslog(LOG_ERR, "Variables missing from namelist\n");
		exit (1);
	}
	if (lseek(kmem, (long)nl[X_DKXFER].n_value, 0) == -1) {
		syslog(LOG_ERR, "can't seek in kmem\n");
		exit(1);
	}
	if (read(kmem, xfer, sizeof xfer)!= sizeof xfer) {
		syslog(LOG_ERR, "can't read kmem\n");
		exit(1);
	}
	cnt = 0;
	for (i=0; i < DK_NDRIVE; i++)
		cnt += xfer[i];
	return (cnt != 0);
}

static
do_exit(stat)
	int stat;
{

#ifndef DEBUG
	if (stat != 0) {
		/*
		 * something went wrong.  Sleep awhile and drain our queue
		 * before exiting.  This keeps inetd from immediately retrying.
		 */
		int cc, fromlen;
		char buf[RPCSMALLMSGSIZE];
		struct sockaddr from;

		sleep(CLOSEDOWN);
		for (;;) {
			fromlen = sizeof (from);
			cc = recvfrom(0, buf, sizeof (buf), 0, &from, &fromlen);
			if (cc <= 0)
				break;
		}
	}
#endif
	exit(stat);
}

