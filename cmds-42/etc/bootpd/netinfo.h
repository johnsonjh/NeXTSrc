/*
 * netinfo.h - Header file for netinfo routines.
 */
#import	<sys/boolean.h>
#import <netinet/in.h>
#import <netinfo/ni.h>
#import <netinfo/ni_util.h>

boolean_t	promiscuous();
int		ni_getnextipaddr(struct in_addr *inaddr);
int		ni_getconfigipaddr(struct in_addr *inaddr);
int		ni_getbootfile(char *bootfile, int len);
boolean_t	ni_hostismine(char *hostname);
ni_name		ni_getnetpasswd();
boolean_t	ni_passwdok(char *passwd);
int		ni_setenbyname(char *name, unsigned char *enaddr);
int		ni_createhost(char *hostname, unsigned char *enaddr,
			      struct in_addr inaddr, char *bootfile);
