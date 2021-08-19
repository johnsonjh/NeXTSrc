/*
 * Definitions of the internally used routines in libc
 * Copyright (C) 1989 by NeXT, Inc.
 */
extern int _lu_running(void);

extern struct passwd *_lu_getpwuid();
extern struct passwd *_old_getpwuid();
extern struct passwd *_lu_getpwnam();
extern struct passwd *_old_getpwnam();
extern int _lu_putpwpasswd();
extern int _old_putpwpasswd();
extern struct passwd *_lu_getpwent();
extern struct passwd *_old_getpwent();
extern void _lu_setpwent();
extern void _old_setpwent();
extern void _lu_endpwent();
extern void _old_endpwent();
extern void _lu_setpwfile();
extern void _old_setpwfile();

extern struct group *_lu_getgrgid();
extern struct group *_old_getgrgid();
extern struct group *_lu_getgrnam();
extern struct group *_old_getgrnam();
extern struct group *_lu_getgrent();
extern struct group *_old_getgrent();
extern void _lu_setgrent();
extern void _old_setgrent();
extern void _lu_endgrent();
extern void _old_endgrent();
extern void _lu_setgrfile();
extern void _old_setgrfile();

extern struct hostent *_lu_gethostbyname();
extern struct hostent *_old_gethostbyname();
extern struct hostent *_res_gethostbyname();
extern struct hostent *_lu_gethostbyaddr();
extern struct hostent *_old_gethostbyaddr();
extern struct hostent *_res_gethostbyaddr();
extern struct hostent *_lu_gethostent();
extern struct hostent *_old_gethostent();
extern void _lu_sethostent();
extern void _old_sethostent();
extern void _lu_endhostent();
extern void _old_endhostent();
extern void _lu_sethostfile();
extern void _old_sethostfile();

extern struct servent *_lu_getservbyname();
extern struct servent *_old_getservbyname();
extern struct servent *_lu_getservbyport();
extern struct servent *_old_getservbyport();
extern struct servent *_lu_getservent();
extern struct servent *_old_getservent();
extern void _lu_setservent();
extern void _old_setservent();
extern void _lu_endservent();
extern void _old_endservent();

extern struct rpcent *_lu_getrpcbyname();
extern struct rpcent *_old_getrpcbyname();
extern struct rpcent *_lu_getrpcbynumber();
extern struct rpcent *_old_getrpcbynumber();
extern struct rpcent *_lu_getrpcent();
extern struct rpcent *_old_getrpcent();
extern void _lu_setrpcent();
extern void _old_setrpcent();
extern void _lu_endrpcent();
extern void _old_endrpcent();

extern struct protoent *_lu_getprotobyname();
extern struct protoent *_old_getprotobyname();
extern struct protoent *_lu_getprotobynumber();
extern struct protoent *_old_getprotobynumber();
extern struct protoent *_lu_getprotoent();
extern struct protoent *_old_getprotoent();
extern void _lu_setprotoent();
extern void _old_setprotoent();
extern void _lu_endprotoent();
extern void _old_endprotoent();

extern struct netent *_lu_getnetbyname();
extern struct netent *_old_getnetbyname();
extern struct netent *_lu_getnetbyaddr();
extern struct netent *_old_getnetbyaddr();
extern struct netent *_lu_getnetent();
extern struct netent *_old_getnetent();
extern void _lu_setnetent();
extern void _old_setnetent();
extern void _lu_endnetent();
extern void _old_endnetent();


extern void _lu_prdb_set();
extern prdb_ent *_lu_prdb_get();
extern prdb_ent *_lu_prdb_getbyname();
extern void _lu_prdb_end();
extern void _old_prdb_set();
extern prdb_ent *_old_prdb_get();
extern prdb_ent *_old_prdb_getbyname();
extern void _old_prdb_end();


extern int _lu_endmntent();
extern int _old_endmntent();
extern FILE *_lu_setmntent();
extern FILE *_old_setmntent();
extern int _old_addmntent();
extern struct mntent *_lu_getmntent();
extern struct mntent *_old_getmntent();


extern void _lu_setnetgrent();
extern int _lu_getnetgrent();
extern int _lu_innetgr();
extern void _lu_endnetgrent();
extern void _old_setnetgrent();
extern int _old_getnetgrent();
extern int _old_innetgr();
extern void _old_endnetgrent();


