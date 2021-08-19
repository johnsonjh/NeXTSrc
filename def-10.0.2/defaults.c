
/*
	defaults.c
  	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Parkhurst
	  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h

	This file holds code to read a user's .NeXTdefaults file, and
	return requested info to the application.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdio.h>
#import <sys/file.h>
#import <sys/param.h>
#import <sys/times.h>
#import <sys/time.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <errno.h>
/*#import "nextstd.h" */
#import "defaults.h"

extern int getuid(void);
extern int access(const char *path, int mode);
extern int mkdir(const char *name, int mode);
extern const char *NXHomeDirectory(void);

#define READ_BUF_SIZE   1024

static const char *GLOBAL = "GLOBAL";

struct RegisteredDefault {
    time_t         mtime;
    char           *owner;
    char           *name;
    char           *value;
    struct RegisteredDefault *next;
};

static char    *defaultAlloc();
static void     cmdLineToDefaults();
static Database *_openDefaults();
static int _closeDefaults();
static int _registerDefault();
static struct RegisteredDefault *_isRegistered();
static char *_doReadDefault();
static char *_doWriteDefault();
static int _writeAndRegisterDefault();
static int _lockDefaults();
static void _unlockDefaults();
static void _constructKey(Datum *keyDatum, const char *owner, const char *name);
static void _destroyKey(Datum *keyDatum);
static char *blockAlloc();
static void _resetRegisteredDefault(struct RegisteredDefault *rd, const char *owner, const char *name, const char *value);

static Database *defaultsDB = NULL;
static char    *readBuf = NULL;

static struct RegisteredDefault *firstRD = 0;
static struct RegisteredDefault *lastRD = 0;

#define DEFAULTBLOCKSIZE 512
static char    *dBlock = 0;
static char    *dPos = 0;
static char   **firstBlock = NULL;
static char   **lastBlock = NULL;
static time_t db_mtime = 0;
extern char *strcpy();

int
NXRegisterDefaults(const char *owner, const NXDefaultsVector vector)
 /*
  * Registers the defaults in the vector.  Registration of defaults is
  * required and accomplishes two things.  (1) It specifies an initial
  * value for the default in the case when it doesn't appear in the
  * .NeXTdefaults database or on the command line.  (2) It allows more
  * efficient reading of the defaults from the defaults database.  This is
  * because all defaults specified in the vector are looked up at once
  * without having to open and close the database for each lookup. 
  * If you want to use the defaults mechanism don't expect to be able
  * to parse argv.  These functions remove items from the command line.
  * After the items have been removed and there should be NXArgc items
  * left in the argv array.
  *
  * A common place to register your defaults is in the initialize method of
  * the class that will use them. 
  * 
  * 0 is returned if the database couldn't be opened. Otherwise 1.
  */
{
    struct RegisteredDefault *rd;
    const struct _NXDefault *d;
    char           *value;
    int  haveDefaultsFile; /* !!! cmf 061489 */
    

    if( !owner || !vector )
      return( 0 );
      
 /* Register command line arguments.  */
    db_mtime = 0;
    cmdLineToDefaults( owner, vector );

    haveDefaultsFile = (int)_openDefaults(); /* !!! cmf 061489 */

 /*
  * The default gets its value with the following precedence: 
  *
  * 1)  Look for the default in the registration table with specified owner 
  *
  * 2)  Look for the default in the database with the specified owner 
  *
  * 3)  Look for the default in the registration table with GLOBAL owner 
  *
  * 4)  Look for the default in the database with GLOBAL owner 
  *
  * 5)  Use the initial default value from the vector 
  */
    for (d = vector; d->name; d++) {
	if (_isRegistered(owner, d->name))	/* Already registered */
	    continue;

	if (haveDefaultsFile && (value = _doReadDefault(owner, d->name, 0)))
	/* Database w/ owner */
	    _registerDefault(owner, d->name, value);

	else if (rd = _isRegistered( GLOBAL, d->name))	
	/* Table with GLOBAL owner */
	    _registerDefault(owner, d->name, rd->value);

	else if (haveDefaultsFile && (value = _doReadDefault(GLOBAL, d->name, 0)))
	/* Database with GLOBAL owner */
	    _registerDefault(owner, d->name, value);

	else {			/* inital value from vector */
	    if (d->value) {
		value = (char *)defaultAlloc(strlen(d->value) + 1);
		strcpy(value, d->value);
	    } else
		value = d->value;
	    _registerDefault(owner, d->name, value);
	}
    }
    if (haveDefaultsFile) /* !!! cmf 061489 */
        _closeDefaults(); /* !!! cmf 061489 */
    return (1);
}


const char     *
NXGetDefaultValue(const char *owner, const char *name)
 /*
  * This routine returns a pointer to the default requested.  If the owner
  * is null then the default is a system wide default.  The defaults
  * database can be overridden by supplying command line arguments at
  * launch time.  This is done with the following syntax: 
  *
  * app -default1 value -default2 value. 
  *
  * If a the default doesn't exist for the given owner then a search is made
  * for that default as a system wide default. Memory is
  * allocated for the default value. 
  */
{
    struct RegisteredDefault *rd;
    char           *defaultString;

    if( !owner || !name )
        return( 0 );
	
 /*
  * 1)  Look for the default in the registration table with specified
  * owner 
  *
  * 2)  Look for the default in the database with the specified owner 
  *
  * 3)  Look for the default in the registration table with GLOBAL owner 
  *
  * 4)  Look for the default in the database with GLOBAL owner 
  *
  * In any case if we find a default that isn't registered then register it
  * in the database. 
  */
    if (rd = _isRegistered(owner, name))
	return (rd->value);

    if (!defaultsDB) {
	if (!_openDefaults())
	    return (0);
    }
    if ((defaultString = _doReadDefault(owner, name, 0))) {
	;
    } else if ((rd = _isRegistered(GLOBAL, name))) {
	defaultString = rd->value;
    } else
	defaultString = _doReadDefault(GLOBAL, name, 0);
    _closeDefaults();
    /* ??? are you sure that defaultString is always initialized ? */
    _registerDefault(owner, name, defaultString);
    return (defaultString);
}


const char     *
NXReadDefault(const char *owner, const char *name)
 /*
  * This routine guarantees a read of the defaults database.  If the
  * exact owner name default is not found in the database then a NULL
  * is returned.  This routine does not reset the internal
  * cache and does not look for the named default as a system wide 
  * default.  You can update the internal cache by calling NXSetDefault
  * with the value returned from this functions.  Also you can look for
  * the system wide named default by send a NULL for the owner parameter.
  */
{
    char           *defaultString;

    if( !owner || !name )
        return( 0 );
	
    if (!defaultsDB) {
	if (!_openDefaults())
	    return (0);
    }
    defaultString = _doReadDefault(owner, name, 0);
    _closeDefaults();
    return (defaultString);
}


void
NXUpdateDefaults()
 /*
  * This routine checks the last modification of the defaults database.
  * The timestamp of each internally registered default is checked 
  * against the time stamp of the database.  If the time stamp of the
  * database is greater than the timestamp of the default then the
  * database if re-read.  Note that this function will override the
  * defaults that have been set by the command line, or with
  * NXSetDefault().  This routine checks every cached default against
  * the time stamp of the database.
  */
{
    struct RegisteredDefault *rd = firstRD;
    char *value;

    if (!defaultsDB) {
	if (!_openDefaults())
	    return;
    }
    
    while (rd) {
        if (rd->mtime < db_mtime ) {
	    value = _doReadDefault( rd->owner, rd->name, 0 );
	    if (value)
	        _resetRegisteredDefault(rd, rd->owner, rd->name, value);
	}
	rd = rd->next;
    }

    _closeDefaults();

    return;
}

const char *
NXUpdateDefault( const char *owner, const char *name )
 /*
  * This routine checks the last modification of the defaults database.
  * The time stamp of each internally registered default is checked 
  * against the time stamp of the database.  If the time stamp of the
  * database is greater than the timestamp of the default then the
  * database if re-read.  Note that this function will override the
  * defaults that have been set by the command line, or with
  * NXSetDefault().
  * This routine the new value if it is changed or NULL if it isn't
  * updated.
  */
{
    struct RegisteredDefault *rd = firstRD;
    char *value = NULL;

    if( !owner || !name )
        return( 0 );
	
    if (!defaultsDB) {
	if (!_openDefaults())
	    return value;
    }
    if ( rd = _isRegistered(owner, name)){
        if (rd->mtime < db_mtime ) {
            value = _doReadDefault( rd->owner, rd->name, 0 );
	    if (value)
	        _resetRegisteredDefault(rd, rd->owner, rd->name, value);
	}
    }
    else
        value = _doReadDefault( owner, name, 0 );

    _closeDefaults();

    return( value );
}


int
NXSetDefault(const char *owner, const char *name, const char *value)
 /*
  * Sets the default's value for this process.  Future calls to
  * NXGetDefaultValue will return the string passed for the value.  This
  * does not write the default to the defaults database.   Returns
  * 1 on success, 0 on failure.
  */
{
    struct RegisteredDefault *rd;

    if( !owner || !name || !value )
        return( 0 );
	
    if ( rd = _isRegistered(owner, name))
	_resetRegisteredDefault(rd, owner, name, value);
    else
	_registerDefault(owner, name, value);
    return( 1 );
}


int
NXWriteDefault(const char *owner, const char *name, const char *value)
 /*
  * Writes a default and its value into the default database.  If the
  * owner is 0 then the default is system wide.  The newly written default
  * value for the owner and name will be returned if NXGetDefaultValue
  * is called with the same owner and name.
  *
  * 0 is returned if an error occured writing the default.  Otherwise a 1 is
  * returned. 
  */
{
    int           returnValue;

    if( !owner || !name || !value )
        return( 0 );
	
    if ((!defaultsDB) && (!_openDefaults()))
	return (0);

    returnValue = _writeAndRegisterDefault( owner, name, value );
    _closeDefaults();
    return (returnValue);
}


int
NXWriteDefaults( const char *owner, NXDefaultsVector vect )
 /*
  * Writes the defaults in the vector into the default database.  If the
  * owner is 0 then the default is system wide.  Writing defaults overrides
  * any registered default.
  *
  * The number of successfully written defaults is returned.
  */
{
    register struct _NXDefault *d;
    register int num = 0;

    if( !owner || !vect )
        return( 0 );
	
    if ((!defaultsDB) && (!_openDefaults()))
	return (0);

    for (d = vect; d->name; d++)
      if( _writeAndRegisterDefault( owner, d->name, d->value))
	num++;

    _closeDefaults();
    return (num);
}
      


int
NXRemoveDefault(const char *owner, const char *name)
 /*
  * Remove a default from the defaults database.  Returns 0 if the 
  * default could not be removed.  Otherwise a 1 is returned.
  */
{
    Data            deleteData;
    int             returnVal;

    if( !owner || !name )
        return( 0 );
	
    if (!defaultsDB)
	if (!_openDefaults())
	    return (0);

    if (!name)
	return (0);
    _constructKey(&deleteData.k, owner, name);
    returnVal = dbDelete(defaultsDB, &deleteData);
    _destroyKey(&deleteData.k);

    _closeDefaults();
    return (returnVal);
}



/*** Private functions ***/

static Database       *
_openDefaults()
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
    const char	   *home;
    struct stat stat_buf;
    int             dir;

    home = NXHomeDirectory();

 /* Construct the path ~/.NeXT/.NeXTdefaults. */
    if (home && home[0] == '/') {

      /* Allocate a buffer used to read default data from the database.  */
	if (!(readBuf = (char *)malloc(READ_BUF_SIZE)))
	    return (0);
    /*
     * Use the readBuf as a temp buffer to construct the .NeXTdefaults
     * path.  This is nasty but we don't really cause any problems by
     * using the readBuf this way. 
     */
    /* First see if the ~/.NeXT directory exists. */
	strcpy(readBuf, home);
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

	if( !dbExists(readBuf) ){
	  if( dbCreate(readBuf) ){
	    int uid=getuid(), gid=getgid(), euid=geteuid(), egid=getegid();
	    
#ifdef notdef
	    printf( "uid %d, euid %d, gid %d, egid %d\n", uid,euid,gid,egid );
#endif notdef
	    if( uid != euid || gid != egid ){
	      strcat(readBuf, ".L" );
	      chown( readBuf, uid, gid );
	      readBuf[strlen(readBuf)-2] = 0;
	      strcat(readBuf, ".D" );
	      chown( readBuf, uid, gid );
	      readBuf[strlen(readBuf)-2] = 0;
	    }
	  }
	}
	defaultsDB = dbInit( readBuf );
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


static int
_closeDefaults()
 /*
  * Close the defaults database.  Deallocate MOST memory allocated. 
  */
{
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
    return (1);
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


static void
_resetRegisteredDefault(struct RegisteredDefault *rd, const char *owner, const char *name, const char *value)
{
    /*
     *  This code results in a memory leak.  The previously allocated
     *  memory for the RegisteredDefault isn't deallocated.
     */
    rd->value = value ? strcpy( defaultAlloc( strlen(value)+1), value ) : 0;
    rd->owner = owner ? strcpy( defaultAlloc( strlen(owner)+1), owner ) : 0;
    rd->name = name ? strcpy( defaultAlloc( strlen(name)+1), name ) : 0;
    rd->mtime = db_mtime;
}


static struct RegisteredDefault *
_isRegistered(owner, name)
    char           *owner, *name;
{
    struct RegisteredDefault *rd = firstRD;

    while (rd) {
	if (strcmp(rd->name, name) == 0) {
	    if (!(owner && rd->owner)) {	/* One owner is NULL */
		if (owner == rd->owner)
		    return (rd);
	    } else if (strcmp(rd->owner, owner) == 0)
		return (rd);
	}
	rd = rd->next;
    }
    return (0);
}
	      

static char           *
_doReadDefault(owner, name, buf)
    char           *owner;
    char           *name;
    char           *buf;

 /*
  * Read a default from the defaults database.  Owner should typically be
  * the application that uses the default.  If owner is null then the
  * default will be registered as a system default.  The name is the
  * default name. 
  *
  * The string pointer for the requested value is returned, or null if
  * the default isn't in the database.
  */
{
    Data            readData;
    char           *defaultString;


    if ( !owner || !name)
	return (0);

 /*
  * Get the default value. 
  */
    _constructKey(&readData.k, owner, name);
    readData.c.s = readBuf;
    if (!dbFetch(defaultsDB, &readData)) {
#ifdef FUNCTION_SHOULD_LOOK_FOR_OWNERLESS_DEFAULT
    /*
     * Look for the default without the owner. 
     */
	_constructKey(&readData.k, (char *)0, name);
	if (!dbFetch(defaultsDB, &readData))
	    return (0);
#endif
	return (0);
    }
    _destroyKey(&readData.k);

    defaultString = defaultAlloc(readData.c.n);
    strcpy(defaultString, readData.c.s);

    return (defaultString);
}


static char           *
_doWriteDefault(owner, name, value)
    char           *owner;
    char           *name;
    char           *value;
{
    Data            writeData;


    if ( !owner || !name)
	return (0);

    if (!value)
	value = "";

    _constructKey(&writeData.k, owner, name);
    writeData.c.s = value;
    writeData.c.n = strlen(value) + 1;

    if (!dbStore(defaultsDB, &writeData))
	return (0);

    _destroyKey(&writeData.k);

    return (value);
}


static int
_writeAndRegisterDefault( owner, name, value )
    char *owner;
    char *name;
    char *value;
{
    struct RegisteredDefault *rd;
    int rVal;

    if ((rVal = _doWriteDefault(owner, name, value) ? 1 : 0)) {
        if (rd = _isRegistered(owner, name))
	    _resetRegisteredDefault(rd, owner, name, value);
	else
	    _registerDefault(owner, name, value);
    }
    return( rVal );
}


static int _lockDefaults()
{
    return (dbWaitLock(defaultsDB, 5, 30));
}



static void _unlockDefaults()
{
    dbUnlock(defaultsDB);
}



static void
_constructKey(Datum *keyDatum, const char *owner, const char *name)
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


static void _destroyKey(Datum *keyDatum)
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


static void
_NXGobbleArgs(int numToEat, int *argc, char **argv)

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
cmdLineToDefaults( char *owner, const NXDefaultsVector vector )
 /*
  * Sucks arguments we need off the command line and places them the
  * parameters data. 
  */
{
    extern int      NXArgc;
    extern char   **NXArgv;
    register int   *argc = &NXArgc;
    register char **argv = NXArgv;
    const struct _NXDefault *d;

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


static char    *firstArg = NULL;
static int      firstLen = 0;

extern char *strtok();

static char    *
rstrtok(s1, s2)
    register char  *s1, *s2;
 /*
  * our right-to-left version of strtok.  Exception: rstrtok does not work if
  * s2 is of length > 1. 
  */
{
    register char  *cp;
    register int    s1len, clen;

    if (s1) {
	firstArg = s1;		/* Save away */
	firstLen = s1len = strlen(s1);
    } else {
	s1 = firstArg;		/* Cache in reg */
	s1len = firstLen;
    }
    clen = 0;
    cp = s1 + s1len - 1;
    while (cp >= s1) {
	if (*cp == *s2) {	/* Change this line to a search to */
	    if (clen) {		/* 1-char limit on s2. FIX */
		firstLen = cp - s1;
		cp++;
		*(cp + clen) = 0;
		return (cp);
	    } else {
		clen--;
	    }
	}
	cp--;
	clen++;
    }
    if (clen) {
	*(s1 + clen) = 0;
	firstLen = 0;
	return (s1);
    } else
	return (NULL);
} /* rstrtok */


#pragma CC_OPT_OFF
#ifdef NX_ALERT
int
NXFilePathSearch(const char *envVarName, const char *defaultPath,
			int leftToRight, const char *fileName,
			int (*funcPtr)(), void *funcArg)
 /*
  * Used to look down a directory list for one or more files by a certain
  * name.  The directory list is obtained from the given environment
  * variable name, using the given parameter if not.  If leftToRight is
  * true, the list will be searched left to right; otherwise, right to
  * left.  In each such directory, if the file by the given name can be
  * accessed, then the given function is called with its first argument
  * the path to the file, its second argument as the given value.  
  * If the function returned zero, NXFilePathSearch will then 
  * return with zero. If the function returned a
  * negative value, NXFilePathSearch will return with the negative value. If
  * the function returns a positive value, NXFilePathSearch will continue to
  * traverse the driectory list and call the function.  If it successfully
  * reaches the end of the list, it returns 0. 
  */
{
    char            filePath[MAXPATHLEN], pathBuffer[MAXPATHLEN];
    const char	   *pathPtr;
    char           *dirName, *pathName, *tempName;
    const char	   *home;
    int             funcReturn, wdChecked, len;

 /*
  * Get the pathName out of the environment variable, if one is supplied;
  * else use default 
  */
    if ((!envVarName) || (!(pathPtr = (char *)getenv(envVarName))))
	pathPtr = defaultPath;
 /* If no parameter, you're outa here */
    if (!pathPtr)
	return (-1);
 /* make a copy, since the token scanners write to the path */
    pathName = (char *)__builtin_alloca(sizeof(char) * strlen(pathPtr)+1);
    strcpy(pathName, pathPtr);

    wdChecked = 0;

    for (tempName = pathName;
	 dirName = (char *)(leftToRight ? strtok(tempName, ":") : rstrtok(tempName, ":"));
	 tempName = NULL) {
	len = strlen(dirName);

    /* Special cases for dirName */
	if ((*dirName == '.') && (len == 1))
	{	/* working directory */
	    if (wdChecked)
		continue;
	    wdChecked = 1;
	    /* Get the working directory pathName */
	    if (!getwd(pathBuffer))
		continue;
	    len = strlen(pathBuffer);
	    dirName = pathBuffer;
	} else if ((*dirName == '~') && (
			(len == 1)||(*(dirName + 1) == '/')) )
	{	/* home directory */
	    home = NXHomeDirectory();
	    NX_ASSERT(home && home[0] == '/', "Bad home directory.");
	    if (home && home[0] == '/') {	/* ~ is valid */
	    	strcpy(pathBuffer,home);
		strcat(pathBuffer,dirName+1);
		dirName = pathBuffer;
		len = strlen(dirName);
	    } else
		continue;	/* No home directory */
	} /* else dirName,len point at a regular directory name */
	strcpy(filePath, dirName);
	filePath[len++] = '/';
	filePath[len] = 0;
	strcat(filePath, fileName);
	if (access(filePath, R_OK) >= 0) {
	    if ((funcReturn = (*funcPtr)(filePath, funcArg)) <= 0)
		return (funcReturn);
	}
    }
    return (0);
} /* NXFilePathSearch */
#endif
#pragma CC_OPT_ON

/* creates a unique temporary file.  Can deal with suffixed files correctly
   (unlike mktemp).  Args are a writable string in which to create the
   filename, and a position within the string to write a unique part of
   the name.  The string should have 6 characters at this position than can
   be written to.  It returns the first argument.
*/

char *
NXGetTempFilename( char *name, int pos )
{
    struct timeval time;
    int num, firstNum;
    char numBuf[10];

  /* get 6 digits of reasonable randomness, seed independent */
    gettimeofday( &time, NULL );
    num = firstNum = ((time.tv_usec>>8) * 439821) % (1000*1000);
    do {
	sprintf( numBuf, "%06d", num );
	bcopy( numBuf, name+pos, 6 );
	if( access( name, F_OK ) == -1 )
	    break;		/* we found a good one */
	if( --num < 0 )
	    num = 999999;
    } while( num != firstNum );
    return name;
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
0.91
----
 5/19/89 trey	minimized static data

0.92
----
 6/08/89 trey	made NXGetDefaultValue, NXReadDefault, NXUpdateDefault
		 return const char *
 6/16/89 rjw	Removed support of 0 owner.  The backdrop owner was replaced
		by "GLOBAL".  Added error handling for 0 owner.
*/

