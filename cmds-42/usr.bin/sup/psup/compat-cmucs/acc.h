/*
 *  see getacent(3)
 *
 **********************************************************************
 * HISTORY
 * 11-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Add external declarations for account routines.  Change
 *	ac_passwd to ac_attrs for new accounting file information.
 *
 **********************************************************************
 */

struct	account
{
	char	*ac_name;
	char	**ac_attrs;
	int	 ac_uid;
	int	 ac_aid;
	char	*ac_created;
	char	*ac_expires;
	char	*ac_sponsor;
	time_t	 ac_ctime;
	time_t	 ac_xtime;
};

struct account *getacent(), *getacauid(), *getacanam();
