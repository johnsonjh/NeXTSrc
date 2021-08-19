/*
 * nibindd glue 
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <netinfo/ni.h>
#include <sys/socket.h>
#include <stdio.h>
#include "clib.h"

/*
 * Initiate nibindd connection
 */
void *
nibind_new(
	   struct in_addr *addr
	   )
{
	struct sockaddr_in sin;
	int sock = RPC_ANYSOCK;

	sin.sin_port = 0;
	sin.sin_family = AF_INET;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	sin.sin_addr = *addr;
	return ((void *)clnttcp_create(&sin, NIBIND_PROG, NIBIND_VERS, 
				       &sock, 0, 0));
}

/*
 * List registered netinfods
 */
ni_status
nibind_listreg(
	       void *nb,
	       nibind_registration **regvec,
	       unsigned *reglen
	       )
{
	nibind_listreg_res *res;

	res = nibind_listreg_1(NULL, nb);
	if (res == NULL) {
 		return (NI_FAILED);
	}
	if (res->status == NI_OK) {
		*regvec = res->nibind_listreg_res_u.regs.regs_val;
		*reglen = res->nibind_listreg_res_u.regs.regs_len;
	}
	return (res->status);
}

/*
 * Create a master netinfod
 */
ni_status
nibind_createmaster(
		    void *nb,
		    ni_name tag
		    )
{
	ni_status *status;

	status = nibind_createmaster_1(&tag, nb);
	if (status == NULL) {
		return (NI_FAILED);
	}
	return (*status);
}
	
/*
 * Create a clone netinfod
 */
ni_status
nibind_createclone(
		   void *nb,
		   ni_name tag,
		   ni_name master_name,
		   struct in_addr *master_addr,
		   ni_name master_tag
		   )
{
	ni_status *status;
	nibind_clone_args args;

	args.tag = tag;
	args.master_name = master_name;
	args.master_addr = master_addr->s_addr;
	args.master_tag = master_tag;
	status = nibind_createclone_1(&args, nb);
	if (status == NULL) {
		return (NI_FAILED);
	}
	return (*status);
}

/*
 * Destroy a netinfod
 */
ni_status
nibind_destroydomain(
		     void *nb,
		     ni_name tag
		     )
{
	ni_status *status;

	status = nibind_destroydomain_1(&tag, nb);
	if (status == NULL) {
		return (NI_FAILED);
	}
	return (*status);
}
		   

/*
 * Free up connection to nibindd
 */
void
nibind_free(
	    void *nb
	    )
{
	clnt_destroy(((CLIENT *)nb));
}
