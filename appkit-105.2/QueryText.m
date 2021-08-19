/*
	QueryText.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
	Author: Chris Franklin
  
	DEFINED IN: The Application Kit
	HEADER FILES: QueryText.h
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "SavePanel_Private.h"
#import "Application.h"
#import "QueryText.h"
#import "OpenPanel.h"
#import "nextstd.h"
#import "publicWraps.h"
#import <sys/param.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <math.h>
#import <sys/time.h>
#import <sys/dir.h>
#import <errno.h>
#import <sys/signal.h>
#import <sys/fcntl.h>
#import <pwd.h>
#import <string.h>
#import <zone.h>

typedef struct _pwname {
    struct _pwname     *next;
    char                name[2];
}                   pwname;

static pwname      *curPwName = NULL;
static pwname      *firstPwName = NULL;

/*   
 * The following 4 routines cover setpwent, getpwent, etc.
 * to reduce reading the passwd file to once per completion.
 */

static void getPwList(NXZone *zone)
{
    register struct passwd *pw;
    register pwname    *pn;
    register pwname    *lastpn = NULL;

    setpwent();
    while (pw = getpwent()) {
	NX_ZONEMALLOC(zone, pn, pwname, sizeof(pwname) - 1 + strlen(pw->pw_name));
	pn->next = NULL;
	strcpy(pn->name, pw->pw_name);
	if (lastpn) {
	    lastpn->next = pn;
	} else {
	    firstPwName = pn;
	}
	lastpn = pn;
    }
    endpwent();
    curPwName = firstPwName;
}

static void
freePwList()
{
    register pwname    *pn;
    register pwname    *temppn;

    pn = firstPwName;
    while (pn) {
	temppn = pn->next;
	NX_FREE(pn);
	pn = temppn;
    }
    firstPwName = NULL;
}

static pwname      *
setPwName()
{
    return curPwName = firstPwName;
}

static pwname      *
getPwName()
{
    pwname             *pn;

    pn = curPwName;
    if (pn)
	curPwName = pn->next;
    return pn;
}

/*
 * Concatentate source onto dest.
 * Total is limited to max characters and is guaranteed to be NULL terminated.
 */

static void
catWithMax(char *dest, char *source, int max)
{
    while ((--max >= 0) && *dest)
	dest++;
    while (--max >= 0)
	if (!(*dest++ = *source++))
	    return;
    *dest = '\0';
}

/*
 *  Copies source to dest, stopping at max chars or '/0'.
 *  Always leaves room to null terminate dest.
 */

static void
copyWithMax(char *dest, char *source, int max)
{
    while (--max >= 0)
	if ((*dest++ = *source++) == 0)
	    return;
    *dest = '\0';
}


/*
 *  expands ~user/path to userHomeDirectory/path.  returns
 *  NULL if user is not in password file, else returns
 *  pointer to expanded path. Note ~/foo is currentuserHomeDirectory/foo.
 */

static char *expandHome(char *retPath, char *oldPath)
{
    register char      *oc, *nc;
    const char	       *home;
    register struct passwd *pw;
    char                user[40];

    if (*oldPath != '~') {
	return (strcpy(retPath, oldPath));
    }
    nc = user;
    oc = oldPath + 1;
    while (*oc && *oc != '/') {
	*nc++ = *oc++;
    }
    *nc = '\0';
    if (!user[0]) {
	home = NXHomeDirectory();
	strcpy(retPath, (home && home[0] == '/') ? home : "/");
    } else {
	pw = getpwnam(user);
	if (pw == NULL) {
	    return NULL;
	}
	strcpy(retPath, pw->pw_dir);
    }
    strcat(retPath, oc);
    return retPath;
}


/*
 * Break path into into 2 parts: directory path and file name.
 */

static void
splitPath(char *path, char *dirPart, char *filePart)
{
    register char      *c;

    c = strrchr(path, '/');
    if (c == NULL) {
	copyWithMax(filePart, path, MAXNAMLEN);
	*dirPart = '\0';
    } else {
	c++;
	copyWithMax(filePart, c, MAXNAMLEN);
	copyWithMax(dirPart, path, c - path);
    }
}

/*    
 * If first match (items == 1), set completedName to name,
 * else shorten completedName to first difference.
 * If shorten completedName to len (the original match string's length),
 * then stop (return YES).
 */

static BOOL trimToName(char *completedName, char *name, int len, int items)
{
    char               *cn;
    char               *nn;

    if (items == 1) {
	copyWithMax(completedName, name, MAXNAMLEN);
    } else {
	cn = completedName;
	nn = name;
	while (*nn && *nn == *cn) {
	    nn++;
	    cn++;
	}
	*cn = '\0';
	if ((cn - completedName) == len) {
	    return YES;
	}
    }

    return NO;
}


/*   
 * Called in 2 places: first, with completedName == NULL, we just
 * count users (files if dir != NULL) whose names are an extension of name.
 * Second, we compute extended name by starting with first 
 * match, and trimming that name to be the prefix shared with all
 * names matching original name.
 *
 *   algorithm:
 *
 *   iterate through names
 *       for each name for which name is a prefix
 *           count name
 *           if completedName != NULL
 *               compute completed name
 */

static int
forAllDo(char **filterTypes, char *dirName, char *name, char *completedName, DIR * dir)
{
    int                 length;
    int                 items;
    char               *fname;
    pwname             *pn;
    struct direct      *dirp;
    register char      *nc;
    register char      *fc;
    char	      **ft;
    char		statname[MAXPATHLEN+1];
    struct stat		st;
    char	       *ext, *leaf;

    items = 0;
    length = strlen(name);
    if (*dirName) {
	strcpy(statname, dirName);
	strcat(statname, "/");
    } else {
	statname[0] = '\0';
    }
    leaf = strrchr(statname, '/');
    if (leaf) leaf++;
    for (;;) {
	if (dir) {
	    fname = (dirp = readdir(dir)) ? dirp->d_name : NULL;
	    if (fname && filterTypes && *filterTypes) {
		fc = fname;
		nc = name;
		while (*nc && *fc == *nc) {
		    fc++;
		    nc++;
		}
		if (!*nc) {
		    ext = strrchr(fname, '.');
		    if (ext) {
			ext++;
		    }
		    ft = filterTypes;
		    while (*ft && (!ext || strcmp(*ft, ext))) ft++;
		    if (!*ft && leaf) {
			strcpy(leaf, fname);
			if (!stat(statname, &st) &&
			    !(st.st_mode & S_IFDIR)) {
			    fname = "";
			}
		    }
		}
	    }
	} else {
	    fname = (pn = getPwName()) ? pn->name : NULL;
	}
	if (!fname) {
	    break;
	}
	nc = name;
	fc = fname;
	for (;;) {
	    if (!*nc) {
		items++;
		if (completedName &&
		    trimToName(completedName, fname, length, items)) {
		    return items;
		}
		break;
	    }
	    if (*nc++ != *fc++) {
		break;
	    }
	}
    }

    return items;
}

/*
 * Perform path completion on incomplete path rawPath.
 */

static int completePath(NXZone *zone, char **filterTypes, char *rawPath, int length)
{
    DIR                *dir;
    int                 nfound;
    char                expandedDir[MAXPATHLEN + 1];
    char                dirPart[MAXPATHLEN + 1];
    char                name[MAXNAMLEN + 1];
    char                completedName[MAXNAMLEN + 1];

    if (*rawPath == '~' && !strchr(rawPath, '/')) {
	getPwList(zone);
	copyWithMax(name, &rawPath[1], MAXNAMLEN);
	dir = NULL;
    } else {
	splitPath(rawPath, dirPart, name);
	if (!expandHome(expandedDir, dirPart) ||
	    !(dir = opendir(*expandedDir ? expandedDir : "."))) {
	    return 0;
	}
    }

    nfound = forAllDo(filterTypes, expandedDir, name, NULL, dir);
    if (nfound) {
	if (dir) {
	    rewinddir(dir);
	} else {
	    setPwName();
	}
	nfound = forAllDo(filterTypes, expandedDir, name, completedName, dir);
    }
    if (dir) {
	closedir(dir);
    } else {
	freePwList();
    }
    if (nfound == 0) {
	return 0;
    }
    if (dir) {
	copyWithMax(rawPath, dirPart, length);
    } else {
	copyWithMax(rawPath, "~", 1);
    }

    catWithMax(rawPath, completedName, length);

    return nfound;
}

int NXCompleteFilename(char *path, int length)
{
    return completePath(NXDefaultMallocZone(), NULL, path, length);
}

@implementation QueryText:Text

#define ESCAPE 0x1B

static int          fromKeyDown = 0;

static unsigned short
QueryFilter(register unsigned short theChar,
	    int flags, unsigned short charSet)
{
    int                 fkd;

    fkd = fromKeyDown;
    fromKeyDown = 0;
    if (theChar == ESCAPE || (!fkd && theChar == NX_CR)) {
	return 0;
    } else {
	return NXFieldFilter(theChar, flags, charSet);
    }
}

+ newFrame:(NXRect *)frameRect text:(char *)theText alignment:(int)mode
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- initFrame:(NXRect *)frameRect text:(char *)theText alignment:(int)mode
{
    [super initFrame:frameRect text:theText alignment:mode];
    charFilterFunc = QueryFilter;
    completionEnabled = returnCompletes = NO;
    return self;
}

- setCharFilter:(NXCharFilterFunc) aFunc
{
    charFilterFunc = QueryFilter;
    return self;
}

- setCompletionEnabled:(BOOL)flag
{
    completionEnabled = flag;
    return self;
}

- (BOOL)returnCompletes
{
    return returnCompletes;
}

- setReturnCompletes:(BOOL)flag
{
    returnCompletes = flag;
    return self;
}

- (BOOL)completionEnabled
{
    return completionEnabled;
}

- _completeFileName
{
    int                 length, matches, nlength;
    char                bufText[MAXPATHLEN + 3];
    char                buf2[MAXPATHLEN + 1];
    char	      **filterTypes;

    length = [self getSubstring:bufText start:0 length:MAXPATHLEN + 3] - 1;
    if (length > MAXPATHLEN) {
    /* ecode = ERRNAMELONG; */
	 /* error #10009 */ ;
	return NULL;
    }
    if ((matches = [window _completeName:bufText maxLen:MAXPATHLEN]) < 0) {
	if ([window respondsTo:@selector(_filterTypes)]) {
	    filterTypes = [window _filterTypes];
	} else {
	    filterTypes = NULL;
	}
	matches = completePath([self zone], filterTypes, bufText, MAXPATHLEN);
    }
    if (matches != 1)
	NXBeep();
    if (bufText[0] == '~') {
	strcpy(buf2, bufText);
	expandHome(bufText, buf2);
	[self setSel:0 :length];
	length = 0;
    } else if (sp0.cp != spN.cp || sp0.cp != length) {
	[self setSel:length :length];
    }
    nlength = strlen(bufText);
    if (nlength - length) 
	[self replaceSel:(bufText + length) length:nlength - length];
    [self setSel:nlength :nlength];
    [self scrollSelToVisible];
    if ([delegate respondsTo:@selector(textDidGetKeys:isEmpty:)])
	[delegate textDidGetKeys:self isEmpty:(nlength > 1)];
    return self;
}



- keyDown:(NXEvent *)theEvent
{
    id                  retval;

    if (completionEnabled) {
	if (theEvent->data.key.charCode == ESCAPE) {
	    return[self _completeFileName];
	}
	if (returnCompletes && theEvent->data.key.charCode == NX_CR) {
	    if (![self _completeFileName]) {
		return NULL;
	    }
	}
    }
    fromKeyDown = 1;
    retval = [super keyDown:theEvent];

    return retval;
}

- paste:sender
{
    id                  retval;

    retval = [super paste:sender];
    [window textDidGetKeys:self isEmpty:(textLength > 1)];
    return retval;
}


@end

/*
  
Modifications (since 0.8):
  
 2/05/89 pah	very minor modifications to support SavePanel architecture
 6/26/89 bgy	removed _simpleReplace and used replaceSel:length:

0.96
----
 7/20/89 pah	make escape completion pay attention to filter types

84
--
 5/14/90 pah	QueryText beeps if more than one or no possibilities found.

*/


