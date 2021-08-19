/*
 *  oklogin - validate login name, group, account and password
 *
 **********************************************************************
 * HISTORY
 * 05-Mar-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added bzero of malloced a and g structs.
 *
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed code to preserve group list order after the first two
 *	entries.  This will allow routines searching for a group to
 *	terminate linear searches after group numbers are out of range.
 *
 * 01-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed group list pointer to be statically allocated here.
 *
 * 25-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to keep better track of free space allocated.  All
 *	pointers returned now point to static buffers.  The caller is
 *	responsible for copying any information.  The group and account
 *	buffers are those used by getgrent and getacent so they must
 *	be save before called again.  The only time that this routine
 *	should ever lose memory is if malloc() fails, which is usually
 *	tough for the caller to deal with anyway.
 *
 * 11-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Modified to run under 4.2 at CMU CS.
 *
 * 14-Jun-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <stdio.h>
#include <libc.h>
#include <sys/param.h>
#include <pwd.h>
#include <grp.h>
#include <acc.h>
#include <sys/ttyloc.h>
#include <access.h>

#define Static static

Static int acctf;			/* true if accounts file exists */
Static int newacctf;			/* true if new format accounts file */
Static int grplist[NGROUPS+1];		/* valid group list for initgroups */

Static char acctname[BUFSIZ+1];		/* account name if we returned one */

#define MAXACTATTRS 32

Static char *acentline;			/* address of line in getacent */
Static struct account *acentaccount;	/* address of account in getacent */
Static char **acentattrs;		/* address of ac_attrs in getacent */

Static struct a {
    struct a *a_next;
    int a_login:1, a_project:1, a_default:1;
    struct account *a_act;
} *alist;

#define a_name		a_act->ac_name
#define a_attrs		a_act->ac_attrs
#define a_uid		a_act->ac_uid
#define a_aid		a_act->ac_aid
#define a_created	a_act->ac_created
#define a_expires	a_act->ac_expires
#define a_sponsor	a_act->ac_sponsor
#define a_ctime		a_act->ac_ctime
#define a_xtime		a_act->ac_xtime

/* allocate and copy an accounting entry */

Static struct a *saveact(af)
register struct account *af;
{
    register struct a *at;
    register char **p;

    if (acentline == NULL) {
	acentaccount = af;
	acentline = af->ac_name;
	acentattrs = af->ac_attrs;
    }
    if ((at = (struct a *)malloc(sizeof(struct a))) == NULL)
	return(NULL);
    bzero(at, sizeof(struct a));
    if ((at->a_act = (struct account *)malloc(sizeof(struct account))) == NULL)
	return(NULL);
    bcopy(acentaccount, at->a_act, sizeof(struct account));
    if ((at->a_name = (char *)malloc(BUFSIZ+1)) == NULL)
	return(NULL);
    bcopy(acentline, at->a_name, BUFSIZ+1);
    if ((at->a_attrs = (char **)malloc(MAXACTATTRS*sizeof(char *))) == NULL)
	return(NULL);
    bcopy(acentattrs, at->a_attrs, MAXACTATTRS*sizeof(char *));
    at->a_default = at->a_login = at->a_project = 0;
    for (p = at->a_attrs; *p; p++) {
	*p += (at->a_name - acentline);
	if (strcmp(*p, "DEFAULT") == 0)
	    at->a_default = 1;
	if (strcmp(*p, "LOGIN") == 0)
	    at->a_login = 1;
	else if (strcmp(*p, "PROJECT") == 0)
	    at->a_project = 1;
    }
    at->a_created += (at->a_name - acentline);
    at->a_expires += (at->a_name - acentline);
    at->a_sponsor += (at->a_name - acentline);
    at->a_xtime = atot(at->a_expires);
    return(at);
}

Static struct account *restoreact(af)
register struct account *af;
{
    register char **p;

    bcopy(af, acentaccount, sizeof(struct account));
    bcopy(af->ac_name, acentline, BUFSIZ+1);
    bcopy(af->ac_attrs, acentattrs, MAXACTATTRS*sizeof(char *));
    for (p = acentattrs; *p; p++)
	*p += (acentline - af->ac_name);
    acentaccount->ac_created += (acentline - af->ac_name);
    acentaccount->ac_expires += (acentline - af->ac_name);
    acentaccount->ac_sponsor += (acentline - af->ac_name);
    return(acentaccount);
}

#define	MAXGRP	200

Static char *grentline;			/* address of line in getgrent */
Static struct group *grentgroup;	/* address of group in getgrent */
Static char **grentmem;			/* address of gr_mem in getgrent */

Static struct g {
    struct g *g_next;
    struct a *g_act;
    struct group *g_grp;
} *glist;

#define g_name		g_grp->gr_name
#define g_passwd	g_grp->gr_passwd
#define g_gid		g_grp->gr_gid
#define g_mem		g_grp->gr_mem

/* allocate and copy a group entry */

Static struct g *savegrp(gf)
register struct group *gf;
{
    register struct g *gt;
    register struct a *al;
    register char **p;

    if (grentline == NULL) {
	grentgroup = gf;
	grentline = gf->gr_name;
	grentmem = gf->gr_mem;
    }
    if ((gt = (struct g *)malloc(sizeof(struct g))) == NULL)
	return(NULL);
    bzero(gt, sizeof(struct g));
    if ((gt->g_grp = (struct group *)malloc(sizeof(struct group))) == NULL)
	return(NULL);
    bcopy(grentgroup, gt->g_grp, sizeof(struct group));
    if ((gt->g_name = (char *)malloc(BUFSIZ+1)) == NULL)
	return(NULL);
    bcopy(grentline, gt->g_name, BUFSIZ+1);
    if ((gt->g_mem = (char **)malloc(MAXGRP*sizeof(char *))) == NULL)
	return(NULL);
    bcopy(grentmem, gt->g_mem, MAXGRP*sizeof(char *));
    for (p = gt->g_mem; *p; p++)
	*p += (gt->g_name - grentline);
    gt->g_passwd += (gt->g_name - grentline);
    gt->g_act = NULL;
    for (al = alist; al; al = al->a_next)
	if (gt->g_gid == al->a_aid) {
	    gt->g_act = al;
	    break;
	}
    return(gt);
}

Static struct group *restoregrp(gf)
register struct group *gf;
{
    register char **p;

    bcopy(gf, grentgroup, sizeof(struct group));
    bcopy(gf->gr_name, grentline, BUFSIZ+1);
    bcopy(gf->gr_mem, grentmem, MAXGRP*sizeof(char *));
    for (p = grentmem; *p; p++)
	*p += (grentline - gf->gr_name);
    grentgroup->gr_passwd += (grentline - gf->gr_name);
    return(grentgroup);
}

Static int freereturn(value)
int value;
{
    register struct g *gl;
    register struct a *al;

    /* free group list */
    while (gl = glist) {
	glist = gl->g_next;
	free(gl->g_mem);
	free(gl->g_name);
	free(gl->g_grp);
	free(gl);
    }
    /* free account list */
    while (al = alist) {
	alist = alist->a_next;
	free(al->a_attrs);
	free(al->a_name);
	free(al->a_act);
	free(al);
    }
    return(value);
}

/*
 *  Dereferencing macros for reference parameters
 */
#define	pw	(*pwp)
#define	gr	(*grp)
#define	ac	(*acp)
#define	acct	(*acctp)
#define	grl	(*grlp)

oklogin(name, group, acctp, password, pwp, grp, acp, grlp)
char *name;
char *group;
char **acctp;
char *password;
struct passwd **pwp;
struct group **grp;
struct account **acp;
int **grlp;
{
    register struct account *ap;
    register struct group *gp;
    register struct a *al;
    register struct a *alp;
    register struct g *gl;
    register struct g *glp;
    register char **p;
    time_t exptime = time(0);
    struct a *adef = NULL;
    struct g *gdef = NULL, *agdef = NULL;
    int *grplp;

    glp = glist = NULL;
    alp = alist = NULL;

    /* get password entry, return error on failure */
    if (pw == NULL)
	pw = getpwnam(name);
    if (pw == NULL)
	return(ACCESS_CODE_NOUSER);

    /* if there is a password and we want it checked, do so */
    if (*(pw->pw_passwd) && password != NULL) {
	if (strcmp(pw->pw_passwd, crypt(password, pw->pw_passwd)) != 0)
	    return(ACCESS_CODE_BADPASSWORD);
    }

    /* if there is an accounts file, read it */
    if (acctf = setacent()) {
	if (ap = getacent()) {
	    newacctf = (strcmp(ap->ac_name,"root") == 0 && ap->ac_uid == 0 &&
			ap->ac_aid == 1 && *ap->ac_attrs != NULL);
	    do {
		if (strcmp(ap->ac_name, name) != 0) {
		    if (alist == NULL) continue;
		    break;
		}
		if ((al = saveact(ap)) == NULL)
		    return(ACCESS_CODE_OOPS);
		if (alp == NULL)
		    alp = alist = al;
		else
		    alp = alp->a_next = al;
		if (al->a_default)
		    if (adef)
			return(freereturn(ACCESS_CODE_MANYDEFACC));
		    else
			adef = al;
	    } while (ap = getacent());
	} else
	    acctf = 0;
	endacent();
    }

    setgrent();
    while (gp = getgrent()) {
	for (p = gp->gr_mem; *p; p++)
	    if (strcmp(*p, name) == 0) {
		if ((gl = savegrp(gp)) == NULL)
		    return(ACCESS_CODE_OOPS);
		if (glp == NULL)
		    glp = glist = gl;
		else
		    glp = glp->g_next = gl;
		if (gl->g_gid == pw->pw_gid)
		    gdef = gl;
		if (adef && gl->g_gid == adef->a_aid)
		    agdef = gl;
		break;
	    }
    }
    endgrent();

    if (group) {
	for (gl = glist; gl; gl = gl->g_next)
	    if (strcmp(gl->g_name, group) == 0)
		break;
	if (gl == NULL)
	    return(freereturn(ACCESS_CODE_NOTGRPMEMB));
    } else if ((gl = gdef) == NULL)
	return(freereturn(ACCESS_CODE_NOTDEFMEMB));
    gr = restoregrp(gl->g_grp);
    if (acctf) {
	if (newacctf) {
	    if ((al = gl->g_act) == NULL)
		return(freereturn(ACCESS_CODE_NOACCFORGRP));
	    if (exptime > al->a_xtime)
		return(freereturn(ACCESS_CODE_GRPEXPIRED));
	}
	if (acct == NULL) {
	    if (!newacctf) {
		if ((al = gl->g_act) == NULL || exptime > al->a_xtime) {
		    glp = NULL;
		    for (gl = glist; gl; gl = gl->g_next) {
			if ((al = gl->g_act) == NULL)
			    continue;
			if (glp == NULL || al->a_xtime > glp->g_act->a_xtime)
			    glp = gl;
		    }
		    if ((gl = glp) == NULL)
			return(freereturn(ACCESS_CODE_NOACCFORGRP));
		}
		acct = strcpy(acctname,gl->g_name);
	    } else if (al->a_login || al->a_project)
		acct = strcpy(acctname,gl->g_name);
	    else if (adef) {
		if (agdef == NULL)
		    return(freereturn(ACCESS_CODE_NOGRPDEFACC));
		acct = strcpy(acctname,agdef->g_name);
	    } else if (group == NULL)
		return(freereturn(ACCESS_CODE_ACCNOTVALID));
	    else {
		if (gdef == NULL)
		    return(freereturn(ACCESS_CODE_NOTDEFMEMB));
		else if (gdef->g_act == NULL)
		    return(freereturn(ACCESS_CODE_NOACCFORGRP));
		else
		    acct = strcpy(acctname,gdef->g_name);
	    }
	}
	for (gl = glist; gl; gl = gl->g_next)
	    if (strcmp(gl->g_name, acct) == 0)
		break;
	if (gl == NULL)
	    return(freereturn(ACCESS_CODE_NOGRPFORACC));
	if ((al = gl->g_act) == NULL)
	    return(freereturn(ACCESS_CODE_NOACCFORGRP));
	if (exptime > al->a_xtime)
	    return(freereturn(ACCESS_CODE_ACCEXPIRED));
	if (newacctf && !al->a_login && !al->a_project)
	    return(freereturn(ACCESS_CODE_ACCNOTVALID));
	ac = restoreact(al->a_act);
    }

    /* generate group list from valid accounts */
    grl = grplist;
    grplp = &grplist[1];
    *grl = 0;

    /* first is requested or default group */
    *grplp++ = gr->gr_gid;
    (*grl)++;

    /* second is account, if different */
    if (acctf && ac->ac_aid != gr->gr_gid) {
	*grplp++ = ac->ac_aid;
	(*grl)++;
    }

    /* add any left that aren't expired */
    for (gl = glist; gl; gl = gl->g_next) {
	if (gl->g_gid == gr->gr_gid || *grl >= NGROUPS ||
	    (acctf && (gl->g_gid == ac->ac_aid ||
	     gl->g_act == NULL || exptime > gl->g_act->a_xtime)))
	    continue;
	*grplp++ = gl->g_gid;
	(*grl)++;
    }

    /* return ok */
    return(freereturn(ACCESS_CODE_OK));
}
