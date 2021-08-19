#import <stdio.h>
#import <db/db.h>
#import "defaults.h"
#import "open.h"

#define	NELEM(x)	(sizeof(x)/sizeof(x[0]))

#define	SUCCESS	0
#define	ERROR	1
#define	FATAL	2

static char *prog;

static void
usage()
{
	fprintf(stderr, "Usage:  %s owner name value\n", prog);
	fprintf(stderr, "        %s -g name value\n", prog);
	fprintf(stderr, "        %s < OWNER_NAME_VALUE_FILE\n", prog);
}

static void
error(char *msg, int arg)
{
	fprintf(stderr, "%s: ", prog);
	fprintf(stderr, msg, arg);
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
dwrite(char *owner, char *name, char *value)
{
	if (!NXWriteDefault(owner, name, value)) {
		error("Couldn't write into database", 0);
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
	char	       *cp;
	char	       *owner;
	char	       *name;
	char	       *value;
	int             count;
	int		err = SUCCESS;
	int		is_global = 0;

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
				exit(FATAL);
			default:
				error("Unknown option: -%c", (int)*cp);
				usage();
				exit(FATAL);
			}
		} while (*++cp);
	}

	if (argc == 0) {	/* Get input from stdin */
		if (is_global) {
			error("-g flag requires name and value", 0);
			usage();
			exit(1);
		}
		while (fgets(buf1, sizeof(buf1), stdin)) {
			strcpy(buf2, buf1);
			count = argvize(buf1, args, NELEM(args));
			if (count == 0)
				continue;
			if (count != 3) {
				error("invalid input: %s", (int)buf2);
				exit(FATAL);
			}
			err |= dwrite(args[0], args[1], args[2]);
		}
		exit(err);
	}
	
	if (argc != (is_global ? 2 : 3)) {
		usage();
		exit(FATAL);
	}

	owner = is_global ? "GLOBAL" : *argv++;
	name = *argv++;
	value = *argv;

	exit(dwrite(owner, name, value));
}
