/*	@(#)bootparam_subr.c	1.10 88/02/29 D/NFS */
#ifndef lint
static  char sccsid[] = "@(#)bootparam_subr.c 1.3 87/11/17 SMI";
#endif

/*
 * Subroutines that implement the bootparam services.
 */

#include <sys/time.h>
#include <rpcsvc/bootparam.h>
#include <netdb.h>
#include <nlist.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#define KERNEL		/* to get RTHASHSIZ */
#include <net/route.h>
#undef KERNEL
#include <net/if.h>			/* for structs ifnet and ifaddr */
#include <netinet/in.h>

#ifdef ifa_broadaddr
#include <netinet/in_var.h>		/* for struct in_ifaddr (4.3bsd only) */
#endif ifa_broadaddr

#define LINESIZE	1024	

int debug = 0;

/*
 * Whoami turns a client address into a client name
 * and suggested route machine.
*/
bp_whoami_res *
bootparamproc_whoami_1(argp)
	bp_whoami_arg *argp;
{
	static bp_whoami_res res;
	struct in_addr clnt_addr;
	struct in_addr route_addr, get_route();
	struct hostent *hp;
	static char clnt_entry[LINESIZE];
	static char domain[32];
	int reason;

	if (argp->client_address.address_type != IP_ADDR_TYPE) {
		if (debug) {
			fprintf(stderr,
			   "Whoami failed: unknown address type %d\n",
				argp->client_address.address_type);
		}
		return (NULL);
	}
	bcopy ((char *) &argp->client_address.bp_address.ip_addr,
	       (char *) &clnt_addr,  sizeof (clnt_addr));
	hp = gethostbyaddr(&clnt_addr, sizeof clnt_addr, AF_INET);
	if (hp == NULL) {
		if (debug) {
			fprintf(stderr,
				"Whoami failed: gethostbyaddr for %s.\n",
				inet_ntoa (clnt_addr));
		}
		return (NULL);
	}
	if ((reason = bp_getclntent(hp->h_name, clnt_entry)) != 0) {
		if (debug) {
			fprintf(stderr, "bp_getclntent(%s) failed (%s).\n",
				hp->h_name,
				reason > 0? (char *)yperr_string(reason)
					  : "entry not found");
		}
		return (NULL);
	}
	res.client_name = hp->h_name;
	getdomainname(domain, sizeof domain);
	res.domain_name = domain;
	res.router_address.address_type = IP_ADDR_TYPE;
	route_addr = get_route(clnt_addr);
	bcopy ((char *) &route_addr, 
	       (char *) &res.router_address.bp_address.ip_addr, 
	       sizeof (res.router_address.bp_address.ip_addr));

	if (debug) {
		fprintf(stderr, 
		    "Whoami returning name \"%s\" domain \"%s\" router %s\n",
		    res.client_name, res.domain_name,
		    inet_ntoa (res.router_address.bp_address.ip_addr));
	}
	return (&res);
}


struct nlist nl[] = {
#define	N_RTHOST	0
	{{ "_rthost" }},
#define	N_RTNET		1
	{{ "_rtnet" }},
	{{""}},
};

#ifdef NeXT_MOD
char *system = "/mach";
#else
char *system = "/vmunix";
#endif NeXT_MOD
char *kmemf  = "/dev/kmem";
int kmem;

/*
 * nlist the kernel once at program startup time.
 */
init_nlist()
{
	if (nl[0].n_type == 0) {
		nlist(system, nl);
		if (nl[0].n_type == 0) {
			fprintf(stderr, "no namelist for %s\n", system);
			exit(1);
		}
		kmem = open(kmemf, 0);
		if (kmem < 0) {
			fprintf(stderr, "cannot open %s\n", kmemf);
			exit(1);
		}
	}
}

/*
 * get_route paws through the kernel's routing table looking for a route
 * via a gateway that is on the same network and subnet as the client.
 * Any gateway will do because the selected gateway will return ICMP redirect
 * messages to the client if it can not route the traffic itself.
 */

struct in_addr
get_route(client_addr)
	struct in_addr client_addr;
{
	off_t hostaddr, netaddr;
	struct mbuf mb;
	register struct rtentry *rt;
	register struct mbuf *m;
	struct netent *np;
	struct mbuf *routehash[RTHASHSIZ];
	struct sockaddr_in *def_sin = NULL;
	int i, doinghost = 1;
	u_long net_subnetmask;	/* subnet mask in network order */

	hostaddr = nl[N_RTHOST].n_value;
	if (hostaddr == 0) {
		fprintf(stderr, "rthost: symbol not in namelist\n");
		exit(1);
	}
	netaddr = nl[N_RTNET].n_value;
	if (netaddr == 0) {
		fprintf(stderr, "rtnet: symbol not in namelist\n");
		exit(1);
	}
	if (lseek(kmem, (off_t) hostaddr, 0) == (off_t) -1) {
		perror ("lseek rthost");
		exit (1);
	}
	if (read(kmem, (char *) routehash, sizeof (routehash)) !=
	    sizeof (routehash)) {
		fprintf (stderr, "read rthost in kmem failed.\n");
		exit (1);
	}
again:
	for (i = 0; i < RTHASHSIZ; i++) {
		if (routehash[i] == 0)
			continue;
		for (m = routehash[i]; m; m = mb.m_next) {
			struct sockaddr_in *sin;
#ifdef ifa_broadaddr
			struct ifnet ifnet;
			struct ifaddr *ifa;
			struct ifaddr ifaddr;
			struct in_ifaddr in_ifaddr;
			struct in_ifaddr *ia;
#endif ifa_broadaddr

			if (lseek(kmem, (off_t) m, 0) == (off_t) -1) {
				perror ("lseek routehash");
				exit (1);
			}
			if (read(kmem, (char *) &mb, sizeof (mb)) != 
			    sizeof (mb)) {
				fprintf (stderr, "read routehash from kmem failed.\n");
				exit (1);
			}
			rt = mtod(&mb, struct rtentry *);
			
			/* 
			 * reject routes that are down and routes that are not
			 * on the same net as the client.
			 */
			if ((rt->rt_flags & RTF_UP) == 0) {
				continue;
			}

			sin = (struct sockaddr_in *)&rt->rt_gateway;
			if (netof(client_addr.s_addr) !=
			    netof(sin->sin_addr.s_addr))
				continue;

#ifdef ifa_broadaddr
			/*
			 * reject routes that are not on the same subnet
			 * as the client.
			 */
			if (lseek (kmem, (off_t) rt->rt_ifp, 0) == 
			    (off_t) -1) {
				perror ("lseek rt->rt_ifp");
				exit (1);
			}
			if (read (kmem, (char *) &ifnet, 
				  sizeof (struct ifnet)) != 
			    sizeof (struct ifnet)) {
			  	fprintf (stderr, "read rt->rt_ifp from kmem failed.\n");
				exit (1);
			}

			ifa = ifnet.if_addrlist;

			while (ifa != (struct ifaddr *) 0) {
				if (lseek (kmem, (off_t) ifa, 0) == 
				    (off_t) -1) {
					perror ("lseek ifaddr");
					exit (1);
				}
				if (read (kmem, (char *) &ifaddr, 
					  sizeof (struct ifaddr)) != 
				    sizeof (struct ifaddr)) {
					fprintf (stderr, "read ifaddr from kmem failed.\n");
					exit (1);
				}

				if (ifaddr.ifa_addr.sa_family == AF_INET)
					break;

				ifa = ifaddr.ifa_next;
			}

			if (ifa == (struct ifaddr *) 0)
				continue;

			/* 
			 * read same ifaddr struct again, this time as an
			 * in_ifaddr struct.
			 */
			if (lseek (kmem, (off_t) ifa, 0) == (off_t) -1) {
				perror ("lseek in_ifaddr");
				exit (1);
			}
			if (read (kmem, (char *) &in_ifaddr, 
				  sizeof (struct in_ifaddr)) != 
			    sizeof (struct in_ifaddr)) {
				fprintf ("read in_ifaddr from kmem failed.\n");
				exit (1);
			}
			  
			net_subnetmask = htonl(in_ifaddr.ia_subnetmask);
			if ((net_subnetmask & sin->sin_addr.s_addr) !=
			    (net_subnetmask & client_addr.s_addr)) {
				if (debug) {
				  /*
				   * must do this in two writes since inet_ntoa
				   * uses static storage for returned result
				   */
				  fprintf (stderr,
					"subnet mismatch.  gateway = %s, ",
					inet_ntoa (sin->sin_addr));
				  fprintf (stderr, "client_addr = %s\n",
					inet_ntoa (client_addr));
				}
				continue;
			}
#endif ifa_broadaddr
				
			/*
			 * route passes all of our tests so far.  If it is
			 * marked as a gateway, return it now.  Otherwise,
			 * save it as a default to return if there are no
			 * other gateways found.
			 */
			if ((rt->rt_flags & RTF_GATEWAY) == 0) {
				if (def_sin == NULL)
					def_sin = sin;
			} else {
				return (sin->sin_addr);
			}
		}
	}

	if (doinghost) {
		if (lseek(kmem, (off_t) netaddr, 0) == (off_t) -1) {
			perror ("lseek rtnet");
			exit (1);
		}
		if (read(kmem, (char *) routehash, sizeof (routehash)) !=
		    sizeof (routehash)) {
			fprintf ("read rtnet in kmem failed.\n");
			exit (1);
		}
		doinghost = 0;
		goto again;
	}

	if (def_sin) {
		return (def_sin->sin_addr);
	}

	if (debug) {
		fprintf(stderr, "No route found for client %s.\n", 
		       inet_ntoa (client_addr));
	}

	bzero ((char *) &client_addr, sizeof (client_addr));
	return (client_addr);
}

/*
 *  argument to netof is in network order.  IN_CLASS? macros assume
 *  host order, so we must make the appropriate conversions.
 */
netof(i)
	register int i;
{
	i = ntohl(i);
	if (IN_CLASSA(i))
		return (htonl( (i & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT));
	else if (IN_CLASSB(i))
		return (htonl( (i & IN_CLASSB_NET) >> IN_CLASSB_NSHIFT));
	else
		return (htonl( (i & IN_CLASSC_NET) >> IN_CLASSC_NSHIFT));
}

/*
 * Getfile gets the client name and the key and returns its server
 * and the pathname for that key.
 */
bp_getfile_res *
bootparamproc_getfile_1(argp)
	bp_getfile_arg *argp;
{
	static bp_getfile_res res;
	static char clnt_entry[LINESIZE];
	struct hostent *hp;
	char *cp;
	int reason;

	if ((reason = bp_getclntkey(argp->client_name, argp->file_id,
	     clnt_entry)) != 0) {
		if (debug) {
			fprintf(stderr, "bp_getclntkey(%s, %s) failed (%s).\n",
				argp->client_name, argp->file_id,
				reason > 0? (char *)yperr_string(reason)
					  : "entry not found");
		}
		return (NULL);
	}
	if ((cp = (char *)index(clnt_entry, ':')) == 0) {
		return (NULL);
	}
	*cp++ = '\0';
	res.server_name = clnt_entry;
	res.server_path = cp;
	if (*res.server_name == 0) {
		res.server_address.address_type = IP_ADDR_TYPE;
		bzero(&res.server_address.bp_address.ip_addr,
		      sizeof(res.server_address.bp_address.ip_addr));
	} else {
		if ((hp = gethostbyname(res.server_name)) == NULL) {
			if (debug) {
				fprintf(stderr,
				   "getfile_1: gethostbyname(%s) failed\n",
					res.server_name);
			}
			return (NULL);
		}
		res.server_address.address_type = IP_ADDR_TYPE;
		bcopy ((char *) hp->h_addr,
		       (char *) &res.server_address.bp_address.ip_addr,
		       sizeof (res.server_address.bp_address.ip_addr));
	}
	if (debug) {
		getf_printres(&res);
	}
	return (&res);
}

getf_printres(res)
	bp_getfile_res *res;
{
	fprintf(stderr, "getfile_1: file is \"%s\" %s \"%s\"\n",
		res->server_name,
		inet_ntoa (res->server_address.bp_address.ip_addr),
		res->server_path);
}
