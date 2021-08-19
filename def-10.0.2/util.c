#include <pwd.h>

#define NULL 0;

static char	   *homeDirectory = NULL;
static char	   *userName = NULL;
static int	    lastKnownUid = -2;
static void getUserInfo()
{
    struct passwd      *upwd;
    int			uid;
    extern char	       *NXCopyStringBuffer(char *string);

    uid = getuid();
    if (uid != lastKnownUid) {
	if (upwd = getpwuid(uid)) {
	    if (homeDirectory) {
		free(homeDirectory);
		homeDirectory = NULL;
	    }
	    if (upwd->pw_dir)
		homeDirectory = NXCopyStringBuffer(upwd->pw_dir);
	    if (userName) {
		free(userName);
		userName = NULL;
	    }
	    if (upwd->pw_name)
		userName = NXCopyStringBuffer(upwd->pw_name);
	} else {
	    homeDirectory = NULL;
	    userName = NULL;
	}
	lastKnownUid = uid;
    }
}

const char *
NXHomeDirectory()
{
    getUserInfo();
    return homeDirectory;
}


