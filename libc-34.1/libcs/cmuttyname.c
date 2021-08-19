/*
 **********************************************************************
 * HISTORY
 * 11-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <libc.h>
#include <c.h>

#define	TMR_HEX		0
#define	TMR_ALPHA	1

static struct ttymap {
	int tm_major;
	char tm_base;
	char tm_radix;
	short tm_count;
} ttymap[] = {
	-1,	'p',	TMR_HEX,	80,
	-1,	'P',	TMR_HEX,	80,
	-1,	'f',	TMR_ALPHA,	26,
	-1,	'p',	TMR_ALPHA,	26
};

static char ttytemp[32] = "/dev/tty??";
static char ttyreturn[32];

char *cmuttyname(rdev)
dev_t rdev;
{
	struct stat stb;
	register int i, rmin, rmaj = major(rdev);
	register struct ttymap *tm;

	for (i = 0; i < sizeofA(ttymap); i++) {
		tm = &ttymap[i];
		if (tm->tm_base == '\0')
			continue;
		if (tm->tm_major < 0) {
			ttytemp[8] = tm->tm_base;
			ttytemp[9] = tm->tm_radix == TMR_HEX ? '0' : 'a';
			if (stat(ttytemp, &stb) < 0) {
				tm->tm_base = '\0';
				continue;
			}
			tm->tm_major = major(stb.st_rdev);
		}
		if (tm->tm_major == rmaj) {
			if ((rmin = minor(rdev)) >= tm->tm_count)
				return (NULL);
			switch (tm->tm_radix) {
			case TMR_HEX:
				ttytemp[8] = tm->tm_base+((rmin>>4)&0xf);
				ttytemp[9] = (rmin&0xf) < 10 ?
					     (rmin&0xf) + '0' :
					     (rmin&0xf) - 10 + 'a';
				break;
			case TMR_ALPHA:
				ttytemp[8] = tm->tm_base;
				ttytemp[9] = rmin + 'a';
				break;
			}
			return(strcpy(ttyreturn, ttytemp));
		}
	}
	return (NULL);
}
