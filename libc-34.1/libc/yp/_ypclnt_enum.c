/* @(#)_ypclnt_enum.c	1.4 87/09/11 3.2/4.3NFSSRC */
#ifndef lint
static	char sccsid[] = "@(#)_ypclnt_enum.c 1.1 86/09/24 Copyr 1985 Sun Micro";
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
 * This requests the yp server associated with a given domain to return the 
 * first key/value pair from the map data base.  The returned key should be 
 * used as an input to the next call to ypclnt_next.
 */

int
_ypclnt_dofirst (domain, map, pdomb, timeout, key, keylen, val, vallen)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **key;
	int  *keylen;
	char **val;
	int  *vallen;

{
	struct yprequest req;
	struct ypresponse resp;
	enum clnt_stat clnt_stat;
	unsigned int retval = 0;

	req.yp_reqtype = YPFIRST_REQTYPE;
	req.ypfirst_req_domain = domain;
	req.ypfirst_req_map = map;
	
	resp.ypfirst_resp_keyptr = NULL;
	resp.ypfirst_resp_keysize = 0;
	resp.ypfirst_resp_valptr = NULL;
	resp.ypfirst_resp_valsize = 0;



	/*
	 * Do the get first request.  If the rpc call failed, return with status
	 * from this point.
	 */
	
	if( (clnt_stat = (enum clnt_stat) clnt_call(pdomb->dom_client,
	    YPPROC_FIRST, xdr_yprequest, &req, xdr_ypresponse, &resp,
	    timeout) ) != RPC_SUCCESS) {
		return(YPERR_RPC);
	}

	/* See if the request succeeded */
	
	if (resp.ypfirst_resp_status != YP_TRUE) {
		retval = _map_ypprot_err(resp.ypfirst_resp_status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval) {

		if ((*key =
		    (char *) malloc(resp.ypfirst_resp_keysize + 2) ) != NULL) {

			if ((*val = (char *) malloc(
			    resp.ypfirst_resp_valsize + 2) ) == NULL) {
				free(*key);
				retval = YPERR_RESRC;
			}
		
		} else {
			retval = YPERR_RESRC;
		}
	}

	/* Copy the returned key and value byte strings into the new memory */

	if (!retval) {
		*keylen = resp.ypfirst_resp_keysize;
		bcopy(resp.ypfirst_resp_keyptr, *key, resp.ypfirst_resp_keysize);
		(*key)[resp.ypfirst_resp_keysize] = '\n';
		(*key)[resp.ypfirst_resp_keysize + 1] = '\0';
		
		*vallen = resp.ypfirst_resp_valsize;
		bcopy(resp.ypfirst_resp_valptr, *val, resp.ypfirst_resp_valsize);
		(*val)[resp.ypfirst_resp_valsize] = '\n';
		(*val)[resp.ypfirst_resp_valsize + 1] = '\0';
	}
	
	CLNT_FREERES(pdomb->dom_client, xdr_ypresponse, &resp); 
	return(retval);
}

/*
 * This requests the yp server associated with a given domain to return the 
 * "next" key/value pair from the map data base.  The input key should be one 
 * returned by ypclnt_first or a previuos call to ypclnt_next.  The returned 
 * key should be used as an input to the next call to ypclnt_next.
 */
int
_ypclnt_donext (domain, map, inkey, inkeylen, pdomb, timeout,
    outkey, outkeylen, val, vallen)
	char *domain;
	char *map;
	char *inkey;
	int  inkeylen;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **outkey;		/* return: key array associated with val */
	int  *outkeylen;	/* return: bytes in key */
	char **val;		/* return: value array associated with outkey */
	int  *vallen;		/* return: bytes in val */

{
	struct yprequest req;
	struct ypresponse resp;
	enum clnt_stat clnt_stat;
	unsigned int retval = 0;

	req.yp_reqtype = YPNEXT_REQTYPE;
	req.ypnext_req_domain = domain;
	req.ypnext_req_map = map;
	req.ypnext_req_keyptr = inkey;
	req.ypnext_req_keysize = inkeylen;
	
	resp.ypnext_resp_keyptr = NULL;
	resp.ypnext_resp_keysize = 0;
	resp.ypnext_resp_valptr = NULL;
	resp.ypnext_resp_valsize = 0;



	/*
	 * Do the get next request.  If the rpc call failed, return with status
	 * from this point.
	 */
	
	if( (clnt_stat = (enum clnt_stat) clnt_call(pdomb->dom_client,
	    YPPROC_NEXT, xdr_yprequest, &req, xdr_ypresponse, &resp,
	    timeout) ) != RPC_SUCCESS) {
		return(YPERR_RPC);
	}

	/* See if the request succeeded */
	
	if (resp.ypnext_resp_status != YP_TRUE) {
		retval = _map_ypprot_err(resp.ypnext_resp_status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval) {
		if ( (*outkey = (char *) malloc(
		    resp.ypnext_resp_keysize + 2) ) != NULL) {

			if ( (*val = (char *) malloc(
			    resp.ypnext_resp_valsize + 2) ) == NULL) {
				free(*outkey);
				retval = YPERR_RESRC;
			}
		
		} else {
			retval = YPERR_RESRC;
		}
	}

	/* Copy the returned key and value byte strings into the new memory */

	if (!retval) {
		*outkeylen = resp.ypnext_resp_keysize;
		bcopy(resp.ypnext_resp_keyptr, *outkey,
		    resp.ypnext_resp_keysize);
		(*outkey)[resp.ypnext_resp_keysize] = '\n';
		(*outkey)[resp.ypnext_resp_keysize + 1] = '\0';
		
		*vallen = resp.ypnext_resp_valsize;
		bcopy(resp.ypnext_resp_valptr, *val, resp.ypnext_resp_valsize);
		(*val)[resp.ypnext_resp_valsize] = '\n';
		(*val)[resp.ypnext_resp_valsize + 1] = '\0';
	}
	
	CLNT_FREERES(pdomb->dom_client, xdr_ypresponse, &resp);
	return(retval);

}

