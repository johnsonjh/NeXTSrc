/*
	ulimit -- system call emulation for Bourne shell on 4.2BSD

	last edit:	22-Aug-1983	D A Gwyn
*/

#include	<errno.h>

extern int	getrlimit(), setrlimit();
extern int	errno;

long
ulimit( cmd, newlimit )
	int	cmd;			/* subcommand */
	long	newlimit;		/* desired new limit */
	{
	struct	{
		long	rlim_cur;
		long	rlim_max;
		}	limit;		/* data being gotten/set */

	switch ( cmd )
		{
	case 1: 			/* get file size limit */
		if ( getrlimit( 1, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max / 512L;

	case 2: 			/* set file size limit */
		limit.rlim_cur = limit.rlim_max = newlimit * 512L;
		return setrlimit( 1, &limit );

	case 3: 			/* get maximum break value */
		if ( getrlimit( 2, &limit ) != 0 )
			return -1L;	/* errno is already set */
		return limit.rlim_max;

	default:
		errno = EINVAL;
		return -1L;
		}
	}
