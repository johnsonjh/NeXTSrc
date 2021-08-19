/*
	NXFaxCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB


#import "NXFaxText.h"
#import "NXFaxPanel.h"
#import <streams/streams.h>
#import <string.h>
#import <NXCType.h>
#import <defaults.h>
#import "appkitPrivate.h"

#define FAXDATE ((faxDate == NULL) ? (faxDate = \
KitString("Printing","\"Date\"",NULL)) : faxDate)
#define FAXPAGES ((faxPages == NULL) ? (faxPages = \
KitString("Printing","\"Pages\"",NULL)) : faxPages)
#define FAXTO ((faxTo == NULL) ? (faxTo = \
KitString("Printing","\"To\"",NULL)) : faxTo)
#define FAXFROM ((faxFrom == NULL) ? (faxFrom = \
KitString("Printing","\"From\"",NULL)) : faxFrom)

static const char *faxDate = NULL;
static const char *faxPages = NULL;
static const char *faxTo = NULL;
static const char *faxFrom = NULL;

@implementation NXFaxText


/*  scans forward to next colon in text, which must either be preceded by
*   newline or start of text, sets priorNewline if to the
 *  newline discovered immediately preceding colon.
 */
static int nextColon (id self,int startPos,int *priorNewline)
{
    char c;
    int cpos,nl,nc;
    NXStream *s;
    
    nl = -1;
    nc = -1;
    cpos = startPos;
    s = [self stream]; /* don't have to free this - owned by Text Object */
    NXSeek(s,cpos,NX_FROMSTART);
    while (1) {
	c = NXGetc(s);
	if (c == EOF)
	    goto exitPoint;
	if (c == '\n')
	    nl = cpos;
	if (c == ':' && (nl >= 0 || !startPos)) {
	    nc = cpos;
	    goto exitPoint;
	}
	cpos++;
    }
exitPoint:   
    *priorNewline = nl;
    return nc;
}

/*  backups 2 colons so that we can tab into previous field */

static int backupAField (id self,int startPos)
{
#define LOOKINGFOR1STCOLON 0
#define LOOKINGFOR1STNL 1
#define LOOKINGFOR2NDCOLON 2

    char c;
    int cpos,state,nc;
    NXStream *s;
    
    state = LOOKINGFOR1STCOLON;
    nc = -1;
    cpos = startPos;
    s = [self stream]; /* don't have to free this - owned by Text Object */
    while (cpos >= 0) {
        NXSeek(s,cpos,NX_FROMSTART);
	c = NXGetc(s);
	switch(state) {
	case LOOKINGFOR1STCOLON:
	    if (c == ':')
	        state = LOOKINGFOR1STNL;
	    break;
	case LOOKINGFOR1STNL:
	    if (c == '\n')
	        state = LOOKINGFOR2NDCOLON;
	    break;
	case LOOKINGFOR2NDCOLON:
	    if (c == ':') {
		nc = cpos ? cpos-1 : 0;
		goto exitPoint;
	    }
	    break;
	}
	cpos--;
    }
exitPoint:   
    return nc;
}

/*  backups to newLine or start of text so we can find true start of field
    when field value contains colons. */

static int backupToNewline (id self,int startPos)
{
    char c;
    int cpos,nc;
    NXStream *s;
    
    nc = -1;
    cpos = startPos;
    s = [self stream]; /* don't have to free this - owned by Text Object */
    while (cpos > 0) {
        NXSeek(s,cpos,NX_FROMSTART);
	c = NXGetc(s);
	if (c == '\n')
	    goto exitPoint; 
	cpos--;
    }
exitPoint:   
    return cpos;
}

/*  returns -1 if not found, starting stream pos if found.  case insensitive
 *  search.
 */
static int search(id self,int first,int last,const char *target)
{
    char c,ct;
    const char *tc;
    int cpos;
    NXStream *s;
    
    cpos = first;
    s = [self stream]; /* don't have to free this - owned by Text Object */
    while (cpos <= last) {
        NXSeek(s,cpos,NX_FROMSTART);
	tc = target;
	c = NXGetc(s);
        ct = *tc++;	
	while(1) {
	    if (!ct) return cpos;
	    if (c == EOF) return -1;
	    if (NXToLower(c) != NXToLower(ct)) break;
	    c = NXGetc(s);
	    ct = *tc++;
	}
	cpos++;
    }

    return -1;
}

/*  returns 1 if field is going to be automatically filled in by cover
 *  sheet generation (currently, data and pages fields).  return 0 else.
 */
static int isProtected(id self,int first,int last)
{
    const char *s;
    
    s = FAXDATE;
    if (s && search(self,first,last,s) >= 0)
        return 1;
    s = FAXPAGES;
    if (s && search(self,first,last,s) >= 0)
        return 1;
	
    return 0;
}

- keyDown: (NXEvent *) theEvent {
    NXSelPt start,end;
    int newstart,newend,newline,backstart;
    unsigned short charCode;
    
    charCode = theEvent->data.key.charCode;
    if (charCode == '\t' || charCode == NX_BTAB) {
        [self getSel:&start:&end];
	newstart = end.cp;
        if (charCode == NX_BTAB) /* back tab'ers! */
            newstart = backupAField(self,start.cp);
	if (newstart >= 0) {
	    newstart = backupToNewline(self,newstart);
	    newstart = nextColon(self,newstart,&newline);
	}
	while (newstart >= 0) {
	    newstart++;
	    newend = nextColon(self,newstart,&newline);
	    if (newend >=0) {
	        if (newline >= 0)
		    newend = newline;
	    } else
	        newend = [self textLength];
	    if (isProtected(self,newstart,newend)) {
		if (charCode == NX_BTAB) { /* back tab'ers! */
		    backstart = backupAField(self,newstart);
		    if (backstart >= 0) {
			backstart = backupToNewline(self,backstart);
			backstart = nextColon(self,backstart,&newline);
		    }
		    newstart = (backstart < 0 || backstart+1 == newstart) ?
		    		 -1 : backstart;
		} else
	            newstart = nextColon(self,newend,&newline);
	    } else {
		[self setSel : newstart : newend];
		[self scrollSelToVisible];
		break;
	    }
	}
        return self;
    } else
	return [super keyDown : theEvent];
}

- selectFirstField {
    int newstart,newend,newline;

    newend = 0;
    newstart = nextColon(self,0,&newline);
    while (newstart >= 0) {
	newstart++;
	newend = nextColon(self,newstart,&newline);
	if (newend >=0) {
	    if (newline >= 0)
		newend = newline;
	} else
	    newend = [self textLength];
	if (!isProtected(self,newstart,newend))
	    break;
	newstart = nextColon(self,newend,&newline);
    }
    if (newstart < 0)
        newstart = newend = 0;
    [self setSel :newstart : newend];
    return self;
}

- setField : (const char *) name to : (char *) value {
    int newstart,newend,newline,fieldCp;
    int found;
    
    fieldCp = 0;
    while (1) {
	newstart = nextColon(self,fieldCp,&newline);
	if (newstart >= 0) {
	    newstart++;
	    newend = nextColon(self,newstart,&newline);
	    if (newend >=0) {
		if (newline >= 0)
		    newend = newline;
	    } else
		newend = [self textLength];
	    found = search(self,newstart,newend,name);
	    if (found >= 0) {
		[self setSel :found : found+strlen(name)];
		[self replaceSel : value];
		break;
	    } else
	        fieldCp = newend;
	} else
	    break;
    } 
    return self;
}

- setDate : (int) theTime {
    extern const char        *ctime( /* clock */ );	/* long *clock; */

#define MAX_TIME_CHARS 128
    char                timeBuffer[MAX_TIME_CHARS];

    strncpy(timeBuffer, ctime((const time_t *)&theTime), MAX_TIME_CHARS);
    timeBuffer[MAX_TIME_CHARS - 1] = '\0';
    timeBuffer[strlen(timeBuffer) - 1] = '\0';	/* lop off \n */
    [self setField : FAXDATE to : timeBuffer];
    return self;
}

- setPages : (int) thePages {
    char pageString[20];
    
    sprintf(pageString,"%d",thePages);
    [self setField : FAXPAGES to : pageString];
    return self;
}

- setTo : (char *) recipient {
    [self setField : FAXTO to : recipient];
    return self;
}

- setFrom : (char *) sender
{
    [self setField : FAXFROM to : sender];
    return self;
}

/*
79
--
 4/??/90 chris	Created by cmf  

81
--
 4/??/90 chris	Fixed bug in search(), which led to tab working incorrectly
  		on last field.
81
--
 4/??/90 chris	Made tab work correctly with colons internal to field value
 
82
--
 4/??/90 chris	Changes to use of defaults per trey's request.
 
85
-
 5/18/90 chris	Modified keyDown: to deal with new encoding for backtab	
 
92
--
 8/16/90 chris	Modified for _NXKitString()
*/

@end





