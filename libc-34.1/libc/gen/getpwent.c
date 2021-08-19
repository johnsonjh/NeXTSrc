#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)getpwent.c	1.6 88/05/11 4.0NFSSRC SMI"; /* from UCB 5.2 3/9/86 SMI 1.25 */
#endif

#include <stdio.h>
#include <pwd.h>
#include <ndbm.h>                   
#include <rpcsvc/ypclnt.h>
#include <sys/file.h>

#define MAXINT 0x7fffffff;

#ifdef	NeXT
#include <stdlib.h>
#include <string.h>
static struct passwd *interpret();
static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();
static struct passwd *getuidfromyellow();
static matchname();
static matchuid();
static uidof();
static getnextfromyellow();
static getfirstfromyellow();
static freeminuslist();
static addtominuslist();
static onminuslist();
static yellowup();
#else
extern void rewind();
extern long strtol();
extern int strcmp();
extern int strlen();
extern int fclose();
extern char *strcpy();
extern char *strncpy();
extern char *malloc();
#endif

struct passwd *_old_getpwent();

void _old_setpwent(), _old_endpwent();

static struct _pwjunk {
	struct passwd _NULLPW;
	FILE *_pwf;	/* pointer into /etc/passwd */
	char *_yp;		/* pointer into yellow pages */
	int _yplen;
	char *_oldyp;	
	int _oldyplen;
	struct list {
		char *name;
		struct list *nxt;
	} *_minuslist;
	struct passwd _interppasswd;
	char _interpline[BUFSIZ+1];
	char *_domain;
	char *_PASSWD;
        int _usingyellow;
        int _firsttime;
} *__pwjunk;
#define	NULLPW (_pw->_NULLPW)
#define pwf (_pw->_pwf)
#define yp (_pw->_yp)
#define yplen (_pw->_yplen)
#define	oldyp (_pw->_oldyp)
#define	oldyplen (_pw->_oldyplen)
#define minuslist (_pw->_minuslist)
#define interppasswd (_pw->_interppasswd)
#define interpline (_pw->_interpline)
#define domain (_pw->_domain)
#define PASSWD (_pw->_PASSWD)
#define usingyellow (_pw->_usingyellow)
#define firsttime (_pw->_firsttime)
static char EMPTY[] = "";
static struct passwd *interpret();
static struct passwd *interpretwithsave();
static struct passwd *save();
static struct passwd *getnamefromyellow();
static struct passwd *getuidfromyellow();

#if	NeXT
static DBM	*_pw_db;   
static int	_pw_stayopen;
#else
DBM	*_pw_db;   
int	_pw_stayopen;
#endif	NeXT

#if	NeXT
/*
 * Just like fgets, but ignores comment lines
 */
static char *
striphash_fgets(
		char *buf,
		unsigned size,
		FILE *fptr
		)
{
	char *res;

	for (;;) {
		res = fgets(buf, size, fptr);
		if (res == NULL) {
			return (NULL);
		}
		if (*res != '#') {
			break;
		}
	}
	return (res);
}
/*
 * Make a define so we only have to change it once
 */
#define fgets(buf, size, fptr)  striphash_fgets(buf, size, fptr)

#endif	NeXT


static struct _pwjunk *              
_pwjunk()
{
	register struct _pwjunk *_pw = __pwjunk;

	if (_pw == 0) {
		_pw = (struct _pwjunk *)calloc(1, sizeof (*__pwjunk));
		if (_pw == 0)
			return (0);
		PASSWD = "/etc/passwd";
		__pwjunk = _pw;
	}
	return (__pwjunk);
}

static struct passwd *
fetchpw(key)
	datum key;
{
	register struct _pwjunk *_pw = _pwjunk();
        register char *cp, *tp;
        static struct passwd passwd;
#if	NeXT
#else
        char line[BUFSIZ+1];
#endif	NeXT

        if (key.dptr == 0)
                return ((struct passwd *)NULL);
        key = dbm_fetch(_pw_db, key);
        if (key.dptr == 0)
                return ((struct passwd *)NULL);
        cp = key.dptr;
#if	NeXT
	tp = interpline;
#else
        tp = line;
#endif	NeXT

#ifdef	__GNU__
# define	EXPAND(e)	passwd.pw_##e = tp; while (*tp++ = *cp++);
#else	!__GNU__
# define	EXPAND(e)	passwd.pw_/**/e = tp; while (*tp++ = *cp++);
#endif	!__GNU__
        EXPAND(name);
        EXPAND(passwd);
        bcopy(cp, (char *)&passwd.pw_uid, sizeof (int));
        cp += sizeof (int);
        bcopy(cp, (char *)&passwd.pw_gid, sizeof (int));
        cp += sizeof (int);
        bcopy(cp, (char *)&passwd.pw_quota, sizeof (int));
        cp += sizeof (int);
        EXPAND(comment);
        EXPAND(gecos);
        EXPAND(dir);
        EXPAND(shell);
        return (&passwd);
}

struct passwd *
_old_getpwnam(nam)
	register char *nam;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	char line[BUFSIZ+1];
        datum key;

	if (_pw == 0)
		return (0);
	_old_setpwent();
        if (usingyellow)
        {
		if (!pwf)
			return NULL;
		while (fgets(line, BUFSIZ, pwf) != NULL) {
			if ((pw = interpret(line, strlen(line))) == NULL)
				continue;
			if (matchname(line, &pw, nam)) {
				if (!_pw_stayopen)
					_old_endpwent();
				return pw;
			}
		}
		if (!_pw_stayopen)
			_old_endpwent();
		return NULL;
        }   
        /* otherwise, use the dbm method */
        if (_pw_db == (DBM *)0 &&
                (_pw_db = dbm_open(PASSWD, O_RDONLY)) == (DBM *)0) {
            oldcode:
                    setpwent();
                    while ((pw = _old_getpwent()) && strcmp(nam, pw->pw_name))               
                            ;
                    if (!_pw_stayopen)
                            _old_endpwent();
                    return (pw);
            }
            if (flock(dbm_dirfno(_pw_db), LOCK_SH) < 0) {
                    dbm_close(_pw_db);
                    _pw_db = (DBM *)0;
                    goto oldcode;
            }
            key.dptr = nam;
            key.dsize = strlen(nam);
            pw = fetchpw(key);             
            (void) flock(dbm_dirfno(_pw_db), LOCK_UN);
            if (!_pw_stayopen) {
                    dbm_close(_pw_db);
                    _pw_db = (DBM *)0;
            }
            return (pw);
}

struct passwd *
_old_getpwuid(uid)
	int uid;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	char line[BUFSIZ+1];
        datum key;

	if (_pw == 0)
		return (0);
	_old_setpwent();
        if (usingyellow)
        {
    		if (!pwf)
			return NULL;
		while (fgets(line, BUFSIZ, pwf) != NULL) {
			if ((pw = interpret(line, strlen(line))) == NULL)
				continue;
			if (matchuid(line, &pw, uid)) {
				if (!_pw_stayopen)           
					_old_endpwent();
				return pw;
			}
		}
		if (!_pw_stayopen)
			_old_endpwent();
		return NULL;
 	   } 
           /* otherwise, use the dbm method */
           if (_pw_db == (DBM *)0 &&
               (_pw_db = dbm_open(PASSWD, O_RDONLY)) == (DBM *)0) {
           oldcode:
                   _old_setpwent();
                   while ((pw = _old_getpwent()) && pw->pw_uid != uid)
                           ;
                   if (!_pw_stayopen)
                           _old_endpwent();
                   return (pw);
           }
           if (flock(dbm_dirfno(_pw_db), LOCK_SH) < 0) {
                   dbm_close(_pw_db);
                   _pw_db = (DBM *)0;
                   goto oldcode;
           }
           key.dptr = (char *) &uid;
           key.dsize = sizeof uid;
           pw = fetchpw(key);
           (void) flock(dbm_dirfno(_pw_db), LOCK_UN);
           if (!_pw_stayopen) {
                   dbm_close(_pw_db);
                   _pw_db = (DBM *)0;
           }
           return (pw);
}

void
_old_setpwent()
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return;
	if (domain == NULL) {
		(void) usingypmap(&domain, NULL);
	}
	if(domain == NULL)
		usingyellow = 0;
	else {
		firsttime = 1;
		usingyellow = !yp_bind(domain);
	}
	if (pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
}

void
_old_endpwent()
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return;
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
	if (_pw_db != (DBM *)0) {             
		dbm_close(_pw_db);                  
		_pw_db = (DBM *)0;                 
		_pw_stayopen = 0;                 
	}                                        
  
	if (yp)
		free(yp);
	yp = NULL;
	freeminuslist();
	endnetgrent();                
}

_old_setpwfile(file)
	char *file;
{
	register struct _pwjunk *_pw = _pwjunk();

	if (_pw == 0)
		return (0);
	PASSWD = file;
	return (1);
}

static char *
pwskip(p)
register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
		*p++ = '\0';
	return(p);
}

struct passwd *
_old_getpwent()
{
	register struct _pwjunk *_pw = _pwjunk();
	char line1[BUFSIZ+1];
	static struct passwd *savepw;
	struct passwd *pw;
	char *user; 
	char *mach;
	char *dom;

	if (_pw == 0)
		return (0);
	if (domain == NULL) {
		(void) usingypmap(&domain, NULL);     
	}
	if (domain == NULL) 
		usingyellow = 0;
	else if (firsttime == 0) {
		firsttime = 1;
		usingyellow = !yp_bind(domain);
	}
	if (pwf == NULL && (pwf = fopen(PASSWD, "r")) == NULL) {
		return (NULL); 
	}

	for (;;) {
		if (yp) {
			pw = interpretwithsave(yp, yplen, savepw); 
			free(yp);
#if	NeXT
			yp = NULL;
#endif	NeXT			
			if (pw == NULL)
				return(NULL);
			getnextfromyellow();
			if (!onminuslist(pw)) {
				return(pw);
			}
		} else if (getnetgrent(&mach,&user,&dom)) {
			if (user) {
				pw = getnamefromyellow(user, savepw);
				if (pw != NULL && !onminuslist(pw)) {
					return(pw);
				}
			}
		} else {
			endnetgrent();
			if (fgets(line1, BUFSIZ, pwf) == NULL)  {
				return(NULL);
			}
			if ((pw = interpret(line1, strlen(line1))) == NULL)
				return(NULL);
			switch(line1[0]) {
			case '+':
                                if (!usingyellow)
                                    continue;
				if (strcmp(pw->pw_name, "+") == 0) {
					getfirstfromyellow();
					savepw = save(pw);
				} else if (line1[1] == '@') {
					savepw = save(pw);
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* include the whole yp database */
						getfirstfromyellow();
					} else {
						setnetgrent(pw->pw_name+2);
					}
				} else {
					/* 
					 * else look up this entry in yellow pages
				 	 */
					savepw = save(pw);
					pw = getnamefromyellow(pw->pw_name+1, savepw);
					if (pw != NULL && !onminuslist(pw)) {
						return(pw);
					}
				}
				break;
			case '-':
                                if (!usingyellow)
                                    continue;
				if (line1[1] == '@') {
					if (innetgr(pw->pw_name+2,(char *) NULL,"*",domain)) {
						/* everybody was subtracted */
						return(NULL);
					}
					setnetgrent(pw->pw_name+2);
					while (getnetgrent(&mach,&user,&dom)) {
						if (user) {
							addtominuslist(user);
						}
					}
					endnetgrent();
				} else {
					addtominuslist(pw->pw_name+1);
				}
				break;
			default:
				if (!onminuslist(pw)) {
					return(pw);
				}
				break;
			}
		}
	}
}

static
matchname(line1, pwp, name)
	char line1[];
	struct passwd **pwp;
	char *name;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *savepw;
	struct passwd *pw = *pwp;

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getnamefromyellow(name, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					savepw = save(pw);
					pw = getnamefromyellow(name,savepw);
					if (pw) {
						*pwp = pw;
						return 1;
					}
				}
				return 0;
			}
			if (strcmp(pw->pw_name+1, name) == 0) {

				savepw = save(pw);
				pw = getnamefromyellow(pw->pw_name+1, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				}
				else
					return 0;
			}
			break;
		case '-':
			if (line1[1] == '@') {
				if (innetgr(pw->pw_name+2,(char *) NULL,name,domain)) {
					*pwp = NULL;
					return 1;
				}
			}
			else if (strcmp(pw->pw_name+1, name) == 0) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			if (strcmp(pw->pw_name, name) == 0)
				return 1;
	}
	return 0;
}

static
matchuid(line1, pwp, uid)
	char line1[];
	struct passwd **pwp;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *savepw;
	struct passwd *pw = *pwp;
	char group[256];

	if (_pw == 0)
		return (0);
	switch(line1[0]) {
		case '+':
			if (strcmp(pw->pw_name, "+") == 0) {
				savepw = save(pw);
				pw = getuidfromyellow(uid, savepw);
				if (pw) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				savepw = save(pw);
				pw = getuidfromyellow(uid,savepw);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = pw;
					return 1;
				} else {
					return 0;
				}
			}
			savepw = save(pw);
			pw = getnamefromyellow(pw->pw_name+1, savepw);
			if (pw && pw->pw_uid == uid) {
				*pwp = pw;
				return 1;
			} else
				return 0;
			break;
		case '-':
			if (line1[1] == '@') {
				(void) strcpy(group,pw->pw_name+2);
				pw = getuidfromyellow(uid,&NULLPW);
				if (pw && innetgr(group,(char *) NULL,pw->pw_name,domain)) {
					*pwp = NULL;
					return 1;
				}
			} else if (uid == uidof(pw->pw_name+1)) {
				*pwp = NULL;
				return 1;
			}
			break;
		default:
			if (pw->pw_uid == uid)
				return 1;
	}
	return 0;
}

static
uidof(name)
	char *name;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;

	if (_pw == 0)
		return (0);
	pw = getnamefromyellow(name, &NULLPW);
	if (pw)
		return pw->pw_uid;
	else
		return MAXINT;
}

static
getnextfromyellow()
{
	register struct _pwjunk *_pw = _pwjunk();
	int reason;
	char *key = NULL;
	int keylen;

	if (_pw == 0)
		return;
	reason = yp_next(domain, "passwd.byname",oldyp, oldyplen, &key
	    ,&keylen,&yp,&yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static
getfirstfromyellow()
{
	register struct _pwjunk *_pw = _pwjunk();
	int reason;
	char *key = NULL;
	int keylen;

	if (_pw == 0)
		return;
	reason =  yp_first(domain, "passwd.byname", &key, &keylen, &yp, &yplen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_first failed is %d\n", reason);
#endif
		yp = NULL;
	}
	if (oldyp)
		free(oldyp);
	oldyp = key;
	oldyplen = keylen;
}

static struct passwd *
getnamefromyellow(name, savepw)
	char *name;
	struct passwd *savepw;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;

	if (_pw == 0)
		return (0);
	reason = yp_match(domain, "passwd.byname", name, strlen(name)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
getuidfromyellow(uid, savepw)
	int uid;
	struct passwd *savepw;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct passwd *pw;
	int reason;
	char *val;
	int vallen;
	char uidstr[20];

	if (_pw == 0)
		return (0);
	(void) sprintf(uidstr, "%d", uid);
	reason = yp_match(domain, "passwd.byuid", uidstr, strlen(uidstr)
		, &val, &vallen);
	if (reason) {
#ifdef DEBUG
fprintf(stderr, "reason yp_next failed is %d\n", reason);
#endif
		return NULL;
	} else {
		pw = interpret(val, vallen);
		free(val);
		if (pw == NULL)
			return NULL;
		if (savepw->pw_passwd && *savepw->pw_passwd)
			pw->pw_passwd =  savepw->pw_passwd;
		if (savepw->pw_gecos && *savepw->pw_gecos)
			pw->pw_gecos = savepw->pw_gecos;
		if (savepw->pw_dir && *savepw->pw_dir)
			pw->pw_dir = savepw->pw_dir;
		if (savepw->pw_shell && *savepw->pw_shell)
			pw->pw_shell = savepw->pw_shell;
		return pw;
	}
}

static struct passwd *
interpretwithsave(val, len, savepw)
	char *val;
	struct passwd *savepw;
{
	struct passwd *pw;
	
	if ((pw = interpret(val, len)) == NULL)
		return NULL;
	if (savepw->pw_passwd && *savepw->pw_passwd)
		pw->pw_passwd =  savepw->pw_passwd;
	if (savepw->pw_gecos && *savepw->pw_gecos)
		pw->pw_gecos = savepw->pw_gecos;
	if (savepw->pw_dir && *savepw->pw_dir)
		pw->pw_dir = savepw->pw_dir;
	if (savepw->pw_shell && *savepw->pw_shell)
		pw->pw_shell = savepw->pw_shell;
	return pw;
}

static struct passwd *
interpret(val, len)
	char *val;
{
	register struct _pwjunk *_pw = _pwjunk();
	register char *p;
	char *end;
	long x;
	register int ypentry;

	if (_pw == 0)
		return (0);
	(void) strncpy(interpline, val, len);
	p = interpline;
	interpline[len] = '\n';
	interpline[len+1] = 0;

	/*
 	 * Set "ypentry" if this entry references the Yellow Pages;
	 * if so, null UIDs and GIDs are allowed (because they will be
	 * filled in from the matching Yellow Pages entry).
	 */
	ypentry = (*p == '+');

	interppasswd.pw_name = p;
	p = pwskip(p);
	interppasswd.pw_passwd = p;
	p = pwskip(p);
	if (*p == ':' && !ypentry)
		/* check for non-null uid */
		return (NULL);
	x = strtol(p, &end, 10);       
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	interppasswd.pw_uid = x;
	if (*p == ':' && !ypentry)
		/* check for non-null gid */
		return (NULL);
	x = strtol(p, &end, 10);	
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	interppasswd.pw_gid = x;
	interppasswd.pw_quota = 0;
	interppasswd.pw_comment = EMPTY;
	interppasswd.pw_gecos = p;
	p = pwskip(p);
	interppasswd.pw_dir = p;
	p = pwskip(p);
	interppasswd.pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return(&interppasswd);
}

static
freeminuslist() {
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;

	if (_pw == 0)
		return;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		free(ls->name);
		free((char *) ls);
	}
	minuslist = NULL;
}

static
addtominuslist(name)
	char *name;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;
	char *buf;

	if (_pw == 0)
		return;
	ls = (struct list *) malloc(sizeof(struct list));
	buf = malloc((unsigned) strlen(name) + 1);
	(void) strcpy(buf, name);
	ls->name = buf;
	ls->nxt = minuslist;
	minuslist = ls;
}

/* 
 * save away psswd, gecos, dir and shell fields, which are the only
 * ones which can be specified in a local + entry to override the
 * value in the yellow pages
 */
static struct passwd *
save(pw)
	struct passwd *pw;
{
	static struct passwd *sv;

	/* free up stuff from last call */
	if (sv) {
		free(sv->pw_passwd);
		free(sv->pw_gecos);
		free(sv->pw_dir);
		free(sv->pw_shell);
		free((char *) sv);
	}
	sv = (struct passwd *) malloc(sizeof(struct passwd));

	sv->pw_passwd = malloc((unsigned) strlen(pw->pw_passwd) + 1);
	(void) strcpy(sv->pw_passwd, pw->pw_passwd);

	sv->pw_gecos = malloc((unsigned) strlen(pw->pw_gecos) + 1);
	(void) strcpy(sv->pw_gecos, pw->pw_gecos);

	sv->pw_dir = malloc((unsigned) strlen(pw->pw_dir) + 1);
	(void) strcpy(sv->pw_dir, pw->pw_dir);

	sv->pw_shell = malloc((unsigned) strlen(pw->pw_shell) + 1);
	(void) strcpy(sv->pw_shell, pw->pw_shell);

	return sv;
}

static
onminuslist(pw)
	struct passwd *pw;
{
	register struct _pwjunk *_pw = _pwjunk();
	struct list *ls;
	register char *nm;

	if (_pw == 0)
		return (0);
	nm = pw->pw_name;
	for (ls = minuslist; ls != NULL; ls = ls->nxt) {
		if (strcmp(ls->name,nm) == 0) {
			return(1);
		}
	}
	return(0);
}

#include <ctype.h>
#define DIGIT(x)	(isdigit(x) ? (x) - '0' : \
			islower(x) ? (x) + 10 - 'a' : (x) + 10 - 'A')
#define MBASE	('z' - 'a' + 1 + 10)

#if	NeXT
/*
 * strtol is defined more precisely in the ANSI library
 */
#else	/* NeXT */
/*
 *  strtol used by getpwent and getgrent
 */
long
strtol(str, ptr, base)
register char *str;
char **ptr;
register int base;
{
	register long val;
	register int c;
	int xx, neg = 0;

	if (ptr != (char **)0)
		*ptr = str; /* in case no number is formed */
	if (base < 0 || base > MBASE)
		return (0); /* base is invalid -- should be a fatal error */
	if (!isalnum(c = *str)) {
		while (isspace(c))
			c = *++str;
		switch (c) {
		case '-':
			neg++;
		case '+': /* fall-through */
			c = *++str;
		}
	}
	if (base == 0)
		if (c != '0')
			base = 10;
		else if (str[1] == 'x' || str[1] == 'X')
			base = 16;
		else
			base = 8;
	/*
	 * for any base > 10, the digits incrementally following
	 *	9 are assumed to be "abc...z" or "ABC...Z"
	 */
	if (!isalnum(c) || (xx = DIGIT(c)) >= base)
		return (0); /* no number formed */
	if (base == 16 && c == '0' && isxdigit(str[2]) &&
	    (str[1] == 'x' || str[1] == 'X'))
		c = *(str += 2); /* skip over leading "0x" or "0X" */
	for (val = -DIGIT(c); isalnum(c = *++str) && (xx = DIGIT(c)) < base; )
		/* accumulate neg avoids surprises near MAXLONG */
		val = base * val - xx;
	if (ptr != (char **)0)
		*ptr = str;
	return (neg ? val : -val);
}
#endif	/* NeXT */
