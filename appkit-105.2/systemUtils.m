/*
	systemUtils.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson

	Here are a bunch of little utils that we used to always toss at
	the end of Application.m.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application.h"
#import <defaults.h>
#import "nextstd.h"
#import "pbtypes.h"
#import "pbs.h"
#import <netinet/in.h>		/* to please <nfs/nfs_clnt.h> */
#import <string.h>
#import <nfs/nfs_clnt.h>	/* for HOSTNAMESZ */
#import <servers/netname.h>
#import <pwd.h>
#import <mach_error.h>
#import <mach.h>
#import <sys/loader.h>
#import <ldsyms.h>
#import <kern/sched.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXObjectCategoryZone=0\n");
    asm(".globl .NXObjectCategoryZone\n");
#endif

@interface Object(_NXZone)
+ (BOOL)_canAlloc;
@end

@implementation Object(_NXZone)

+ (BOOL)_canAlloc
{
    return YES;
}

@end


/* Pings the current connection to PS */
void NXPing(void)
{
    DPSWaitContext(DPSGetCurrentContext());
}


/* home dir, user name and machine name chaching */

static char	   *homeDirectory = NULL;
static char	   *userName = NULL;
static int	    lastKnownUid = -2;

static void getUserInfo(void)
{
    char	       *s;
    struct passwd      *upwd;
    int			uid;
    NXZone	       *zone = NXApp ? [NXApp zone] : NXDefaultMallocZone();
    
    uid = getuid();
    if (lastKnownUid == -2 && !homeDirectory && !userName) {
	s = getenv("HOME");
	if (s && *s) {
	    homeDirectory = NXCopyStringBufferFromZone(s, zone);
	    s = getenv("USER");
	    if (s && *s) {
		userName = NXCopyStringBufferFromZone(s, zone);
		lastKnownUid = uid;
	    }
	}
    }
    if (uid != lastKnownUid) {
 	upwd = getpwuid(uid);
	if (homeDirectory) {
	    free(homeDirectory);
	    homeDirectory = NULL;
	}
	if (upwd && upwd->pw_dir) {
	    homeDirectory = NXCopyStringBufferFromZone(upwd->pw_dir, zone);
	} else {
	    s = getenv("HOME");
	    if (s) {
		homeDirectory = NXCopyStringBufferFromZone(getenv("HOME"), zone);
	    } else {
		homeDirectory = "/";
	    }
	}
	if (userName) {
	    free(userName);
	    userName = NULL;
	}
	if (upwd && upwd->pw_name) {
	    userName = NXCopyStringBufferFromZone((upwd->pw_name), zone);
	} else {
	    s = getenv("USER");
	    if (s) {
		userName = NXCopyStringBufferFromZone(getenv("USER"), zone);
	    } else {
		userName = "nobody";
	    }
	}
	lastKnownUid = uid;
    }
}

const char *NXHomeDirectory(void)
{
    getUserInfo();
    return homeDirectory;
}


const char *NXUserName(void)
{
    getUserInfo();
    return userName;
}
    

const char *_NXHostName(void)
{
    static NXAtom machineName = NULL;

    if (!machineName) {
	char nameBuffer[HOSTNAMESZ];

	gethostname(nameBuffer, HOSTNAMESZ);
	machineName = NXUniqueString(nameBuffer);
    }
    return machineName;
}
    

extern port_t		name_server_port;
static port_t		AppkitServerPort = PORT_NULL;
static int		AppkitServerVersion = 0;

/* tries to find the most recent appkit server it can.  If hostArg is NULL, that means to look on the same machine as the window server we're using. */
port_t _NXLookupAppkitServer(int *version, const char *hostArg, char *hostFound)
{
    kern_return_t       ret;
    const char		*lookupHost = NULL;	/* host where we will look */
    const char		*lookupName;		/* netname of server */
    const char		*defValue;		/* value of a default */
    const char		*WSHost;		/* Win Server host */
    port_t		newPort = PORT_NULL;
    int			newVersion = 0;
    BOOL		doCache;		/* Cache this lookup? */

  /* quick test for common case */
    if (!hostArg && AppkitServerPort && !hostFound) {
	newPort = AppkitServerPort;
	newVersion = AppkitServerVersion;
    } else {
      /* figure out the real host and name, and if this involves the cache */
	defValue = NXGetDefaultValue([NXApp appName], "NXHost");
	WSHost = defValue ? defValue : _NXHostName();  /* ??? bug #5013, might be inherited and on different host */
	lookupHost = hostArg ? hostArg : WSHost;
	if (!hostArg) {
	    defValue = NXGetDefaultValue(_NXAppKitDomainName, "PBSName");
	    lookupName = defValue ? defValue : PBS_NAME;
	} else
	    lookupName = PBS_NAME;
	doCache = !strcmp(lookupHost, WSHost);
    
	if (doCache && AppkitServerPort) {
	    newPort = AppkitServerPort;
	    newVersion = AppkitServerVersion;
	} else {
	    ret = netname_look_up(name_server_port, !strcmp(lookupHost, _NXHostName()) ? "" : (char *)lookupHost, (char *)lookupName, &newPort);
	    if (ret == KERN_SUCCESS) {
		_NXPBSRendezVous(MAXINT, newPort, CURRENT_MINOR_VERS, &newVersion);
		if (doCache) {
		    AppkitServerVersion = newVersion;
		    AppkitServerPort = newPort;
		}
	    } else
		NXLogError("Pasteboard lookup failed on host %s: (%s)\n", lookupHost, mach_error_string(ret));
	}
    }
    if (version)
	*version = newVersion;
    if (hostFound)
	strcpy(hostFound, lookupHost);
    return newPort;
}

int _NXGetShlibVersion(void)
{
    struct load_command *loadCmd;
    struct fvmlib_command *shlibLC;
    int i;
    static int shlibVers = -1;

    if (shlibVers == -1) {
	loadCmd = (struct load_command *)(&(_mh_execute_header)+1);
	for (i = 0; i < _mh_execute_header.ncmds; i++) {
	    if (loadCmd->cmd == LC_LOADFVMLIB) {
		shlibLC = (struct fvmlib_command *)loadCmd;
		if (shlibLC->fvmlib.header_addr == 0x06000000) {
		    shlibVers = shlibLC->fvmlib.minor_version;
		    break;
		}
	    }
	    loadCmd = (struct load_command *)(((char *)loadCmd) + loadCmd->cmdsize);
	}
      /* presumably no shlib found because app was linked against _p library */
	if (shlibVers == -1)
	    shlibVers = CURRENT_MINOR_VERS;
    }
    return shlibVers;
}


void _NXSetupPriorities(void)
{
    port_t myThread = thread_self();
    kern_return_t kret;

    kret = thread_priority(myThread, MAXPRI_USER - 2, FALSE);
    AK_ASSERT(kret == KERN_SUCCESS, "_NXSetupPriorities: thread_priority failed!");
    kret = thread_policy(myThread, POLICY_INTERACTIVE, 0);
    AK_ASSERT(kret == KERN_SUCCESS, "_NXSetupPriorities: thread_policy failed!");
}


#define ISMETA(cls)		((cls)->info & CLS_META) 
#define GETMETA(cls)		(ISMETA(cls) ? (cls) : (cls)->isa)

static BOOL isKitClass(Class cls)
{
    BOOL checkSegments = NO;
    static char *akClassesStart = NULL, *akClassesEnd = NULL;
    static char *lsysClassesStart = NULL, *lsysClassesEnd = NULL;
    int sectSize;

    cls = GETMETA(cls);

    if (!checkSegments) {
	checkSegments = YES;
	akClassesStart = getsectdatafromlib("libNeXT_s", "__OBJC", "__meta_class", &sectSize);
	if (akClassesStart)
	    akClassesEnd = akClassesStart + sectSize;
	lsysClassesStart = getsectdatafromlib("libsys_s", "__OBJC", "__meta_class", &sectSize);
	if (lsysClassesStart)
	    lsysClassesEnd = lsysClassesStart + sectSize;
    }

    return ((char *)cls > akClassesStart && (char *)cls < akClassesEnd) || ((char *)cls > lsysClassesStart && (char *)cls < lsysClassesEnd);
}


static BOOL lookupMethod(Class cls, SEL sel)
{
    register int n;
    register Method smt;
    struct objc_method_list *mlist;
    
    while (cls && !isKitClass(cls)) {
	for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
	    smt = mlist->method_list;
	    n = mlist->method_count;
	    
	    while (--n >= 0) {
		if (sel == smt->method_name)
		    return YES;
		smt++;
	    }
	}
	cls = cls->super_class;
    }
    
    return NO;
}


BOOL _NXCanAllocInZone(id factory, SEL newSel, SEL initSel)
{
    if (![factory _canAlloc])
	return NO;
#ifdef SHLIB
    if (!isKitClass((Class)factory)) {
	BOOL hasNew, is1_0;

	hasNew = lookupMethod(GETMETA((Class)factory), newSel);
	is1_0 = _NXGetShlibVersion() <= MINOR_VERS_1_0;
	if (hasNew)
	    return !is1_0 && lookupMethod((Class)factory, initSel);
	else
	    return !is1_0 || !lookupMethod((Class)factory, initSel);
    } else
#endif
	return YES;	/* => 2.0 apps must convert to inits to use profiling */
}

BOOL _NXSubclassSel(id factory, SEL sel)
{
#ifdef SHLIB
    if (isKitClass((Class)factory)) {
	return NO;
    } else {
	BOOL overrides;

	overrides = lookupMethod((Class)factory, sel);
	return overrides;
    }
#endif
    return YES;
}

/* forks and execs the given program with no arguments.  This does the Unix
   magic to detach the process from the app so we dont have to reap it.
 */
BOOL _NXExecDetached(const char *host, const char *fullpath)
{
    int fd;
    const char *argv[5];
    char *simpleName;

    if (host && *host) {
	argv[0] = "/usr/ucb/rsh";
	argv[1] = "rsh";
	argv[2] = host;
	argv[3] = fullpath;
	argv[4] = NULL;
    } else {
	simpleName = strrchr(fullpath, '/');
	argv[0] = fullpath;
	argv[1] = simpleName ? simpleName + 1 : fullpath;
	argv[2] = NULL;
    }
    switch (fork()) {
	case 0:			/* the child */
	  /* Close inherited file descriptors */
	    for (fd = getdtablesize(); fd > 2; fd--)
		(void)close(fd);

	  /* make us a child of init, so parent doesnt have to wait() on us */
	    fd = open("/dev/tty", O_RDWR, 0);
	    if (fd >= 0) {
		(void)ioctl(fd, TIOCNOTTY, NULL);
		(void)close(fd);
	    }

	    execl(argv[0], argv[1], argv[2], argv[3], argv[4]);
	    NXLogError("Unable to exec %s", fullpath);
	    exit(0);
	case -1:		/* an error */
	    return NO;
	default:		/* the parent */
	    return YES;
    }
}

/*

Modifications
  
85
--
 6/02/90 trey	Created.  Moved in some stuff from Application
		wrote _NXGetShlibVersion

87
--
 7/16/90 trey	added _NXSetupPriorities

92
--
 8/14/90 trey	fixed isKitClass to look for libsys classes
 8/19/90 bryan	added _NXSubclassSel to discover is a class subclasses or
 		overrides a method.

*/
