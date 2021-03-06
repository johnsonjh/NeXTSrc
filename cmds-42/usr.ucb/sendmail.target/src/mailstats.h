/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
**
**	"@(#)mailstats.h	5.1 (Berkeley) 5/2/86";
**
*/

/*
**  Statistics structure.
*/
#if	NeXT_MOD
# define NAMELEN 16	/* length of each mailer name */
#endif	NeXT_MOD

struct statistics
{
	time_t	stat_itime;		/* file initialization time */
	short	stat_size;		/* size of this structure */
	long	stat_nf[MAXMAILERS];	/* # msgs from each mailer */
	long	stat_bf[MAXMAILERS];	/* kbytes from each mailer */
	long	stat_nt[MAXMAILERS];	/* # msgs to each mailer */
	long	stat_bt[MAXMAILERS];	/* kbytes to each mailer */
#if	NeXT_MOD
	char 	stat_names[MAXMAILERS][NAMELEN];
					/* symbolic names of mailers */
#endif	NeXT_MOD
};
