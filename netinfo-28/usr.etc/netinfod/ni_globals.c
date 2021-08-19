/*
 * Globals used by the NetInfo server. Most are just constants
 * Copyright (C) 1989 by NeXT, Inc.
 */

#include <netinfo/ni.h>

/*
 * Constants
 */
const char NAME_NAME[] =  "name";
const char NAME_MACHINES[] = "machines";
const char NAME_IP_ADDRESS[] = "ip_address";
const char NAME_SERVES[] = "serves";
const char NAME_DOT[] = ".";
const char NAME_DOTDOT[] = "..";
const char NAME_MASTER[] = "master";
#ifdef notdef
const char NAME_LOOPBACK[] = "127.0.0.1";
#endif
const char NAME_UID[] = "uid";
const char NAME_PASSWD[] = "passwd";
const char NAME_USERS[] = "users";
const char NAME_NETWORKS[] = "networks";
const char NAME_ADDRESS[] = "address";
const char NAME_TRUSTED_NETWORKS[] = "trusted_networks";
const char ACCESS_USER_SUPER[] = "root";
const char ACCESS_USER_ANYBODY[] = "*";
const char ACCESS_NAME_PREFIX[] = "_writers_";
const char ACCESS_DIR_KEY[] = "_writers";

/*
 * Variables
 */
void *db_ni;		/* handle to the database we serve */
unsigned db_checksum;	/* checksum of this database */
int shutdown;		/* flag to signal time to shutdown server */
int i_am_clone;		/* on if server is clone */
unsigned master_addr;	/* address of master, if clone server */
int cleanupwait;	/* time to wait before cleaning up */
#ifdef DEBUG
int debugging;	
#endif
/* 
 * for clone: have done transfer in last time period
 */
unsigned have_transferred;

int tcp_sock = -1;
int udp_sock = -1;
