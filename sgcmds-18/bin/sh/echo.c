/*	@(#)echo.c	1.1	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 *	DAG -- changed to support 7th Edition "-n" option;
 *	still not fully compatible since \ escapes are interpreted
 */
#include	"defs.h"

#define	exit(a)	flushb();return(a)

extern int exitval;

echo(argc, argv)
char **argv;
{
	register char	*cp;
	register int	i, wd;
	int	j;
#if BRL || BERKELEY
	BOOL		no_nl;		/* no newline if set */
#endif
	
	if(--argc == 0) {
		prc_buff('\n');
		exit(0);
	}
#if BRL || BERKELEY
	if(no_nl = eq(argv[1], dashn))	/* old-style no-newline flag */
	{
		--argc;			/* skip over "-n" argument */
		++argv;
	}
#endif

	for(i = 1; i <= argc; i++) 
	{
		sigchk();
		for(cp = argv[i]; *cp; cp++) 
		{
			if(*cp == '\\')
			switch(*++cp) 
			{
				case 'b':
					prc_buff('\b');
					continue;

				case 'c':
					exit(0);

				case 'f':
					prc_buff('\f');
					continue;

				case 'n':
					prc_buff('\n');
					continue;

				case 'r':
					prc_buff('\r');
					continue;

				case 't':
					prc_buff('\t');
					continue;

				case 'v':
					prc_buff('\v');
					continue;

				case '\\':
					prc_buff('\\');
					continue;
				case '0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					prc_buff(wd);
					--cp;
					continue;

				default:
					cp--;
			}
			prc_buff(*cp);
		}
		if(i != argc)
			prc_buff(' ');
	}
#if BRL || BERKELEY
	if(!no_nl)
#endif
	    prc_buff('\n');
	exit(0);
}
