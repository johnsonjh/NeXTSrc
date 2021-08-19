/* @(#)domain.c	1.2 87/09/08 3.2/4.3NFSSRC */
/*
**  Sendmail
**  Copyright (c) 1986  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1986 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/
# include <sys/param.h>
# include "sendmail.h"

#ifdef MXDOMAIN
SCCSID(@(#)domain.c 1.5 87/06/26 SMI); /* from UCB 5.3 7/21/86 */

#if	NeXT_MOD
#include <sys/errno.h>
#endif	NeXT_MOD
# include <arpa/nameser.h>
# include <resolv.h>
# include <netdb.h>

#ifdef NeXT_MOD
#define getshort res_getshort
#endif

typedef union {
	HEADER qb1;
	char qb2[PACKETSZ];
} querybuf;

static char hostbuf[BUFSIZ];

#if	NeXT_MOD
extern int h_errno;
extern int errno;
#else	NeXT_MOD
int h_errno;
#endif	NeXT_MOD

/*
 * Look up Mail Exchanger records, first in cache, then to name server.
 * return values for mx->mx_number:
 *	-3 hard host not found
 *	-2 hard error looking up MX record
 *	-1 server fail, try again
 *	 0
 *	 1 TOPS-20 FORMERR kludge, or no records but valid name
 *	   or we are the first record
 */
struct mxinfo *getmxrr(host, maxmx, localhost)
	char *host;
	int maxmx;
	char *localhost;
{

	HEADER *hp;
	char *eom, *bp, *cp;
	querybuf buf, answer;
	int n, n1, i, j, nmx, ancount, qdcount, buflen;
	int seenlocal;
	u_short prefer[BUFSIZ];
	u_short pref, localpref, type, class;
	STAB *st;
	register struct mxinfo *mx;
	
#ifdef DEBUG
	if (tTd(8, 2)) 
		_res.options |= RES_DEBUG;
#endif

	st = stab(host, ST_MX, ST_FIND);
	if (st != NULL) {
	    /*
	     * found in the cache, just return it.
	     */
	     mx = &st->s_value.sv_mxinfo;
	     AlreadyKnown = (mx->mx_number == -1);
	     return(mx);
	}
	AlreadyKnown = FALSE;
	st = stab(host, ST_MX, ST_ENTER);
	mx = &st->s_value.sv_mxinfo;

	n = res_mkquery(QUERY, host, C_IN, T_MX, (char *)NULL, 0, NULL,
		(char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("res_mkquery failed\n");
#endif
		h_errno = NO_RECOVERY;
		mx->mx_number = -2;
		return(mx);
	}
	n = res_send((char *)&buf, n, (char *)&answer, sizeof(answer));
	if (n < 0) {
#ifdef DEBUG
#if	NeXT_MOD
		if (tTd(8, 1) || _res.options & RES_DEBUG) {
			int terrno = errno;
			printf("res_send failed\n");
			errno = terrno;
		}
#else	NeXT_MOD
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("res_send failed\n");
#endif	NeXT_MOD
#endif
#if	NeXT_MOD
		if (errno == ECONNREFUSED || errno == ETIMEDOUT) {
			/*
			 * No domain name server there.  We'll assume
			 * that the resolver has not been configured
			 * and we will leave the hostname unchanged.
			 */
			h_errno = HOST_NOT_FOUND;
			mx->mx_number = -3;
		} else {
			h_errno = TRY_AGAIN;
			mx->mx_number = -1;
		}
#else	NeXT_MOD
		h_errno = TRY_AGAIN;
		mx->mx_number = -1;
#endif	NeXT_MOD
		return(mx);
	}
	eom = (char *)&answer + n;
	/*
	 * find first satisfactory answer
	 */
	hp = (HEADER *) &answer;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	if (hp->rcode != NOERROR || ancount == 0) {
#ifdef DEBUG
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode, ancount);
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				/* Check if it's an authoritive answer */
				if (hp->aa) {
					h_errno = HOST_NOT_FOUND;
					mx->mx_number = -3;
				} else {
					h_errno = TRY_AGAIN;
					mx->mx_number = -1;
				}
				return(mx);
			case SERVFAIL:
				h_errno = TRY_AGAIN;
				mx->mx_number = -1;
				return(mx);
# define OLDJEEVES
#ifdef OLDJEEVES
			/*
			 * Jeeves (TOPS-20 server) still does not
			 * support MX records.  For the time being,
			 * we must accept FORMERRs as the same as
			 * NOERROR.
			 */
			case FORMERR:
#endif OLDJEEVES
			case NOERROR:
				mx->mx_hosts[0] = newstr(host);
				mx->mx_number = 1;
				return(mx);
#ifndef OLDJEEVES
			case FORMERR:
#endif OLDJEEVES
			case NOTIMP:
			case REFUSED:
				h_errno = NO_RECOVERY;
				mx->mx_number = -2;
				return(mx);
		}
		mx->mx_number = -1;
		return(mx);
	}
	bp = hostbuf;
	nmx = 0;
	seenlocal = 0;
	buflen = sizeof(hostbuf);
	cp = (char *)&answer + sizeof(HEADER);
	if (qdcount) {
		cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	}
	while (--ancount >= 0 && cp < eom && nmx < maxmx) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = getshort(cp);
 		cp += sizeof(u_short);
		/*
		class = getshort(cp);
		*/
 		cp += sizeof(u_short) + sizeof(u_long);
		n = getshort(cp);
		cp += sizeof(u_short);
		if (type != T_MX)  {
#ifdef DEBUG
			if (tTd(8, 1) || _res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		pref = getshort(cp);
		cp += sizeof(u_short);
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		if (sameword(bp, localhost))
		{
			seenlocal = 1;
			localpref = pref;
			continue;
		}
		prefer[nmx] = pref;
		mx->mx_hosts[nmx++] = newstr(bp);
		n1 = strlen(bp)+1;
		bp += n1;
		buflen -= n1;
	}
	  /*
	   * Scan the response for useful additional records, too.
	   * Normally these will be address records for the forwarder,
	   * or the host itself (if there are no MX records).
	   */
	for (ancount = ntohs(hp->arcount);
	   --ancount >= 0 && cp < eom; cp += n) {
		struct in_addr sin, *addlist;

		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = getshort(cp);
 		cp += sizeof(u_short);
		class = getshort(cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = getshort(cp);
		cp += sizeof(u_short);
		if (type != T_A || class != C_IN)  {
#ifdef DEBUG
			if (tTd(8, 1) || _res.options & RES_DEBUG)
				printf("unexpected add. type %d, size %d\n",
					type, n);
#endif
			continue;
		}
		(void) bcopy(cp, &sin, n);
		st = stab(bp, ST_HOST, ST_FIND);
		if (st == NULL) {
		    /*
		     * not found in the cache, add it.
		     */
		    st = stab(bp, ST_HOST, ST_ENTER);
		    if (st == NULL)
			continue;
		    st->s_value.sv_host.h_addrlist[0].s_addr = INADDR_ANY;
		    st->s_value.sv_host.h_down = 0;
		}
		st->s_value.sv_host.h_valid = 1;
		st->s_value.sv_host.h_exists = 1;
		addlist = st->s_value.sv_host.h_addrlist;
		while (addlist->s_addr != INADDR_ANY) {
		    if (addlist->s_addr == sin.s_addr)
			break;
		    addlist++;
		}
		if (addlist->s_addr == INADDR_ANY) {
		    addlist++->s_addr = sin.s_addr;
		    addlist->s_addr = INADDR_ANY;
		}
	}
	if (nmx == 0) {
		mx->mx_hosts[0] = newstr(host);
		mx->mx_number = 1;
		return(mx);
	}
	/* sort the records */
	for (i = 0; i < nmx; i++) {
		for (j = i + 1; j < nmx; j++) {
			if (prefer[i] > prefer[j]) {
				int temp;
				char *temp1;

				temp = prefer[i];
				prefer[i] = prefer[j];
				prefer[j] = temp;
				temp1 = mx->mx_hosts[i];
				mx->mx_hosts[i] = mx->mx_hosts[j];
				mx->mx_hosts[j] = temp1;
			}
		}
		if (seenlocal && (prefer[i] >= localpref))
		{
			nmx = i;
			/*
			 * We are the first MX, might as well try delivering
			 * since nobody is supposed to have more info.
			 */
			if (nmx == 0)
			{
				mx->mx_hosts[0] = newstr(host);
				mx->mx_number = 1;
				return(mx);
			}
			break;
		}
	}
	mx->mx_number = nmx;
	return(mx);
}


getcanonname(host, hbsize)
	char *host;
	int hbsize;
{

	HEADER *hp;
	char *eom, *cp;
	querybuf buf, answer;
	int n, ancount, qdcount;
	u_short type;
	char nbuf[BUFSIZ];
	int first;

	n = res_mkquery(QUERY, host, C_IN, T_ANY, (char *)NULL, 0, NULL,
		(char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("res_mkquery failed\n");
#endif
		h_errno = NO_RECOVERY;
		return;
	}
	n = res_send((char *)&buf, n, (char *)&answer, sizeof(answer));
	if (n < 0) {
#ifdef DEBUG
#if	NeXT_MOD
		if (tTd(8, 1) || _res.options & RES_DEBUG) {
			int terrno = errno;
			printf("res_send failed\n");
			errno = terrno;
		}
#else	NeXT_MOD
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("res_send failed\n");
#endif	NeXT_MOD
#endif
#if	NeXT_MOD
		if (errno == ECONNREFUSED || errno == ETIMEDOUT) {
			/*
			 * No domain name server there.  We'll assume
			 * that the resolver has not been configured
			 * and we will leave the hostname unchanged.
			 */
			h_errno = HOST_NOT_FOUND;
		} else {
			h_errno = TRY_AGAIN;
		}
#else	NeXT_MOD
		h_errno = TRY_AGAIN;
#endif	NeXT_MOD
		return;
	}
	eom = (char *)&answer + n;
	/*
	 * find first satisfactory answer
	 */
	hp = (HEADER *) &answer;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	/*
	 * We don't care about errors here, only if we got an answer
	 */
	if (ancount == 0) {
#ifdef DEBUG
		if (tTd(8, 1) || _res.options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode, ancount);
#endif
		return;
	}
	cp = (char *)&answer + sizeof(HEADER);
	if (qdcount) {
		cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	}
	first = 1;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
		    sizeof(nbuf))) < 0)
			break;
		if (first) {
			(void)strncpy(host, nbuf, hbsize);
			host[hbsize - 1] = '\0';
			first = 0;
		}
		cp += n;
		type = getshort(cp);
 		cp += sizeof(u_short);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME)  {
			/*
			 * Assume that only one cname will be found.  More
			 * than one is undefined.
			 */
			if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
			    sizeof(nbuf))) < 0)
				break;
			(void)strncpy(host, nbuf, hbsize);
			host[hbsize - 1] = '\0';
			getcanonname(host, hbsize);
			break;
		}
		cp += n;
	}
	return;
}
#endif MXDOMAIN
