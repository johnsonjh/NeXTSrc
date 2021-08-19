/*
 * System routines
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#undef SUN_RPC		/* &^%$#@! */
#define SUN_RPC 1 	/* &^%$#@! */
#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <syslog.h>
#include <stdarg.h>
#include "clib.h"

char *
sys_hostname(
	     void
	     )
{

	static char myhostname[MAXHOSTNAMELEN + 1];

	if (myhostname[0] == 0) {
		(void)gethostname(myhostname, sizeof(myhostname));
	}
	return (myhostname);
}

void
sys_errmsg(
	   char *message,
	   ...
	   )
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_ERR, message, ap);
	va_end(ap);
}

void
sys_logmsg(
	   char *message,
	   ...
	   )
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_ERR, message, ap);
	va_end(ap);
}

void
sys_panic(
	  char *message,
	  ...
	  )
{
	va_list ap;

	va_start(ap, message);
	vsyslog(LOG_ERR, message, ap);
	va_end(ap);
	syslog(LOG_ALERT, "netinfod aborting");
	abort();
}

int
sys_spawn(const char *fname, ...)
{
	va_list ap;
	char *args[10]; /* XXX */
	int i;
	int pid;
	
	va_start(ap, (char *)fname);
	args[0] = (char *)fname;
	for (i = 1; args[i] = va_arg(ap, char *); i++) {
	}
	va_end(ap);

	switch (pid = fork()) {
	case -1:
		return (-1);
	case 0:
		execv(args[0], args);
		_exit(-1);
	default:
		return (pid);
	}
}

unsigned long
sys_address(
	    void
	    )
{
	struct ifconf ifc;
        struct ifreq ifreq;
	char buf[1024]; /* XXX */
	int i;
	int sock;
	struct sockaddr_in *sin;

	socket_lock();
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	socket_unlock();
	if (sock < 0) {
		return (htonl(INADDR_LOOPBACK));
	}
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
		socket_close(sock);
		return (htonl(INADDR_LOOPBACK));
        }
        for (i = 0; i <  ifc.ifc_len/sizeof (struct ifreq); i++) {
		ifreq = ifc.ifc_req[i];
		if (ioctl(sock, SIOCGIFFLAGS, &ifreq) < 0) {
			continue;
		}
		sin = (struct sockaddr_in *)&ifreq.ifr_addr;
		if (!(ifreq.ifr_flags & IFF_LOOPBACK) &&
		    (sin->sin_addr.s_addr != 0)) {
			socket_close(sock);
			return (sin->sin_addr.s_addr);
		}
	}
	socket_close(sock);
	return (htonl(INADDR_LOOPBACK));
}

int
sys_ismyaddress(
		unsigned long addr
		)
{
	struct ifconf ifc;
        struct ifreq ifreq;
	char buf[1024]; /* XXX */
	int i;
	int sock;
	struct sockaddr_in *sin;

	socket_lock();
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	socket_unlock();
	if (sock < 0) {
		return (0);
	}
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
		socket_close(sock);
		return (0);
        }
        for (i = 0; i <  ifc.ifc_len/sizeof (struct ifreq); i++) {
		ifreq = ifc.ifc_req[i];
		if (ioctl(sock, SIOCGIFFLAGS, &ifreq) < 0) {
			continue;
		}
		sin = (struct sockaddr_in *)&ifreq.ifr_addr;
		if (sin->sin_addr.s_addr != 0 &&
		    sin->sin_addr.s_addr == addr) {
			socket_close(sock);
			return (1);
		}
	}
	socket_close(sock);
	return (0);
}

int
sys_standalone(
	       void
	       )
{
	return (sys_address() == htonl(INADDR_LOOPBACK));
}


long
sys_time(void)
{
	struct timeval tv;

	(void)gettimeofday(&tv, NULL);
	return (tv.tv_sec);
}
