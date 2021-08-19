/* New User Installation Program
 *
 *	Brian Reid, Erik Hedberg, Jeff Mogul, and Fred Yankowski
 *	Stanford University
 *
 *	This program helps system administrators manage login accounts.
 *	it reflects the way we do things at Stanford, but can probably
 *	be adapted to almost any situation.
 *
 *	It was originally written by Fred Yankowski as an MS project
 *	in 1980.  Several years of experience at using it showed us
 *	lots of things that we would like it to do differently; Erik
 *	Hedberg added many changes.  In summer 1984 Brian Reid tore it
 *	all apart, adapted it to 4.2BSD, modified it to use the C
 *	library as much as possible added error checking and
 *	command-line parsing, rewrote code that didn't amuse him, and
 *	changed the structure of it so that it read in a configuration
 *	file instead of having compile-time options. Jeff Mogul added
 *	the -d option.  Peter King at Stanford put in filling of holes
 *	in the password file.  The man page is entirely by Brian Reid.
 *	Peter King at NeXT adapted it for use on the NeXT Computer
 *	System and to deal with YP.
 *
 *	This program does not contain any Unix source code, nor is it
 *	copied from any. It is completely public domain, and you are
 *	free to use, distribute, modify, or sell it, make T-shirts out
 *	of it, or do whatever you want.
 *
 *	Note that to avoid using YP with getpwuid, getpwnam, getgrgid
 *	and getgrnam we load in the original 4.3BSD routines.  These
 *	routines are not public domain.
 *
 *	This program has been severly modified for the addition of
 *	accounts under netinfo in the NeXT environment.  Modifications
 *	were made by Lee Tucker.  This system is now tightly bound to 
 *      netinfo and will not work without it.   It is now also necessary
 * 	to replace the getpwuid, getpwnam, with new routines that access
 *	only the selected netinfo domain back to netinfo.  Locking of
 *      passwd files is no longer necessary as all edits occur immediately
 *	in NetInfo.
 */


#include <stdio.h>
#include <libc.h>
#include <ctype.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "nu.h"
#define  CONTROL_D '\003'


long	nsymbols;
char    This_host[100];			/* Holds result of gethostname() */

int     numtopnodes = 0;
struct topnode  topnode[MAXGROUPS];	/* to be filled in from nu.cf */

int     incritsect;			/* Set to true when the program is in
					   the section involved with updating
					   the files and creating new files
					   and, hence, should not be
					   interruptable by ^C. */

int     wasinterrupted;			/* Set to true if interrupt occurs
					   during a call to 'System' */

char    cmd[BUF_LINE];			/* Buffer to construct commands in */

char    editor[BUF_WORD];		/* Who is running this program */

FILE * logf;				/* File to log actions in */
int     whichmail;			/* record which kind of mail the user
					   wants */
struct stat statbuf;			/* for stat and lstat calls */

void *ncpy (char *to, char *from, int len)
{
    register int    i;
    for (i = 1; i <= len; i++)
	*to++ = *from++;
}

/* YesNo
 *	Gets either a yes or a no answer. "Yy\n" for yes, "Nn" for no.
 */
int YesNo (char defaultans)
{
    char    temp[BUF_WORD], ans;

    for (;;) {
	fgets (temp, BUF_WORD, stdin);
	MapLowerCase (temp);
	ans = temp[0];
	if (ans == '\n')
	    ans = defaultans;
	if (ans == 'y')
	    return (TRUE);
	if (ans == 'n')
	    return (FALSE);
	printf ("Answer y or n. [return means %c] ", defaultans);
    }
}

/* UseDefault
 *	returns TRUE if NULL entered (meaning use default) or
 *	FALSE after reading text to use in place of default
 */
int UseDefault (char *str, char *def)
{
    char    temp[BUF_WORD];
    printf (" [%s] ", def);
    gets (temp);
    if (strlen(temp) == 0) {
	if (str != def)
	    strcpy (str, def);
	return (TRUE);
    }
    else {
	strcpy (str, temp);
	return (FALSE);
    }
}

/* GetUserID
 *	returns the userid for the new user.  The routine scans the
 *	password file to find the highest userid so far assigned.
 *	The new userid is then set to be the one more than this value.
 *	This calculated default can be overridden, but the password
 *	file is searched to insure that the chosen userid will be
 *	unique.
 */
int GetUserID () 
{
    int     maxuid, newuid;
    char    defaultID[BUF_WORD], buf[BUF_WORD];
    struct passwd* pwd;

    if (IntV("PWFillHoles")) {
	    /* Scan the passwd file for the first userid above PWUserBase */
	    for (newuid = IntV("PWUserBase"); mygetpwuid(newuid) != NULL;
		 newuid++)
		    ;
    } else {
	    /* scan the passwd file for the highest userid */
	    maxuid = 0;
	    newuid = getMaxUid();
    }
    sprintf (defaultID, "%d", newuid);

    for (;;) {
	printf ("User id number? (small integer) ");
	UseDefault (buf, defaultID);
	newuid = atoi (buf);
	if (newuid <= 0) {
	    printf ("Userid must be >0, and should be >10.\n");
	    continue;
	}
	if ((pwd = mygetpwuid (newuid)) == NULL)
	    return (newuid);
	else
	    printf ("User id %d is already assigned to %s (%s)\n",
		    pwd->pw_uid, pwd->pw_name, pwd->pw_gecos);
    }
}

/* GetGroup
 *	writes the group name in grpname and returns the numeric gid.
 *	A groupid can be entered as a number or the name for a
 *	group (as defined in /etc/group).  The getgr-- functions are
 *	used to peruse the group file.  Legal symbolic names are
 *	mapped into the corresponding groupid.  Input numeric
 *	groupid's are mapped into the corresponding symbolic name
 *	(if such exists) for verification.
 */
int GetGroupID (char *group)
{
    struct group   *agrp;
    int     gid;
    char    buf[BUF_WORD], def[BUF_WORD];

 /* Get group name for IntV("DefaultGroup") */
    if (agrp = mygetgrgid (IntV ("DefaultGroup")))
	strcpy (def, agrp->gr_name);
    else
	strcpy (def, "unknown");

    for (;;) {
	printf ("Which user group? (name or number; ? for list)");
	UseDefault (buf, def);
	if ((strcmp (buf, "?") == 0 || (strcmp (buf, "help") == 0))) {
	    char    hbuf[BUF_LINE], *hptr;
	    int     gcount;
	    hptr = hbuf;
	    gcount = 0;
	    sprintf (hptr, "Available groups are:");
	    listGroups();
	    if (gcount != 0)
		puts(hptr);
	    continue;
	}

	if (isdigit (*buf)) {
    /* presumably, a numeric groupid has been entered */
	    gid = atoi (buf);
	    if (agrp = mygetgrgid (gid))
		strcpy (buf, agrp->gr_name);
	    else
		strcpy (buf, "unknown");
	    printf ("Selected groupid is %d (%s), OK? (y or n) [y] ", gid, buf);
	    if (YesNo ('y')) {
		strcpy (group, buf);
		return (gid);
	    }
	}
	else {
    /* some symbolic group name has been entered */
	    if (agrp = mygetgrnam (buf)) {
		strcpy (group, agrp->gr_name);
		return (agrp->gr_gid);
	    }
	    else
		printf ("Sorry, %s is not a registered group name.\nIf you want a list of groups, type '?'\n", buf);
	}
    }
}

/* MapLowerCase
 *	maps the given string into lower-case.
 */
void *MapLowerCase (char *b)
{
    while (*b) {if (isupper (*b)) *b++ += 'a'-'A'; else b++;}
}

/* MapUpperCase
 *	maps the given string into lower-case.
 */
void *MapUpperCase (char *b)
{
    while (*b) {if (islower (*b)) *b++ -= 'a'-'A'; else b++;}
}

int HasBadChars (char *b)
{
    while (*b) {
	if ((NOT isalpha (*b)) && (NOT isdigit (*b)) && (*b != '_'))
	    return (TRUE);
	b++;
    }
    return (FALSE);
}

/* GetLoginName
 * 	Prompts for a login name and checks to make sure it is legal.
 *	The login name is returned, null-terminated, in "buf".
 */
void *GetLoginName (char *buf)
{
    int     done = FALSE, i, j;
    char   *aptr;
    datum aliaskey, aliasname;
    struct passwd* pwd;

    while (NOT done) {
	printf ("Login name? (1-8 alphanumerics) [no default] ");
	gets (buf);
	if (strlen(buf) == 0)
	    continue;

	MapLowerCase (buf);
	if (HasBadChars (buf)) {
	    printf ("Sorry, the login name can contain only alphanumerics or '_'\n");
	    continue;
	}

	if (strlen (buf) > IntV ("MaxNameLength")) {
	    printf ("Sorry, login names must not contain more than ");
	    printf ("%d characters\n", IntV ("MaxNameLength"));
	    buf[IntV ("MaxNameLength")] = 0;
	    printf ("Should it be truncated to '%s'? ", buf);
	    if (NOT YesNo ('y'))
		continue;		/* start over again */
	}

 /* check to see that the login is unique */
	if (pwd = mygetpwnam (buf)) {
	    printf ("Sorry, the login '%s' is already in use (id %d, %s).\n",
		    buf, pwd->pw_uid, pwd->pw_gecos);
	    continue;
	}
	done = TRUE;
 /* check to make sure that the login does not conflict with an alias. This
    whole section of code is ridiculous overkill, but I was in the mood
    (BKR). */
    }
}

/* GetPassword
 *	read password (null is still allowed)
 */
char   *GetPassword (ascpw, cryptpw)
char   *ascpw, *cryptpw;
{
    char    saltc[2], c;
    long    salt;
    register int    i;
    char    pw1[40], pw2[40];

    printf("\n%s\n%s\n%s\n\n",
	   "Passwords are very important to system security.  When you are",
	   "entering a new password, make sure that it is at least 6 characters",
	   "long and that it contains at least one character that is not a letter.");
    do {
	strcpy (pw1, getpass ("Enter password: "));
	strcpy (pw2, getpass ("Retype password, please: "));
	if (!strcmp (pw1, pw2))
	    break;
	printf ("They don't match. Please try again.\n");
    } while (TRUE);

    strcpy (ascpw, pw1);
    if (strlen (ascpw)) {
	time (&salt);
	salt += getpid ();
	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
	    c = saltc[i] + '.';
	    if (c > '9')
		c += 7;
	    if (c > 'Z')
		c += 6;
	    saltc[i] = c;
	}
	strcpy (cryptpw, (crypt (ascpw, saltc)));
    }
    else
	strcpy (cryptpw, "");
}

/* GetRealName
 *	read the new user's actual name.
 */
GetRealName (buf)
char   *buf;
{
    int     done;
    done = FALSE;
    while (NOT done) {
	printf ("Enter actual user name: ");
	gets (buf);
	if (index (buf, ':'))
	    printf ("Sorry, the name must not contain ':'\n");
	else if (index (buf, ','))
	    printf ("Sorry, the name must not contain ','\n");
	else
	    done = TRUE;
    }
}

/* GetLoginDir
 *	Writes the new user's login directory in the cpw_dir field.
 */
int GetLoginDir (struct cpasswd *np)
{
    int     DontClobberDir;
    register int    i, done;
    char    defdir[BUF_WORD];		/* default login directory */

    strcpy (defdir, StrV ("DefaultHome"));
    strcat (defdir, "/");

    for (i = 0; topnode[i].gid; i++) {
	if (np->cpw_gid == topnode[i].gid) {
	    strcpy (defdir, topnode[i].topnodename);
	    strcat (defdir, "/");
	    break;
	}
    }
    strcat (defdir, np->cpw_name);

    done = FALSE;
    while (NOT done) {
	printf ("Login directory? ");
	UseDefault (np->cpw_dir, defdir);
	if (index (np->cpw_dir, ':')) {
	    printf ("Sorry, the name must not contain ':'\n");
	    continue;
	}
	DontClobberDir = 0;
	if (stat (np->cpw_dir, &statbuf) == 0) {
	    printf ("%s already exists. Do you want to clobber it? (y or n) [n] ", np->cpw_dir);
	    if (YesNo ('n'))
		break;
	    printf ("Do you want to use %s, but not touch its contents? (y or n) [y] ", np->cpw_dir);
	    if (NOT YesNo ('y'))
		continue;
	    DontClobberDir = 1;
	    break;
	}
	break;
    }
    return (DontClobberDir);
}

/* GetLoginSH
 *	returns the new user's login shell directory.  The default,
 *	which may be overridden, is StrV("DefaultShell").
 */
void *GetLoginSH (char *buf)
{
    FILE * shellf;
    int     done;

    done = FALSE;
    while (NOT done) {
	printf ("Enter shell");
	UseDefault (buf, StrV ("DefaultShell"));
	shellf = fopen (buf, "r");
	if (shellf == NULL) {
	    printf ("I trust you realize that there is no such file.\n");
	    printf ("Is that ok? (y or n) [y] ");
	    if (NOT YesNo ('y'))
		continue;
	}
	if (index (buf, ':'))
	    printf ("Sorry, shell file name must not contain ':'\n");
	else
	    done = TRUE;
	fclose(shellf);
    }
}


/* System
 *	
 *	Like the regular "system", but does the right thing with ^C
 */
int System (char *cmdstring)
{
    int     status, pid, waitstat;

    if (IntV("Debug"))
    	printf("System(%s)\n", cmdstring);
    wasinterrupted = FALSE;

    fflush(stdout);
    fflush(stderr);

    if (!(pid = vfork ())) {
	execl ("/bin/sh", "sh", "-c", cmdstring, 0);
	_exit (127);
    }
    while ((waitstat = wait (&status)) != pid && waitstat != -1);
    if (waitstat == -1)
	status = -1;
    return (status);
}
/* CallSys
 *	repeats a System call until the call returns without error,
 *	or until it returns with an error but without the flag being
 *	set indicating that an interrupt occurred during the call.
 *	This flag, 'wasinterrupted', is set FALSE just before a
 *	System call, and is set to TRUE only if the Catch routine is
 *	called during the critical (uninterruptable) section of the
 *	program.
 */
int CallSys (char *cmd)
{
    register int    status;

    while (status = System (cmd))
	if (NOT wasinterrupted)		/* regular system error */
	    return (status);		/* system call bombed */
    return (status);			/* executed without problem */
}

/* DoCommand
 *	calls 'Callsys' to execute the string 'cmd', and supplies
 *	some messages.  Exits if 'fatal' is TRUE.
 */
void *DoCommand (char *cmd, int fatal, int safe)
{
    register int    status;

    if (IntV ("Debug"))
	printf ("%s", cmd);
    if (IntV ("Debug") == 0 || safe) {
	if (status = CallSys (cmd)) {
	    printf ("nu: '%s' failed, status %d\n", cmd, status);
	    perror ("nu");
	    if (fatal) {
		exit (ERROR);
	    }
	}
    }
    else
	printf ("Unsafe command, skipped in Debug mode.\n");
}

/* AddToPasswd
 *	takes the userrect holding the new entry for the passwd file, and
 *	inserts this into NetInfo.
 */
void *AddToPasswd (struct cpasswd *userrec, int caps, char *argv[])
{
    ni_proplist fullUser;
    ni_property	workingProperty;
    ni_id 	newChild;
    ni_status	whatHappened;
    char	num2String[7];

    NI_INIT(&fullUser);
    NI_INIT(&workingProperty);
    workingProperty.nip_name = "name";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_name, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "passwd";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_passwd, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "uid";
    sprintf(num2String, "%d", userrec->cpw_uid);
    ni_namelist_insert(&workingProperty.nip_val, num2String, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "gid";
    sprintf(num2String, "%d", userrec->cpw_gid);
    ni_namelist_insert(&workingProperty.nip_val, num2String, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "realname";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_person, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "home";
	if (IntV ("WantSymbolicLinks") != 0) {
		strcpy (userrec->cpw_linkdir, StrV ("SymbolicLinkDir"));
		strcat (userrec->cpw_linkdir, "/");
		strcat (userrec->cpw_linkdir, userrec->cpw_name);
	}
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_dir, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "shell";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_shell, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "_writers_passwd";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_name, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    whatHappened = ni_create(niHandle, &userDir, fullUser, &newChild, NI_INDEX_NULL);
    /* here is where we build the proplist and ni_write it*/
    if (whatHappened != NI_OK) {
	printf ("Unable to write account to NetInfo.\n");
	exit (ERROR);
    }
    else {
	printf ("Account added to NetInfo.\n");
    }

}

/* ReplaceInPasswd
 *	takes the userrect holding the new entry for the passwd file, and
 *	replaces this into NetInfo.
 */
ni_status ReplaceInPasswd (struct cpasswd *userrec, ni_id whatDir)
{
    ni_proplist fullUser;
    ni_property	workingProperty;
    ni_status	whatHappened;
    char	num2String[7];

    NI_INIT(&fullUser);
    NI_INIT(&workingProperty);
    workingProperty.nip_name = "name";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_name, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "passwd";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_passwd, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "uid";
    sprintf(num2String, "%d", userrec->cpw_uid);
    ni_namelist_insert(&workingProperty.nip_val, num2String, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "gid";
    sprintf(num2String, "%d", userrec->cpw_gid);
    ni_namelist_insert(&workingProperty.nip_val, num2String, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "realname";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_person, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "home";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_dir, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "shell";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_shell, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    workingProperty.nip_name = "_writers_passwd";
    ni_namelist_insert(&workingProperty.nip_val, userrec->cpw_name, NI_INDEX_NULL);
    ni_proplist_insert(&fullUser, workingProperty, NI_INDEX_NULL);
    ni_namelist_free(&workingProperty.nip_val);
    whatHappened = ni_write(niHandle, &whatDir, fullUser);
    return (whatHappened);

}

/* LogAddition
 *	creates an entry for a file that holds information about
 *	newuser's, corresponding to the information in the passwd
 *	file.  This file is used only informally, to keep track
 *	recent additions.
 */
void *LogAddition (char *buf)
{
    FILE * logf;
    long    clock;

    if (logf = fopen (StrV ("Logfile"), "a")) {
	clock = time (0);
	fprintf (logf, "%s\tadded by %s on %s\n", buf, editor, ctime (&clock));
	fclose (logf);
    }
    else
	fprintf (stderr, "Can't make log file entry\n");
}

/* CreateDir
 *	calls a shell script that creates a new directory, to be
 *	the new user's login dir.  This new user becomes the owner
 *	of the directory.
 */
void *CreateDir (struct cpasswd *np, int clobber)
{
    sprintf (cmd, "%s %d %d %s %s %d %d %s\n",
	    StrV ("CreateDir"),
	    np->cpw_uid,
	    np->cpw_gid,
	    np->cpw_dir,
	    np->cpw_linkdir,
	    clobber,
	    IntV ("Debug"),
		np->cpw_name
	);
    DoCommand (cmd, FATAL, SAFE);
}

/* InstallFiles
 *	Call the shell script that puts files in the new directory
 *	and makes sure they have the right ownership.
 */
void *InstallFiles (struct cpasswd *np)
{
    sprintf (cmd, "%s %s %d %d %d %d\n",
	    StrV ("CreateFiles"),
	    np->cpw_dir,
	    np->cpw_uid,
	    np->cpw_gid,
	    (whichmail == 'm') && IntV ("WantMHsetup"),
	    IntV ("Debug")
	);
    DoCommand (cmd, NONFATAL, SAFE);

}
/* PwPrint
 *	Print the fields of a passwd structure.
 */
void *PwPrint (struct cpasswd *cpw)
{
    printf ("   1)  login ...... %s\n", cpw->cpw_name);
    printf ("   2)  password ... ");
    if (cpw->cpw_passwd[0])
	printf ("%s (encrypted)\n", cpw->cpw_passwd);
    else
	printf ("(none)\n");
    printf ("   3)  name ....... %s\n", cpw->cpw_person);
    printf ("   4)  userid ..... %d\n", cpw->cpw_uid);
    printf ("   5)  groupid .... %d (%s)\n", cpw->cpw_gid, cpw->cpw_group);
    printf ("   6)  login dir .. %s\n", cpw->cpw_dir);
    printf ("   7)  login sh ... %s\n", cpw->cpw_shell[0] ? cpw->cpw_shell
	    : "/bin/sh (null default)");
}

/* Verified
 *	print the current account data, ask if it is Ok, and return
 *	TRUE if it is Ok, false otherwise.	
 */
int Verified (struct cpasswd *np)
{
/*
 *  Clear all waiting input and output chars.
 *  Actually we just want to clear any waiting input chars so
 *  we have a chance to see the values before confirming them.
 *  We have to sleep a second to let waiting output chars print.
 */
    sleep (1);
    ioctl (0, TIOCFLUSH, 0);

    PwPrint (np);

    printf ("Are these values OK? (y or n)  [y] ");
    fflush (stdout);
    if (YesNo ('y'))
	return (TRUE);
    else {
	printf ("Do you want to continue? (y or n) [y] ");
	if (YesNo ('y'))
	    return (FALSE);
	else {
	    exit (OK);
	}
    }
}


/* Catch
 *	is called whenever ^C is typed at the terminal.  If the
 *	critical section flag is set, the program should not be
 *	aborted, and so the routine just returns.  If the flag is not
 *	set, the terminate routine is called to halt the program.
 */
int Catch () 
{
    signal (SIGINT, SIG_IGN);		/* ignore ^C's for a short time */

    if (incritsect) {
	printf ("\nSorry, 'nu' is in a critical section and should not terminate here.\n");
	printf ("Please try again later. The safest thing to do is to let it finish,\n");
	printf ("then go back and kill the new account with 'nu -k name'.\n");
	wasinterrupted = TRUE;
	signal (SIGINT, Catch);
 /* further ^C's trapped to 'Catch()' */
	return (TRUE);
    }
    else {
	printf ("\nProgram aborted.\n");
	exit (ERROR);
    }
}

/* Additions
 *	This is the driver routine for adding new users. It is called from
 *	main() if the "-a" option is found.
 */
void *Additions (int allCaps, char *argv[])
{
    int     done, noclobber, i;
    char    access[BUF_WORD], buffer[BUF_LINE];
    struct cpasswd  new;
    long	salt;
    char    saltc[2], c;
    ni_id   tempID;

    done = FALSE;	/* becomes TRUE when the user is satisfied with */
    while (NOT done) {	/*   the data, as it has been set up. */

	if (allCaps == TRUE) {  /* Perform from a single command line instead of querying for everything. */
	    /* don't error check much, count on the app to do this for the most part */
	    strncpy(new.cpw_name, argv[2], BUF_WORD);
	    strncpy(new.cpw_person, argv[3], BUF_WORD);
	    new.cpw_uid = atoi(argv[4]);
	    new.cpw_gid = atoi(argv[5]);
	    strncpy(new.cpw_shell, argv[6], BUF_WORD);
	    strncpy(new.cpw_asciipw, argv[7], BUF_WORD);
	    strncpy(new.cpw_dir, argv[8], BUF_WORD);
	    if (argv[9][0] == 'Y')
		noclobber = TRUE;
	    else {
		noclobber = FALSE;
		if (argv[10][0] = 'Y')
		    whichmail = 'm';
		else
		    whichmail = 'u';
	    }
	    /* Note, no null password allowed in the app */
	    time (&salt);
	    salt += getpid ();
	    saltc[0] = salt & 077;
	    saltc[1] = (salt >> 6) & 077;
	    for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
		    c += 7;
		if (c > 'Z')
		    c += 6;
		saltc[i] = c;
	    }
	    strcpy (new.cpw_passwd, (crypt (new.cpw_asciipw, saltc)));
	}
	else {
	    GetLoginName (new.cpw_name);
	    if (aliasDir.nii_object != userDir.nii_object) {
		sprintf(buffer, "/aliases/%s", new.cpw_name);
		if (ni_pathsearch(niHandle, &tempID, buffer) == NI_OK) {
		    printf("The user name you are attempting to use, matches an already exiting alias.\nPlease remove the alias, or choose another name.\n");
		    return;
		}
	    }
	    GetPassword (new.cpw_asciipw, new.cpw_passwd);
	    GetRealName (new.cpw_person);
	    new.cpw_uid = GetUserID ();
	    new.cpw_gid = GetGroupID (new.cpw_group);
	    noclobber = GetLoginDir (&new);
	    GetLoginSH (new.cpw_shell);
	    if (noclobber == 0 
		&& IntV("WantMHsetup")) {
		printf ("Do you want an initialized ~/Mail for MH? (y or n) [y] ");
		if (YesNo ('y'))
		    whichmail = 'm';
		else
		    whichmail = 'u';
	    }
	}
	if (IntV ("WantSymbolicLinks") != 0) {
	    strcpy (new.cpw_linkdir, StrV ("SymbolicLinkDir"));
	    strcat (new.cpw_linkdir, "/");
	    strcat (new.cpw_linkdir, new.cpw_name);
	}
	else
	    strcpy (new.cpw_linkdir, new.cpw_dir);

	if (aliasDir.nii_object != userDir.nii_object) {
	    sprintf(buffer, "/aliases/%s", new.cpw_name);
	    if (ni_pathsearch(niHandle, &tempID, buffer) == NI_OK) {
		printf("The user name you are attempting to use, matches an already exiting alias.\nPlease remove the alias, or choose another name");
		return;
	    }
	}
	else;
	if ((allCaps == TRUE) || (Verified (&new))) {
	    incritsect = TRUE;		/* should not be interrupted */
	    AddToPasswd (&new, allCaps, argv);
	    if (noclobber == TRUE) {
		CreateDir (&new, 0);
	    }
	    else {
		CreateDir (&new, 1);
		InstallFiles (&new);
	    }
	    sprintf (buffer, "%s:%s:%d:%d:%s:%s:%s\n",
		    new.cpw_name, new.cpw_passwd, new.cpw_uid, new.cpw_gid,
		    new.cpw_person, new.cpw_linkdir, new.cpw_shell);
	    LogAddition (buffer);
	    incritsect = FALSE;

	    if (allCaps == FALSE) {
		printf ("\nDo you wish to add more new users? (y or n)  [y] ");
		done = NOT YesNo ('y');
	    }
	    else
		done = TRUE;
	}
    }
}


/* Xfer
 *	copies the values from a 'passwd' structure (which
 *	is static in a system 'mygetpw---' routine) into a
 *	'cpasswd' structure.  This is done so that multiple
 *	'passwd' entries can be saved.
 */
void *Xfer(struct passwd *pwd, struct cpasswd *cpw)
{
    struct group   *agrp;

    strcpy (cpw->cpw_name, pwd->pw_name);
    strcpy (cpw->cpw_passwd, pwd->pw_passwd);
    cpw->cpw_asciipw[0] = 0;
    cpw->cpw_uid = pwd->pw_uid;
    cpw->cpw_gid = pwd->pw_gid;
    if (agrp = mygetgrgid (pwd->pw_gid))
	strcpy (cpw->cpw_group, agrp->gr_name);
    else
	strcpy (cpw->cpw_group, "unknown");
    strcpy (cpw->cpw_person, pwd->pw_gecos);
    strcpy (cpw->cpw_dir, pwd->pw_dir);
    strcpy (cpw->cpw_shell, pwd->pw_shell);
}


/* PromptForID
 *	queries the user interactively for the identifier of
 *	an entry in /etc/passwd.  If the ID is numeric, it
 *	is assumed to be the userid; otherwise, it is assumed
 *	to be the login.  A pointer to a structure holding
 *	the 'passwd' entry is returned.  The routine will not
 *	terminate until a valid entry is found.
 */
struct passwd  *PromptForID () {
    char    resp[BUF_WORD];
    int     theuid, done;
    struct passwd  *pwd;

    done = FALSE;
    while (NOT done) {
	strcpy(resp, "");
	printf ("\nEnter user identifier (login or uid): ");
	gets(resp);
	if ((resp == NULL) || (strlen(resp) == 0) /*|| freof(stdin) */) {
	    /* User hit a control-d or carriage return time to quit */
	    printf ("\n");
	    exit(OK);
	}
	if (isdigit (*resp)) {
    /* presumably, a uid has been entered */
	    theuid = atoi (resp);
    /* search passwd for entry with uid = theuid */
	    if ((pwd = mygetpwuid (theuid)) == NULL)
		printf ("Sorry, that uid is not in use\n");
	    else
		done = TRUE;
	}
	else {
    /* if the entry is sensible, it is a login-name */
	    if ((pwd = mygetpwnam (resp)) == NULL)
		printf ("Sorry, that login is not in use\n");
	    else
		done = TRUE;
	}
    }
    return (pwd);
}

/* GetMod
 *	asks user to identify a particular passwd entry, prints
 *	the entry, prompts for changes to the entry, and leaves
 *	'ent' pointing to an appropriately modified copy of the
 *	entry.
 */
void *GetMod (int allCaps, int uid, char *argv[])
{
    struct passwd  *pwd;		/* struct used by system 'getpw--'
					   routines */
    struct cpasswd *ent;
    char    reply[BUF_WORD], saltc[2], niBuf[BUF_LINE];
    long	salt;
    ni_id   tempID;
    ni_status  whatHappened;
    int     morechgs = TRUE,
            needID = TRUE,
	    i, c;
	    
    ent = (struct cpasswd *)malloc(sizeof(struct cpasswd));
    if (allCaps == TRUE) {
	pwd = mygetpwuid(uid);
	Xfer (pwd, ent);		/* Copy to local storage */
	strcpy(ent->cpw_name, argv[3]);
	strcpy(ent->cpw_asciipw, argv[4]);
	if (strlen (ent->cpw_asciipw)) {
	    time (&salt);
	    salt += getpid ();
	    saltc[0] = salt & 077;
	    saltc[1] = (salt >> 6) & 077;
	    for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
		    c += 7;
		if (c > 'Z')
		    c += 6;
		saltc[i] = c;
	    }
	    strcpy (ent->cpw_passwd, (crypt (ent->cpw_asciipw, saltc)));
	}
	else
	    strcpy (ent->cpw_passwd, "");
	ent->cpw_uid = atoi(argv[5]);
	ent->cpw_gid = atoi(argv[6]);
	strcpy(ent->cpw_person, argv[7]);
	strcpy(ent->cpw_dir, argv[8]);
	strcpy(ent->cpw_shell, argv[9]);
    }
    else
    while (morechgs) {
	if (needID) {
	    pwd = PromptForID ();
	    Xfer (pwd, ent);		/* Copy to local storage */
	    needID = FALSE;
	}
	printf ("Entry is now:\n");
	PwPrint (ent);
	printf ("\nSelect field to be modified ");
	printf ("(1-7, q (discard changes), or e (make changes): ");
	gets (reply);
	switch (*reply) {
	    case '1': 			/* get new login */
		GetLoginName (ent->cpw_name);
		break;
	    case '2': 			/* get new password */
		GetPassword (ent->cpw_asciipw, ent->cpw_passwd);
		break;
	    case '3': 			/* get new name */
		GetRealName (ent->cpw_person);
		break;
	    case '5': 			/* get groupid */
		ent->cpw_gid = GetGroupID (ent->cpw_group);
		break;
	    case '6': 			/* get login directory */
		GetLoginDir (ent);
		break;
	    case '7': 			/* get login shell */
		GetLoginSH (ent->cpw_shell);
		break;
	    case 'q': 			/* another entry */
		needID = TRUE;
		free(ent);
		return;
	    case 'e': 			/* done */
		morechgs = FALSE;
		continue;		/* fall out of while loop */
	    default: 
		printf ("Sorry, invalid selection: %s\n", reply);
		continue;
	}
    }
    sprintf (niBuf, "/users/name=%s", pwd->pw_name);
    whatHappened = ni_pathsearch(niHandle, &tempID, niBuf);
    if (whatHappened != NI_OK) {
	printf("Unable to find user being modified in NetInfo. ");
	exit(ERROR);
    }
    whatHappened =  ReplaceInPasswd (ent, tempID);
    if (whatHappened != NI_OK) {
	printf("Unable modify user. ");
	exit(ERROR);
    }
    free(ent);
}

/* Modify
 *	is the main routine when NU is called in modify mode.  It
 *	prompts the user for modifications to the passwd file;
 *	stacks up to MAXMODS of these modified entries; sorts the
 *	stack by userid; and merges the stack with the current
 *	passwd file, creating the updated version of the file.
 */
void *Modify (int allCaps, int uid, char *argv[]) 
{
    struct cpasswd  mods[MAXMODS + 1];
    int     done;

    if (allCaps == FALSE) {
	done = FALSE;
	printf ("\n\t\t>>> Modify mode <<<\n");
	while (NOT done) {
	    GetMod (allCaps, uid, argv);
	    printf ("\nDo you want to modify any more /etc/passwd entries? (y or n) [y] ");
	    done = NOT YesNo ('y');
	}
	printf ("done.\n");
    }
    else
	GetMod (allCaps, uid, argv);

}

/*
 * Kill various accounts
 */
void *KillUser (int where, char *argv[])
{
    struct passwd  *pwd;
    char    pathbuf[BUF_LINE], logindir[BUF_LINE];
    int     cc;
    register int    i;
    ni_id  myDir;
    ni_status whatHappened;

    pwd = mygetpwnam(argv[where]);
    if (pwd != NULL) {
	cc = readlink (pwd->pw_dir, logindir, BUF_LINE);
	if (cc == -1)
	    strcpy (logindir, pwd->pw_dir);
	else
	    logindir[cc] = 0;
	
	sprintf (cmd, "%s %s %s %s %s %d\n",
		StrV ("DestroyAccts"),
		pwd->pw_name,
		logindir,
		pwd->pw_dir,
		StrV ("Logfile"),
		IntV ("Debug")
	    );
	DoCommand (cmd, FATAL, SAFE);
	sprintf(pathbuf, "/users/name=%s", argv[where]);
	printf("Removing /users/%s from NetInfo domain.\n", pwd->pw_name);
	whatHappened = ni_pathsearch(niHandle, &myDir, pathbuf);
	if (whatHappened != NI_OK) {
	    printf("Unable to find user being deleted in NetInfo. ");
	    exit(ERROR);
	}
	whatHappened = ni_destroy(niHandle, &userDir, myDir);
	if (whatHappened != NI_OK) {
	    printf("Unable delete user. ");
	    exit(ERROR);
	}
	if (findUserInHmmm(pwd->pw_name, "groups", "users", groupDir) == TRUE)
	    printf("User %s still exists in the groups database.\nYou must delete all references to this user by hand.\n",
		pwd->pw_name);
	if ((aliasDir.nii_object != userDir.nii_object) && (findUserInHmmm(pwd->pw_name, "aliases", "members", aliasDir) == TRUE))
	    printf("User %s still exists in the aliases database.\nYou must delete all references to this user by hand.\n",
		pwd->pw_name);
    }
    else
	printf ("nu: no such user as %s\n", argv[where]);
    }

/*
 * Delete accounts: Do what KillUser() does but don't erase
 * /etc/passwd entries, so accounting information is available.
 * Also, work interactively; structurally similar to Modify(),
 * except that it doesn't postpone updates, so it can bomb out
 * if an error occurs.
 */
void *DeleteAccounts(int allCaps, char *argv[])
{
    struct cpasswd  del[2];
    int     done;


    if (allCaps == TRUE) {
	(GetDel (del, del, 0, allCaps, atoi(argv[2])));
    }
    else {
	printf ("\n\t\t>>> Deletion mode <<<\n");
	done = FALSE;
	while (NOT done) {
	    if (GetDel (del, del, 0, allCaps, -2)) {
		printf ("done.\n");
	    }
	    printf("\nDo you want to delete any more users? (y or n) [y] ");
	    done = NOT YesNo ('y');
	}
    }
}

/* GetDel
 *	asks user to identify a particular passwd entry, prints
 *	the entry, prompts to check if entry should be deleted,
 *	modifies the password field if so, and leaves
 *	'ent' pointing to an appropriately modified copy of the
 *	entry.  Returns false if no deletion is wanted
 */
int GetDel (struct cpasswd *ent, struct cpasswd *stack, int size, int allCaps, int theuid)
{
    struct passwd  *pwd;		/* struct used by system 'getpw--'
				       routines */
    char logindir[BUF_LINE];
    char	delcmd[BUF_LINE], niBuf[BUF_LINE];
    int cc;
    ni_id	tempID;
    ni_status	whatHappened;

    if (allCaps == TRUE) {
	pwd = mygetpwuid(theuid);
	Xfer (pwd, ent);		/* Copy to local storage */
    }
    else {
	pwd = PromptForID ();
	printf ("Entry is now:\n");
	Xfer (pwd, ent);		/* Copy to local storage */
	PwPrint (ent);
	printf ("\nDo you want to delete this entry? (y or n) [y] ");
	if (NOT YesNo('y')) {
	    return(FALSE);	/* no deletion */
	}
    }
    if (ent->cpw_passwd[0] == '*') {
	printf("\nThis account is already disabled.\n");
	return(0);
    }
    cc = readlink(ent->cpw_dir, logindir, BUF_LINE);
    if (cc == -1)
	strcpy(logindir, ent->cpw_dir);
    else
	logindir[cc] = 0;
    sprintf(delcmd, "%s %s %s %s %s %d\n",
	    StrV("DeleteAccts"),
	    ent->cpw_name,
	    logindir,
	    ent->cpw_dir,
	    StrV("Logfile"),
	    IntV("Debug")
	    );
    DoCommand(delcmd, FATAL, SAFE);
    strcpy(ent->cpw_asciipw, "[untypeable password]");
    strcpy(ent->cpw_passwd, "*");	/* cannot match a typed password */
    strcpy(ent->cpw_shell, "/bin/noshell");
    strcpy(ent->cpw_dir, "/nosuchdir");
    sprintf (niBuf, "/users/name=%s", ent->cpw_name);
    whatHappened = ni_pathsearch(niHandle, &tempID, niBuf);
    if (whatHappened != NI_OK) {
	printf("Unable to find user being disabled in NetInfo. ");
	exit(ERROR);
    }
    whatHappened =  ReplaceInPasswd (ent, tempID);
    if (whatHappened != NI_OK) {
	printf("Unable disable user. ");
	exit(ERROR);
    if (findUserInHmmm(pwd->pw_name, "groups", "users", groupDir) == TRUE)
	printf("User %s still exists in the groups database.\nYou must delete all references to this user by hand.\n",
	    pwd->pw_name);
    if ((aliasDir.nii_object != userDir.nii_object) && (findUserInHmmm(pwd->pw_name, "aliases", "members", aliasDir) == TRUE))
	printf("User %s still exists in the aliases database.\nYou must delete all references to this user by hand.\n",
	    pwd->pw_name);
    }
}

/* 
  This procedure reads in the nu.cf configuration file and uses its contents
  to initialize various tables and configuration variables.
*/
void *ReadCf () {
    FILE * cfile;
    char    lbuf[BUF_LINE];
    char    c, *cp, *op, *name, *sv;
    long    iv;
    int     i, istat;
    int     DefaultGroupHasHome = 0;
    if ((cfile = fopen (CONFIGFILE, "r")) == NULL) {
	fprintf (stderr, "nu: Unable to open configuration file \"%s\".\n",
		CONFIGFILE);
	exit (ERROR);
    }
    while (fgets (cp = lbuf, BUF_LINE, cfile) != NULL) {
	sv = name = (char *) 0;
	iv = 0;
	op = (char *) 0;
	for (cp = lbuf; *cp != 0; cp++) {
	    switch (*cp) {
		case '\t': 
		case ' ': 
		    *cp = 0;
		    continue;
		case ';': 
		    goto exitforloop;
		case '=': 
		    name = (char *) malloc (cp - op + 2);
		    *cp = 0;
		    strcpy (name, op);
		    cp++;
		    while (*cp == ' ' || *cp == '\t')
			cp++;
		    if (strcmp (name, "GroupHome") == 0) {
			iv = atol (cp);
			for (; *cp != '"' && *cp != 0; cp++);
			cp++;
			for (op = cp; *cp != '"' && *cp != 0; cp++);
			sv = (char *) malloc (cp - op + 2);
			*cp = 0;
			strcpy (sv, op);
			if (numtopnodes + 1 < MAXGROUPS) {
			    topnode[numtopnodes].gid = (int) iv;
			    topnode[numtopnodes].topnodename = sv;
			    topnode[numtopnodes + 1].gid = 0;
			}
			numtopnodes++;
			if (iv == IntV ("DefaultGroup"))
			    DefaultGroupHasHome = 1;
		    }
		    else {
			if (*cp == '"') {
			    cp++;
			    for (op = cp; *cp != '"' && *cp != 0; cp++);
			    sv = (char *) malloc (cp - op + 2);
			    *cp = 0;
			    strcpy (sv, op);
			}
			else {
			    iv = atol (cp);
			}
			if (nsymbols < MAXSYMBOLS) {
			    Symbols[nsymbols].SymbName = name;
			    Symbols[nsymbols].Svalue = sv;
			    Symbols[nsymbols].ivalue = iv;
			}
			nsymbols++;
		    }
		    goto exitforloop;
		default: 
		    if ((int) op == 0)
			op = cp;
		    continue;
	    }
	}
exitforloop: continue;
    }
    fclose(cfile);
    if (numtopnodes == 0) {
	fprintf (stderr, "nu: no GroupHome info in %s\n", CONFIGFILE);
	exit (ERROR);
    }
    if (numtopnodes >= MAXGROUPS) {
	fprintf (stderr, "nu: %s defines %d GroupHomes; limit is %d.\n",
		CONFIGFILE, numtopnodes, MAXGROUPS - 1);
	exit (ERROR);
    }
    if (nsymbols > MAXSYMBOLS) {
	fprintf (stderr, "nu: %s defines %d symbols; limit is %d.\n",
		CONFIGFILE, nsymbols, MAXSYMBOLS);
	exit (ERROR);
    }
    for (i = 0; topnode[i].gid; i++) {
	istat = stat (topnode[i].topnodename, &statbuf);
	if (istat != 0) {
	    fprintf (stderr, "nu: a GroupHome declaration names %s as the home\n    directory for group %d, but it does not exist. Please fix.\n",
		    topnode[i].topnodename, topnode[i].gid);
	    exit (ERROR);
	}
	if (!((statbuf.st_mode) & S_IFDIR)) {
	    fprintf (stderr, "nu: a GroupHome declaration names %s as the home dir for group %d,\n    but it is not a directory. Please fix.\n",
		    topnode[i].topnodename, topnode[i].gid);
	    exit (ERROR);
	}
    }

    if (!DefaultGroupHasHome) {
	fprintf (stderr, "nu: %s defines DefaultGroup=%d, but there is no\n GroupHome declaration for group %d. Be careful.\n",
		CONFIGFILE, IntV ("DefaultGroup"), IntV ("DefaultGroup"));
    }
}

/* IntV and StrV return integer and string values that were defined in
   the configuration file CONFIGFILE. */
int IntV (char *name)
{
    int     j;
    for (j = 0; j < nsymbols; j++) {
	if (strcmp (Symbols[j].SymbName, name) == 0)
	    return (int) (Symbols[j].ivalue);
    }
    fprintf (stderr, "nu: no definition of \"%s\" in nu.cf; cannot continue.\n", name);
    exit (ERROR);
}

char *StrV (char *name)
{
    int     j;
    for (j = 0; j < nsymbols; j++) {
	if (strcmp (Symbols[j].SymbName, name) == 0)
	    return (char *) (Symbols[j].Svalue);
    }
    fprintf (stderr, "nu: no definition of \"%s\" in nu.cf; cannot continue.\n", name);
    exit (ERROR);
}


/*
 *	M A I N   P R O G R A M
 */

main (int argc, char *argv[])
{
    char   *p;
    struct passwd  *pwd;
    ni_status whatHappened = NI_OK;
    int	index;

    if (argc == 1)
	goto uusage;
    incritsect = FALSE;
    signal (SIGINT, Catch);		/* catch ^C's */
    gethostname (This_host, sizeof(This_host));
    printf ("nu NeXT-1.0 [12 July 1989] (%s:%s)\n", This_host, CONFIGFILE);
    ReadCf ();

    if (argv[1][0] == '-') {
	if ((argv[1][1] == 'A'))
	    whatHappened = ni_open(NULL, argv[11], &niHandle);
	else if ((argv[1][1] == 'M'))
	    whatHappened = ni_open(NULL, argv[10], &niHandle);
	else if ((argv[1][1] == 'D') || (argv[1][1] == 'K'))
	    whatHappened = ni_open(NULL, argv[3], &niHandle);
	else
	    whatHappened = ni_open(NULL, StrV("NetInfoDomain"), &niHandle);
	if (whatHappened != NI_OK) {
	    printf ("Unable to open destination NetInfo domain.\n");
	    exit (ERROR);
	}
	whatHappened = ni_pathsearch(niHandle, &userDir, "/users");
	if (whatHappened != NI_OK) {
	    printf ("Unable to open /users directory in destination NetInfo domain.\n");
	    exit (ERROR);
	}
	whatHappened = ni_pathsearch(niHandle, &groupDir, "/groups");
	if (whatHappened != NI_OK) {
	    printf ("Unable to open /groups directory in destination NetInfo domain.\n");
	    exit (ERROR);
	}
	whatHappened = ni_pathsearch(niHandle, &aliasDir, "/aliases");
	if (whatHappened != NI_OK) {
	    printf ("Unable to open /aliases directory in destination NetInfo domain. Alias checking disabled.\n");
	    aliasDir = userDir;
	}
	if (IntV ("Debug"))
	    printf (">>>In debugging mode (no dangerous system calls)<<<\n");
	else {
	    if (geteuid ()) {		/* not a super-user */
		printf ("Sorry, you must have superuser status to run nu without debug mode.\n");
		exit (ERROR);
	    }
	}
	if (p = getlogin ())
	    strcpy (editor, p);
	else {
	    pwd = mygetpwuid (getuid ());
	    if (pwd)
		strcpy (editor, pwd->pw_name);
	    else
		strcpy (editor, "UNKNOWN!");
	}
	switch (argv[1][1]) {
	    case 'a': 
		Additions (FALSE, argv);
		break;
	    case 'A': 
		if (argc < 11) {
		    printf ("Insufficient data to perform user addition.\n");
		    exit(ERROR);
		}
		Additions (TRUE, argv);
		break;
	    case 'm': 
		Modify (FALSE, -2, argv);
		break;
	    case 'M': 
		if (argc < 10) {
		    printf ("Insufficient data to perform user modification.\n");
		    exit(ERROR);
		}
		else {
		    Modify (TRUE, atoi(argv[2]), argv);
		    break;
		}
	    case 'd':
		DeleteAccounts(FALSE, argv);
		break;
	    case 'D':
		if (argc != 4) {
		    printf ("Insufficient data to disable user.\n");
		    exit(ERROR);
		}
		DeleteAccounts(TRUE, argv);
		break;
	    case 'k': 
		if (argc < 3) {
		    fprintf (stderr, "usage: nu -k user ...\n");
		    exit(ERROR);
		}
		else {
		    for (index = 2; index < argc; index++)
			KillUser (index, argv);
		}
		break;
	    case 'K': 
		if (argc != 4) {
		    fprintf (stderr, "usage: nu -K user domain");
		    exit(ERROR);
		}
		else {
		    KillUser (2, argv);
		}
		break;
	    default: 
		goto uusage;
	}
    }
    else
	goto uusage;
    exit (OK);

uusage: 
    fprintf (stderr,
	    "usage:	nu -a			add new accounts\n");
    fprintf (stderr, "	nu -m			modify existing accounts\n");
    fprintf (stderr, "	nu -d			delete existing accounts\n");
    fprintf (stderr, "	nu -k user1 user2 ...	kill old accounts\n");
    exit (ERROR);
}
