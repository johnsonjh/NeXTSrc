/*
 * libc redefines (so shared libs will work)
 * Copyright (C) 1989 by NeXT, Inc.
 */
#define alias_endent 		_alias_endent
#define alias_getbyname 	_alias_getbyname
#define alias_getent 		_alias_getent
#define alias_setent 		_alias_setent
#define bootp_getbyether 	_bootp_getbyether
#define bootp_getbyip 		_bootp_getbyip
#define bootparams_getbyname 	_bootparams_getbyname
#define endgrent 		_endgrent
#define endhostent 		_endhostent
#define endmntent 		_endmntent
#define endnetent 		_endnetent
#define endnetgrent 		_endnetgrent
#define endprotoent 		_endprotoent
#define endpwent 		_endpwent
#define endrpcent 		_endrpcent
#define endservent 		_endservent
#define getgrent 		_getgrent
#define getgrgid 		_getgrgid
#define getgrnam 		_getgrnam
#define gethostbyaddr 		_gethostbyaddr
#define gethostbyname 		_gethostbyname
#define gethostent 		_gethostent
#define getmntent 		_getmntent
#define getnetbyaddr 		_getnetbyaddr
#define getnetbyname 		_getnetbyname
#define getnetent 		_getnetent
#define getnetgrent 		_getnetgrent
#define getprotobyname 		_getprotobyname
#define getprotobynumber 	_getprotobynumber
#define getprotoent 		_getprotoent
#define getpwent 		_getpwent
#define getpwnam 		_getpwnam
#define getpwuid 		_getpwuid
#define getrpcbyname 		_getrpcbyname
#define getrpcbynumber 		_getrpcbynumber
#define getrpcent 		_getrpcent
#define getservbyname 		_getservbyname
#define getservbyport 		_getservbyport
#define getservent 		_getservent
#define innetgr 		_innetgr
#define prdb_end 		_prdb_end
#define prdb_get 		_prdb_get
#define prdb_getbyname 		_prdb_getbyname
#define prdb_set 		_prdb_set
#define putpwpasswd 		_putpwpasswd
#define setgrent 		_setgrent
#define setgrfile 		_setgrfile
#ifdef notdef
/*
 * Confusion about whether these takes an arg or not in prototypes
 */
#define sethostent 		_sethostent
#define setnetent 		_setnetent
#define setservent 		_setservent
#define setprotoent 		_setprotoent
#define setrpcent 		_setrpcent
#endif
#define sethostfile 		_sethostfile
#define setmntent 		_setmntent
#define setnetgrent 		_setnetgrent
#define setpwent 		_setpwent
#define setpwfile 		_setpwfile
