/*
 * NetInfo server main
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * TODO: shutdown at time of signal if no write transaction is in progress
 */
#include <stdio.h>
#undef SUN_RPC
#define SUN_RPC 1
#include <rpc/rpc.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include "ni_server.h"
#include <sys/time.h>
#include <sys/socket.h>
#include <syslog.h>
#include "system.h"
#include "ni_globals.h"
#include "checksum.h"
#include "getstuff.h"
#include "mm.h"
#include "clib.h"
#include "notify.h"
#include "ni_dir.h"
#include "alert.h"
#include "socket_lock.h"

#define NIBIND_TIMEOUT 60
#define NIBIND_RETRIES 9


#define FD_SLOPSIZE 15 /* # of fds for things other than connections */

extern void ni_prog_2();

static void sigterm(void);
static ni_status start_service(char *);
static ni_status ni_register(ni_name, unsigned, unsigned);
static void usage(char *);
static void closeall(void);
static void detach(void);

extern void waitforparent(void);

void
main(
     int argc, 
     char **argv
     )
{
	ni_name tag;
	ni_status status;
	ni_name myname = argv[0];
	int create = 0;
	ni_name dbsource_name = NULL;
	ni_name dbsource_addr = NULL;
	ni_name dbsource_tag = NULL;
	struct rlimit rlim;

	argc--;
	argv++;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-m") == 0) {
			create++;
		} else if (strcmp(*argv, "-c") == 0) {
			if (argc < 4) {
				usage(myname);
			}
			create++;
			dbsource_name = argv[1];
			dbsource_addr = argv[2];
			dbsource_tag = argv[3];
			argc -= 3;
			argv += 3;
		} else {
			usage(myname);
		}
		argc--;
		argv++;
	}
	if (argc != 1) {
		usage(myname);
	}
	
#ifdef MALLOC_DEBUG
	{
		extern void catch_malloc_problems(int);

		(void)malloc_error(catch_malloc_problems);
		malloc_debug((1 << 2) | (1 << 3));
	}
#endif
	malloc_good_size(4);
	malloc_good_size(8);
	malloc_good_size(12);

	closeall();

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	openlog("netinfod", LOG_NDELAY | LOG_PID, LOG_DAEMON);

        tag = argv[0];
	if (create) {
		if (dbsource_addr == NULL) {
			status = dir_mastercreate(tag);
		} else {
			status = dir_clonecreate(tag,
						 dbsource_name,
						 dbsource_addr,
						 dbsource_tag);
		}
		if (status != NI_OK) {
			exit(status);
		}
	}
	
	signal(SIGTERM, (void *)sigterm);
	signal(SIGPIPE, SIG_IGN);

	status = start_service(tag);
	if (status != NI_OK) {
		exit(status);
	}
	if (i_am_clone) {
		dir_clonecheck();
	} else {
		(void) notify_start();
	}
	detach();
	svc_run(getdtablesize() - FD_SLOPSIZE);
	ni_shutdown(db_ni, db_checksum);
	sys_logmsg("netinfo server exiting");
	exit(0);
}


static ni_status
register_it(
	    ni_name tag
	    )
{
	SVCXPRT *transp;
	ni_status status;
	unsigned udp_port;
	unsigned tcp_port;

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		return (NI_SYSTEMERR);
	}
	if (!svc_register(transp, NI_PROG, NI_VERS, ni_prog_2, 0)) {
		return (NI_SYSTEMERR);
	}
	udp_port = transp->xp_port;
	udp_sock = transp->xp_sock;

	transp = svctcp_create(RPC_ANYSOCK, NI_SENDSIZE, NI_RECVSIZE);
	if (transp == NULL) {
		return (NI_SYSTEMERR);
	}
	if (!svc_register(transp, NI_PROG, NI_VERS, ni_prog_2, 0)) {
		return (NI_SYSTEMERR);
	}
	tcp_port = transp->xp_port;
	tcp_sock = transp->xp_sock;

	if (ni_name_match(tag, "local")) {	/* XXX */
		waitforparent();
	}
	status = ni_register(tag, udp_port, tcp_port);
	if (status != NI_OK) {
		return (status);
	}
	return (NI_OK);
}



static ni_status
start_service(
	      ni_name tag
	      )
{
	ni_name master;
	ni_status status;
	ni_name dbname;
	unsigned long addr;

	dir_cleanup(tag);
	dir_getnames(tag, &dbname, NULL, NULL);
	status = ni_init(dbname, &db_ni);
	ni_name_free(&dbname);
	if (status != NI_OK) {
		return (status);
	}
	checksum_compute(&db_checksum, db_ni);
	if (getmaster(db_ni, &master, NULL)) {
		addr = getaddress(db_ni, master);
		if (addr != 0) {
			if (!sys_ismyaddress(addr)) {
				i_am_clone++;		    
			}
		}
	}
	ni_name_free(&master);
	status = register_it(tag);
	return (status);
}


static void
sigterm(
	void
	)
{
	shutdown++;
}

static ni_status
ni_register(
	    ni_name tag,
	    unsigned udp_port,
	    unsigned tcp_port
	    )
{
	nibind_registration reg;
	ni_status status;
	CLIENT *cl;
	int sock;
	struct sockaddr_in sin;
	struct timeval tv;

	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sin.sin_port = 0;
	sin.sin_family = AF_INET;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	sock = socket_open(&sin, NIBIND_PROG, NIBIND_VERS);
	if (sock < 0) {
		return (NI_SYSTEMERR);
	}
	tv.tv_sec = NIBIND_TIMEOUT / (NIBIND_RETRIES + 1);
	tv.tv_usec = 0;
	cl = clntudp_create(&sin, NIBIND_PROG, NIBIND_VERS, tv, &sock);
	if (cl == NULL) {
		(void)socket_close(sock);
		return (NI_SYSTEMERR);
	}
	reg.tag = tag;
	reg.addrs.udp_port = udp_port;
	reg.addrs.tcp_port = tcp_port;
	tv.tv_sec = NIBIND_TIMEOUT;
	if (clnt_call(cl, NIBIND_REGISTER, xdr_nibind_registration,
		  &reg, xdr_ni_status, &status, tv) != RPC_SUCCESS) {
		clnt_destroy(cl);
		(void)socket_close(sock);
		return (NI_SYSTEMERR);
	}
	clnt_destroy(cl);
	(void)socket_close(sock);
	return (status);
}

static void 
usage(
      char *myname
      )
{
	fprintf(stderr, "usage: %s [-d] [-m ] [-s name addr tag] tag\n");
	exit(1);
}


static void
closeall(void)
{
	int i;

	for (i = getdtablesize() - 1; i >= 0; i--) {
		close(i);
	}
	/*
	 * We keep 0, 1 & 2 open to avoid using them. If we didn't, a
	 * library routine might do a printf to our descriptor and screw
	 * us up.
	 */
	open("/dev/null", O_RDWR, 0);
	dup(0);
	dup(0);
}


static void
detach(void)
{
	int ttyfd;

	ttyfd = open("/dev/tty", O_RDWR, 0);
	if (ttyfd > 0) {
		ioctl(ttyfd, TIOCNOTTY, (char *)0);
		close(ttyfd);
	}
}


#ifdef MALLOC_DEBUG
void 
catch_malloc_problems(
		      int problem
		      )
{
	abort();
}
#endif
