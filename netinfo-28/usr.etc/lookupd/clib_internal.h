/*
 * C library definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include "libc_redefines.h"
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <mntent.h>
#include <printerdb.h>
#include <aliasdb.h>

/*
 * Confusion about whether sethostent takes an arg or not in prototypes
 */
#ifndef sethostent
#define sethostent _sethostent
#endif
#ifndef setnetent
#define setnetent _setnetent
#endif
#ifndef setprotoent
#define setprotoent _setprotoent
#endif
#ifndef setservent
#define setservent _setservent
#endif
#ifndef setrpcent
#define setrpcent _setrpcent
#endif

void setpwent();
void endpwent();
void setgrent();
void endgrent();
void sethostent();
void endhostent();
void setservent();
void endservent();
void setprotoent();
void endprotoent();
void setrpcent();
void endrpcent();
void setrpcent();
void endrpcent();
void setnetent();
void endnetent();

void setnetgrent();
void endnetgrent();
int getnetgrent();
int innetgr();

int bootparams_getbyname();
int bootp_getbyether();
int bootp_getbyip();

extern int _ni_running(void);
extern void ni_getchecksums(unsigned *, unsigned **);
extern int _yp_running(void);
extern int _res_running(void);

extern struct passwd *_ni_getpwuid();
extern struct passwd *_old_getpwuid();
extern struct passwd *_ni_getpwnam();
extern struct passwd *_old_getpwnam();
extern int _ni_putpwpasswd();
extern int _old_putpwpasswd();
extern struct passwd *_ni_getpwent();
extern struct passwd *_old_getpwent();
extern void _ni_setpwent();
extern void _old_setpwent();
extern void _ni_endpwent();
extern void _old_endpwent();
extern void _ni_setpwfile();
extern void _old_setpwfile();

extern struct group *_ni_getgrgid();
extern struct group *_old_getgrgid();
extern struct group *_ni_getgrnam();
extern struct group *_old_getgrnam();
extern struct group *_ni_getgrent();
extern struct group *_old_getgrent();
extern void _ni_setgrent();
extern void _old_setgrent();
extern void _ni_endgrent();
extern void _old_endgrent();
extern void _ni_setgrfile();
extern void _old_setgrfile();

extern struct hostent *_ni_gethostbyname();
extern struct hostent *_old_gethostbyname();
extern struct hostent *_res_gethostbyname();
extern struct hostent *_ni_gethostbyaddr();
extern struct hostent *_old_gethostbyaddr();
extern struct hostent *_res_gethostbyaddr();
extern struct hostent *_ni_gethostent();
extern struct hostent *_old_gethostent();
extern void _ni_sethostent();
extern void _old_sethostent();
extern void _ni_endhostent();
extern void _old_endhostent();
extern void _ni_sethostfile();
extern void _old_sethostfile();

extern struct servent *_ni_getservbyname();
extern struct servent *_old_getservbyname();
extern struct servent *_ni_getservbyport();
extern struct servent *_old_getservbyport();
extern struct servent *_ni_getservent();
extern struct servent *_old_getservent();
extern void _ni_setservent();
extern void _old_setservent();
extern void _ni_endservent();
extern void _old_endservent();

extern struct rpcent *_ni_getrpcbyname();
extern struct rpcent *_old_getrpcbyname();
extern struct rpcent *_ni_getrpcbynumber();
extern struct rpcent *_old_getrpcbynumber();
extern struct rpcent *_ni_getrpcent();
extern struct rpcent *_old_getrpcent();
extern void _ni_setrpcent();
extern void _old_setrpcent();
extern void _ni_endrpcent();
extern void _old_endrpcent();

extern struct protoent *_ni_getprotobyname();
extern struct protoent *_old_getprotobyname();
extern struct protoent *_ni_getprotobynumber();
extern struct protoent *_old_getprotobynumber();
extern struct protoent *_ni_getprotoent();
extern struct protoent *_old_getprotoent();
extern void _ni_setprotoent();
extern void _old_setprotoent();
extern void _ni_endprotoent();
extern void _old_endprotoent();

extern struct netent *_ni_getnetbyname();
extern struct netent *_old_getnetbyname();
extern struct netent *_ni_getnetbyaddr();
extern struct netent *_old_getnetbyaddr();
extern struct netent *_ni_getnetent();
extern struct netent *_old_getnetent();
extern void _ni_setnetent();
extern void _old_setnetent();
extern void _ni_endnetent();
extern void _old_endnetent();


extern void _ni_prdb_set();
extern const prdb_ent *_ni_prdb_get();
extern const prdb_ent *_ni_prdb_getbyname();
extern void _ni_prdb_end();
extern void _old_prdb_set();
extern prdb_ent *_old_prdb_get();
extern prdb_ent *_old_prdb_getbyname();
extern void _old_prdb_end();


extern int _ni_endmntent();
extern int _old_endmntent();
extern FILE *_ni_setmntent();
extern FILE *_old_setmntent();
extern int _old_addmntent();
extern struct mntent *_ni_getmntent();
extern struct mntent *_old_getmntent();

extern void _ni_alias_setent();
extern aliasent *_ni_alias_getent();
extern aliasent *_ni_alias_getbyname();
extern void _ni_alias_endent();
extern void _old_alias_setent();
extern aliasent *_old_alias_getent();
extern aliasent *_old_alias_getbyname();
extern void _old_alias_endent();


void _ni_setnetgrent();
void _ni_endnetgrent();
int _ni_getnetgrent();
int _ni_innetgr();
void _old_setnetgrent();
void _old_endnetgrent();
int _old_getnetgrent();
int _old_innetgr();

int _ni_bootparams_getbyname();
int _ni_bootp_getbyether();
int _ni_bootp_getbyip();
int _old_bootparams_getbyname();
int _old_bootp_getbyether();
int _old_bootp_getbyip();
