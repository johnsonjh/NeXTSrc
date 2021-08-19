/*
	epsUtil.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer

	This file contains various functions useful in dealing with
	EPS files. Currently there are four public functions:

	_NXEPSGetBoundingBox()
	_NXEPSDeclareDocumentFonts()
	_NXEPSBeginDocument()
	_NXEPSEndDocument()
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>
#import <stdio.h>
#import <objc/objc.h>
#import <ctype.h>
#import <stdio.h>
#import <string.h>
#import <appkit/graphics.h>
#import <appkit/Font.h>
#import <streams/streams.h>
#import <dpsclient/dpsclient.h>

#define EPSHEADER "%!PS-Adobe"
#define MAXPSLINELEN 256	/* As defined in the structured PS document */

static BOOL getLine (NXStream *stream, char *line)
{
    int cnt = 0, ch;

    while (((ch = NXGetc(stream)) != EOF) && (ch != '\n') && (ch != '\r')) {
	if (++cnt < MAXPSLINELEN) *line++ = ch;
    }
    *line = '\0';
    return ((ch == EOF) && !cnt) ? NO : YES;
}

static BOOL match (char *line, char *str, BOOL skipSpaces)
{
    if (skipSpaces) while(isspace(*line)) line++;
    return (strncmp(line, str, strlen(str)) ? NO : YES);
}

/*
 * gotoTrailer() assumes that the file is somewhere in the header (right after
 * the line with (atend)) and runs through the file looking for the %%Trailer.
 * The stream is left pointing at the beginning of the next line.
 * This function skips over nested files.
 */
static BOOL gotoTrailer (NXStream *stream)
{
    int nest = 0;
    char line[MAXPSLINELEN];
    while (getLine(stream, line)) {
	if ((line[0] == '%') && (line[1] == '%')) {
	    if (match(line+2, "Begin", NO) &&
		((match(line+7, "File", NO)||match(line+7, "Document", NO)))) {
		nest++;
	    } else if (match(line+2, "End", NO) &&
		((match(line+5, "File", NO)||match(line+5, "Document", NO)))) {
		nest--;
	    } if ((nest == 0) && match(line+2, "Trailer", NO)) {
		return YES;
	    }
	}
    }
    return NO;
}

/*
 * Get the bounding box of the EPS stream.  If the file doesn't have a valid
 * PS header or doesn't have a valid bounding box this function returns NO.
 * Otherwise it returns YES and the bounding box can be found in r. The stream 
 * is left pointing at the line after the bounding box.  The bounding box can
 * be in the header or the trailer (if (atend) was present). 
 *
 * Enter this function with the stream properly positioned.
 *
 * ??? This function isn't too smart about skipping to the trailer; however,
 * the problem isn't an easy one, and EPS files should really have their
 * bounding boxes in the header, anyway!
 */
BOOL _NXEPSGetBoundingBox (NXStream *stream, NXRect *r)
{
    char line[MAXPSLINELEN];

    if (!getLine(stream, line) || !match(line, EPSHEADER, NO)) {
	return NO;
    }

    while ((NXGetc(stream) == '%') && (NXGetc(stream) == '%')) {
	getLine(stream, line);
	if (match(line, "BoundingBox:", NO)) {
	    if (sscanf(line+12, "%f%f%f%f",
		    &NX_X(r), &NX_Y(r), &NX_WIDTH(r), &NX_HEIGHT(r)) == 4) {
		NX_WIDTH(r) -= NX_X(r);
		NX_HEIGHT(r) -= NX_Y(r);
		return YES;
	    } else if (!match(line+12, "(atend)", YES) || 
		       !gotoTrailer(stream)) {
		return NO;
	    }
	}
    }
    return NO;
}

/*
 * Declare the fonts used in an EPS program.  If the file doesn't have a valid
 * PS header or doesn't have a valid bounding box this function returns NO and
 * no fonts are declared.  Otherwise this function returns YES and any fonts
 * declared in the header (or trailer, if the (atend) comment was encountered)
 * are declared through the use of the Font class useFont: method.
 *
 * Enter this function with the stream properly positioned.
 * Also see caveat above concerning parsing of stuff in the trailer section.
 */
BOOL _NXEPSDeclareDocumentFonts (NXStream *stream)
{
    char line[MAXPSLINELEN], *curLoc, *fontName;

    if (!getLine(stream, line) || !match(line, EPSHEADER, NO)) {
	return NO;
    }

    while ((NXGetc(stream) == '%') && (NXGetc(stream) == '%')) {
	getLine(stream, line);
	if (match(line, "DocumentFonts:", NO)) {
	    if (match(line+14, "(atend)", YES)) {
		if (!gotoTrailer(stream)) {
		    return NO;
		}
	    } else {
		do {	/* Now we start parsing fonts... */
		    curLoc = (line[2] == '+') ? line + 3 : line + 14;
		    while (fontName = strtok(curLoc, " \t\n")) {
			[Font useFont:fontName];
			curLoc = NULL;
		    }
		} while (getLine(stream, line) && match(line, "%%+", NO));
	        return YES;
	    }
	}
    }
    return YES;
}

/*
 * Probably no need to declare these in print package; size of EPS files
 * usually overwhelms this small header...
 */
void _NXEPSBeginDocument (const char *docName)
{
    DPSPrintf(DPSGetCurrentContext(),
	    "\n/__NXEPSSave save def /showpage {} def "
	    "0 setgray 0 setlinecap 1 setlinewidth\n"
	    "0 setlinejoin 10 setmiterlimit [] 0 setdash newpath "
	    "count /__NXEPSOpCount exch def "
	    "/__NXEPSDictCount countdictstack def\n"
	    "%%%%BeginDocument: %s\n", docName ? docName : "");
}

void _NXEPSEndDocument ()
{
    DPSPrintf(DPSGetCurrentContext(),
	    "\n%%%%EndDocument\n"
	    "count __NXEPSOpCount sub {pop} repeat "
	    "countdictstack __NXEPSDictCount sub {end} repeat "
	    "__NXEPSSave restore\n");
}

/*

Modifications (since 85):

85
--
 5/15/90 aozer	Created

86
--
 6/11/90 aozer	Made gotoTrailer ignore Begin/EndDocument (used to just ignore
		Begin/EndFile) on Peter's suggestion
 6/11/90 aozer	Added _NXEPSBeginDocument and _NXEPSEndDocument

87
--
 6/21/90 aozer	Restore dict & op stacks before restoring to _NXEPSSave state.

94
--
 9/19/90 aozer	Made getLine() look for \r in addition to \n.

*/
    