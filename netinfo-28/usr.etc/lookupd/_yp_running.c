/* Copyright (C) 1989 by NeXT, Inc. */
/*
 * Determine whether or not to use the yellow pages service to do lookups.
 */
int
_yp_running()
{
	static int yp_running = -1;
	char *domain;
	extern int yp_get_default_domain(char **);
	extern int yp_bind(char *);

	if (yp_running < 0) {
		if (yp_get_default_domain(&domain) == 0 &&
		    yp_bind(domain) == 0) {
			yp_running = 1;
		} else {
			yp_running = 0;
		}
	}
	return (yp_running);
}
