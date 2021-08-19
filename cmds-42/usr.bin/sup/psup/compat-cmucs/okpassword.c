/*
 * Routine to check for insecure passwords.  See okpassword(3sys) for
 * details.
 *
 **********************************************************************
 * HISTORY
 * 08-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created CMU CS 4.2 version.  Removed time check for initial
 *	installation.  Changed string.h to libc.h.  Made the substr()
 *	subroutine static.
 *
 * 28-Apr-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <stdio.h>
#include <libc.h>
#include <c.h>

#define	MAXPWDLEN	8
#define	MAXDICTLEN	128+1
#define	DICTFILE	"/etc/passwd.dict"

static char *errors[] = {
    "Your password is insecure because it is less than 6 characters",
    "Your password is insecure because it contains a part of your user id",
    "Your password is insecure because it contains a part of your name",
    "Your password is insecure because it is in the list of common passwords",
};

char *okpassword(pwd, user, name)
char *pwd, *user, *name;
{
    register char *p;
    int plen, mlp, len;
    char pwdbuf[MAXPWDLEN + 1], buffer[MAXDICTLEN];
    FILE *dict;

    if (pwd == NULL || *pwd == '\0')
	return(NULL);
    plen = strlen(pwd);
    if (plen < 6)
	return(errors[0]);
    if (plen > MAXPWDLEN)
	pwd[plen = MAXPWDLEN] = '\0';
    pwd = folddown(pwdbuf, pwd);
    user = folddown(buffer, user);
    if ((len = strlen(user)) > 2 && len <= plen && substr(user, pwd))
	return(errors[1]);
    mlp = (plen >= MAXPWDLEN);
    name = folddown(buffer, name);
    while (*(p = nxtarg(&name, NULL)) != '\0')
	if ((len = strlen(p)) > 2 &&
	    ((len <= plen && substr(p, pwd)) ||
	     (mlp && strncmp(p, pwd, plen) == 0)))
	    return(errors[2]);
    if ((dict = fopen(DICTFILE, "r")) == NULL)
	return(NULL);
    while (fgets(buffer, MAXDICTLEN, dict) != NULL) {
	if (buffer[0] == ';' || buffer[0] == '\n')
	    continue;
	buffer[len = (strlen(buffer) - 1)] = '\0';
	if (len >= plen &&
	    ((mlp) ? strncmp(buffer, pwd, plen) : strcmp(buffer, pwd)) == 0)
	    return(errors[3]);
    }
    fclose(dict);
    return(NULL);
}

static substr(sub, str)
register char *sub, *str;
{
    register sl, diff, i;

    sl = strlen(sub);
    diff = strlen(str) - sl;
    for (i = 0; i <= diff; str++, i++)
	if (strncmp(sub, str, sl) == 0)
	    return(TRUE);
    return(FALSE);
}
