static	char *sccsid = "@(#)calendar.c	4.5 (Berkeley) 84/05/07";
/* /usr/lib/calendar produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/
#include <sys/time.h>
#ifdef NeXT_MOD
#include <pwd.h>
#include <stdio.h>
#endif NeXT_MOD

#define DAY (3600*24L)

char *month[] = {
	"[Jj]an",
	"[Ff]eb",
	"[Mm]ar",
	"[Aa]pr",
	"[Mm]ay",
	"[Jj]un",
	"[Jj]ul",
	"[Aa]ug",
	"[Ss]ep",
	"[Oo]ct",
	"[Nn]ov",
	"[Dd]ec"
};
struct tm *localtime();

tprint(t)
long t;
{
	struct tm *tm;
	tm = localtime(&t);
	printf("(^|[ 	(,;])((%s[^ \t]*[ \t]*|(0%d|%d)/)0*%d)([^0123456789]|$)\n",
		month[tm->tm_mon],
		tm->tm_mon + 1, tm->tm_mon + 1, tm->tm_mday);
	printf("(^|[ 	(,;])((\\*[ \t]*)0*%d)([^0123456789]|$)\n",
		tm->tm_mday);
}

#ifdef NeXT_MOD
extern int main(int argc, char **argv, char **environ)
#else
main()
#endif NeXT_MOD
{
	long t;
#ifdef NeXT_MOD
	/*
	 * NIDUMP "should" be more functional in allowing Joe Random User
	 * (in this case C.S. @ U of W) to be able to print out all the
	 * passwords in a domain.  Alas, for 1.0 this isn't the case.
	 * 	Morris Meyer   8/20/89
	 */

	if (argc == 2 && (strcmp ("-p", argv[1]) == 0))  {
		struct passwd *pwd;
		
		setpwent();
		while ((pwd = getpwent()) != (struct passwd *)NULL)  {
			printf ("%s:%s:%d:%d:%s:%s:%s\n", 
				pwd->pw_name, pwd->pw_passwd,
				pwd->pw_uid, pwd->pw_gid,
				pwd->pw_gecos, pwd->pw_dir, pwd->pw_shell);
		}
		endpwent();
		exit (0);
	}
#endif NeXT_MOD
	time(&t);
	tprint(t);
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		tprint(t);
	case 6:
		t += DAY;
		tprint(t);
	default:
		t += DAY;
		tprint(t);
	}
}
