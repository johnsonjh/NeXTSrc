/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */


#ifndef lint
static char sccsid[] = "@(#)tables.c	5.5 (Berkeley) 2/14/86";
#endif not lint

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/ioctl.h>
#include <errno.h>

#ifndef DEBUG
#define	DEBUG	0
#endif

#ifdef	NeXT_MOD
#if	DEBUG
int	install = 0;		/* if 1 call kernel */
#else	DEBUG
int	install = 1;		/* if 1 call kernel */
#endif	DEBUG
#else	NeXT_MOD
int	install = !DEBUG;		/* if 1 call kernel */
#endif	NeXT_MOD
int	delete = 1;
/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *
rtlookup(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int doinghost = 1;

	if (dst->sa_family >= AF_MAX)
		return (0);
	(*afswitch[dst->sa_family].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (equal(&rt->rt_dst, dst))
			return (rt);
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		goto again;
	}
	return (0);
}

/*
 * Find a route to dst as the kernel would.
 */
struct rt_entry *
rtfind(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int af = dst->sa_family;
	int doinghost = 1, (*match)();

	if (af >= AF_MAX)
		return (0);
	(*afswitch[af].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_hash != hash)
			continue;
		if (doinghost) {
			if (equal(&rt->rt_dst, dst))
				return (rt);
		} else {
			if (rt->rt_dst.sa_family == af &&
			    (*match)(&rt->rt_dst, dst))
				return (rt);
		}
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		match = afswitch[af].af_netmatch;
		goto again;
	}
	return (0);
}

rtadd(dst, gate, metric, state)
	struct sockaddr *dst, *gate;
	int metric, state;
{
	struct afhash h;
	register struct rt_entry *rt;
	struct rthash *rh;
	int af = dst->sa_family, flags;
	u_int hash;

	if (af >= AF_MAX)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	flags = (*afswitch[af].af_ishost)(dst) ? RTF_HOST : 0;
	if (flags & RTF_HOST) {
		hash = h.afh_hosthash;
		rh = &hosthash[hash & ROUTEHASHMASK];
	} else {
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
	}
	rt = (struct rt_entry *)malloc(sizeof (*rt));
	if (rt == 0)
		return;
	rt->rt_hash = hash;
	rt->rt_dst = *dst;
	rt->rt_router = *gate;
	rt->rt_metric = metric;
	rt->rt_timer = 0;
	rt->rt_flags = RTF_UP | flags;
	rt->rt_state = state | RTS_CHANGED;
	rt->rt_ifp = if_ifwithnet(&rt->rt_router);
	if (metric)
		rt->rt_flags |= RTF_GATEWAY;
	insque(rt, rh);
	TRACE_ACTION(ADD, rt);
	/*
	 * If the ioctl fails because the gateway is unreachable
	 * from this host, discard the entry.  This should only
	 * occur because of an incorrect entry in /etc/gateways.
	 */
	if (install && ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0) {
		syslog(LOG_ERR,"SIOCADDRT: %m");
		if (errno == ENETUNREACH) {
			TRACE_ACTION(DELETE, rt);
			remque(rt);
			free((char *)rt);
		}
	}
}

rtchange(rt, gate, metric)
	struct rt_entry *rt;
	struct sockaddr *gate;
	short metric;
{
	int doioctl = 0, metricchanged = 0;
	struct rtentry oldroute;

	if (!equal(&rt->rt_router, gate))
		doioctl++;
	if (metric != rt->rt_metric)
		metricchanged++;
	if (doioctl || metricchanged) {
		TRACE_ACTION(CHANGE FROM, rt);
		if (doioctl) {
			oldroute = rt->rt_rt;
			rt->rt_router = *gate;
		}
		rt->rt_metric = metric;
		if ((rt->rt_state & RTS_INTERFACE) && metric) {
			rt->rt_state &= ~RTS_INTERFACE;
			syslog(LOG_ERR,
				"changing route from interface %s (timed out)",
				rt->rt_ifp->int_name);
		}
		if (metric)
			rt->rt_flags |= RTF_GATEWAY;
		else
			rt->rt_flags &= ~RTF_GATEWAY;
		rt->rt_state |= RTS_CHANGED;
		TRACE_ACTION(CHANGE TO, rt);
	}
	if (doioctl && install) {
		if (ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0)
			syslog(LOG_ERR,"SIOCADDRT %m");
		if (delete)
		if (ioctl(s, SIOCDELRT, (char *)&oldroute) < 0)
			syslog(LOG_ERR,"SIOCDELRT %m");
	}
}

rtdelete(rt)
	struct rt_entry *rt;
{

	if (rt->rt_state & RTS_INTERFACE) {
		syslog(LOG_ERR, "deleting route to interface %s (timed out)",
			rt->rt_ifp->int_name);
	}
	TRACE_ACTION(DELETE, rt);
	if (install && ioctl(s, SIOCDELRT, (char *)&rt->rt_rt))
		perror("SIOCDELRT");
	remque(rt);
	free((char *)rt);
}

rtinit()
{
	register struct rthash *rh;

	for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
	for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
}
