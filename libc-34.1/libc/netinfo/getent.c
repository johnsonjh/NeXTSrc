#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <mntent.h>
#include <printerdb.h>

#include "clib_internal.h"

/*
 * The resolver should not be tried if it is not running. Right now,
 * we do not have a way to detect this, so we always try it.
 */
#define _res_running() 1 

typedef enum lookup_state {
	LOOKUP_NETINFO,
	LOOKUP_OLD,
} lookup_state;

static lookup_state pw_state = LOOKUP_NETINFO;
static lookup_state gr_state = LOOKUP_NETINFO;
static lookup_state h_state = LOOKUP_NETINFO;
static lookup_state n_state = LOOKUP_NETINFO;
static lookup_state p_state = LOOKUP_NETINFO;
static lookup_state r_state = LOOKUP_NETINFO;
static lookup_state m_state = LOOKUP_NETINFO;
static lookup_state s_state = LOOKUP_NETINFO;
static lookup_state prdb_state = LOOKUP_NETINFO;

#define SETSTATE(_ni_set, _old_set, state, stayopen) \
{ \
	if (_ni_running()) { \
		_ni_set(stayopen); \
		if (_yp_running()) { \
			_old_set(stayopen); \
		} \
		*state = LOOKUP_NETINFO; \
	} else { \
		_old_set(stayopen); \
		*state = LOOKUP_OLD; \
	} \
} 

#define UNSETSTATE(_ni_unset, _old_unset, state) \
{ \
	if (_ni_running()) { \
		_ni_unset(); \
		if (_yp_running()) { \
			_old_unset(); \
		} \
	} else { \
		_old_unset(); \
	} \
	*state = LOOKUP_NETINFO; \
}

#define GETENT(_ni_get, _old_get, state, res_type) \
{ \
	res_type *res; \
\
	if (_ni_running()) { \
		if (*state == LOOKUP_NETINFO) { \
			res = _ni_get(); \
			if (res != NULL) { \
				return (res); \
			} \
			if (_yp_running()) { \
				*state = LOOKUP_OLD; \
				res = _old_get(); \
			} else { \
				return (NULL); \
			} \
		} else { \
			res = _old_get(); \
		} \
	} else { \
		res = _old_get(); \
	} \
	return (res); \
}

#define LOOKUP1(_ni_lookup, _old_lookup, arg, res_type) \
{ \
	res_type *res; \
 \
	if (_ni_running()) { \
		res = _ni_lookup(arg); \
		if (res != NULL) { \
			return (res); \
		} \
		if (_yp_running()) { \
			res = _old_lookup(arg); \
		} \
	} else { \
		res = _old_lookup(arg); \
	} \
	return (res); \
}

#define LOOKUP2(_ni_lookup, _old_lookup, arg1, arg2, res_type) \
{ \
	res_type *res; \
 \
	if (_ni_running()) { \
		res = _ni_lookup(arg1, arg2); \
		if (res != NULL) { \
			return (res); \
		} \
		if (_yp_running()) { \
			res = _old_lookup(arg1, arg2); \
		} \
	} else { \
		res = _old_lookup(arg1, arg2); \
	} \
	return (res); \
}


struct passwd *
getpwuid(
	 int uid
	 )
{
	LOOKUP1(_ni_getpwuid, _old_getpwuid,  uid, struct passwd);
}

struct passwd *
getpwnam(
	 char *name
	 )
{
	LOOKUP1(_ni_getpwnam, _old_getpwnam,  name, struct passwd);
}

/*
 * putpwpasswd() is not supported with anything other than NetInfo
 * right now
 */
#define unix_passwd(name, oldpass, newpass) 0
#define yp_passwd(name, oldpass, newpass) 0
int
putpwpasswd(
	    char *login,
	    char *old_passwd, /* cleartext */
	    char *new_passwd /* encrypted */
	    )
{
	int res;
	
	if (_ni_running()) {
		res = _ni_putpwpasswd(login, old_passwd, new_passwd);
		if (_yp_running()) {
			res = yp_passwd(login, old_passwd, new_passwd);
		}
	} else {
		res = unix_passwd(login, old_passwd, new_passwd);
	}
	return (res);
}

struct passwd *
getpwent(
	 void
	 )
{
	GETENT(_ni_getpwent, _old_getpwent, &pw_state, struct passwd);
}


void
setpwent(
	 void
	 )
{
	SETSTATE(_ni_setpwent, _old_setpwent, &pw_state, 0);
}


void
endpwent(
	 void
	 )
{
	UNSETSTATE(_ni_endpwent, _old_endpwent, &pw_state);
}

void
setpwfile(
	  FILE *f
	  )
{
	/*
	 * This makes no sense with NetInfo or YP
	 */
	_old_setpwfile(f);
}


struct group *
getgrgid(
	 int gid
	 )
{
	LOOKUP1(_ni_getgrgid, _old_getgrgid, gid, struct group);
}



struct group *
getgrnam(
	 char *name
	 )
{
	LOOKUP1(_ni_getgrnam, _old_getgrnam, name, struct group);
}



struct group *
getgrent(
	 void
	 )
{
	GETENT(_ni_getgrent, _old_getgrent, &gr_state, struct group);
}



void
setgrent(
	 void
	 )
{
	SETSTATE(_ni_setgrent, _old_setgrent, &gr_state, 0);
}




void
endgrent(
	 void
	 )
{
	UNSETSTATE(_ni_endgrent, _old_endgrent, &gr_state);
}



void
setgrfile(
	  FILE *f
	  )
{
	/*
	 * This makes no sense with NetInfo or YP
	 */
	_old_setgrfile(f);
}



struct hostent *
gethostbyname(
	      char *name
	      )
{
	struct hostent *res;
 
	if (_ni_running()) {
		res = _ni_gethostbyname(name);
		if (res != NULL) { 
			return (res); 
		} 
		if (_res_running()) {
			res = _res_gethostbyname(name);
			if (res != NULL) {
				return (res);
			}
		}
		if (_yp_running()) { 
			res = _old_gethostbyname(name);
		} 
	} else { 
		res = _old_gethostbyname(name);
	} 
	return (res); 
}



struct hostent *
gethostbyaddr(
	      char *addr,
	      int len,
	      int type
	      )
{
	struct hostent *res;
 
	if (_ni_running()) {
		res = _ni_gethostbyaddr(addr, len, type);
		if (res != NULL) { 
			return (res); 
		} 
		if (_res_running()) {
			res = _res_gethostbyaddr(addr, len, type);
			if (res != NULL) {
				return (res);
			}
		}
		if (_yp_running()) { 
			res = _old_gethostbyaddr(addr, len, type);
		} 
	} else { 
		res = _old_gethostbyaddr(addr, len, type);
	} 
	return (res); 
}



struct hostent *
gethostent(
	   void
	   )
{
	GETENT(_ni_gethostent, _old_gethostent, &h_state, 
		       struct hostent);
}



void
sethostent(
	   int stayopen
	   )
{
	/*
	 * This makes no sense with NetInfo or YP
	 */
	SETSTATE(_ni_sethostent, _old_sethostent, &h_state, stayopen);
}



void
endhostent(
	   void
	   )
{
	UNSETSTATE(_ni_endhostent, _old_endhostent, &h_state);
}



void
sethostfile(
	    FILE *f
	    )
{
	_old_sethostent(f);
}

struct servent *
getservbyname(
	      char *name,
	      char *proto
	      )
{
	LOOKUP2(_ni_getservbyname, _old_getservbyname, name, proto,
			struct servent);
}



struct servent *
getservbyport(
	      int port,
	      char *proto
	      )
{
	LOOKUP2(_ni_getservbyport, _old_getservbyport, port, proto,
			struct servent);
}



struct servent *
getservent(
	   void
	   )
{
	GETENT(_ni_getservent, _old_getservent, &s_state, 
		       struct servent);
}



void
setservent(
	   int stayopen
	   )
{
	SETSTATE(_ni_setservent, _old_setservent, &s_state, stayopen);
}



void
endservent(
	   void
	   )
{
	UNSETSTATE(_ni_endservent, _old_endservent, &s_state);
}



struct rpcent *
getrpcbyname(
	     char *name
	     )
{
	LOOKUP1(_ni_getrpcbyname, _old_getrpcbyname, name,
			struct rpcent);
}



struct rpcent *
getrpcbynumber(
	       unsigned long number
	       )
{
	LOOKUP1(_ni_getrpcbynumber, _old_getrpcbynumber, number,
			struct rpcent);
}



struct rpcent *
getrpcent(
	  void
	  )
{
	GETENT(_ni_getrpcent, _old_getrpcent, &r_state, 
		       struct rpcent);
}



void
setrpcent(
	  int stayopen
	  )
{
	SETSTATE(_ni_setrpcent, _old_setrpcent, &r_state, stayopen);
}



void
endrpcent(
	  void
	  )
{
	UNSETSTATE(_ni_endrpcent, _old_endrpcent, &r_state);
}



struct protoent *
getprotobyname(
	       char *name
	       )
{
	LOOKUP1(_ni_getprotobyname, _old_getprotobyname, name,
			struct protoent);
}



struct protoent *
getprotobynumber(
		 unsigned long number
		 )
{
	LOOKUP1(_ni_getprotobynumber, _old_getprotobynumber, number,
			struct protoent);
}



struct protoent *
getprotoent(
	    void
	    )
{
	GETENT(_ni_getprotoent, _old_getprotoent, &p_state, 
		       struct protoent);
}



void
setprotoent(
	    int stayopen
	    )
{
	SETSTATE(_ni_setprotoent, _old_setprotoent, &p_state, stayopen);
}




void
endprotoent(
	    void
	    )
{
	UNSETSTATE(_ni_endprotoent, _old_endprotoent, &p_state);
}



struct netent *
getnetbyname(
	     char *name
	     )
{
	LOOKUP1(_ni_getnetbyname, _old_getnetbyname, name,
			struct netent);
}



struct netent *
getnetbyaddr(
	     long addr,
	     int type
	     )
{
	LOOKUP2(_ni_getnetbyaddr, _old_getnetbyaddr, addr, type,
			struct netent);
}



struct netent *
getnetent(
	  void
	  )
{
	GETENT(_ni_getnetent, _old_getnetent, &n_state, 
		       struct netent);
}



void
setnetent(
	  int stayopen
	  )
{
	SETSTATE(_ni_setnetent, _old_setnetent, &n_state, stayopen);
}




void
endnetent(
	  void
	  )
{
	UNSETSTATE(_ni_endnetent, _old_endnetent, &n_state);
}



void
prdb_set(
	 char *domain
	 )
{
	SETSTATE(_ni_prdb_set, _old_prdb_set, &prdb_state, domain);
}



prdb_ent *
prdb_get(
	 void
	 )
{
	GETENT(_ni_prdb_get, _old_prdb_get, &prdb_state, prdb_ent);
}




prdb_ent *
prdb_getbyname(
	       char *name
	       )
{
	LOOKUP1(_ni_prdb_getbyname, _old_prdb_getbyname, name, prdb_ent);
}


void
prdb_end(
	 void
	 )
{
	UNSETSTATE(_ni_prdb_end, _old_prdb_end, &prdb_state);
}

#define NI_MNTENT_FILE ((FILE *)-1)
static FILE *standard_file;

struct mntent *
getmntent(
	  FILE *f
	  )
{
	struct mntent *res;

	if (f == standard_file) {
		if (m_state == LOOKUP_NETINFO) { 
			res = _ni_getmntent(f); 
			if (res != NULL) { 
				return (res); 
			} 
			if (f != NI_MNTENT_FILE) {
				m_state = LOOKUP_OLD; 
				res = _old_getmntent(f); 
			} else { 
				return (NULL); 
			} 
		} else { 
			res = _old_getmntent(f); 
		} 
	} else { 
		res = _old_getmntent(f); 
	} 
	return (res); 
}


int
addmntent(
	  FILE *f,
	  struct mntent *mnt
	  )
{
	if (f == NI_MNTENT_FILE) {
		/*
		 * Not supported with NetInfo
		 */
		return (0);
	}
	return (_old_addmntent(f, mnt));
}


FILE *
setmntent(
	  char *file,
	  char *type
	  )
{
	if (_ni_running() && file == NULL) {
		m_state = LOOKUP_NETINFO;
		_ni_setmntent(NULL, type);
		if (_yp_running()) {
			standard_file = _old_setmntent(MNTTAB, type);
		} else {
			standard_file = NI_MNTENT_FILE;
		}
		return (standard_file);
	} else {
		return (_old_setmntent((file == NULL ? MNTTAB : file), type));
	}
}


int
endmntent(
	  FILE *f
	  )
{
	int res;

	if (f != standard_file) {
		res = _old_endmntent(f);
	} else {
		res = _ni_endmntent(NI_MNTENT_FILE);
		if (f != NI_MNTENT_FILE) {
			res = _old_endmntent(f);
		}
	}
	m_state = LOOKUP_NETINFO;
	return (res);
}

