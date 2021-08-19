/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nmxportstat.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/utils/RCS/nmxportstat.c,v $
 *
 */

#ifndef	lint
static char     rcsid[] = "$Header: nmxportstat.c,v 1.1 88/09/30 15:47:17 osdev Exp $";
#endif not lint

/*
 * Examines and collates port statistics.
 */

/*
 * HISTORY:
  2-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
	Started.

 */

#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ls_defs.h"
#include "nm_defs.h"

char	*malloc();
char	*free();

#define MAX_STATS	100
#define MAX_SETS	20
typedef struct {
    netaddr_t		host_id;
    char		host_name[64];
    port_stat_ptr_t	stats[MAX_STATS];
} stat_set_t, *stat_set_ptr_t;

stat_set_t	stat_sets[MAX_SETS];


read_stats_file(filename, set_no)
char	*filename;
int	set_no;
{
    FILE		*statfile;
    int			i, j, no_entries;
    struct stat		filestat;
    char		*dp;
    struct hostent	*hp;
    stat_set_ptr_t	set_ptr;

    set_ptr = &stat_sets[set_no];
    for (i = 0; i < MAX_STATS; i ++) set_ptr->stats[i] = (port_stat_ptr_t)0;

    if ((statfile = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "Cannot open \"%s\".\n", filename);
	return 0;
    }
    if (fstat(fileno(statfile),&filestat)) {
    	perror("Cannot stat file");
	return 0;
    }
    no_entries = filestat.st_size / sizeof(port_stat_t);
    printf("Port statistics file \"%s\": number of entries = %d.\n", filename, no_entries);
    if (no_entries > MAX_STATS) {
	fprintf("Too many entries --- truncating to %d.\n", MAX_STATS);
	no_entries = MAX_STATS;
    }

    /*
     * Read in the IP address.
     */
    for (i = 0, dp = (char *)&set_ptr->host_id; i < 4; i ++, dp ++) *dp = fgetc(statfile);
    if ((hp = gethostbyaddr((char *)&(set_ptr->host_id), 4, AF_INET)) == 0) {
	strcpy(set_ptr->host_name, "");
    }
    else {
	strcpy(set_ptr->host_name, hp->h_name);
    }
    printf("    host id = %s, host name = %s.\n", inet_ntoa(set_ptr->host_id), set_ptr->host_name);

    /*
     * Read in the statistics records.
     */
    for (i = 0; i < no_entries; i ++) {
	if ((set_ptr->stats[i] = (port_stat_ptr_t)malloc(sizeof(port_stat_t))) == 0) {
	    fprintf(stderr, "Cannot allocate space for a statistics record.\n");
	    return 0;
	}
	for (j = 0, dp = (char *)set_ptr->stats[i]; j < sizeof(port_stat_t); j++, dp++)
	    *dp = fgetc(statfile);
    }
    return 1;
}


collate_stats(this_set, max_sets)
int	this_set;
int	max_sets;
{
    int			i, j, k;
    long		cur_puid_high, cur_puid_low;
    port_stat_ptr_t	cur_stats;

    for (i = 0; i < MAX_STATS; i++ ) {
	if (stat_sets[this_set].stats[i] != (port_stat_ptr_t)0) {
	    cur_stats = stat_sets[this_set].stats[i];
	    cur_puid_high = cur_stats->nport_id_high;
	    cur_puid_low = cur_stats->nport_id_low;
	    printf("Network Port %x.%x:\n", cur_puid_high, cur_puid_low);
	    printf("    Receiver = %s, Owner = %s\n", inet_ntoa(cur_stats->nport_receiver),
				inet_ntoa(cur_stats->nport_owner));
	    printf("    On %s (%s), lport = %x, alive = %d.\n", stat_sets[this_set].host_name,
				inet_ntoa(stat_sets[this_set].host_id),
				cur_stats->port_id, cur_stats->alive);
	    if (cur_stats->messages_sent)
		printf("\tmessages_sent = %d\n", cur_stats->messages_sent);
	    if (cur_stats->messages_rcvd)
		printf("\tmessages_rcvd = %d\n", cur_stats->messages_rcvd);
	    if (cur_stats->send_rights_sent)
		printf("\tsend_rights_sent = %d\n", cur_stats->send_rights_sent);
	    if (cur_stats->send_rights_rcvd_sender)
		printf("\tsend_rights_rcvd_sender = %d\n", cur_stats->send_rights_rcvd_sender);
	    if (cur_stats->send_rights_rcvd_recown)
		printf("\tsend_rights_rcvd_recown = %d\n", cur_stats->send_rights_rcvd_recown);
	    if (cur_stats->rcv_rights_xferd)
		printf("\trcv_rights_xferd = %d\n", cur_stats->rcv_rights_xferd);
	    if (cur_stats->own_rights_xferd)
		printf("\town_rights_xferd = %d\n", cur_stats->own_rights_xferd);
	    if (cur_stats->all_rights_xferd)
		printf("\tall_rights_xferd = %d\n", cur_stats->all_rights_xferd);
	    if (cur_stats->tokens_sent)
		printf("\ttokens_sent = %d\n", cur_stats->tokens_sent);
	    if (cur_stats->tokens_requested)
		printf("\ttokens_requested = %d\n", cur_stats->tokens_requested);
	    if (cur_stats->xfer_hints_sent)
		printf("\txfer_hints_sent = %d\n", cur_stats->xfer_hints_sent);
	    if (cur_stats->xfer_hints_rcvd)
		printf("\txfer_hints_rcvd = %d\n", cur_stats->xfer_hints_rcvd);
	    free(cur_stats);
	    stat_sets[this_set].stats[i] = (port_stat_ptr_t)0;

	    for (j = this_set + 1; j < max_sets; j ++) {
		for (k = 0; k < MAX_STATS; k++ ) {
		    if (stat_sets[j].stats[k] != (port_stat_ptr_t)0) {
			cur_stats = stat_sets[j].stats[k];
			if ((cur_stats->nport_id_high == cur_puid_high)
				&& (cur_stats->nport_id_low == cur_puid_low))
			{
			    printf("    On %s (%s), lport = %x, alive = %d.\n",
					stat_sets[j].host_name, inet_ntoa(stat_sets[j].host_id),
					cur_stats->port_id, cur_stats->alive);
			    if (cur_stats->messages_sent)
				printf("\tmessages_sent = %d\n", cur_stats->messages_sent);
			    if (cur_stats->messages_rcvd)
				printf("\tmessages_rcvd = %d\n", cur_stats->messages_rcvd);
			    if (cur_stats->send_rights_sent)
				printf("\tsend_rights_sent = %d\n", cur_stats->send_rights_sent);
			    if (cur_stats->send_rights_rcvd_sender)
				printf("\tsend_rights_rcvd_sender = %d\n",
					cur_stats->send_rights_rcvd_sender);
			    if (cur_stats->send_rights_rcvd_recown)
				printf("\tsend_rights_rcvd_recown = %d\n",
					cur_stats->send_rights_rcvd_recown);
			    if (cur_stats->rcv_rights_xferd)
				printf("\trcv_rights_xferd = %d\n", cur_stats->rcv_rights_xferd);
			    if (cur_stats->own_rights_xferd)
				printf("\town_rights_xferd = %d\n", cur_stats->own_rights_xferd);
			    if (cur_stats->all_rights_xferd)
				printf("\tall_rights_xferd = %d\n", cur_stats->all_rights_xferd);
			    if (cur_stats->tokens_sent)
				printf("\ttokens_sent = %d\n", cur_stats->tokens_sent);
			    if (cur_stats->tokens_requested)
				printf("\ttokens_requested = %d\n", cur_stats->tokens_requested);
			    if (cur_stats->xfer_hints_sent)
				printf("\txfer_hints_sent = %d\n", cur_stats->xfer_hints_sent);
			    if (cur_stats->xfer_hints_rcvd)
				printf("\txfer_hints_rcvd = %d\n", cur_stats->xfer_hints_rcvd);
			    free(cur_stats);
			    stat_sets[j].stats[k] = (port_stat_ptr_t)0;
			}
		    }
		}
	    }
	}
    }
}


main(argc, argv)
int	argc;
char	**argv;
{
    int		no_sets = 0, i;

    if (argc == 0) exit(-1);

    argc--;
    argv++;
    while (argc--) {
	if (no_sets >= MAX_SETS) {
	    fprintf(stderr, "Maximum number (%d) of sets of statistics reached.\n", MAX_SETS);
	}
	else if (read_stats_file(*argv++, no_sets)) no_sets++;
    }
    printf("\n");
    for (i = 0; i < no_sets; i++) collate_stats(i, no_sets);
}



    


