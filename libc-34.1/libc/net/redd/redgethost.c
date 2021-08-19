#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netdb.h>
#include <ctype.h>

static union {
	struct hostent redhostent;
	char redbuf[512-sizeof(struct udphdr)];
} redunion;

static int _red_stayopen;
static int redf;

static redopen()
{
	int s;
	static int redsock = 0;
	struct servent *sp = NULL;
	struct sockaddr_in sin;

	if (redsock == 0) {
		if ((sp = getservbyname("red", "udp")) == NULL)
			return(-1);
		redsock = sp->s_port;
	}
	if ((s = socket(AF_INET, SOCK_DGRAM)) < 0)
		return(-1);
	bzero(sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = redsock;
	if (connect(s, &sin, sizeof(sin)) < 0)
		return(-1);
	return(s);
}

setredent(f)
	int f;
{
	if (redf == -1)
		redf = redopen();
	_red_stayopen |= f;
	return(redf);
}

endredent()
{
	if (redf) {
		close(redf);
		redf = -1;
	}
	_red_stayopen = 0;
}

struct hostent *
redgethost(redf, name)
	register redf;
	register char *name;
{
	register struct hostent *h = &redunion.redhostent;
	register char **pp;
	register int n;

	n = strlen(name) + 1;
	strcpy(redunion.redbuf, name);
	if (send(redf, name, n, 0) < n)
		return(NULL);
	if ((n = recv(redf, &redunion, sizeof(redunion))) < sizeof(struct hostent))
		return(NULL);
	h->h_name += (int)&redunion;
	h->h_aliases += (int)&redunion;
	h->h_addr_list += (int)&redunion;
	h->h_attribs += (int)&redunion;
	for (pp = h->h_aliases; *pp != NULL; pp++)
		*pp += (int)&redunion;
	for (pp = h->h_addr_list; *pp != NULL; pp++)
		*pp += (int)&redunion;
	for (pp = h->h_attribs; *pp != NULL; pp++)
		*pp += (int)&redunion;
	return(h);
}

struct hostent *
redgethostbyaddr(addr, len, type)
	char *addr;
	register int len, type;
{
	register redf;
	register struct hostent *h;

	redf = setredent(_red_stayopen);
	if (redf < 0)
		return(NULL);
	h = redgethost(redf, inet_ntoa(*(u_long *)addr));
	if (_red_stayopen > 0)
		endredent();
	return(h);
}

struct hostent *
redgethostbyname(name)
	register char *name;
{
	register redf;
	register struct hostent *h;

	redf = setredent(_red_stayopen);
	if (redf < 0)
		return(NULL);
	h = redgethost(redf, name);
	if (_red_stayopen > 0)
		endredent();
	return(h);
}
