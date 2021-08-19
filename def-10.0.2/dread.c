#import <db/db.h>
#import <stdio.h>
#import	<ctype.h>
#import "defaults.h"
#import "open.h"

static char *prog;

static void
usage()
{
	fprintf(stderr, "Usage:  %s [owner] name\n", prog);
	fprintf(stderr, "        %s -l\n", prog);
	fprintf(stderr, "        %s -g [name]\n", prog);
	fprintf(stderr, "        %s -o owner\n", prog);
	fprintf(stderr, "        %s -n name\n", prog);
}

static void
error(char *msg, int arg)
{
	fprintf(stderr, "%s: ", prog);
	fprintf(stderr, msg, arg);
	fprintf(stderr, "\n");
}

static char *
escapeit(char *string)
{
	static char buf[1024];
	char *start = &buf[1];
	char *bp;

	bp = start;
	*start = 0;
	while (*string) {
		if (isspace(*string) && start != buf) {
			start = buf;
			*start = '"';
		}
		if (*string == '\'' || *string == '"' || *string == '\\')
			*bp++ = '\\';
		*bp++ = *string++;
	}
	if (start == buf)
		*bp++ = '"';
	else if (*start == 0) {
		*bp++ = '"';
		*bp++ = '"';
	}
	*bp = '\0';
	return(start);
}

static void
printit(char *owner, char *name, char *value)
{
	printf("%s ", escapeit(owner));
	printf("%s ", escapeit(name));
	printf("%s\n", escapeit(value));
}

static int
listdb(char *owner, char *name)
{
	Database       *defs;
	extern Database *_openD(void);
	extern void	_closeD(void);
	Data            d;
	char           *dptr;
	char            bufk[1024], bufc[1024];

	if (!(defs = _openD())) {
		error("Can't open defaults database", 0);
		return(1);
	}

	d.k.s = bufk;
	d.c.s = bufc;
	for (dbFirst(defs, &d); dbGet(defs, &d); dbNext(defs, &d)) {
		for (dptr = d.k.s; *dptr != (char)0xff && *dptr; dptr++)
			continue;
		*dptr++ = 0;
		d.k.s[d.k.n] = '\0';
		if (owner) {
			if (!*d.k.s)
				continue;
			if (strcmp(owner, d.k.s))
				continue;
		}
		if (name) {
			if (strcmp(name, dptr))
				continue;
		}
		/*
		 * pre-1.0 databases represented GLOBAL defaults as entries
		 * without keys, we simply ignore them
		 */
		if (*d.k.s)
			printit(d.k.s, dptr, d.c.s);
	}

	_closeD();
	return(0);
}

void
main(argc, argv)
int             argc;
char          **argv;
{
	char           *owner = NULL, *name = NULL, *cp;
	int		list_all = 0, name_match = 0, owner_match = 0;
	int		is_global = 0;
	extern char    *NXGetDefaultValue(char *owner, char *name);

	prog = *argv++; argc--;

	while (argc > 0 && **argv == '-') {
		cp = *argv++ + 1; argc--;
		do {
			switch (*cp) {
			case 'l':
				list_all = 1;
				break;
			case 'n':
				name_match = 1;
				break;
			case 'o':
				owner_match = 1;
				break;
			case 'g':
				is_global = 1;
				break;
			case '\0':
				usage();
				exit(1);
			default:
				error("Unknown option: -%c", (int)*cp);
				usage();
				exit(1);
			}
		} while (*++cp);
	}

	switch (argc) {
	case 0:
		if (list_all)
			break;
		if (is_global) {
			owner = "GLOBAL";
			break;
		}
		/* fall-through */
	default:
		usage();
		exit(1);
	case 1:
		if (list_all) {
			usage();
			exit(1);
		}
		if (owner_match) {
			owner = argv[0];
			list_all = 1;
		} else if (name_match) {
			name = argv[0];
			list_all = 1;
		} else {
			owner = "GLOBAL";
			name = argv[0];
		}
		break;
	case 2:
		if (is_global || owner_match || name_match || list_all) {
			usage();
			exit(1);
		}
		owner = argv[0];
		name = argv[1];
		break;
	}

	if (list_all)
		exit(listdb(owner, name));

	if (!(cp = NXGetDefaultValue(owner, name))) {
		error("couldn't read default", 0);
		exit(1);
	}
	printit(owner, name, cp);
	exit(0);
}
