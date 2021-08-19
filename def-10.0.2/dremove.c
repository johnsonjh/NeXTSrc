#import <db/db.h>
#import "defaults.h"
#import "open.h"

#define	NELEM(x)	(sizeof(x)/sizeof(x[0]))

#define	SUCCESS	0
#define	ERROR	1
#define	FATAL	2

static char *prog;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s owner name\n", prog);
	fprintf(stderr, "       %s -g name\n", prog);
	fprintf(stderr, "       %s < OWNER_NAME_FILE\n", prog);
}

static void
error(char *msg, int arg1, int arg2)
{
	fprintf(stderr, "%s: ", prog);
	fprintf(stderr, msg, arg1, arg2);
	fprintf(stderr, "\n");
}

static int
argvize(char *line, char **args, int maxargs)
{
	int argc;
	char term;

	for (argc = 0; argc < maxargs-1; argc++) {
		while (isspace(*line))
			line++;
		if (*line == '\0')
			break;
		if (*line == '\'' || *line == '"')
			term = *line++;
		else
			term = 0;
		args[argc] = line;
		while (*line && (term ? *line != term : !isspace(*line))) {
			if (*line == '\\') {
				char *cp;

				for (cp = line+1; *cp; cp++)
					*(cp-1) = *cp;
			}
			line++;
		}
		*line++ = '\0';
	}
	args[argc] = NULL;
	return(argc);
}

static int
dremove(char *owner, char *name)
{
	if (!NXRemoveDefault(owner, name)) {
		error("couldn't remove \"%s\" owned by \"%s\"",
		    (int)name, (int)owner);
		return(ERROR);
	}
	return(SUCCESS);
}

void
main(int argc, char **argv)
{
	char            buf1[1024];
	char            buf2[1024];
	char	       *args[5];
	char	       *owner;
	char	       *name;
	char	       *cp;
	int             count;
	int		err = SUCCESS;
	int 		is_global = 0;
	int		minargs;

	prog = *argv++; argc--;

	while (argc > 0 && **argv == '-') {
		cp = *argv++ + 1; argc--;
		do {
			switch (*cp) {
			case 'g':
				is_global = 1;
				break;
			case '\0':
				usage();
				exit(1);
			default:
				error("Unknown option: -%c", (int)*cp, 0);
				usage();
				exit(1);
			}
		} while (*++cp);
	}


	if (argc == 0) {	/* Get input from stdin */
		if (is_global) {
			error("-g flag requires name", 0, 0);
			usage();
			exit(1);
		}
		while (fgets(buf1, sizeof(buf1), stdin)) {
			strcpy(buf2, buf1);
			count = argvize(buf1, args, NELEM(args));
			if (count == 0)	/* ignore blank lines */
				continue;
			if (count < 2 || count > 3) {
				error("invalid input: %s", (int)buf2, 0);
				exit(FATAL);
			}
			err |= dremove(args[0], args[1]);
		}
		exit(err);
	}
	
	minargs = is_global ? 1 : 2;

	if (argc < minargs || argc > minargs+1) {
		usage();
		exit(FATAL);
	}

	owner = is_global ? "GLOBAL" : *argv++;
	name = *argv;

	exit(dremove(owner, name));
}
