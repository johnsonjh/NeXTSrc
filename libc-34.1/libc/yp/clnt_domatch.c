/* @(#)clnt_domatch.c	1.4 87/09/11 3.2/4.3NFSSRC */
#ifndef lint
static	char sccsid[] = "@(#)clnt_domatch.c 1.1 86/09/24 Copyr 1985 Sun Micro";
#endif

#include <dbm.h>			/* Pull this in first */
#undef NULL				/* Remove dbm.h's definition of NULL */
extern void dbmclose();			/* Refer to dbm routine not in dbm.h */
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
extern int _map_ypprot_err();

/*
 * This requests the yp server associated with a
 * given domain to attempt to match the passed key datum in the named
 * map, and to return the associated value datum.
 *	char **val;			* Ptr to ptr to value array	
 *	int  *vallen;			* Ptr to number of bytes in val
 */
int
_ypclnt_domatch (domain, map, key, keylen, pdomb, timeout, val, vallen)
	char *domain;
	char *map;
	char *key;
	int  keylen;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **val;		/* return: value array */
	int  *vallen;		/* return: bytes in val */
{
	struct yprequest req;
	struct ypresponse resp;
	enum clnt_stat clnt_stat;
	unsigned int retval = 0;

	req.yp_reqtype = YPMATCH_REQTYPE;
	req.ypmatch_req_domain = domain;
	req.ypmatch_req_map = map;
	req.ypmatch_req_keyptr = key;
	req.ypmatch_req_keysize = keylen;
	
	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

	/*
	 * Do the match request.  If the rpc call failed, return with status
	 * from this point.
	 */
	
	if( (clnt_stat = (enum clnt_stat) clnt_call(pdomb->dom_client,
	    YPPROC_MATCH, xdr_yprequest, &req, xdr_ypresponse, &resp,
	    timeout) ) != RPC_SUCCESS) {
		return(YPERR_RPC);
	}

	/* See if the request succeeded */
	
	if (resp.ypmatch_resp_status != YP_TRUE) {
		retval = _map_ypprot_err(resp.ypmatch_resp_status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval && (
	    (*val = (char *) malloc(resp.ypmatch_resp_valsize + 2)) == NULL)) {
		retval = YPERR_RESRC;
	}

	/* Copy the returned value byte string into the new memory */

	if (!retval) {
		*vallen = resp.ypmatch_resp_valsize;
		bcopy(resp.ypmatch_resp_valptr, *val, resp.ypmatch_resp_valsize);
		(*val)[resp.ypmatch_resp_valsize] = '\n';
		(*val)[resp.ypmatch_resp_valsize + 1] = '\0';
	}

	CLNT_FREERES(pdomb->dom_client, xdr_ypresponse, &resp);
	return(retval);

}
