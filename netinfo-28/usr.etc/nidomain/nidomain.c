/*
 * nidomain - administer nibindd
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "nibind_glue.h"
#include "clib.h"
#include <netdb.h>

struct in_addr getaddr(char *);
void usage(char *);

typedef enum nibind_opt {
	NB_USAGE,
	NB_LIST,
	NB_CREATEMASTER,
	NB_CREATECLONE,
	NB_DESTROY
} nibind_opt;

void
main(
     int argc,
     char **argv
     )
{
	char *myname;
	char *tag = "";
	char *host;
	char *master = "";
	char *master_name;
	char *master_tag;
	nibind_opt opt;
	ni_index i;
	void *nb;
	struct in_addr addr;
	ni_status status = NI_FAILED;
	nibind_registration *reg;
	unsigned nreg;

	myname = argv[0];
	opt = NB_USAGE;

	argc--;
	argv++;
	opt = NB_USAGE;
	host = NULL;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-l") == 0) {
			argc--;
			argv++;
			opt = NB_LIST;
			break;
		} else if (strcmp(*argv, "-m") == 0) {
			if (argc < 2) {
				break;
			}
			tag = argv[1];
			argc -= 2;
			argv += 2;
			opt = NB_CREATEMASTER;
			break;
		} else if (strcmp(*argv, "-d") == 0) {
			if (argc < 2) {
				break;
			}
			tag = argv[1];
			argc -= 2;
			argv += 2;
			opt = NB_DESTROY;
			break;
		} else if (strcmp(*argv, "-c") == 0) {
			if (argc < 3) {
				break;
			}
			tag = argv[1];
			master = argv[2];
			argc -= 3;
			argv += 3;
			opt = NB_CREATECLONE;
			break;
		} else {
			break;
		}
	}
	if (argc > 1 || opt == NB_USAGE) {
		usage(myname);
	}
	if (argc == 1) {
		host = argv[0];
	}
	if (host != NULL) {
		addr = getaddr(host);
		if (addr.s_addr == -1) {
			fprintf(stderr, "%s: unknown host - %s\n",
				myname, host);
			exit(1);
		}
	} else {
		addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	nb = nibind_new(&addr);
	if (nb == NULL) {
		fprintf(stderr, "%s: cannot connect to server %s\n", myname,
			host == NULL ? "localhost" : host);
		exit(1);
	}
	switch (opt) {
	case NB_CREATEMASTER:
		status = nibind_createmaster(nb, tag);
		break;
	case NB_CREATECLONE:
		master_tag = index(master, '/');
		if (tag == NULL) {
			fprintf(stderr, "%s: missing '/' in %s\n",
				myname, master);
			exit(1);
		}
		*master_tag++ = 0;
		master_name = master;
		addr = getaddr(master_name);
		if (addr.s_addr == -1) {
			fprintf(stderr, "%s: unknown host - %s\n",
				myname, master_name);
			exit(1);
		}
		status = nibind_createclone(nb, tag, master_name, &addr,
					    master_tag);
		break;
	case NB_DESTROY:
		status = nibind_destroydomain(nb, tag);
		break;
	case NB_LIST:
		status = nibind_listreg(nb, &reg, &nreg);
		if (status == NI_OK) {
			for (i = 0; i < nreg; i++) {
				printf("tag=%s udp=%d tcp=%d\n",
				       reg[i].tag,
				       reg[i].addrs.udp_port,
				       reg[i].addrs.tcp_port);
			}
		}
		break;
	default:
		fprintf(stderr, "should never happen\n");
		abort();
	}
	if (status != NI_OK) {
		fprintf(stderr, "%s: operation failed - %s\n",
			myname, ni_error(status));
		exit(1);
	}
	exit(0);
}



void
usage(char *name)
{
	(void)fprintf(stderr, 
	"usage: %s -l [host]                # list domains served\n"
	"       %s -m tag [host]            # create master domain\n"
	"       %s -d tag [host]            # destroy domain\n"
	"       %s -c tag master/tag [host] # create clone of master domain\n",
		name, name, name, name);
	exit(1);
}

/*
 * Translate a string into an IP address. The string
 * is either a number or a name.
 */
struct in_addr
getaddr(
	char *host
	)
{
	struct hostent *h;
	struct in_addr addr;

	addr.s_addr = inet_addr(host);
	if (addr.s_addr != -1) {
		return (addr);
	}
	h = gethostbyname(host);
	if (h != NULL) {
		bcopy(h->h_addr, &addr, sizeof(addr));
		return (addr);
	}
	return (addr);
}
