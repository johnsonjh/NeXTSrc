/*
 * Globals used by the NetInfo server. Most are just constants
 * Copyright (C) 1989 by NeXT, Inc.
 */

/*
 * #defines
 */
#define CONNECTION_CHECK 1	/* turns on connection security checking */
#define ENABLE_CACHE 1		/* enables netinfo cache */

#define NI_RECVSIZE 512
#define NI_SENDSIZE 1024

/*
 * Constants
 */
extern const char NAME_NAME[];
extern const char NAME_MACHINES[];
extern const char NAME_IP_ADDRESS[];
extern const char NAME_SERVES[];
extern const char NAME_DOT[];
extern const char NAME_DOTDOT[];
extern const char NAME_MASTER[];
#ifdef notdef
extern const char NAME_LOOPBACK[];
#endif
extern const char NAME_UID[];
extern const char NAME_PASSWD[];
extern const char NAME_USERS[];
extern const char NAME_NETWORKS[];
extern const char NAME_ADDRESS[];
extern const char NAME_TRUSTED_NETWORKS[];
extern const char ACCESS_USER_SUPER[];
extern const char ACCESS_USER_ANYBODY[];
extern const char ACCESS_NAME_PREFIX[];
extern const char ACCESS_DIR_KEY[];


/*
 * Variables
 */
extern void *db_ni;		/* handle to the database we serve */
extern unsigned db_checksum;	/* checksum of this database */

extern int shutdown;		/* flag to signal time to shutdown server */

extern int debugging;		/* on if debugging only */
extern int i_am_clone;		/* on if server is clone */
extern unsigned master_addr;	/* address of master, if clone server */
/* 
 * for clone: have done transfer in last time period
 */
extern unsigned have_transferred;	

/*
 * Keep track of listener sockets
 */
extern int udp_sock;
extern int tcp_sock;
