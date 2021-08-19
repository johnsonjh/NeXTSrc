/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.
 */

int	opterr = 1,		/* if error message should be printed */
	optind = 1,		/* index into parent argv vector */
	optopt = 0;			/* character checked for validity */
char	*optarg = 0;		/* argument associated with option */

