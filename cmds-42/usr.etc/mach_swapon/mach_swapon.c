/*
 * mach_swapon
 *
 * Copyright (c) 1989 by NeXT, Inc.
 *
 **********************************************************************
 * HISTORY
 * 27-Feb-89  Peter King (king) at NeXT
 *	Created.
 *
 **********************************************************************
 */

/*
 * Include files.
 */
#include <c.h>
#include <errno.h>
#include <stdio.h>
#include <kern/mach_swapon.h>

#include "swent.h"

/*
 * Global variables.
 */
char	*program;			/* The name of this program */
bool	verbose = FALSE;		/* Flag whether to be noisy or not */


/*
 * Function prototypes.
 */
int	autoswap(char *swaptab);
int	call_swapon(swapent_t sw);

/*
 * Main program
 */
main(int argc, char *argv[])
{
	char		*cp;		/* generic character pointer */
	struct swapent	swapent;	/* command line swapent */
	bool		automode = FALSE;
	char		*swaptab = NULL;

	/*
	 * Parse arguments.
	 */
	program = *argv++;
	argc--;
	if (argc <= 0) {
		goto usage;
	}
	while (argc != 0) {
		cp = *argv;
		if (*cp == '-') {
			while (*++cp != '\0') {
				switch (*cp) {
				    case 'a':
					automode = TRUE;
					break;

				    case 'v':
					verbose = TRUE;
					break;

				    case 'o':
					if (argc < 3) {
						/*
						 * Gotta at least have
						 * "-o options filename"
						 */
						goto usage;
					}
					argv++;
					argc--;
					swent_parseopts(*argv, &swapent);
					break;

				    case 'f':
					argv++;
					argc--;
					swaptab = *argv;
					break;

				    default:
					goto usage;
					break;
				}
			}
			argv++;
			argc--;
		} else {
			break;
		}
	}
	/*
	 * Make sure we don't have a filename if we are in automode
	 * and vice versa.
	 */
	if (argc != 0) {
		if (automode) {
			goto usage;
		}
		swapent.sw_file = *argv;
	} else {
		if (!automode) {
			goto usage;
		}
	}

	if (automode == TRUE) {
		if (autoswap(swaptab) != 0) {
			exit(1);
		}
	} else {
		if (call_swapon(&swapent) != 0) {
			exit(1);
		}
	}

	exit(0);
usage:
	fprintf(stderr, "usage: %s [-v] [-o options] filename\n", program);
	fprintf(stderr, "       %s [-v] [-f swaptab] -a\n", program);
	exit(1);
}


/*
 * Routine: autoswap
 * Function:
 *	Parse the configuration file and start swapping on each file
 *	listed in it, unless, of course, it has the "noauto" option
 *	set.
 */
int
autoswap(char *swaptab)
{
	swapent_t	sw;

	/*
	 * Set up to read swap entries.
	 */
	if (swent_start(swaptab) != 0) {
		return (-1);
	}

	/*
	 * Call mach_swapon on the file if it is not "noauto" [sic].
	 */
	while ((sw = swent_get()) != NULL) {
		if (sw->sw_noauto == FALSE &&
		    call_swapon(sw) != 0) {
			swent_rele(sw);
			swent_end();
			return (-1);
		}
		swent_rele(sw);
	}

	/*
	 * Close down the swaptab file.
	 */
	swent_end();

	return (0);
}

/*
 * Routine: call_swapon
 * Function:
 *	Call mach_swapon given the information in the swapent "sw".
 */
int
call_swapon(swapent_t sw)
{
	int error;
	
	error = mach_swapon(sw->sw_file,
			    sw->sw_prefer ? MS_PREFER : 0,
			    sw->sw_lowat,
			    sw->sw_hiwat);
	if (error) {
		fprintf(stderr, "%s: mach_swapon failed: %s\n",
			program, strerror(error));
		return (-1);
	} else if (verbose) {
		printf("%s: swapping on %s\n", program, sw->sw_file);
	}

	return (0);
}

