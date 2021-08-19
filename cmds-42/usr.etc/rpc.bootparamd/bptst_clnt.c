/*
 *  @(#)bptst_clnt.c	1.2 88/02/05 D/NFS
 *  Copyright 1988 Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>

static struct timeval TIMEOUT = { 25, 0 };

bp_whoami_res *
bootparamproc_whoami_1(argp, clnt)
	bp_whoami_arg *argp;
	CLIENT *clnt;
{
	static bp_whoami_res res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, BOOTPARAMPROC_WHOAMI, xdr_bp_whoami_arg, argp, xdr_bp_whoami_res, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}


bp_getfile_res *
bootparamproc_getfile_1(argp, clnt)
	bp_getfile_arg *argp;
	CLIENT *clnt;
{
	static bp_getfile_res res;

	bzero((char *)&res, sizeof(res));
	if (clnt_call(clnt, BOOTPARAMPROC_GETFILE, xdr_bp_getfile_arg, argp, xdr_bp_getfile_res, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

nullproc(clnt)
CLIENT *clnt;
{
	if (clnt_call(clnt, 0, xdr_void, NULL, xdr_void, NULL, TIMEOUT)
	    != RPC_SUCCESS) {
		return (NULL);
	}
	return 1;
}
