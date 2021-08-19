#ifndef ALREADYpiduid
#include <pwd.h>
#include <grp.h>
#include <netinfo/ni.h> 
#endif
#define	ERROR		1		/* exit(...) codes */

extern void   *niHandle;   /* handle for domain being used by nu */
extern ni_id   userDir;	   /* netinfo directory for the users directory */
extern ni_id   groupDir;   /* netinfo directory for the groups directory */

int findUserInHmmm(char *userName, char *dirName, char *hmmmm, ni_id whatID);
void *listGroups();
int getMaxUid();
struct passwd *mygetpwuid(int uid);
struct passwd *mygetpwnam(char *nam);
struct group *mygetgrgid(int gid);
struct group *mygetgrnam(char *nam);

#define ALREADYpiduid 1
