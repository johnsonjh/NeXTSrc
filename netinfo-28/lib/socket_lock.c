/*
 * pmap_getport.c
 * Client interface to pmap rpc service.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>
#include "clib.h"
#include "socket_lock.h"

extern int bindresvport(int, struct sockaddr_in *);

static struct timeval timeout = { 4, 0 };
static struct timeval tottimeout = { 20, 0 };

/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 */
static u_short
pmap_getport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_int protocol;
{
	u_short port = 0;
	int sock = -1;
	register CLIENT *client;
	struct pmap parms;

	socket_lock();
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	socket_unlock();
	if (sock < 0) {
		return (0);
	}
	address->sin_port = htons(PMAPPORT);
	client = clntudp_bufcreate(address, PMAPPROG,
	    PMAPVERS, timeout, &sock, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;  /* not needed or used */
		if (CLNT_CALL(client, PMAPPROC_GETPORT, xdr_pmap, &parms,
		    xdr_u_short, &port, tottimeout) != RPC_SUCCESS){
			rpc_createerr.cf_stat = RPC_PMAPFAILURE;
			clnt_geterr(client, &rpc_createerr.cf_error);
			port = 0;
		} else if (port == 0) {
			rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
		}
	}
	socket_lock();
	if (client != NULL) {
		clnt_destroy(client);
	}
	(void)close(sock);
	socket_unlock();
	address->sin_port = 0;
	return (port);
}

int
socket_close(int sock)
{
	int ret;

	socket_lock();
	ret = close(sock);
	socket_unlock();
	return (ret);
}

int
socket_connect(
	       struct sockaddr_in *raddr,
	       int prog, 
	       int vers
	       )
{
	int sock;
	
	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port = pmap_getport(raddr, prog, vers, 
					 IPPROTO_TCP)) == 0) {
			return (-1);
		}
		raddr->sin_port = htons(port);
	}

	socket_lock();
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	socket_unlock();
	if (sock < 0) {
		return (-1);
	}
	(void)bindresvport(sock, (struct sockaddr_in *)0);
	if (connect(sock, (struct sockaddr *)raddr,
		    sizeof(*raddr)) < 0) {
		socket_close(sock);
		return (-1);
	}
	return (sock);
}

int
socket_open(
	    struct sockaddr_in *raddr,
	    int prog, 
	    int vers
	    )
{
	int sock;
	
	/*
	 * If no port number given ask the pmap for one
	 */
	if (raddr->sin_port == 0) {
		u_short port;
		if ((port = pmap_getport(raddr, prog, vers, 
					 IPPROTO_UDP)) == 0) {
			return (-1);
		}
		raddr->sin_port = htons(port);
	}

	socket_lock();
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	socket_unlock();
	if (sock < 0) {
		return (-1);
	}
	(void)bindresvport(sock, (struct sockaddr_in *)0);
	return (sock);
}

#include <cthreads.h>
static volatile mutex_t socket_mutex;

void
socket_lock(void)
{
	if (socket_mutex == NULL) {
		socket_mutex = mutex_alloc();
	}
	mutex_lock(socket_mutex);
}


void
socket_unlock(void)
{
	mutex_unlock(socket_mutex);
}

