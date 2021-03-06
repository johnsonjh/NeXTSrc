/*	grp.h	4.1	83/05/03	*/

struct	group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	int	gr_gid;
	char	**gr_mem;
};

#ifdef __STRICT_BSD__
struct group *getgrent(), *getgrgid(), *getgrnam();
#else
void setgrent (void);
void endgrent (void);
struct group *getgrent(void);
struct group *getgrgid(int gid);
struct group *getgrnam(char *name);
#endif __STRICT_BSD__
