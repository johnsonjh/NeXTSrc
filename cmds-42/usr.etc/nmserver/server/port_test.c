/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * port_test.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/port_test.c,v $
 *
 */

#ifndef	lint
char port_test_rcsid[] = "$Header: port_test.c,v 1.1 88/09/30 15:41:27 osdev Exp $";
#endif not lint

/*
 * Tests for port records, port operations and port checkups.
 */

/*
 * HISTORY:
 *  6-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Make cthread_debug be a common symbol, as it might not be
 *	enabled in the library.
 *
 *  2-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added command line parameters.
 *
 *  5-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Lock is now inline in port record.
 *
 * 13-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added cthread_init().  Added log initialisation.
 *
 * 15-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Stared from Sanjay Agrawal's source.
 *
 */

#include <cthreads.h>
#include <ctype.h>
#include <mach.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "dispatcher.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "nm_extra.h"
#include "portops.h"
#include "portrec.h"
#include "port_defs.h"
#include "po_defs.h"
#include "srr.h"
#include "timer.h"
#include "uid.h"

static np_string[64];
static input_line[64];
int cthread_debug;



/*
 * tty_wait
 *	Do a non-blocking wait for input from the terminal.
 *
 */
PRIVATE void tty_wait(comstr)
char	*comstr;
BEGIN("tty_wait")
    int rc, n, i;

    while (TRUE) {
	rc = ioctl(0, (int)FIONREAD, (char *)(&n));
	if (rc != 0) {
	    ERROR((msg, "tty_wait.ioctl fails, rc = %d.\n", rc));
	}
	if (n == 0) {
	    cthread_yield();
	    continue;
	}
	for (i = 0; i < n; i++) comstr[i] = getchar();
	comstr[n-1] = '\0';
	return;
    }
END


/*
 * print_port_record
 *	Print out interesting fields of a port record.
 *
 * Parameters:
 *	lport	: the local port to look up.
 *
 */
PRIVATE void print_port_record(lport)
port_t	lport;
BEGIN("print_port_record")
    port_rec_ptr_t	port_rec_ptr;

    if ((port_rec_ptr = pr_lportlookup(lport)) == PORT_REC_NULL) {
	ERROR((msg, "print_port_record.pr_lportlookup fails.\n"));
	RET;
    }
    pr_nporttostring((char *)np_string, (network_port_ptr_t)&(port_rec_ptr->portrec_network_port));
    printf("Port record: local port = %d, network port = %s.\n", port_rec_ptr->portrec_local_port, np_string);
    lk_unlock(&port_rec_ptr->portrec_lock);
    RET;

END


/*
 * get_net_address
 *	Prompts for and returns a network address.
 *
 */
PRIVATE netaddr_t get_net_address()
BEGIN("get_net_address")
    int		num_items;
    ip_addr_t	input_address;
    int		net_owner, net_node_type, host_high, host_low;

    num_items = 0;
    while (num_items < 4) {
	tty_wait((char *)input_line);
	num_items = sscanf((char *)input_line, "%d%d%d%d", &net_owner, &net_node_type, &host_high, &host_low);
	if (num_items < 4) fprintf(stderr, "Need four integers for network address.\n");
	else {
	    input_address.ia_bytes.ia_net_owner = net_owner;
	    input_address.ia_bytes.ia_net_node_type = net_node_type;
	    input_address.ia_bytes.ia_host_high = host_high;
	    input_address.ia_bytes.ia_host_low = host_low;
	}
    }
    RETURN(input_address.ia_netaddr);

END


/*
 * get_network_port
 *	Prompts for and returns a network port.
 *
 */
PRIVATE void get_network_port(nport_ptr)
network_port_ptr_t	nport_ptr;
BEGIN("get_network_port")
    int		num_items;

    nport_ptr->np_sid.np_uid_high = 0;
    nport_ptr->np_sid.np_uid_low = 0;
    num_items = 0;
    while (num_items < 2) {
	printf("Type in the network port's PUID :\n");
	tty_wait((char *)input_line);
	num_items = sscanf((char *)input_line, "%d%d",
				&(nport_ptr->np_puid.np_uid_high), &(nport_ptr->np_puid.np_uid_low));
	if (num_items < 2) fprintf(stderr, "Need two integers for network port's PUID.\n");
    }
    printf("Type in the network port's receiver :\n");
    nport_ptr->np_receiver = get_net_address();
    printf("Type in the network port's owner :\n");
    nport_ptr->np_owner = get_net_address();
    RET;

END


/*
 * create
 *	Allocate a local or a network port and create a port record for it.
 *
 */
PRIVATE void create()
BEGIN("create")
    port_t		lport;
    kern_return_t	kr;
    port_rec_ptr_t	port_rec_ptr;
    char		ch;
    network_port_ptr_t	nport_ptr;
    network_port_t	nport;

    nport_ptr = (network_port_ptr_t)&nport;
    printf("To create a network_port type 'n' else hit return for lport? [lport]: \n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%c", &ch);
    if ((ch == 'n') || (ch == 'N')) {
	get_network_port(nport_ptr);
	if ((port_rec_ptr = pr_ntran(nport_ptr)) == PORT_REC_NULL) {
	    fprintf(stderr, "create.pr_ntran fails.\n");
	    RET;
	}
	lk_unlock(&port_rec_ptr->portrec_lock);
	print_port_record(port_rec_ptr->portrec_local_port);
	RET;
    }
    else {
	kr = port_allocate(task_self(), &lport);
	if (kr != KERN_SUCCESS) {
	    fprintf(stderr, "create.port_allocate fails, kr = %d.\n", kr);
	}
	printf("New local port = %d.\n", lport);
	if ((port_rec_ptr = pr_ltran(lport)) == PORT_REC_NULL) {
	    fprintf(stderr, "create.pr_ltran fails.\n");
	    RET;
	}
	lk_unlock(&port_rec_ptr->portrec_lock);
	print_port_record(lport);
	RET;
    }

	
END


/*
 * get_rights
 *	Prompts for a port right and returns an integer value.
 *
 */
PRIVATE int get_rights()
BEGIN("get_rights")
    char	right;

    printf("Input the rights to be transferred: 's' for send, 'o' for ownership, 'r' for receive, 'a' for all :\n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%c", &right);
    switch (right) {
	case 'o': case 'O': RETURN(PORT_OWNERSHIP_RIGHTS);
	case 'r': case 'R': RETURN(PORT_RECEIVE_RIGHTS);
	case 's': case 'S': RETURN(PORT_SEND_RIGHTS);
	case 'a': case 'A': RETURN(PORT_ALL_RIGHTS);
	default: fprintf(stderr, "Illegal right '%c'.\n", right); RETURN(0);
    }
END


/*
 * mod_nport
 *	Modify a network port - transfer rights to it from another host to ourself.
 *
 */
PRIVATE void mod_nport()
BEGIN("mod_nport")
    network_port_t	nport;
    netaddr_t		source;
    po_data_t		port_data;
    int			right, lport, size;

    get_network_port((network_port_ptr_t)&nport);
    pr_nporttostring((char *)np_string, (network_port_ptr_t)&nport);
    printf("Network port is %s.\n", np_string);

    printf("Type in the source of the transfer :\n");
    source = get_net_address();
    ipaddr_to_string((char *)np_string, source);
    printf("Source is %s.\n", np_string);

    if((right = get_rights()) == 0) {
	fprintf(stderr, "Illegal right.\n");
	RET;
    }

    port_data.pod_size = htons((2 * sizeof(short)) + sizeof(network_port_t));
    port_data.pod_right = htons(right);
    port_data.pod_nport = nport;
    size = po_translate_nport_rights(source, (pointer_t)&port_data, 0, &lport, &right);
    printf("po_translate_nport_rights returns %d, local port = %d, right = %d.\n", size, lport, right);
    print_port_record(lport);
    RET;

END


/*
 * mod_lport
 *	Modify a local port - transfer access rights to it to a remote network server.
 *
 */
mod_lport()
BEGIN("mod_lport")
    static int		client_id = 0;
    port_t		lport;
    netaddr_t		destination;
    po_data_t		port_data;
    int			right, size;

    client_id ++;

    printf("Type in an local port :\n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%d", &lport);
    printf("Local port = %d.\n", lport);

    if((right = get_rights()) == 0) {
	fprintf(stderr, "Illegal right.\n");
	RET;
    }

    printf("Type in the destination of the transfer :\n");
    destination = get_net_address();
    ipaddr_to_string((char *)np_string, destination);
    printf("Destination is %s.\n", np_string);

    size = po_translate_lport_rights(client_id, lport, right, 0, destination, (pointer_t)&(port_data));
    printf("po_translate_lport_rights returns %d.\n", size);
    if (size > 0) {
	pr_nporttostring((char *)np_string, (network_port_ptr_t)&(port_data.pod_nport));
	printf("port_data: size = %d, right = %d, network port = %s.\n",
		ntohs(port_data.pod_size), ntohs(port_data.pod_right), np_string);
	po_port_rights_commit(client_id, PO_RIGHTS_XFER_SUCCESS, destination);
    }
    print_port_record(lport);
    RET;
END


PRIVATE void modify()
BEGIN("modify")
    char	ch;

    printf("To modify a network_port type 'n' else hit return for lport? [lport]: \n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%c", &ch);
    if ((ch == 'n') || (ch == 'N')) {
	mod_nport();
    }
    else {
	mod_lport();
    }
    RET;
END



PRIVATE void remove()
BEGIN("remove")
    port_t		lport;
    port_rec_ptr_t	port_rec_ptr;
    network_port_ptr_t	nport_ptr;
    char		ch;
    network_port_t	nport;

    nport_ptr = (network_port_ptr_t)&nport;
    printf("To remove a network_port type 'n' else hit return for lport? [lport]: \n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%c", &ch);
    if ((ch == 'n') || (ch == 'N')) {
	get_network_port(nport_ptr);
	if ((port_rec_ptr = pr_nportlookup(nport_ptr)) == PORT_REC_NULL) {
	    pr_nporttostring((char *)np_string, nport_ptr);
	    fprintf(stderr, "Network port %s not found.\n", np_string);
	}
	else {
	    pr_nporttostring((char *)np_string, (network_port_ptr_t)&(port_rec_ptr->portrec_network_port));
	    printf("Removing record for local port %d, network port %s.\n",
				port_rec_ptr->portrec_local_port, np_string);
	    pr_destroy(port_rec_ptr);
	}
    }
    else {
	printf("Type in an lport :\n");
	tty_wait((char *)input_line);
	(void)sscanf((char *)input_line, "%d ", &lport);
	if ((port_rec_ptr = pr_lportlookup(lport)) == PORT_REC_NULL) {
	    fprintf(stderr, "Local port %d not found.\n", lport);
	}
	else {
	    pr_nporttostring((char *)np_string, (network_port_ptr_t)&(port_rec_ptr->portrec_network_port));
	    printf("Removing record for local port %d, network port %s.\n",
				port_rec_ptr->portrec_local_port, np_string);
	    pr_destroy(port_rec_ptr);
	}
    }
    RET;
END


PRIVATE void get_command()
BEGIN("get_command")
    char	comchar;
    int		flag = FALSE;

    printf("\nCreate, Modify or Remove a port ?\n");
    while (!flag) {
	tty_wait((char *)input_line);
	(void)sscanf((char *)input_line, "%c", &comchar);
	if (isupper(comchar)) tolower(comchar);
	if((comchar != 'c')&&(comchar != 'm')&&(comchar != 'r')){
	    printf("You must input one of 'c', 'm' or 'r'.\n");
	}
	else flag = TRUE;
    }
    switch(comchar){
	case 'c': create(); break;
	case 'm': modify(); break;
	case 'r': remove(); break;
    }
END


#define USAGE	"Usage: ptest [-c] [-t] [-p #]"

main(argc, argv)
int	argc;
char	**argv;
{
    char	com;
    int		i;

    cthread_init();

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-t") == 0) debug.tracing = 1;
	else if (strcmp(argv[i], "-c") == 0) cthread_debug = 1;
	else if ((strcmp(argv[i], "-p") == 0) && ((i + 1) < argc)) {
	    i ++;
	    debug.print_level = atoi(argv[i]);
	}
	else {
	    fprintf(stderr, "%s\n", USAGE);
	    (void)fflush(stderr);
	    _exit(-1);
	}
    }

    fprintf(stdout, "%s: %s %s print_level = %d.\n", argv[0],
			(debug.tracing ? "tracing" : ""),
			(cthread_debug ? "cthread_debug" : ""),
			debug.print_level);
    (void)fflush(stdout);

    if (!(mem_init())) panic("mem_init fails.");
    if (!(ls_init_1())) panic("ls_init_1 failed.");
    if (!(disp_init())) panic("disp_init fails.");
    if (!(network_init())) panic("network_init fails.");
    if (!(timer_init())) panic("timer_init fails.");
    if (!(uid_init())) panic("uid_init fails.");
    if (!(datagram_init())) panic("datagram_init fails.");
    if (!(srr_init())) panic("srr_init fails.");
    if (!(po_init())) panic("po_init fails.");
    if (!(pr_init())) panic("pr_init fails.");
    printf("Do you want to start the checkups? [n]: \n");
    tty_wait((char *)input_line);
    (void)sscanf((char *)input_line, "%c", &com);
    if ((com == 'y') || (com == 'Y')) {
	fprintf(stderr, "Starting port checkups module.\n");
	if (!(pc_init())) panic("pc_init fails.\n");
    }
    while (TRUE) get_command();
}

