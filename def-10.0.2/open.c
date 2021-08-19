#import <pwd.h>
#import <stdio.h>
#import <sys/file.h>
#import <sys/param.h>
#import <sys/times.h>
#import <sys/time.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <errno.h>
#import <db/db.h>

#import "defaults.h"
#import "open.h"

#define READ_BUF_SIZE   1024


Database *_openDefaults();
int _closeDefaults();


static char    *defaultAlloc();
static void     cmdLineToDefaults();
static int _registerDefault();
static _lockDefaults();
static _unlockDefaults();
static _constructKey();
static _destroyKey();
static char *blockAlloc();

struct RegisteredDefault {
    time_t         mtime;
    char           *owner;
    char           *name;
    char           *value;
    struct RegisteredDefault *next;
};

static char *readBuf;
static Database *defaultsDB;

static struct RegisteredDefault *firstRD = 0;
static struct RegisteredDefault *lastRD = 0;

#define DEFAULTBLOCKSIZE 512
static char    *dBlock = 0;
static char    *dPos = 0;
static char   **firstBlock;
static char   **lastBlock;
static time_t db_mtime;
extern char *strcpy();

/*** Private functions ***/

Database       *
_openD()
 /*
  * Open the defaults database.  If the ~/.NeXT directory doesn't exist
  * then this directory is created.  If the database doesn't exist then it
  * too is created.  This routine also scans the command line parameters.
  * Returns a pointer to the Database if successful, otherwise it returns
  * 0. 
  *
  * After calling _openDefaults you must call _closeDefaults. 
  */
{
    struct passwd  *pw;
    extern struct passwd  *getpwuid();
    struct stat stat_buf;
    int             dir;

    pw = getpwuid(getuid());

 /* Construct the path ~/.NeXT/.NeXTdefaults. */
    if (pw && (pw->pw_dir) && (*pw->pw_dir)) {

      /* Allocate a buffer used to read default data from the database.  */
	if (!(readBuf = (char *)malloc(READ_BUF_SIZE)))
	    return (0);
    /*
     * Use the readBuf as a temp buffer to construct the .NeXTdefaults
     * path.  This is nasty but we don't really cause any problems by
     * using the readBuf this way. 
     */
    /* First see if the ~/.NeXT directory exists. */
	strcpy(readBuf, pw->pw_dir);
	strcat(readBuf, "/.NeXT");

    /* If the dir doesn't exist then try to create it. */
	if ((dir = access(readBuf, F_OK)) < 0) {
	    if (errno == ENOENT) {
		if (mkdir(readBuf, 0777))
		    return (0);
	    } else
		return (0);
	}
	
    /*
     * Now that we know the ~/.NeXT dir exists we can proceed to open the
     * defaults database. 
     */
	strcat(readBuf, "/.NeXTdefaults");

	defaultsDB = dbOpen(readBuf);
	if (!defaultsDB) {
	    free(readBuf);
	    readBuf = 0;
	    return (0);
	}
	/*
         * Get the current time stamp for the database.
	 */
	strcat(readBuf, ".L" );
	stat( readBuf, &stat_buf );
	db_mtime = stat_buf.st_mtime;

	if (!_lockDefaults())
	    return (0);

	return (defaultsDB);
    }
    return (0);
}


void
_closeD()
 /*
  * Close the defaults database.  Deallocate MOST memory allocated. 
  */
{
    char          **nextBlock = 0;

    _unlockDefaults();
 /*
  * Close the database. 
  */
    if (defaultsDB) {
	dbClose(defaultsDB);
	defaultsDB = 0;
    }
 /*
  * Free memory used as temporary read buffer. 
  */
    if (readBuf) {
	free(readBuf);
	readBuf = 0;
    }
#ifdef DONT_FREE   /*  We need this memory to persistent across a _closeDefaults. */

 /*
  * Free memory used to store user default values. 
  */
    while (firstBlock) {
	nextBlock = (char **)*firstBlock;
	free(firstBlock);
	firstBlock = nextBlock;
    }
#endif
}


static int
_registerDefault(owner, name, value)
    char           *owner, *name, *value;

 /*
  * This could be changed to use a hash table. 
  */
{
    if (!firstRD)
	firstRD = lastRD = (struct RegisteredDefault *)
	      defaultAlloc(sizeof(struct RegisteredDefault));
    else {
	lastRD->next = (struct RegisteredDefault *)
	      defaultAlloc(sizeof(struct RegisteredDefault));
	lastRD = lastRD->next;
    }
    lastRD->next = 0;

    lastRD->owner = owner ? strcpy(defaultAlloc( strlen(owner)+1), owner ) : 0;
    lastRD->value = value ? strcpy(defaultAlloc( strlen(value)+1), value ) : 0;
    lastRD->name = name ? strcpy(defaultAlloc( strlen(name)+1), name ) : 0;
    lastRD->mtime = db_mtime;

    return (1);
}





static _lockDefaults()
{
    return (dbWaitLock(defaultsDB, 5, 30));
}



static _unlockDefaults()
{
    dbUnlock(defaultsDB);
}



static _constructKey(keyDatum, owner, name)
    Datum          *keyDatum;
    char           *owner, *name;
{
    char           *cptr;
    int             len;

    if (!owner)
	owner = "";

    len = strlen(owner) + strlen(name) + 1;
    keyDatum->s = cptr = (char *)malloc(len);
    keyDatum->n = len;
    while (*owner)
	*cptr++ = *owner++;
    *cptr++ = 0xff;
    while (*name)
	*cptr++ = *name++;
}


static _destroyKey(keyDatum)
    Datum          *keyDatum;
{
    if (keyDatum)
	free(keyDatum->s);
}


static char    *
defaultAlloc(chars)
    int             chars;

 /*
  * allocates a block of DEFAULTBLOCKSIZE characters at a time, then
  * dishes them out in response to calls. This is to avoid the
  * fragmentation that individual calls to malloc would cause. 
  */
{
    char           *place;


    if (chars > DEFAULTBLOCKSIZE)
	return (blockAlloc(chars));

    if ((!dBlock) || (chars > (dBlock + DEFAULTBLOCKSIZE - dPos)))
	dBlock = dPos = (char *)blockAlloc(DEFAULTBLOCKSIZE);
    place = dPos;
    dPos += chars;

    return (place);
}


static char           *
blockAlloc(size)
    int             size;
{
    char          **place = (char **)malloc(size + sizeof(char *));


    if (firstBlock) {
	*lastBlock = (char *)place;
	lastBlock = place++;
    } else
	firstBlock = lastBlock = place++;

    return ((char *)place);
}


static
_NXGobbleArgs(numToEat, argc, argv)
    register int    numToEat;
    int            *argc;
    register char **argv;

 /*
  * a little accessory routine to the command line arg parsing that
  * removes some args from the argv vector. 
  */
{
    register char **copySrc;

 /* copy the other args down, overwriting those we wish to gobble */
    for (copySrc = argv + numToEat; *copySrc; *argv++ = *copySrc++);
    *argv = 0;			/* leave the list NULL terminated */
    *argc -= numToEat;
}


static void
cmdLineToDefaults( char *owner, NXDefaultsVector vector )
 /*
  * Sucks arguments we need off the command line and places them the
  * parameters data. 
  */
{
    extern int      NXArgc;
    extern char   **NXArgv;
    register int   *argc = &NXArgc;
    register char **argv = NXArgv;
    struct RegisteredDefault *rd;
    struct _NXDefault *d;
    char           *value;

    argv++;			/* skip the command itself */
    while (*argv) {		/* while there are more args */
	if (**argv == '-') {	/* if its a flag (starts with a '=') */
	    /*
	     * If we have something of the form -default value then
	     * register it.
	     */
	    if ( argv[1] && *argv[1] != '-' ) {
	      /* See if this default has been requested. */
	      for (d = vector; d->name; d++) {
		if ( strcmp( argv[0]+1, d->name) == 0 )
		  break;
	      }
	      if( d->name ){
		_registerDefault(owner, argv[0] + 1, argv[1]);
		_NXGobbleArgs(2, argc, argv);
	      }
	      else
		argv++;
	    } 
	    /*
	     * If we have something of the form -default with no value
	     * or -default -default then set the default to null
	     */
	    else {
	      /* See if this default has been requested. */
	      for (d = vector; d->name; d++) {
		if ( strcmp( argv[0]+1, d->name) == 0 )
		  break;
	      }
	      if( d->name ){
		_registerDefault(owner, argv[0] + 1, "");
		_NXGobbleArgs(1, argc, argv);
	      }
	      else
		argv++;
	    }
	} else			/* else its not a flag at all */
	    argv++;
    }
}



/*
Modifications (starting at 0.8):

1/10/89  rjw	Fixed dangling pointer bug.  When a default was registered
		using _registerDefault or _resetRegistedDefault the owner,
		name, and value weren't copied.  This occasionaly resulted in 
		problems when non-constant data was used.
2/17/89  rjw    Add NXUpdateDefaults().  Each default is time stamped.
                NXUpdateDefaults() will re-read the database if the timestamp
		of the database differs from the timestamp of the default.
*/

