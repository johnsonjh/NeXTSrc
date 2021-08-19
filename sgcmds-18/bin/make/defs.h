#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <strings.h>
#include <ctype.h>
#include <stdio.h>

#define SHELLCOM	"/bin/sh"

#define RCS		"RCS"	/* name of RCS dir */
#define RCS_SUF		",v"	/* suffix of RCS files */

/*
 * to install metering, add a statement like
 * #define METERFILE "/usr/sif/make/Meter"
 * to turn metering on, set external variable meteron to 1.
 */

/*
 * define FSTATIC to be static on systems with C compilers
 * supporting file-static; otherwise define it to be null
 */
#define FSTATIC		static

#define NO		0
#define YES		1

#define unequal		strcmp
#define HASHSIZE	(1024*8-1)
#if	NeXT
#define INMAX		NCARGS
#define OUTMAX		NCARGS
#else
#define INMAX		12000
#define OUTMAX		12000
#endif	NeXT
#define QBUFMAX		16000

#define ALLDEPS		1
#define SOMEDEPS	2

#define META		01
#define TERMINAL	02
extern char funny[128];

#ifdef lint
#define ALLOC(x)	((struct x *) 0)
#else
#define ALLOC(x)	((struct x *) ckalloc(sizeof(struct x)))
#endif

/*
 * RCS-specific declarations
 */
extern int	coflag;			/* auto-checkout flag */
extern int	rmflag;			/* auto-remove flag */
extern struct shblock *co_cmd;		/* auto-checkout shell command */
extern struct shblock *rcstime_cmd;	/* rcs modtime shell command */
extern char	*RCSdir;		/* name of RCS dir */
extern char	*RCSsuf; 		/* suffix of RCS files */

extern int dbgflag;
extern int prtrflag;
extern int silflag;
extern int noexflag;
extern int keepgoing;
extern int noruleflag;
extern int touchflag;
extern int questflag;
extern int machdep;
extern int ndocoms;
extern int ignerr;
extern int okdel;
extern int inarglist;
extern char *prompt;
extern char *curfname;

struct nameblock {
	struct nameblock *nxtnameblock;
	char *namep;
	char *alias;
	struct lineblock *linep;
	unsigned int done:3;
	unsigned int septype:3;
	unsigned int rundep:1;
	unsigned int objarch:1;
	char *RCSnamep;			/* name of RCS file, if needed */
	char *archp;
	int hashval;
	time_t modtime;
};
extern struct nameblock *mainname;
extern struct nameblock *firstname;

struct lineblock {
	struct lineblock *nxtlineblock;
	struct depblock *depp;
	struct shblock *shp;
};
extern struct lineblock *sufflist;

struct depblock {
	struct depblock *nxtdepblock;
	struct nameblock *depname;
};

struct shblock {
	struct shblock *nxtshblock;
	char *shbp;
};

struct varblock {
	struct varblock *lftvarblock;
	struct varblock *rgtvarblock;
	char *varname;
	char *varval;
	unsigned int noreset:1;
	unsigned int used:1;
};
extern struct varblock *firstvar;

struct pattern {
	struct pattern *lftpattern;
	struct pattern *rgtpattern;
	char *patval;
};
extern struct pattern *firstpat;

struct dirblock {
	struct dirblock *nxtdirblock;
	char *fname;
};

struct dirhdr {
	struct dirhdr *nxtopendir;
	struct dirblock *nlist;
	DIR *dirfc;
	char *dirn;
	time_t mtime;
};
extern struct dirhdr *firstod;

struct chain {
	struct chain *nextp;
	char *datap;
};
extern struct chain *rmchain;
extern struct chain *deschain;

char *copys(), *concat(), *subst(), *ncat(), *ckalloc();
struct nameblock *srchname(), *makename(), *rcsco();
struct lineblock *runtime();
time_t exists();
