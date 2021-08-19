/*
 * swent.c - Routines for dealing with swaptab entries.
 *
 * Copyright (c) 1989 by NeXT, Inc.
 *
 **********************************************************************
 * HISTORY
 * 28-Feb-89  Peter King (king) at NeXT
 *	Created.
 */

#define	SWAPTAB	"/etc/swaptab"

#include <c.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "swent.h"

static FILE	*swaptab = NULL;
static int	curline;

/*
 * Routine: swent_start
 * Function:
 *	Initialize things for swent_get().  If "file" is specified,
 *	then set us up to read from that file, otherwise set us up
 *	to get the default information.
 */
int
swent_start(char *file)
{

	/* If the swaptab is already open, close it. */
	if (swaptab != NULL) {
		swent_end();
	}

	/* Check if we should use the default. */
	if (file == NULL || *file == '\0') {
		/*
		 * In the future, this might set up NetInfo.
		 */
		file = SWAPTAB;
	}

	/* Now try opening the swaptab. */
	if ((swaptab = fopen(file, "r")) == NULL) {
		perror("swent_start");
		return (-1);
	}
	curline = 0;

	return (0);
}

/*
 * Routine: swent_get
 * Function:
 *	Get the next swaptab entry.
 */
swapent_t
swent_get()
{
	swapent_t	sw;	/* swapent we are creating */
	char		*buf;	/* buffer to read line into */
	char		*cp;	/* generic character pointer */

	/* Make sure the file is open. */
	if (swaptab == NULL) {
		return (SWAPENT_NULL);
	}

	/* Allocate a new entry. */
	if ((sw = (swapent_t) malloc(sizeof (*sw))) == SWAPENT_NULL ||
	    (buf = (char *) malloc(BUFSIZ)) == NULL) {
		fprintf(stderr, "swent_get: out of memory\n");
		if (sw) {
			free((void *)sw);
		}
		return (SWAPENT_NULL);
	}

tryagain:
	/* Read in a line from the file. */
	if ((cp = fgets(buf, BUFSIZ, swaptab)) == NULL) {
		if (ferror(swaptab)) {
			perror("swent_get");
		}
		goto errout;
	}
	curline++;

	/* Make sure that we got the whole line. */
	if ((cp = strchr(cp, '\n')) == NULL) {
		fprintf(stderr, "swent_get: line %d too long, ignoring.\n",
			curline);
		do {
			if ((cp = fgets(buf, BUFSIZ, swaptab)) == NULL) {
				if (ferror(swaptab)) {
					perror("swent_get");
				}
				goto errout;
			}
			cp = strchr(cp, '\n');
		} while (cp == NULL);
		goto tryagain;
	}

	/* Get the first token of non-whitespace characters. */
	if ((cp = strtok(buf, " \t\n")) == NULL) {
		/* Empty (blank) line, skip to the next one. */
		goto tryagain;
	}

	/* Is this a comment line? */
	if (*cp == '#') {
		goto tryagain;
	}

	/* Leading whitespace will break swent_rele, disallow them */
	if (cp != buf) {
		fprintf(stderr,
		    "swent_get: line %d has leading spaces, ignoring\n",
		    curline);
		goto tryagain;
	}

	/* This must be the filename. */
	sw->sw_file = cp;

	/* Get the options. */
	cp = strtok(NULL, " \t\n");

	/* Make sure they aren't a comment. */
	if (cp && *cp == '#') {
		cp = NULL;
	}

	swent_parseopts(cp, sw);

	return(sw);

errout:
	free((void *)buf);
	free((void *)sw);
	return(SWAPENT_NULL);
}
	
/*
 * Routine: swent_rele
 * Function:
 *	Release the swent struct allocated by swent_get.
 */
void
swent_rele(swapent_t sw)
{

	free(sw->sw_file);
	free(sw);
}

/*
 * Routine: swent_end
 * Function:
 *	Clean up and close down swent routines.
 */
void
swent_end()
{

	fclose(swaptab);
	swaptab = NULL;
}

/*
 * Routine: swent_parseopts
 * Function:
 *	Parse the options string into the swapent structure.
 */
void
swent_parseopts(char *str, swapent_t sw)
{
	char	*cp;			/* generic character pointer */
	char	*ep;			/* pointer to the equals sign */
	char	*opt;			/* pointer to the option name */

	/* Initialize the structure. */
	sw->sw_prefer = FALSE;
	sw->sw_noauto = FALSE;
	sw->sw_lowat = 0;
	sw->sw_hiwat = 0;

	/* Parse the options. */
	cp = opt = str;
	while (opt != NULL && *opt != '\0') {
		/*
		 * Terminate option with a '\0' leaving cp pointing
		 * to the next option if it exists.
		 */
		cp = strchr(opt, ',');
		if (cp != NULL) {
			*cp++ = '\0';
		}

		/* Parse option name */
		if (strncmp(opt, "noauto", 6) == 0) {
			sw->sw_noauto = TRUE;
		} else if (strncmp(opt, "prefer", 6) == 0) {
			sw->sw_prefer = TRUE;
		} else if (strncmp(opt, "lowat", 5) == 0) {
			ep = strchr(opt, '=') + 1;
			if (ep) {
				sw->sw_lowat = atoi(ep);
			}
				
		} else if (strncmp(opt, "hiwat", 5) == 0) {
			ep = strchr(opt, '=') + 1;
			if (ep) {
				sw->sw_hiwat = atoi(ep);
			}
		} else {
			fprintf(stderr, "swent_parseopts: unknown option %s, ingoring.\n",
				opt);
		}

		/* Skip to next option if it exists. */
		opt = cp;
	}
}

