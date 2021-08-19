/*	pwd.h	4.1	83/05/03	*/

struct	passwd { /* see getpwent(3) */
	char	*pw_name;
	char	*pw_passwd;
	int	pw_uid;
	int	pw_gid;
	int	pw_quota;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

#ifdef __STRICT_BSD__
struct passwd *getpwent(), *getpwuid(), *getpwnam();
#else
struct passwd *getpwent(void);
struct passwd *getpwuid(int uid);
struct passwd *getpwnam(const char *name);
void endpwent(void);
void setpwent(void);
#endif __STRICT_BSD__

