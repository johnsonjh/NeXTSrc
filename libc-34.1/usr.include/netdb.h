/*	@(#)netdb.h	1.2 88/02/04 4.0NFSSRC SMI;	from UCB 5.7 5/12/86	*/
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Structures returned by network
 * data base library.  All addresses
 * are supplied in host order, and
 * returned in network order (suitable
 * for use in system calls).
 */
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

struct rpcent {
        char    *r_name;        /* name of server for this rpc program */
        char    **r_aliases;    /* alias list */
        int     r_number;       /* rpc program number */
};

#ifdef __STRICT_BSD__
struct hostent	*gethostbyname(), *gethostbyaddr(), *gethostent();
struct netent	*getnetbyname(), *getnetbyaddr(), *getnetent();
struct servent	*getservbyname(), *getservbyport(), *getservent();
struct protoent	*getprotobyname(), *getprotobynumber(), *getprotoent();
struct rpcent	*getrpcbyname(), *getrpcbynumber(), *getrpcent();
#else
struct hostent	*gethostbyname(char *name); 
struct hostent  *gethostbyaddr(char *addr, int len, int type);
struct hostent  *gethostent(void);
void sethostent (int stayopen);
void endhostent (void);

struct netent *getnetbyname(char *name);
struct netent *getnetbyaddr(long net, int type);
struct netent *getnetent(void);
void setnetent (int stayopen);
void endnetent (void);

struct servent *getservbyname(char *name, char *proto);
struct servent *getservbyport(int port, char *proto);
struct servent *getservent(void);
void setservent (int stayopen);
void endservent (void);

struct protoent	*getprotobyname(char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
void setprotoent (int stayopen);
void endprotoent (void);

struct rpcent *getrpcbyname(char *name);
struct rpcent *getrpcbynumber(long number);
struct rpcent *getrpcent(void);
void setrpcent (int stayopen);
void endrpcent (void);
#endif __STRICT_BSD__

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 */

extern  int h_errno;	

#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */
